#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/snd.h"
#include "nnsys/fnd.h"
#include "snd_internal.h"

// NitroSystem's sound archive stream players (NNS_SndArcStrm): a stream of the archive played on one of its stream
// players, its file read and, for IMA-ADPCM, decoded by a thread of its own as the stream asks for each block. The
// file's name is a guess, NitroSystem's sndarc_stream.c from memory with the snd_ prefix the other sound files take.
// swan names NNS_SndArcStrmInit NNS_SndArcStrmInit and CreateThread and ThreadProc CreateThread and
// ThreadProc; the rest swan doesn't name

#define STRM_PLAYER_NUM 4
#define STRM_COMMAND_NUM 8
#define STRM_CHANNEL_MAX 6
#define STRM_BUFFER_SIZE_PER_CH 0x800
#define STRM_THREAD_STACK_SIZE 0x1000
#define STRM_INTERVAL 4
#define STRM_END_WAIT 4
#define STRM_ADPCM_BUFFER_SIZE 0x200
#define STRM_ADPCM_HEADER_SIZE 4
#define STRM_ADPCM_INDEX_MAX 88

// The formats of a stream file
enum {
    STRM_FORMAT_PCM8,
    STRM_FORMAT_PCM16,
    STRM_FORMAT_ADPCM,
};

// The head of a stream file. Its data is in blocks, each channel's block after the other's
typedef struct StrmHead {
    u8 format;
    u8 loopFlag;
    u8 numChannels;
    u8 padding;
    u16 sampleRate;
    u16 timer;
    u32 loopStart;
    u32 loopEnd;
    u32 dataOffset;
    u32 numBlocks;
    u32 blockSize;
    u32 blockSamples;
    u32 lastBlockSize;
    u32 lastBlockSamples;
} StrmHead;

typedef struct StrmFileHeader {
    SNDBinaryFileHeader fileHeader;
    SNDBinaryBlockHeader blockHeader;
    StrmHead head;
} StrmFileHeader;

// The IMA-ADPCM decoder's state of a channel, which a block's data starts with
typedef struct AdpcmState {
    s16 pcm;
    u8 index;
} AdpcmState;

typedef struct NNSSndArcStrmPlayer NNSSndArcStrmPlayer;

// How a player reads its file: from the ROM, or from memory when the file is loaded
typedef BOOL (*StrmFileOpenFunc)(NNSSndArcStrmPlayer *player, u32 fileId);
typedef void (*StrmFileCloseFunc)(NNSSndArcStrmPlayer *player);
typedef int (*StrmFileReadFunc)(NNSSndArcStrmPlayer *player, void *buffer, int size, u32 offset);
typedef void (*StrmFileCancelFunc)(NNSSndArcStrmPlayer *player);

// endWait counts the blocks played after the data ended before the player stops; prepared is set once the first
// block is read. volume is the archive's, userVolume ours, both indices of the decibel table
struct NNSSndArcStrmPlayer {
    NNSSndStrm stream;
    FSFile file;
    union {
        u32 fileOffset;
        const u8 *fileAddress;
    };
    StrmFileHeader header;
    NNSSndFader fader;
    AdpcmState adpcm[STRM_CHANNEL_MAX];
    struct {
        BOOL active : 1;
        BOOL playing : 1;
        BOOL startReq : 1;
        BOOL autoStop : 1;
        BOOL seeking : 1;
        BOOL dataEnd : 1;
        BOOL singleChannel : 1;
    } flags;
    int endWait;
    BOOL prepared;
    int cmdCount;
    int allocCount;
    u8 numChannels;
    u8 padding;
    u8 chNoList[STRM_CHANNEL_MAX];
    void *buffer;
    u32 bufSize;
    NNSSndStrmCallback strmCallback;
    void *strmCallbackArg;
    NNSSndArcStrmCallback callback;
    void *callbackArg;
    int strmNo;
    int playerNo;
    NNSSndStrmHandle *handle;
    int prio;
    int volume;
    int userVolume;
    int curVolume;
    u32 curPos;
    StrmFileOpenFunc open;
    StrmFileCloseFunc close;
    StrmFileReadFunc read;
    StrmFileCancelFunc cancel;
};

// A block for the thread to fill, as the stream's callback asked for it
typedef struct StrmCommand {
    NNSFndLink link;
    NNSSndArcStrmPlayer *player;
    NNSSndStrmCallbackStatus status;
    int numChannels;
    void *buffer[STRM_CHANNEL_MAX];
    u32 len;
} StrmCommand;

