#include "snd_internal.h"
#include <stddef.h>

// NitroSystem's sequence players (NNS_SndPlayer), which share the driver's 16 players among the sequences of 32
// player numbers by priority, and the handles that sequences are controlled through. The file name is a guess, from
// NitroSystem's names for its sound files. NNS_SndPlayerSetPlayerVolume, NNS_SndPlayerSetVolume,
// NNS_SndPlayerSetTrackPitch, NNS_SndPlayerSetTrackModDepth and NNS_SndPlayerSetTrackModSpeed are
// NNS_SndPlayerSetPlayerVolume, NNS_SndPlayerSetVolume, NNS_SndPlayerSetTrackPitch, NNS_SndPlayerSetTrackModDepth and
// NNS_SndPlayerSetTrackModSpeed by their code

#define NNS_SND_PLAYER_NUM 32
#define SND_PLAYER_NUM 16

#define NNS_SND_VOLUME_MAX 127
#define SND_TRACK_MASK_ALL 0xffff
// The quietest volume in decibels, which a sequence fading out stops at
#define SND_VOLUME_DB_MIN -723

// A player number: its sequences playing, by priority, and its free heaps
struct NNSSndPlayer {
    NNSFndList playerList;
    NNSFndList heapList;
    u32 playableSeqCount;
    u32 allocChBitFlag;
    u8 volume;
};

// A heap of a player number, made in a sound heap, and the sequence player using it if any
struct NNSSndPlayerHeap {
    NNSFndLink link;
    NNSSndHeapHandle heap;
    NNSSndSeqPlayer *player;
    int playerNo;
};

static void StopSeq(NNSSndSeqPlayer *seqPlayer, int fadeFrame);
static void PauseSeq(NNSSndSeqPlayer *seqPlayer, BOOL flag);
static void InitPlayer(NNSSndSeqPlayer *seqPlayer);
static void InsertPlayerList(NNSSndPlayer *player, NNSSndSeqPlayer *seqPlayer);
static void InsertPrioList(NNSSndSeqPlayer *seqPlayer);
static void ForceStopSeq(NNSSndSeqPlayer *seqPlayer);
static NNSSndSeqPlayer *AllocSeqPlayer(int prio);
static void ShutdownPlayer(NNSSndSeqPlayer *seqPlayer);
static void PlayerHeapDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void SetPlayerPriority(NNSSndSeqPlayer *seqPlayer, int prio);

static NNSFndList sFreeList;
static NNSFndList sPrioList;
static NNSSndSeqPlayer sSeqPlayer[SND_PLAYER_NUM];
static NNSSndPlayer sPlayer[NNS_SND_PLAYER_NUM];

// A volume 0 to 127 in decibels, as NitroSDK's SND_CalcDecibel gives it
static inline int CalcDecibel(int scale) {
    return SNDi_DecibelTable[scale];
}

static inline BOOL NNS_SndHandleIsValid(const NNSSndHandle *handle) {
    return handle->player != NULL;
}

void NNS_SndPlayerSetPlayerVolume(int playerNo, int volume) {
    sPlayer[playerNo].volume = volume;
}

void NNS_SndPlayerSetPlayableSeqCount(int playerNo, int seqCount) {
    sPlayer[playerNo].playableSeqCount = (u16)seqCount;
}

void NNS_SndPlayerSetAllocatableChannel(int playerNo, u32 chBitFlag) {
    sPlayer[playerNo].allocChBitFlag = chBitFlag;
}

BOOL NNS_SndPlayerCreateHeap(int playerNo, NNSSndHeapHandle heap, u32 size) {
    NNSSndPlayerHeap *playerHeap;
    NNSSndHeapHandle frmHeap;

    playerHeap = NNS_SndHeapAlloc(heap, sizeof(NNSSndPlayerHeap) + size, PlayerHeapDisposeCallback, 0, 0);
    if (playerHeap == NULL) {
        return FALSE;
    }
    playerHeap->player = NULL;
    playerHeap->playerNo = playerNo;
    playerHeap->heap = NULL;

    frmHeap = NNS_SndHeapCreate(playerHeap + 1, size);
    if (frmHeap == NULL) {
        return FALSE;
    }
    playerHeap->heap = frmHeap;

    NNS_FndAppendListObject(&sPlayer[playerNo].heapList, playerHeap);
    return TRUE;
}

