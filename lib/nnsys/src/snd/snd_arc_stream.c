#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/snd.h"
#include "nnsys/fnd.h"
#include "snd_internal.h"

// NitroSystem's sound archive stream players (NNS_SndArcStrm): a stream of the archive played on one of its stream
// players, its file read and, for IMA-ADPCM, decoded by a thread of its own as the stream asks for each block. The
// SDK's file is sndarc_stream.c. The names are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them,
// except the STRM_ macros, StrmFileHeader, StrmHead and sArcStrm, which are ours (sArcStrm groups three of
// NitroSystem's statics); what is newer than that reconstruction, such as the bigger stream records, keeps our names
// too. swan names NNS_SndArcStrmInit, CreateThread, StrmThread and sArcStrm NNS_SndStreamInit, NNS_SndStreamThreadInit,
// NNS_SndStreamThreadProc and g_NNSSndStreamExists; the rest swan doesn't name. The block filling follows the public
// decompilations of NitroSystem that pret's pokeplatinum and pokeheartgold build (github.com/ntrtwl/NitroSystem) and of
// Kingdom Hearts Re:coded (github.com/kh-recoded-decomp/kh-recoded-decomp)

#define NNS_SND_STRM_PLAYER_NUM 4
#define COMMAND_BUFFER_NUM 8
#define STRM_CHANNEL_MAX 6
#define STRM_BUFFER_SIZE_PER_CH 0x800
#define STRM_THREAD_STACK_SIZE 0x1000
#define STRM_INTERVAL 4
#define STRM_END_WAIT 4
#define BLOCK_SIZE 0x200
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
    u8 pad_;
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
    s16 prevSample;
    u8 prevIndex;
} AdpcmState;

typedef struct NNSSndStrmPlayer NNSSndStrmPlayer;

// How a player reads its file: from the ROM, or from memory when the file is loaded
typedef BOOL (*StrmFileOpenFunc)(NNSSndStrmPlayer *player, u32 fileId);
typedef void (*StrmFileCloseFunc)(NNSSndStrmPlayer *player);
typedef int (*StrmFileReadFunc)(NNSSndStrmPlayer *player, void *buffer, int size, u32 offset);
typedef void (*StrmFileCancelFunc)(NNSSndStrmPlayer *player);

// endWait counts the blocks played after the data ended before the player stops; prepared is set once the first
// block is read. volume is the archive's, userVolume ours, both indices of the decibel table
struct NNSSndStrmPlayer {
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
typedef struct LoadCommand {
    NNSFndLink link;
    NNSSndStrmPlayer *player;
    NNSSndStrmCallbackStatus status;
    int numChannels;
    void *buffer[STRM_CHANNEL_MAX];
    u32 bufLen;
} LoadCommand;

// A thread that fills blocks, woken when a command is added to its list
typedef struct NNSSndStrmThread {
    OSThread thread;
    u64 stack[STRM_THREAD_STACK_SIZE / sizeof(u64)];
    OSThreadQueue threadQ;
    OSMutex mutex;
    NNSFndList commandList;
} NNSSndStrmThread;

static BOOL NNS_SndArcStrmSetupPlayer(NNSSndHeapHandle heap);
static NNSSndStrmPlayer *AllocPlayer(NNSSndStrmHandle *handle, int playerNo, int prio);
static void FreePlayer(NNSSndStrmPlayer *player);
static BOOL PrepareStrm(NNSSndStrmHandle *handle, const NNSSndArcStrmInfo *info, int playerNo, int prio, int strmNo,
                        u32 offset, NNSSndStrmCallback strmCallback, void *strmCallbackArg,
                        NNSSndArcStrmCallback callback, void *callbackArg);
static void StopStrm(NNSSndStrmPlayer *player, int fadeFrames);
static void ForceStopStrm(NNSSndStrmPlayer *player);
static void ShutdownPlayer(NNSSndStrmPlayer *player);
static BOOL AllocChannel(NNSSndStrmPlayer *player, int numChannels, const u8 chNoList[]);
static void FreeChannel(NNSSndStrmPlayer *player);
static void CreateThread(NNSSndStrmThread *thread, u32 prio);
static void RemoveCommandByPlayer(NNSFndList *list, NNSSndStrmPlayer *player);
static LoadCommand *ReadCommandBuffer(NNSFndList *list);
static LoadCommand *AllocCommandBuffer(void);
static void FreeCommandBuffer(LoadCommand *command);
static void DisposeCallback(void *mem, u32 size, uptr data1, uptr data2);
static void StrmCallback(NNSSndStrmCallbackStatus status, int numChannels, void *buffer[], u32 len,
                         NNSSndStrmFormat format, void *arg);
static void OnDataEnd(NNSSndStrmPlayer *player);
static void MakeWaveData(LoadCommand *command);
static void SetupStreamFunction(NNSSndStrmPlayer *player, u32 fileId);
static BOOL OpenFileStream(NNSSndStrmPlayer *player, u32 fileId);
static void CloseFileStream(NNSSndStrmPlayer *player);
static int ReadFileStream(NNSSndStrmPlayer *player, void *buffer, int size, u32 offset);
static void CancelFileStream(NNSSndStrmPlayer *player);
static BOOL OpenMemoryStream(NNSSndStrmPlayer *player, u32 fileId);
static void CloseMemoryStream(NNSSndStrmPlayer *player);
static int ReadMemoryStream(NNSSndStrmPlayer *player, void *buffer, int size, u32 offset);
static void CancelMemoryStream(NNSSndStrmPlayer *player);
static void StrmThread(void *arg);

static const s8 cAdpcmIndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8,
};