// A thread that fills blocks, woken when a command is added to its list
typedef struct StrmThread {
    OSThread thread;
    u64 stack[STRM_THREAD_STACK_SIZE / sizeof(u64)];
    OSThreadQueue queue;
    OSMutex mutex;
    NNSFndList commandList;
} StrmThread;

static BOOL SetupPlayers(NNSSndHeapHandle heap);
static NNSSndArcStrmPlayer *AllocPlayer(NNSSndStrmHandle *handle, int playerNo, int prio);
static void DeactivatePlayer(NNSSndArcStrmPlayer *player);
static BOOL PreparePlayer(NNSSndStrmHandle *handle, const NNSSndArcStrmInfo *info, int playerNo, int prio, int strmNo,
                          u32 offset, NNSSndStrmCallback strmCallback, void *strmCallbackArg,
                          NNSSndArcStrmCallback callback, void *callbackArg);
static void StopPlayer(NNSSndArcStrmPlayer *player, int fadeFrames);
static void ShutdownStrmPlayer(NNSSndArcStrmPlayer *player);
static void ClosePlayer(NNSSndArcStrmPlayer *player);
static BOOL AllocChannel(NNSSndArcStrmPlayer *player, int numChannels, const u8 chNoList[]);
static void FreeChannel(NNSSndArcStrmPlayer *player);
static void CreateThread(StrmThread *thread, u32 prio);
static void ClearCommands(NNSFndList *list, NNSSndArcStrmPlayer *player);
static StrmCommand *PopCommand(NNSFndList *list);
static StrmCommand *AllocCommand(void);
static void FreeCommand(StrmCommand *command);
static void HeapDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void ArcStrmCallback(NNSSndStrmCallbackStatus status, int numChannels, void *buffer[], u32 len,
                            NNSSndStrmFormat format, void *arg);
static void StartNextStrm(NNSSndArcStrmPlayer *player);
static void ProcessCommand(StrmCommand *command);
static void SetFileFuncs(NNSSndArcStrmPlayer *player, u32 fileId);
static BOOL OpenRomFile(NNSSndArcStrmPlayer *player, u32 fileId);
static void CloseRomFile(NNSSndArcStrmPlayer *player);
static int ReadRomFile(NNSSndArcStrmPlayer *player, void *buffer, int size, u32 offset);
static void CancelRomFile(NNSSndArcStrmPlayer *player);
static BOOL OpenMemFile(NNSSndArcStrmPlayer *player, u32 fileId);
static void CloseMemFile(NNSSndArcStrmPlayer *player);
static int ReadMemFile(NNSSndArcStrmPlayer *player, void *buffer, int size, u32 offset);
static void CancelMemFile(NNSSndArcStrmPlayer *player);
static void ThreadProc(void *arg);

static const s8 sAdpcmIndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8,
};

static const s16 sAdpcmStepTable[STRM_ADPCM_INDEX_MAX + 1] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

// prepareThread, when set, fills the first block of each stream instead of the stream thread. The decoding buffer
// holds a block of ADPCM data while it is decoded, under sDecodeMutex
static struct {
    BOOL initialized;
    StrmThread *prepareThread;
    void *decodeBuffer;
} sArcStrm;
static NNSFndList sFreeCommandList;
static OSMutex sDecodeMutex;
static StrmCommand sStrmCommands[STRM_COMMAND_NUM];
static u32 sDecodeBuffer[STRM_ADPCM_BUFFER_SIZE / sizeof(u32)];
static NNSSndArcStrmPlayer sPlayers[STRM_PLAYER_NUM];
static StrmThread sThread;

void NNS_SndArcStrmInit(u32 threadPrio, NNSSndHeapHandle heap) {
    int i;

    if (sArcStrm.initialized) {
        SetupPlayers(heap);
        return;
    }
    sArcStrm.initialized = TRUE;

    NNS_FndInitList(&sFreeCommandList, 0);
    for (i = 0; i < STRM_COMMAND_NUM; i++) {
        NNS_FndAppendListObject(&sFreeCommandList, &sStrmCommands[i]);
    }
    OS_InitMutex(&sDecodeMutex);
    sArcStrm.decodeBuffer = sDecodeBuffer;

    for (i = 0; i < STRM_PLAYER_NUM; i++) {
        NNSSndArcStrmPlayer *player = &sPlayers[i];

        player->flags.active = FALSE;
        finit(&player->file);
        NNS_SndStrmInit(&player->stream);
        player->playerNo = i;
        player->numChannels = 0;
        player->buffer = NULL;
        player->bufSize = 0;
        player->allocCount = 0;
    }

    SetupPlayers(heap);
    CreateThread(&sThread, threadPrio);
}