void NNS_SndPlayerStopSeq(NNSSndHandle *handle, int fadeFrame) {
    StopSeq(handle->player, fadeFrame);
}

void NNS_SndPlayerPause(NNSSndHandle *handle, BOOL flag) {
    PauseSeq(handle->player, flag);
}

void NNS_SndHandleInit(NNSSndHandle *handle) {
    handle->player = NULL;
}

void NNS_SndHandleReleaseSeq(NNSSndHandle *handle) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    handle->player->handle = NULL;
    handle->player = NULL;
}

int NNS_SndPlayerCountPlayingSeqByPlayerNo(int playerNo) {
    return sPlayer[playerNo].playerList.numObjects;
}

int NNS_SndPlayerCountPlayingSeqBySeqNo(int seqNo) {
    int count = 0;
    NNSSndSeqPlayer *seqPlayer = NULL;

    while ((seqPlayer = NNS_FndGetNextListObject(&sPrioList, seqPlayer)) != NULL) {
        if (seqPlayer->seqType == NNS_SND_SEQ_TYPE_SEQ && seqPlayer->seqNo == seqNo) {
            count++;
        }
    }
    return count;
}

void NNS_SndPlayerSetVolume(NNSSndHandle *handle, int volume) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    handle->player->extVolume = volume;
}

void NNS_SndPlayerSetInitialVolume(NNSSndHandle *handle, int volume) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    handle->player->initVolume = volume;
}

void NNS_SndPlayerMoveVolume(NNSSndHandle *handle, int targetVolume, int frames) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    if (handle->player->status != NNS_SND_SEQ_PLAYER_STATUS_FADEOUT) {
        NNSi_SndFaderSet(&handle->player->fader, targetVolume << 8, frames);
    }
}

void NNS_SndPlayerSetChannelPriority(NNSSndHandle *handle, int prio) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetPlayerChannelPriority(handle->player->playerNo, prio);
}

void NNS_SndPlayerSetTrackMute(NNSSndHandle *handle, u32 trackBitMask, BOOL flag) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetTrackMute(handle->player->playerNo, trackBitMask, flag);
}

void NNS_SndPlayerSetTrackMuteEx(NNSSndHandle *handle, u32 trackBitMask, int muteType) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetTrackMuteEx(handle->player->playerNo, trackBitMask, muteType);
}

void NNS_SndPlayerSetTrackVolume(NNSSndHandle *handle, u32 trackBitMask, int volume) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetTrackVolume(handle->player->playerNo, trackBitMask, CalcDecibel(volume));
}

void NNS_SndPlayerSetTrackPitch(NNSSndHandle *handle, u32 trackBitMask, int pitch) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetTrackPitch(handle->player->playerNo, trackBitMask, pitch);
}

void NNS_SndPlayerSetTrackPan(NNSSndHandle *handle, u32 trackBitMask, int pan) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetTrackPan(handle->player->playerNo, trackBitMask, pan);
}

void NNS_SndPlayerSetTrackModDepth(NNSSndHandle *handle, u32 trackBitMask, int depth) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetTrackModDepth(handle->player->playerNo, trackBitMask, depth);
}

void NNS_SndPlayerSetTrackModSpeed(NNSSndHandle *handle, u32 trackBitMask, int speed) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetTrackModSpeed(handle->player->playerNo, trackBitMask, speed);
}

void NNS_SndPlayerSetTempoRatio(NNSSndHandle *handle, int ratio) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    SND_SetPlayerTempoRatio(handle->player->playerNo, ratio);
}

void NNSi_SndPlayerSetSeqNo(NNSSndHandle *handle, int seqNo) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    handle->player->seqType = NNS_SND_SEQ_TYPE_SEQ;
    handle->player->seqNo = seqNo;
}