static const s16 cAdpcmStepSizeTable[STRM_ADPCM_INDEX_MAX + 1] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

// prepareThread, when set, fills the first block of each stream instead of the stream thread. The decoding buffer
// holds a block of ADPCM data while it is decoded, under sDecodeBufferMutex
static struct {
    BOOL initialized;
    NNSSndStrmThread *prepareThread;
    void *decodeBuffer;
} sArcStrm;
static NNSFndList sFreeCommandList;
static OSMutex sDecodeBufferMutex;
static LoadCommand sLoadCommandBuffer[COMMAND_BUFFER_NUM];
// The decoding buffer's explicit alignment is a guess, which the ROM can't show: 4 to 16 fit its address (0x1b0
// bytes into a section at 0x02149950, so not 32). The card reads blocks into it, and NNS_SndArcStrmInit, which
// takes the buffer's address, keeps its loops entered with a branch to the test only with an alignment
static u32 sDecodeBufferArea[BLOCK_SIZE / sizeof(u32)] ATTRIBUTE_ALIGN(4);
static NNSSndStrmPlayer sStrmPlayer[NNS_SND_STRM_PLAYER_NUM];
static NNSSndStrmThread sStrmThread;

void NNS_SndArcStrmInit(u32 threadPrio, NNSSndHeapHandle heap) {
    int i;
    int playerNo;

    if (sArcStrm.initialized) {
        NNS_SndArcStrmSetupPlayer(heap);
        return;
    }
    sArcStrm.initialized = TRUE;

    NNS_FndInitList(&sFreeCommandList, 0);
    for (i = 0; i < COMMAND_BUFFER_NUM; i++) {
        NNS_FndAppendListObject(&sFreeCommandList, &sLoadCommandBuffer[i]);
    }
    OS_InitMutex(&sDecodeBufferMutex);
    sArcStrm.decodeBuffer = sDecodeBufferArea;

    for (playerNo = 0; playerNo < NNS_SND_STRM_PLAYER_NUM; playerNo++) {
        NNSSndStrmPlayer *player = &sStrmPlayer[playerNo];

        player->flags.active = FALSE;
        FS_InitFile(&player->file);
        NNS_SndStrmInit(&player->stream);
        player->playerNo = playerNo;
        player->numChannels = 0;
        player->buffer = NULL;
        player->bufSize = 0;
        player->allocCount = 0;
    }

    NNS_SndArcStrmSetupPlayer(heap);
    CreateThread(&sStrmThread, threadPrio);
}