// Gives each player its channels from the archive's stream player info and, with a heap, its buffer
static BOOL SetupPlayers(NNSSndHeapHandle heap) {
    int playerNo;
    int i;

    for (playerNo = 0; playerNo < STRM_PLAYER_NUM; playerNo++) {
        NNSSndArcStrmPlayer *player = &sPlayers[playerNo];
        const NNSSndArcStrmPlayerInfo *info = NNS_SndArcGetStrmPlayerInfo(playerNo);

        if (info == NULL) {
            continue;
        }
        player->numChannels = info->numChannels;
        for (i = 0; i < info->numChannels; i++) {
            player->chNoList[i] = info->chNoList[i];
        }

        if (heap != NNS_SND_HEAP_INVALID_HANDLE) {
            u32 size = player->numChannels * STRM_BUFFER_SIZE_PER_CH;
            void *buffer = NNS_SndHeapAlloc(heap, size, HeapDisposeCallback, (u32)player, 0);

            if (buffer == NULL) {
                return FALSE;
            }
            ShutdownStrmPlayer(player);
            player->buffer = buffer;
            player->bufSize = size;
        }
    }
    return TRUE;
}

BOOL NNS_SndArcStrmPrepare(NNSSndStrmHandle *handle, int strmNo, u32 offset) {
    const NNSSndArcStrmInfo *info = NNS_SndArcGetStrmInfo(strmNo);

    if (info == NULL) {
        return FALSE;
    }
    return PreparePlayer(handle, info, info->playerNo, info->playerPrio, strmNo, offset, NULL, NULL, NULL, NULL);
}

// A check through this inline loads the handle's player again for the use after it
static inline BOOL IsValidHandle(const NNSSndStrmHandle *handle) {
    return handle->player != NULL;
}

void NNS_SndArcStrmPreparedStart(NNSSndStrmHandle *handle) {
    if (IsValidHandle(handle)) {
        handle->player->flags.startReq = TRUE;
    }
}

BOOL NNS_SndArcStrmStart(NNSSndStrmHandle *handle, int strmNo, u32 offset) {
    if (!NNS_SndArcStrmPrepare(handle, strmNo, offset)) {
        return FALSE;
    }
    NNS_SndArcStrmPreparedStart(handle);
    return TRUE;
}

void NNS_SndArcStrmStop(NNSSndStrmHandle *handle, int fadeFrames) {
    if (IsValidHandle(handle)) {
        StopPlayer(handle->player, fadeFrames);
    }
}

void NNS_SndArcStrmInitHandle(NNSSndStrmHandle *handle) {
    handle->player = NULL;
}

void NNS_SndArcStrmReleaseHandle(NNSSndStrmHandle *handle) {
    if (handle->player != NULL) {
        handle->player->handle = NULL;
        handle->player = NULL;
    }
}

u32 NNS_SndArcStrmGetCurrentPlayingPos(NNSSndStrmHandle *handle) {
    NNSSndArcStrmPlayer *player;

    if (!IsValidHandle(handle)) {
        return 0;
    }
    player = handle->player;
    // The rate is divided as a u32: as an int it would be sign-extended to 64 bits
    return (u64)player->curPos * 1000 / (u32)player->header.head.sampleRate;
}

BOOL NNS_SndArcStrmAllocChannel(int playerNo) {
    NNSSndArcStrmPlayer *player = &sPlayers[playerNo];

    if (player->flags.active) {
        return FALSE;
    }
    if (!AllocChannel(player, player->numChannels, player->chNoList)) {
        return FALSE;
    }
    return TRUE;
}

void NNS_SndArcStrmFreeChannel(int playerNo) {
    FreeChannel(&sPlayers[playerNo]);
}

// A volume 0 to 127 in decibels, as NitroSDK's SND_CalcDecibel gives it
static inline s16 CalcDecibel(int scale) {
    return VOLUME_DB_TABLE[scale];
}