void NNSi_SndPlayerSetSeqArcNo(NNSSndHandle *handle, int seqArcNo, int index) {
    if (!NNS_SndHandleIsValid(handle)) {
        return;
    }
    handle->player->seqType = NNS_SND_SEQ_TYPE_SEQARC;
    handle->player->seqNo = seqArcNo;
    handle->player->seqArcIndex = index;
}

u32 NNS_SndPlayerGetTick(NNSSndHandle *handle) {
    NNSSndSeqPlayer *seqPlayer;

    if (!NNS_SndHandleIsValid(handle)) {
        return 0;
    }
    seqPlayer = handle->player;
    if (seqPlayer->startFlag) {
        return SND_GetPlayerTickCounter(seqPlayer->playerNo);
    }
    return 0;
}

BOOL NNS_SndPlayerReadDriverTrackInfo(NNSSndHandle *handle, int trackNo, SNDTrackInfo *trackInfo) {
    if (!NNS_SndHandleIsValid(handle)) {
        return FALSE;
    }
    return NNSi_SndReadDriverTrackInfo(handle->player->playerNo, trackNo, trackInfo);
}

void NNSi_SndPlayerInit(void) {
    int i;
    NNSSndSeqPlayer *seqPlayer;
    NNSSndPlayer *player;

    NNS_FndInitList(&sPrioList, offsetof(NNSSndSeqPlayer, prioLink));
    NNS_FndInitList(&sFreeList, offsetof(NNSSndSeqPlayer, prioLink));

    for (i = 0; i < SND_PLAYER_NUM; i++) {
        seqPlayer = &sSeqPlayer[i];
        seqPlayer->status = NNS_SND_SEQ_PLAYER_STATUS_NONE;
        seqPlayer->playerNo = i;
        NNS_FndAppendListObject(&sFreeList, seqPlayer);
    }

    for (i = 0; i < NNS_SND_PLAYER_NUM; i++) {
        player = &sPlayer[i];
        NNS_FndInitList(&player->playerList, offsetof(NNSSndSeqPlayer, playerLink));
        NNS_FndInitList(&player->heapList, offsetof(NNSSndPlayerHeap, link));
        player->volume = NNS_SND_VOLUME_MAX;
        player->playableSeqCount = 1;
        player->allocChBitFlag = 0;
    }
}

void NNSi_SndPlayerMain(void) {
    NNSSndSeqPlayer *seqPlayer;
    NNSSndSeqPlayer *next;
    u32 playerStatus = SND_GetPlayerStatus();
    int volume;

    for (seqPlayer = NNS_FndGetNextListObject(&sPrioList, NULL); seqPlayer != NULL; seqPlayer = next) {
        next = NNS_FndGetNextListObject(&sPrioList, seqPlayer);

        if (!seqPlayer->startFlag && SND_IsFinishedCommandTag(seqPlayer->commandTag)) {
            seqPlayer->startFlag = TRUE;
        }
        if (seqPlayer->startFlag && !(playerStatus & (1 << seqPlayer->playerNo))) {
            ShutdownPlayer(seqPlayer);
            continue;
        }

        NNSi_SndFaderUpdate(&seqPlayer->fader);

        volume = CalcDecibel(seqPlayer->initVolume) + CalcDecibel(seqPlayer->extVolume) +
                 CalcDecibel(seqPlayer->player->volume) + CalcDecibel(NNSi_SndFaderGet(&seqPlayer->fader) >> 8);
        if (volume < -0x8000) {
            volume = -0x8000;
        } else if (volume > 0x7fff) {
            volume = 0x7fff;
        }
        if (volume != seqPlayer->volume) {
            SND_SetPlayerVolume(seqPlayer->playerNo, volume);
            seqPlayer->volume = volume;
        }

        if (seqPlayer->status == NNS_SND_SEQ_PLAYER_STATUS_FADEOUT && NNSi_SndFaderIsFinished(&seqPlayer->fader)) {
            ForceStopSeq(seqPlayer);
        }

        if (seqPlayer->prepareFlag) {
            SND_StartPreparedSeq(seqPlayer->playerNo);
            seqPlayer->prepareFlag = FALSE;
        }
    }
}