// Gives each player its channels from the archive's stream player info and, with a heap, its buffer
static BOOL NNS_SndArcStrmSetupPlayer(NNSSndHeapHandle heap) {
    int playerNo;
    int i;

    for (playerNo = 0; playerNo < NNS_SND_STRM_PLAYER_NUM; playerNo++) {
        NNSSndStrmPlayer *player = &sStrmPlayer[playerNo];
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
            void *buffer = NNS_SndHeapAlloc(heap, size, DisposeCallback, (uptr)player, 0);

            if (buffer == NULL) {
                return FALSE;
            }
            ForceStopStrm(player);
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
    return PrepareStrm(handle, info, info->playerNo, info->playerPrio, strmNo, offset, NULL, NULL, NULL, NULL);
}

// A check through this inline loads the handle's player again for the use after it
static inline BOOL NNS_SndStrmHandleIsValid(const NNSSndStrmHandle *handle) {
    return handle->player != NULL;
}

void NNS_SndArcStrmStartPrepared(NNSSndStrmHandle *handle) {
    if (NNS_SndStrmHandleIsValid(handle)) {
        handle->player->flags.startReq = TRUE;
    }
}

BOOL NNS_SndArcStrmStart(NNSSndStrmHandle *handle, int strmNo, u32 offset) {
    if (!NNS_SndArcStrmPrepare(handle, strmNo, offset)) {
        return FALSE;
    }
    NNS_SndArcStrmStartPrepared(handle);
    return TRUE;
}

void NNS_SndArcStrmStop(NNSSndStrmHandle *handle, int fadeFrames) {
    if (NNS_SndStrmHandleIsValid(handle)) {
        StopStrm(handle->player, fadeFrames);
    }
}

void NNS_SndStrmHandleInit(NNSSndStrmHandle *handle) {
    handle->player = NULL;
}

void NNS_SndStrmHandleRelease(NNSSndStrmHandle *handle) {
    if (handle->player != NULL) {
        handle->player->handle = NULL;
        handle->player = NULL;
    }
}

u32 NNS_SndArcStrmGetCurrentPlayingPos(NNSSndStrmHandle *handle) {
    NNSSndStrmPlayer *player;

    if (!NNS_SndStrmHandleIsValid(handle)) {
        return 0;
    }
    player = handle->player;
    // The rate is divided as a u32: as an int it would be sign-extended to 64 bits
    return (u64)player->curPos * 1000 / (u32)player->header.head.sampleRate;
}

BOOL NNS_SndArcStrmAllocChannel(int playerNo) {
    NNSSndStrmPlayer *player = &sStrmPlayer[playerNo];

    if (player->flags.active) {
        return FALSE;
    }
    if (!AllocChannel(player, player->numChannels, player->chNoList)) {
        return FALSE;
    }
    return TRUE;
}

void NNS_SndArcStrmFreeChannel(int playerNo) {
    FreeChannel(&sStrmPlayer[playerNo]);
}

// A volume 0 to 127 in decibels, as NitroSDK's SND_CalcDecibel gives it
static inline s16 SND_CalcDecibel(int scale) {
    return SNDi_DecibelTable[scale];
}

void NNSi_SndArcStrmMain(void) {
    NNSSndStrmPlayer *player;
    int volume;
    int i;

    for (i = 0, player = sStrmPlayer; i < NNS_SND_STRM_PLAYER_NUM; i++, player++) {
        if (!player->flags.active) {
            continue;
        }
        if (player->endWait == 0) {
            ForceStopStrm(player);
            continue;
        }

        if (player->flags.startReq && player->prepared) {
            NNS_SndStrmStart(&player->stream);
            player->flags.playing = TRUE;
            player->flags.startReq = FALSE;
        }

        if (player->flags.playing) {
            NNSi_SndFaderUpdate(&player->fader);
            volume = SND_CalcDecibel(NNSi_SndFaderGet(&player->fader) >> 8) + SND_CalcDecibel(player->volume) +
                     SND_CalcDecibel(player->userVolume);
            if (volume != player->curVolume) {
                NNS_SndStrmSetVolume(&player->stream, volume);
                player->curVolume = volume;
            }

            if (player->flags.autoStop && NNSi_SndFaderIsFinished(&player->fader)) {
                ForceStopStrm(player);
            }
        }
    }
}

// Takes a player for handle, stopping what it plays if prio is at least its priority
static NNSSndStrmPlayer *AllocPlayer(NNSSndStrmHandle *handle, int playerNo, int prio) {
    NNSSndStrmPlayer *player;

    if (handle->player != NULL) {
        NNS_SndStrmHandleRelease(handle);
    }

    player = &sStrmPlayer[playerNo];
    if (player->buffer == NULL) {
        return NULL;
    }
    if (player->flags.active) {
        if (prio < player->prio) {
            return NULL;
        }
        ForceStopStrm(player);
    }

    player->prio = prio;
    player->flags.active = TRUE;
    player->handle = handle;
    handle->player = player;
    return player;
}

static void FreePlayer(NNSSndStrmPlayer *player) {
    if (player->handle != NULL) {
        player->handle->player = NULL;
        player->handle = NULL;
    }
    player->flags.active = FALSE;
    player->flags.startReq = FALSE;
    player->flags.playing = FALSE;
}

// Opens the stream on a player and sets its stream up, from offset milliseconds in
static BOOL PrepareStrm(NNSSndStrmHandle *handle, const NNSSndArcStrmInfo *info, int playerNo, int prio, int strmNo,
                        u32 offset, NNSSndStrmCallback strmCallback, void *strmCallbackArg,
                        NNSSndArcStrmCallback callback, void *callbackArg) {
    NNSSndStrmPlayer *player;
    NNSSndStrmFormat format;
    int numChannels;

    player = AllocPlayer(handle, playerNo, prio);
    if (player == NULL) {
        return FALSE;
    }

    SetupStreamFunction(player, info->fileId);
    if (!player->open(player, info->fileId)) {
        FreePlayer(player);
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
    if (info->flags & NNS_SND_ARC_STRM_FORCE_STEREO) {
        numChannels = 2;
    }
    if (numChannels > player->numChannels) {
        numChannels = player->numChannels;
    }
    player->flags.singleChannel = (numChannels == 1);

    if (!AllocChannel(player, numChannels, player->chNoList)) {
        player->close(player);
        FreePlayer(player);
        return FALSE;
    }
    if (!NNS_SndStrmSetup(&player->stream, format, player->buffer, player->bufSize * numChannels / player->numChannels,
                          player->header.head.timer, STRM_INTERVAL, StrmCallback, player)) {
        FreeChannel(player);
        player->close(player);
        FreePlayer(player);
        return FALSE;
    }

    if (numChannels == 2) {
        NNS_SndStrmSetChannelPan(&player->stream, 0, 0);
        NNS_SndStrmSetChannelPan(&player->stream, 1, 127);
    }
    return TRUE;
}

// Fades the player out over fadeFrames, then stops it
static void StopStrm(NNSSndStrmPlayer *player, int fadeFrames) {
    if (!player->flags.playing) {
        ForceStopStrm(player);
        return;
    }
    if (fadeFrames == 0) {
        ForceStopStrm(player);
        return;
    }
    NNSi_SndFaderSet(&player->fader, 0, fadeFrames);
    player->flags.autoStop = TRUE;
    player->prio = 0;
}

static void ForceStopStrm(NNSSndStrmPlayer *player) {
    OS_LockMutex(&sStrmThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_LockMutex(&sArcStrm.prepareThread->mutex);
    }

    if (player->flags.playing) {
        NNS_SndStrmStop(&player->stream);
    }
    if (player->flags.active) {
        player->cancel(player);
    }
    ShutdownPlayer(player);

    OS_UnlockMutex(&sStrmThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_UnlockMutex(&sArcStrm.prepareThread->mutex);
    }
}

static void ShutdownPlayer(NNSSndStrmPlayer *player) {
    if (player->flags.active) {
        FreeChannel(player);
        player->close(player);
        RemoveCommandByPlayer(&sStrmThread.commandList, player);
        if (sArcStrm.prepareThread != NULL) {
            RemoveCommandByPlayer(&sArcStrm.prepareThread->commandList, player);
        }
        FreePlayer(player);
    }
}

static BOOL AllocChannel(NNSSndStrmPlayer *player, int numChannels, const u8 chNoList[]) {
    if (player->allocCount == 0) {
        if (!NNS_SndStrmAllocChannel(&player->stream, numChannels, chNoList)) {
            return FALSE;
        }
    }
    player->allocCount++;
    return TRUE;
}

static void FreeChannel(NNSSndStrmPlayer *player) {
    if (player->allocCount != 0) {
        player->allocCount--;
        if (player->allocCount == 0) {
            NNS_SndStrmFreeChannel(&player->stream);
        }
    }
}

static void CreateThread(NNSSndStrmThread *thread, u32 prio) {
    OS_CreateThread(&thread->thread, StrmThread, thread, thread->stack + STRM_THREAD_STACK_SIZE / sizeof(u64),
                    STRM_THREAD_STACK_SIZE, prio);
    NNS_FndInitList(&thread->commandList, 0);
    OS_InitMutex(&thread->mutex);
    OS_InitThreadQueue(&thread->threadQ);
    OS_WakeupThreadDirect(&thread->thread);
}

// Drops the commands of player from list
static void RemoveCommandByPlayer(NNSFndList *list, NNSSndStrmPlayer *player) {
    OSIntrMode enabled = OS_DisableInterrupts();
    LoadCommand *command = NNS_FndGetNextListObject(list, NULL);

    while (command != NULL) {
        LoadCommand *next = NNS_FndGetNextListObject(list, command);

        if (command->player == player) {
            NNS_FndRemoveListObject(list, command);
            FreeCommandBuffer(command);
        }
        command = next;
    }
    OS_RestoreInterrupts(enabled);
}

static LoadCommand *ReadCommandBuffer(NNSFndList *list) {
    OSIntrMode enabled = OS_DisableInterrupts();
    LoadCommand *command = NNS_FndGetNextListObject(list, NULL);

    if (command != NULL) {
        NNS_FndRemoveListObject(list, command);
        command->player->cmdCount--;
    }
    OS_RestoreInterrupts(enabled);
    return command;
}

static LoadCommand *AllocCommandBuffer(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    LoadCommand *command = NNS_FndGetNextListObject(&sFreeCommandList, NULL);

    if (command != NULL) {
        NNS_FndRemoveListObject(&sFreeCommandList, command);
    }
    OS_RestoreInterrupts(enabled);
    return command;
}

static void FreeCommandBuffer(LoadCommand *command) {
    OSIntrMode enabled = OS_DisableInterrupts();

    NNS_FndAppendListObject(&sFreeCommandList, command);
    OS_RestoreInterrupts(enabled);
}

// The sound heap frees a player's buffer
static void DisposeCallback(void *mem, u32 size, uptr data1, uptr data2) {
    NNSSndStrmPlayer *player = (NNSSndStrmPlayer *)data1;

    if (mem != player->buffer) {
        return;
    }

    OS_LockMutex(&sStrmThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_LockMutex(&sArcStrm.prepareThread->mutex);
    }

    ForceStopStrm(player);
    player->buffer = NULL;
    player->bufSize = 0;
    player->numChannels = 0;
    if (player->allocCount > 0) {
        NNS_SndStrmFreeChannel(&player->stream);
        player->allocCount = 0;
    }

    OS_UnlockMutex(&sStrmThread.mutex);
    if (sArcStrm.prepareThread != NULL) {
        OS_UnlockMutex(&sArcStrm.prepareThread->mutex);
    }
}

// The stream asks for a block: queue it for a thread, dropping the oldest of the player's when two are waiting
static void StrmCallback(NNSSndStrmCallbackStatus status, int numChannels, void *buffer[], u32 len,
                         NNSSndStrmFormat format, void *arg) {
    NNSSndStrmPlayer *player = arg;
    LoadCommand *command;
    NNSSndStrmThread *thread;
    int i;

    if (player->cmdCount >= 2) {
        for (command = NNS_FndGetNextListObject(&sStrmThread.commandList, NULL); command != NULL;
             command = NNS_FndGetNextListObject(&sStrmThread.commandList, command)) {
            if (command->player == player) {
                break;
            }
        }
        for (i = 0; i < command->numChannels; i++) {
            MI_CpuFill8(command->buffer[i], 0, command->bufLen);
        }
        NNS_FndRemoveListObject(&sStrmThread.commandList, command);
        player->cmdCount--;
        FreeCommandBuffer(command);
    }

    command = AllocCommandBuffer();
    command->player = player;
    command->status = status;
    command->numChannels = numChannels;
    for (i = 0; i < numChannels; i++) {
        command->buffer[i] = buffer[i];
    }
    command->bufLen = len;

    thread = &sStrmThread;
    if (status == NNS_SND_STRM_CALLBACK_SETUP && sArcStrm.prepareThread != NULL) {
        thread = sArcStrm.prepareThread;
    }
    player->cmdCount++;
    NNS_FndAppendListObject(&thread->commandList, command);
    OS_WakeupThread(&thread->threadQ);
}

// The data ended: ask the callback for the stream to play next, which must have the same sample rate, and be PCM8
// if this one is
static void OnDataEnd(NNSSndStrmPlayer *player) {
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
    SetupStreamFunction(player, strmInfo->fileId);
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

// Decodes one IMA-ADPCM sample. Written as the public decompilation github.com/ntrtwl/NitroSystem has it, with the
// locals assigned after their declarations, the sample before the index: initialized in their declarations, they give
// MakeWaveData other registers and a shorter frame
static inline s16 DecodeAdpcm(AdpcmState *state, int nibble) {
    int step;
    int pcm;
    int index;
    int diff;

    pcm = state->prevSample;
    index = state->prevIndex;

    step = cAdpcmStepSizeTable[index];

    diff = step >> 3;
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

    index += cAdpcmIndexTable[nibble];

    if (index < 0) {
        index = 0;
    } else if (index > STRM_ADPCM_INDEX_MAX) {
        index = STRM_ADPCM_INDEX_MAX;
    }

    state->prevSample = pcm;
    state->prevIndex = index;

    return pcm;
}

// Fills a block: reads the stream's data from where it is, by block of the file, decoding ADPCM to PCM16
static void MakeWaveData(LoadCommand *command) {
    NNSSndStrmPlayer *player = command->player;
    BOOL loop;
    // NitroSDK's u32, an unsigned long, as in sbc.c: as an unsigned int it takes a late spill slot instead of its
    // declared one (Kingdom Hearts Re:coded's decompilation has a long)
    unsigned long bufOffset;
    u32 len;
    u32 block;
    u32 blockSize;
    u32 blockSamples;
    u32 blockOffset;
    u32 readOffset;
    u32 fileOffset;
    u32 samples;
    u32 dataSize;
    // The read's result's type, which it is compared with: as a u32 it is a converted copy in a late slot
    int readSize;
    int ch;

    if (player->flags.dataEnd && player->endWait > 0) {
        player->endWait--;
    }

    bufOffset = 0;
    len = command->bufLen;
    while (len != 0) {
        if (player->flags.dataEnd) {
            for (ch = 0; ch < command->numChannels; ch++) {
                MI_CpuFill8((u8 *)command->buffer[ch] + bufOffset, 0, len);
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
            void *dest;
            void *readDest;

            dest = readDest = (u8 *)command->buffer[ch] + bufOffset;

            if (ch < player->header.head.numChannels) {
                int result;

                if (player->header.head.format == STRM_FORMAT_ADPCM) {
                    OS_LockMutex(&sDecodeBufferMutex);
                    readDest = sArcStrm.decodeBuffer;
                }
                result = player->read(player, readDest, readSize, fileOffset + ch * blockSize);
                if (result != readSize) {
                    dataSize = 0;
                    samples = 0;
                    loop = FALSE;
                    player->flags.dataEnd = TRUE;
                    if (player->header.head.format == STRM_FORMAT_ADPCM) {
                        OS_UnlockMutex(&sDecodeBufferMutex);
                    }
                    break;
                }

                if (player->header.head.format == STRM_FORMAT_ADPCM) {
                    AdpcmState *state = &player->adpcm[ch];
                    const u8 *src = sArcStrm.decodeBuffer;
                    s16 *out = dest;
                    u32 pos;
                    u32 end;

                    if (blockOffset == 0) {
                        // NitroSystem copies the header through a postincrement of the cast pointer, which
                        // this does in standard C: src steps past it before the copy
                        const AdpcmState *header = (const AdpcmState *)src;

                        src += STRM_ADPCM_HEADER_SIZE;
                        *state = *header;
                    }
                    end = blockOffset + samples;
                    pos = blockOffset;
                    if (pos & 1) {
                        *out++ = DecodeAdpcm(state, (*src >> 4) & 0xf);
                        pos++;
                        src++;
                    }
                    while (pos < (end & ~1)) {
                        *out++ = DecodeAdpcm(state, *src & 0xf);
                        pos++;
                        *out++ = DecodeAdpcm(state, (*src >> 4) & 0xf);
                        pos++;
                        src++;
                    }
                    if (pos < end) {
                        *out++ = DecodeAdpcm(state, *src & 0xf);
                        pos++;
                    }
                    OS_UnlockMutex(&sDecodeBufferMutex);
                }
            } else if (player->flags.singleChannel) {
                MI_CpuFill8(dest, 0, dataSize);
            } else {
                MI_CpuCopy8((u8 *)command->buffer[0] + bufOffset, dest, dataSize);
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
                OnDataEnd(player);
            }
        }
    }

    if (player->strmCallback != NULL) {
        player->strmCallback(command->status, command->numChannels, command->buffer, command->bufLen,
                             player->header.head.format == STRM_FORMAT_PCM8 ? NNS_SND_STRM_FORMAT_PCM8
                                                                            : NNS_SND_STRM_FORMAT_PCM16,
                             player->strmCallbackArg);
    }
    for (ch = 0; ch < command->numChannels; ch++) {
        DC_FlushRange(command->buffer[ch], command->bufLen);
    }
    if (command->status == NNS_SND_STRM_CALLBACK_SETUP) {
        player->prepared = TRUE;
    }
}

// A file in the ROM is read through the archive's file; one loaded is copied from memory
static void SetupStreamFunction(NNSSndStrmPlayer *player, u32 fileId) {
    if (NNS_SndArcGetFileAddress(fileId) == NULL) {
        player->open = OpenFileStream;
        player->close = CloseFileStream;
        player->read = ReadFileStream;
        player->cancel = CancelFileStream;
    } else {
        player->open = OpenMemoryStream;
        player->close = CloseMemoryStream;
        player->read = ReadMemoryStream;
        player->cancel = CancelMemoryStream;
    }
}

static BOOL OpenFileStream(NNSSndStrmPlayer *player, u32 fileId) {
    const char *path;

    if (NNS_SndArcReadFile(fileId, &player->header, sizeof(player->header), 0) != sizeof(player->header)) {
        return FALSE;
    }
    if (!FS_OpenFileFast(&player->file, NNS_SndArcGetFileID())) {
        path = NNSi_SndArcGetFilePath();
        if (path != NULL && !FS_OpenFileEx(&player->file, path, FS_FILEMODE_R)) {
            return FALSE;
        }
    }
    player->fileOffset = NNS_SndArcGetFileOffset(fileId);
    return TRUE;
}

static void CloseFileStream(NNSSndStrmPlayer *player) {
    FS_CloseFile(&player->file);
}

static int ReadFileStream(NNSSndStrmPlayer *player, void *buffer, int size, u32 offset) {
    FS_SeekFile(&player->file, player->fileOffset + offset, FS_SEEK_SET);
    return FS_ReadFile(&player->file, buffer, size);
}

static void CancelFileStream(NNSSndStrmPlayer *player) {
    FS_CancelFile(&player->file);
}

static BOOL OpenMemoryStream(NNSSndStrmPlayer *player, u32 fileId) {
    player->fileAddress = NNS_SndArcGetFileAddress(fileId);
    MI_CpuCopy8(player->fileAddress, &player->header, sizeof(player->header));
    return TRUE;
}

static void CloseMemoryStream(NNSSndStrmPlayer *player) {
}

static int ReadMemoryStream(NNSSndStrmPlayer *player, void *buffer, int size, u32 offset) {
    MI_CpuCopy8(player->fileAddress + offset, buffer, size);
    return size;
}

static void CancelMemoryStream(NNSSndStrmPlayer *player) {
}

// Sleeps until woken, then fills the blocks of its list
static void StrmThread(void *arg) {
    NNSSndStrmThread *thread = arg;

    while (TRUE) {
        LoadCommand *command;

        OS_SleepThread(&thread->threadQ);
        while (TRUE) {
            OS_LockMutex(&thread->mutex);
            command = ReadCommandBuffer(&thread->commandList);
            if (command == NULL) {
                OS_UnlockMutex(&thread->mutex);
                break;
            }
            MakeWaveData(command);
            FreeCommandBuffer(command);
            OS_UnlockMutex(&thread->mutex);
        }
    }
}