void NNSi_SndArcStrmMain(void) {
    NNSSndArcStrmPlayer *player;
    int volume;
    int i;

    for (i = 0, player = sPlayers; i < STRM_PLAYER_NUM; i++, player++) {
        if (!player->flags.active) {
            continue;
        }
        if (player->endWait == 0) {
            ShutdownStrmPlayer(player);
            continue;
        }

        if (player->flags.startReq && player->prepared) {
            NNS_SndStrmStart(&player->stream);
            player->flags.playing = TRUE;
            player->flags.startReq = FALSE;
        }

        if (player->flags.playing) {
            NNSi_SndFaderUpdate(&player->fader);
            volume = CalcDecibel(NNSi_SndFaderGet(&player->fader) >> 8) + CalcDecibel(player->volume) +
                     CalcDecibel(player->userVolume);
            if (volume != player->curVolume) {
                NNS_SndStrmSetVolume(&player->stream, volume);
                player->curVolume = volume;
            }

            if (player->flags.autoStop && NNSi_SndFaderIsFinished(&player->fader)) {
                ShutdownStrmPlayer(player);
            }
        }
    }
}

// Takes a player for handle, stopping what it plays if prio is at least its priority
static NNSSndArcStrmPlayer *AllocPlayer(NNSSndStrmHandle *handle, int playerNo, int prio) {
    NNSSndArcStrmPlayer *player;

    if (handle->player != NULL) {
        NNS_SndArcStrmReleaseHandle(handle);
    }

    player = &sPlayers[playerNo];
    if (player->buffer == NULL) {
        return NULL;
    }
    if (player->flags.active) {
        if (prio < player->prio) {
            return NULL;
        }
        ShutdownStrmPlayer(player);
    }

    player->prio = prio;
    player->flags.active = TRUE;
    player->handle = handle;
    handle->player = player;
    return player;
}

static void DeactivatePlayer(NNSSndArcStrmPlayer *player) {
    if (player->handle != NULL) {
        player->handle->player = NULL;
        player->handle = NULL;
    }
    player->flags.active = FALSE;
    player->flags.startReq = FALSE;
    player->flags.playing = FALSE;
}

// Opens the stream on a player and sets its stream up, from offset milliseconds in
static BOOL PreparePlayer(NNSSndStrmHandle *handle, const NNSSndArcStrmInfo *info, int playerNo, int prio, int strmNo,
                          u32 offset, NNSSndStrmCallback strmCallback, void *strmCallbackArg,
                          NNSSndArcStrmCallback callback, void *callbackArg) {
    NNSSndArcStrmPlayer *player;
    NNSSndStrmFormat format;
    int numChannels;

    player = AllocPlayer(handle, playerNo, prio);
    if (player == NULL) {
        return FALSE;
    }

    SetFileFuncs(player, info->fileId);
    if (!player->open(player, info->fileId)) {
        DeactivatePlayer(player);
        return FALSE;
    }

    player->curPos = (u64)player->header.head.sampleRate * offset / 1000;
    if (player->curPos != 0 && player->header.head.format == STRM_FORMAT_ADPCM) {
        player->flags.seeking = TRUE;
    } else {
        player->flags.seeking = FALSE;
    }

    player->endWait = STRM_END_WAIT;
    player->flags.dataEnd = FALSE;
    player->flags.playing = FALSE;
    player->prepared = FALSE;
    player->flags.startReq = FALSE;
    player->flags.autoStop = FALSE;
    player->cmdCount = 0;
    player->strmCallback = strmCallback;
    player->strmCallbackArg = strmCallbackArg;
    player->callback = callback;
    player->callbackArg = callbackArg;
    player->strmNo = strmNo;
    player->curVolume = 0;
    player->volume = info->volume;
    player->userVolume = 127;
    NNSi_SndFaderInit(&player->fader);
    NNSi_SndFaderSet(&player->fader, 127 << 8, 1);

    switch (player->header.head.format) {
    case STRM_FORMAT_PCM8:
        format = NNS_SND_STRM_FORMAT_PCM8;
        break;
    case STRM_FORMAT_PCM16:
    case STRM_FORMAT_ADPCM:
        format = NNS_SND_STRM_FORMAT_PCM16;
        break;
    }

    numChannels = player->header.head.numChannels;
    if (info->flags & NNS_SND_ARC_STRM_FLAG_STEREO) {
        numChannels = 2;
    }
    if (numChannels > player->numChannels) {
        numChannels = player->numChannels;
    }
    player->flags.singleChannel = (numChannels == 1);

    if (!AllocChannel(player, numChannels, player->chNoList)) {
        player->close(player);
        DeactivatePlayer(player);
        return FALSE;
    }
    if (!NNS_SndStrmSetup(&player->stream, format, player->buffer, player->bufSize * numChannels / player->numChannels,
                          player->header.head.timer, STRM_INTERVAL, ArcStrmCallback, player)) {
        FreeChannel(player);
        player->close(player);
        DeactivatePlayer(player);
        return FALSE;
    }

    if (numChannels == 2) {
        NNS_SndStrmSetChannelPan(&player->stream, 0, 0);
        NNS_SndStrmSetChannelPan(&player->stream, 1, 127);
    }
    return TRUE;
}