NNSSndSeqPlayer *NNSi_SndPlayerAllocSeqPlayer(NNSSndHandle *handle, int playerNo, int prio) {
    NNSSndPlayer *player = &sPlayer[playerNo];
    NNSSndSeqPlayer *seqPlayer;
    NNSSndSeqPlayer *dropPlayer;

    if (NNS_SndHandleIsValid(handle)) {
        NNS_SndHandleReleaseSeq(handle);
    }

    if (player->playerList.numObjects >= player->playableSeqCount) {
        dropPlayer = NNS_FndGetNextListObject(&player->playerList, NULL);
        if (dropPlayer == NULL) {
            return NULL;
        }
        if (prio < dropPlayer->prio) {
            return NULL;
        }
        ForceStopSeq(dropPlayer);
    }

    seqPlayer = AllocSeqPlayer(prio);
    if (seqPlayer == NULL) {
        return NULL;
    }
    InsertPlayerList(player, seqPlayer);

    seqPlayer->handle = handle;
    handle->player = seqPlayer;
    return seqPlayer;
}

void NNSi_SndPlayerFreeSeqPlayer(NNSSndSeqPlayer *seqPlayer) {
    ShutdownPlayer(seqPlayer);
}

void NNSi_SndPlayerStartSeq(NNSSndSeqPlayer *seqPlayer, const void *seqBase, u32 seqOffset, const SNDBankData *bank) {
    NNSSndPlayer *player = seqPlayer->player;

    SND_PrepareSeq(seqPlayer->playerNo, seqBase, seqOffset, bank);
    if (player->allocChBitFlag) {
        SND_SetTrackAllocatableChannel(seqPlayer->playerNo, SND_TRACK_MASK_ALL, player->allocChBitFlag);
    }
    InitPlayer(seqPlayer);

    seqPlayer->commandTag = SND_GetCurrentCommandTag();
    seqPlayer->prepareFlag = TRUE;
    seqPlayer->status = NNS_SND_SEQ_PLAYER_STATUS_PLAY;
}

static void StopSeq(NNSSndSeqPlayer *seqPlayer, int fadeFrame) {
    if (seqPlayer == NULL) {
        return;
    }
    if (seqPlayer->status == NNS_SND_SEQ_PLAYER_STATUS_NONE) {
        return;
    }
    if (fadeFrame == 0) {
        ForceStopSeq(seqPlayer);
        return;
    }
    NNSi_SndFaderSet(&seqPlayer->fader, 0, fadeFrame);
    SetPlayerPriority(seqPlayer, 0);
    seqPlayer->status = NNS_SND_SEQ_PLAYER_STATUS_FADEOUT;
}

static void PauseSeq(NNSSndSeqPlayer *seqPlayer, BOOL flag) {
    if (seqPlayer == NULL) {
        return;
    }
    if (flag != seqPlayer->pauseFlag) {
        SND_PauseSeq(seqPlayer->playerNo, flag);
        seqPlayer->pauseFlag = flag;
    }
}

NNSSndHeapHandle NNSi_SndPlayerAllocHeap(int playerNo, NNSSndSeqPlayer *seqPlayer) {
    NNSSndPlayer *player = &sPlayer[playerNo];
    NNSSndPlayerHeap *heap;

    heap = NNS_FndGetNextListObject(&player->heapList, NULL);
    if (heap == NULL) {
        return NULL;
    }
    NNS_FndRemoveListObject(&player->heapList, heap);

    heap->player = seqPlayer;
    seqPlayer->heap = heap;
    NNS_SndHeapClear(heap->heap);
    return heap->heap;
}

static void InitPlayer(NNSSndSeqPlayer *seqPlayer) {
    seqPlayer->pauseFlag = FALSE;
    seqPlayer->startFlag = FALSE;
    seqPlayer->prepareFlag = FALSE;
    seqPlayer->seqType = NNS_SND_SEQ_TYPE_INVALID;
    seqPlayer->volume = 0;
    seqPlayer->initVolume = NNS_SND_VOLUME_MAX;
    seqPlayer->extVolume = NNS_SND_VOLUME_MAX;

    NNSi_SndFaderInit(&seqPlayer->fader);
    NNSi_SndFaderSet(&seqPlayer->fader, NNS_SND_VOLUME_MAX << 8, 1);
}