// Fades the player out over fadeFrames, then stops it
static void StopPlayer(NNSSndArcStrmPlayer *player, int fadeFrames) {
    if (!player->flags.playing) {
        ShutdownStrmPlayer(player);
        return;
    }
    if (fadeFrames == 0) {
        ShutdownStrmPlayer(player);
        return;
    }
    NNSi_SndFaderSet(&player->fader, 0, fadeFrames);
    player->flags.autoStop = TRUE;
    player->prio = 0;
}

static void ShutdownStrmPlayer(NNSSndArcStrmPlayer *player) {
    OS_LockMutex(&sThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_LockMutex(&sArcStrm.prepareThread->mutex);
    }

    if (player->flags.playing) {
        NNS_SndStrmStop(&player->stream);
    }
    if (player->flags.active) {
        player->cancel(player);
    }
    ClosePlayer(player);

    OS_UnlockMutex(&sThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_UnlockMutex(&sArcStrm.prepareThread->mutex);
    }
}

static void ClosePlayer(NNSSndArcStrmPlayer *player) {
    if (player->flags.active) {
        FreeChannel(player);
        player->close(player);
        ClearCommands(&sThread.commandList, player);
        if (sArcStrm.prepareThread != NULL) {
            ClearCommands(&sArcStrm.prepareThread->commandList, player);
        }
        DeactivatePlayer(player);
    }
}

static BOOL AllocChannel(NNSSndArcStrmPlayer *player, int numChannels, const u8 chNoList[]) {
    if (player->allocCount == 0) {
        if (!NNS_SndStrmAllocChannel(&player->stream, numChannels, chNoList)) {
            return FALSE;
        }
    }
    player->allocCount++;
    return TRUE;
}

static void FreeChannel(NNSSndArcStrmPlayer *player) {
    if (player->allocCount != 0) {
        player->allocCount--;
        if (player->allocCount == 0) {
            NNS_SndStrmFreeChannel(&player->stream);
        }
    }
}

static void CreateThread(StrmThread *thread, u32 prio) {
    scheduler_init_thread(&thread->thread, ThreadProc, thread, thread->stack + STRM_THREAD_STACK_SIZE / sizeof(u64),
                          STRM_THREAD_STACK_SIZE, prio);
    NNS_FndInitList(&thread->commandList, 0);
    OS_InitMutex(&thread->mutex);
    OS_InitThreadQueue(&thread->queue);
    scheduler_start_thread(&thread->thread);
}

// Drops the commands of player from list
static void ClearCommands(NNSFndList *list, NNSSndArcStrmPlayer *player) {
    u32 enabled = CPU_IRQDisable();
    StrmCommand *command = NNS_FndGetNextListObject(list, NULL);

    while (command != NULL) {
        StrmCommand *next = NNS_FndGetNextListObject(list, command);

        if (command->player == player) {
            NNS_FndRemoveListObject(list, command);
            FreeCommand(command);
        }
        command = next;
    }
    CPU_SetIRQMask(enabled);
}

static StrmCommand *PopCommand(NNSFndList *list) {
    u32 enabled = CPU_IRQDisable();
    StrmCommand *command = NNS_FndGetNextListObject(list, NULL);

    if (command != NULL) {
        NNS_FndRemoveListObject(list, command);
        command->player->cmdCount--;
    }
    CPU_SetIRQMask(enabled);
    return command;
}

static StrmCommand *AllocCommand(void) {
    u32 enabled = CPU_IRQDisable();
    StrmCommand *command = NNS_FndGetNextListObject(&sFreeCommandList, NULL);

    if (command != NULL) {
        NNS_FndRemoveListObject(&sFreeCommandList, command);
    }
    CPU_SetIRQMask(enabled);
    return command;
}

static void FreeCommand(StrmCommand *command) {
    u32 enabled = CPU_IRQDisable();

    NNS_FndAppendListObject(&sFreeCommandList, command);
    CPU_SetIRQMask(enabled);
}

// The sound heap frees a player's buffer
static void HeapDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    NNSSndArcStrmPlayer *player = (NNSSndArcStrmPlayer *)data1;

    if (mem != player->buffer) {
        return;
    }

    OS_LockMutex(&sThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_LockMutex(&sArcStrm.prepareThread->mutex);
    }

    ShutdownStrmPlayer(player);
    player->buffer = NULL;
    player->bufSize = 0;
    player->numChannels = 0;
    if (player->allocCount > 0) {
        NNS_SndStrmFreeChannel(&player->stream);
        player->allocCount = 0;
    }

    OS_UnlockMutex(&sThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_UnlockMutex(&sArcStrm.prepareThread->mutex);
    }
}

// The stream asks for a block: queue it for a thread, dropping the oldest of the player's when two are waiting
static void ArcStrmCallback(NNSSndStrmCallbackStatus status, int numChannels, void *buffer[], u32 len,
                            NNSSndStrmFormat format, void *arg) {
    NNSSndArcStrmPlayer *player = arg;
    StrmCommand *command;
    StrmThread *thread;
    int i;

    if (player->cmdCount >= 2) {
        for (command = NNS_FndGetNextListObject(&sThread.commandList, NULL); command != NULL;
             command = NNS_FndGetNextListObject(&sThread.commandList, command)) {
            if (command->player == player) {
                break;
            }
        }
        for (i = 0; i < command->numChannels; i++) {
            sys_memset(command->buffer[i], 0, command->len);
        }
        NNS_FndRemoveListObject(&sThread.commandList, command);
        player->cmdCount--;
        FreeCommand(command);
    }

    command = AllocCommand();
    command->player = player;
    command->status = status;
    command->numChannels = numChannels;
    for (i = 0; i < numChannels; i++) {
        command->buffer[i] = buffer[i];
    }
    command->len = len;

    thread = &sThread;
    if (status == NNS_SND_STRM_CALLBACK_SETUP && sArcStrm.prepareThread != NULL) {
        thread = sArcStrm.prepareThread;
    }
    player->cmdCount++;
    NNS_FndAppendListObject(&thread->commandList, command);
    OS_WakeupThread(&thread->queue);
}

// The data ended: ask the callback for the stream to play next, which must have the same sample rate, and be PCM8
// if this one is
static void StartNextStrm(NNSSndArcStrmPlayer *player) {
    NNSSndArcStrmCallbackInfo info;
    NNSSndArcStrmCallbackParam param;
    const NNSSndArcStrmInfo *strmInfo;
    u8 format;
    u16 sampleRate;

    info.playerNo = player->playerNo;
    info.strmNo = player->strmNo;
    param.strmNo = player->strmNo;
    param.offset = 0;
    if (!player->callback(NNS_SND_ARC_STRM_CALLBACK_DATA_END, &info, &param, player->callbackArg)) {
        return;
    }
    strmInfo = NNS_SndArcGetStrmInfo(param.strmNo);
    if (strmInfo == NULL) {
        return;
    }

    format = player->header.head.format;
    sampleRate = player->header.head.sampleRate;
    player->close(player);
    SetFileFuncs(player, strmInfo->fileId);
    if (!player->open(player, strmInfo->fileId)) {
        return;
    }
    if (sampleRate != player->header.head.sampleRate) {
        return;
    }
    if (format == STRM_FORMAT_PCM8 && player->header.head.format != STRM_FORMAT_PCM8) {
        return;
    }
    if (format != STRM_FORMAT_PCM8 && player->header.head.format == STRM_FORMAT_PCM8) {
        return;
    }

    player->strmNo = param.strmNo;
    player->curPos = (u64)player->header.head.sampleRate * param.offset / 1000;
    if (player->curPos != 0 && player->header.head.format == STRM_FORMAT_ADPCM) {
        player->flags.seeking = TRUE;
    } else {
        player->flags.seeking = FALSE;
    }
    player->flags.dataEnd = FALSE;
}

static inline s16 DecodeAdpcm(AdpcmState *state, int nibble) {
    int index = state->index;
    int pcm = state->pcm;
    int step = sAdpcmStepTable[index];
    int diff = step >> 3;

    if (nibble & 4) {
        diff += step;
    }
    if (nibble & 2) {
        diff += step >> 1;
    }
    if (nibble & 1) {
        diff += step >> 2;
    }
    if (nibble & 8) {
        pcm -= diff;
        if (pcm < -0x8000) {
            pcm = -0x8000;
        }
    } else {
        pcm += diff;
        if (pcm > 0x7fff) {
            pcm = 0x7fff;
        }
    }

    index += sAdpcmIndexTable[nibble];
    if (index < 0) {
        index = 0;
    } else if (index > STRM_ADPCM_INDEX_MAX) {
        index = STRM_ADPCM_INDEX_MAX;
    }

    state->pcm = pcm;
    state->index = index;
    return pcm;
}