static void InsertPlayerList(NNSSndPlayer *player, NNSSndSeqPlayer *seqPlayer) {
    NNSSndSeqPlayer *next = NULL;

    while ((next = NNS_FndGetNextListObject(&player->playerList, next)) != NULL) {
        if (seqPlayer->prio < next->prio) {
            break;
        }
    }
    NNS_FndInsertListObject(&player->playerList, next, seqPlayer);
    seqPlayer->player = player;
}

static void InsertPrioList(NNSSndSeqPlayer *seqPlayer) {
    NNSSndSeqPlayer *next = NULL;

    while ((next = NNS_FndGetNextListObject(&sPrioList, next)) != NULL) {
        if (seqPlayer->prio < next->prio) {
            break;
        }
    }
    NNS_FndInsertListObject(&sPrioList, next, seqPlayer);
}

static void ForceStopSeq(NNSSndSeqPlayer *seqPlayer) {
    if (seqPlayer->status == NNS_SND_SEQ_PLAYER_STATUS_FADEOUT) {
        SND_SetPlayerVolume(seqPlayer->playerNo, SND_VOLUME_DB_MIN);
    }
    SND_StopSeq(seqPlayer->playerNo);
    ShutdownPlayer(seqPlayer);
}

static NNSSndSeqPlayer *AllocSeqPlayer(int prio) {
    NNSSndSeqPlayer *seqPlayer = NNS_FndGetNextListObject(&sFreeList, NULL);

    if (seqPlayer == NULL) {
        seqPlayer = NNS_FndGetNextListObject(&sPrioList, NULL);
        if (prio < seqPlayer->prio) {
            return NULL;
        }
        ForceStopSeq(seqPlayer);
    }
    NNS_FndRemoveListObject(&sFreeList, seqPlayer);

    seqPlayer->prio = prio;
    InsertPrioList(seqPlayer);
    return seqPlayer;
}

static void ShutdownPlayer(NNSSndSeqPlayer *seqPlayer) {
    NNSSndPlayer *player;

    if (seqPlayer->handle != NULL) {
        seqPlayer->handle->player = NULL;
        seqPlayer->handle = NULL;
    }

    player = seqPlayer->player;
    NNS_FndRemoveListObject(&player->playerList, seqPlayer);
    seqPlayer->player = NULL;

    if (seqPlayer->heap != NULL) {
        NNS_FndAppendListObject(&player->heapList, seqPlayer->heap);
        seqPlayer->heap->player = NULL;
        seqPlayer->heap = NULL;
    }

    NNS_FndRemoveListObject(&sPrioList, seqPlayer);
    NNS_FndAppendListObject(&sFreeList, seqPlayer);
    seqPlayer->status = NNS_SND_SEQ_PLAYER_STATUS_NONE;
}

// A player's heap is disposed of with the sound heap it was made in
static void PlayerHeapDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    NNSSndPlayerHeap *playerHeap = mem;

    if (playerHeap->heap == NULL) {
        return;
    }
    NNS_SndHeapDestroy(playerHeap->heap);

    if (playerHeap->player != NULL) {
        playerHeap->player->heap = NULL;
    } else {
        NNS_FndRemoveListObject(&sPlayer[playerHeap->playerNo].heapList, playerHeap);
    }
}

static void SetPlayerPriority(NNSSndSeqPlayer *seqPlayer, int prio) {
    NNSSndPlayer *player = seqPlayer->player;

    if (player != NULL) {
        NNS_FndRemoveListObject(&player->playerList, seqPlayer);
        seqPlayer->player = NULL;
    }
    NNS_FndRemoveListObject(&sPrioList, seqPlayer);

    seqPlayer->prio = prio;
    if (player != NULL) {
        InsertPlayerList(player, seqPlayer);
    }
    InsertPrioList(seqPlayer);
}