// Fills a block: reads the stream's data from where it is, by block of the file, decoding ADPCM to PCM16
static void ProcessCommand(StrmCommand *command) {
    NNSSndArcStrmPlayer *player = command->player;
    BOOL loop;
    u32 bufOffset;
    u32 len;
    u32 blockSize;
    u32 blockOffset;
    u32 fileOffset;
    u32 samples;
    u32 dataSize;
    u32 readSize;
    u8 *dest;
    u32 block;
    u32 blockSamples;
    u32 readOffset;
    int ch;

    if (player->flags.dataEnd && player->endWait > 0) {
        player->endWait--;
    }

    bufOffset = 0;
    len = command->len;
    while (len != 0) {
        if (player->flags.dataEnd) {
            for (ch = 0; ch < command->numChannels; ch++) {
                sys_memset((u8 *)command->buffer[ch] + bufOffset, 0, len);
            }
            break;
        }

        block = player->curPos / player->header.head.blockSamples;
        if (block < player->header.head.numBlocks - 1) {
            blockSize = player->header.head.blockSize;
            blockSamples = player->header.head.blockSamples;
        } else {
            blockSize = player->header.head.lastBlockSize;
            blockSamples = player->header.head.lastBlockSamples;
        }
        blockOffset = player->curPos - block * player->header.head.blockSamples;

        samples = len;
        if (player->header.head.format != STRM_FORMAT_PCM8) {
            samples = len / 2;
        }
        if (player->flags.seeking) {
            if (blockOffset == 0) {
                player->flags.seeking = FALSE;
            } else {
                samples = blockOffset;
                blockOffset = 0;
            }
        }

        loop = FALSE;
        if (blockOffset + samples >= blockSamples) {
            samples = blockSamples - blockOffset;
            if (block >= player->header.head.numBlocks - 1) {
                if (player->header.head.loopFlag) {
                    loop = TRUE;
                } else {
                    player->flags.dataEnd = TRUE;
                }
            }
        }

        readOffset = blockOffset;
        dataSize = samples;
        switch (player->header.head.format) {
        case STRM_FORMAT_PCM8:
            readSize = samples;
            break;
        case STRM_FORMAT_PCM16:
            readOffset *= 2;
            dataSize = samples * 2;
            readSize = dataSize;
            break;
        case STRM_FORMAT_ADPCM:
            readOffset /= 2;
            readSize = (blockOffset + samples + 1) / 2 - readOffset;
            if (blockOffset == 0) {
                readSize += STRM_ADPCM_HEADER_SIZE;
            } else {
                readOffset += STRM_ADPCM_HEADER_SIZE;
            }
            dataSize *= 2;
            break;
        }
        fileOffset = readOffset + player->header.head.numChannels * (block * player->header.head.blockSize);
        fileOffset += player->header.head.dataOffset;

        for (ch = 0; ch < command->numChannels; ch++) {
            dest = (u8 *)command->buffer[ch] + bufOffset;

            if (ch < player->header.head.numChannels) {
                void *readBuffer = dest;

                if (player->header.head.format == STRM_FORMAT_ADPCM) {
                    OS_LockMutex(&sDecodeMutex);
                    readBuffer = sArcStrm.decodeBuffer;
                }
                if (player->read(player, readBuffer, readSize, fileOffset + ch * blockSize) != readSize) {
                    dataSize = 0;
                    samples = 0;
                    loop = FALSE;
                    player->flags.dataEnd = TRUE;
                    if (player->header.head.format == STRM_FORMAT_ADPCM) {
                        OS_UnlockMutex(&sDecodeMutex);
                    }
                    break;
                }

                if (player->header.head.format == STRM_FORMAT_ADPCM) {
                    AdpcmState *state = &player->adpcm[ch];
                    const u8 *src = sArcStrm.decodeBuffer;
                    u32 pos;

                    if (blockOffset == 0) {
                        *state = *(const AdpcmState *)src;
                        src += STRM_ADPCM_HEADER_SIZE;
                    }
                    pos = blockOffset;
                    if (blockOffset & 1) {
                        *(s16 *)dest = DecodeAdpcm(state, (*src >> 4) & 0xf);
                        dest += sizeof(s16);
                        pos++;
                        src++;
                    }
                    while (pos < ((blockOffset + samples) & ~1)) {
                        *(s16 *)dest = DecodeAdpcm(state, *src & 0xf);
                        *(s16 *)(dest + sizeof(s16)) = DecodeAdpcm(state, (*src >> 4) & 0xf);
                        pos += 2;
                        dest += 2 * sizeof(s16);
                        src++;
                    }
                    if (pos < blockOffset + samples) {
                        *(s16 *)dest = DecodeAdpcm(state, *src & 0xf);
                    }
                    OS_UnlockMutex(&sDecodeMutex);
                }
            } else if (player->flags.singleChannel) {
                sys_memset(dest, 0, dataSize);
            } else {
                sys_memcpy((u8 *)command->buffer[0] + bufOffset, dest, dataSize);
            }
        }

        if (player->flags.seeking) {
            player->flags.seeking = FALSE;
        } else {
            if (loop) {
                player->curPos = player->header.head.loopStart;
            } else {
                player->curPos += samples;
            }
            bufOffset += dataSize;
            len -= dataSize;
            if (player->flags.dataEnd && player->callback != NULL) {
                StartNextStrm(player);
            }
        }
    }

    if (player->strmCallback != NULL) {
        player->strmCallback(command->status, command->numChannels, command->buffer, command->len,
                             player->header.head.format == STRM_FORMAT_PCM8 ? NNS_SND_STRM_FORMAT_PCM8
                                                                            : NNS_SND_STRM_FORMAT_PCM16,
                             player->strmCallbackArg);
    }
    for (ch = 0; ch < command->numChannels; ch++) {
        cp15_flushDC(command->buffer[ch], command->len);
    }
    if (command->status == NNS_SND_STRM_CALLBACK_SETUP) {
        player->prepared = TRUE;
    }
}

// A file in the ROM is read through the archive's file; one loaded is copied from memory
static void SetFileFuncs(NNSSndArcStrmPlayer *player, u32 fileId) {
    if (NNS_SndArcGetFileAddress(fileId) == NULL) {
        player->open = OpenRomFile;
        player->close = CloseRomFile;
        player->read = ReadRomFile;
        player->cancel = CancelRomFile;
    } else {
        player->open = OpenMemFile;
        player->close = CloseMemFile;
        player->read = ReadMemFile;
        player->cancel = CancelMemFile;
    }
}

static BOOL OpenRomFile(NNSSndArcStrmPlayer *player, u32 fileId) {
    const char *path;

    if (NNS_SndArcReadFile(fileId, &player->header, sizeof(player->header), 0) != sizeof(player->header)) {
        return FALSE;
    }
    if (!romfs_fopen_id(&player->file, NNSi_SndArcGetFileID())) {
        path = NNSi_SndArcGetFilePath();
        if (path != NULL && !romfs_fopen_core(&player->file, path, FS_FILEMODE_R)) {
            return FALSE;
        }
    }
    player->fileOffset = NNS_SndArcGetFileOffset(fileId);
    return TRUE;
}

static void CloseRomFile(NNSSndArcStrmPlayer *player) {
    romfs_fclose(&player->file);
}

static int ReadRomFile(NNSSndArcStrmPlayer *player, void *buffer, int size, u32 offset) {
    romfs_fseek(&player->file, player->fileOffset + offset, FS_SEEK_SET);
    return romfs_fread(&player->file, buffer, size);
}

static void CancelRomFile(NNSSndArcStrmPlayer *player) {
    FS_CancelFile(&player->file);
}

static BOOL OpenMemFile(NNSSndArcStrmPlayer *player, u32 fileId) {
    player->fileAddress = NNS_SndArcGetFileAddress(fileId);
    sys_memcpy(player->fileAddress, &player->header, sizeof(player->header));
    return TRUE;
}

static void CloseMemFile(NNSSndArcStrmPlayer *player) {
}

static int ReadMemFile(NNSSndArcStrmPlayer *player, void *buffer, int size, u32 offset) {
    sys_memcpy(player->fileAddress + offset, buffer, size);
    return size;
}

static void CancelMemFile(NNSSndArcStrmPlayer *player) {
}

// Sleeps until woken, then fills the blocks of its list
static void ThreadProc(void *arg) {
    StrmThread *thread = arg;

    while (TRUE) {
        StrmCommand *command;

        scheduler_yield(&thread->queue);
        while (TRUE) {
            OS_LockMutex(&thread->mutex);
            command = PopCommand(&thread->commandList);
            if (command == NULL) {
                OS_UnlockMutex(&thread->mutex);
                break;
            }
            ProcessCommand(command);
            FreeCommand(command);
            OS_UnlockMutex(&thread->mutex);
        }
    }
}
