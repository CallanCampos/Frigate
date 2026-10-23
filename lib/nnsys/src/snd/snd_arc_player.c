#include "nitro/snd.h"
#include "snd_internal.h"

// NitroSystem's sound archive player (NNS_SndArcPlayer): sets the sequence players up from the archive's player info,
// and starts the archive's sequences on them, loading each sequence and its bank into the heap of the sequence player
// when they aren't loaded yet. The SDK's file is sndarc_player.c, and the names are NitroSystem's, as ntrtwl's
// reconstruction of NitroSystem has them

// The number of sound players
#define NNS_SND_PLAYER_NUM 32

static BOOL StartSeq(NNSSndHandle *handle, int playerNo, int bankNo, int playerPrio, const NNSSndArcSeqInfo *info,
                     int seqNo);
static BOOL StartSeqArc(NNSSndHandle *handle, int playerNo, int bankNo, int playerPrio, const NNSSndSeqArcSeqInfo *info,
                        const NNSSndSeqArc *seqArc, int seqArcNo, int index);

BOOL NNS_SndArcPlayerSetup(NNSSndHeapHandle heap) {
    const NNSSndArcPlayerInfo *info;
    int playerNo;
    int i;

    // What is left of a check that an archive is current
    NNS_SndArcGetCurrent();
    for (playerNo = 0; playerNo < NNS_SND_PLAYER_NUM; playerNo++) {
        info = NNS_SndArcGetPlayerInfo(playerNo);
        if (info == NULL) {
            continue;
        }
        NNS_SndPlayerSetPlayableSeqCount(playerNo, info->seqMax);
        NNS_SndPlayerSetAllocatableChannel(playerNo, info->allocChBitFlag);
        if (info->heapSize != 0 && heap != NULL) {
            for (i = 0; i < info->seqMax; i++) {
                if (!NNS_SndPlayerCreateHeap(playerNo, heap, info->heapSize)) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

BOOL NNS_SndArcPlayerStartSeq(NNSSndHandle *handle, int seqNo) {
    const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(seqNo);

    if (info == NULL) {
        return FALSE;
    }
    return StartSeq(handle, info->param.playerNo, info->param.bankNo, info->param.playerPrio, info, seqNo);
}

BOOL NNS_SndArcPlayerStartSeqEx(NNSSndHandle *handle, int playerNo, int bankNo, int playerPrio, int seqNo) {
    const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(seqNo);

    if (info == NULL) {
        return FALSE;
    }
    if (playerPrio < 0) {
        playerPrio = info->param.playerPrio;
    }
    if (bankNo < 0) {
        bankNo = info->param.bankNo;
    }
    if (playerNo < 0) {
        playerNo = info->param.playerNo;
    }
    return StartSeq(handle, playerNo, bankNo, playerPrio, info, seqNo);
}

BOOL NNS_SndArcPlayerStartSeqArc(NNSSndHandle *handle, int seqArcNo, int index) {
    const NNSSndArcSeqArcInfo *info = NNS_SndArcGetSeqArcInfo(seqArcNo);
    const NNSSndSeqArc *seqArc;
    const NNSSndSeqArcSeqInfo *seqInfo;

    if (info == NULL) {
        return FALSE;
    }
    seqArc = NNS_SndArcGetFileAddress(info->fileId);
    if (seqArc == NULL) {
        return FALSE;
    }
    seqInfo = NNSi_SndSeqArcGetSeqInfo(seqArc, index);
    if (seqInfo == NULL) {
        return FALSE;
    }
    return StartSeqArc(handle, seqInfo->param.playerNo, seqInfo->param.bankNo, seqInfo->param.playerPrio, seqInfo,
                       seqArc, seqArcNo, index);
}

static BOOL StartSeq(NNSSndHandle *handle, int playerNo, int bankNo, int playerPrio, const NNSSndArcSeqInfo *info,
                     int seqNo) {
    NNSSndSeqPlayer *seqPlayer;
    NNSSndHeapHandle heap;
    const SNDSequenceData *seq;
    SNDBankData *bank;

    seqPlayer = NNSi_SndPlayerAllocSeqPlayer(handle, playerNo, playerPrio);
    if (seqPlayer == NULL) {
        return FALSE;
    }
    heap = NNSi_SndPlayerAllocHeap(playerNo, seqPlayer);
    if (NNSi_SndArcLoadBank(bankNo, NNS_SND_ARC_LOAD_BANK | NNS_SND_ARC_LOAD_WAVE, heap, FALSE, &bank) !=
        NNS_SND_ARC_LOAD_SUCESS) {
        NNSi_SndPlayerFreeSeqPlayer(seqPlayer);
        return FALSE;
    }
    if (NNSi_SndArcLoadSeq(seqNo, NNS_SND_ARC_LOAD_SEQ, heap, FALSE, &seq) != NNS_SND_ARC_LOAD_SUCESS) {
        NNSi_SndPlayerFreeSeqPlayer(seqPlayer);
        return FALSE;
    }
    NNSi_SndPlayerStartSeq(seqPlayer, (const u8 *)seq + seq->baseOffset, 0, bank);
    NNS_SndPlayerSetInitialVolume(handle, info->param.volume);
    NNS_SndPlayerSetChannelPriority(handle, info->param.channelPrio);
    NNS_SndPlayerSetSeqNo(handle, seqNo);
    return TRUE;
}

// The sequence archive must be loaded; only the bank is loaded here
static BOOL StartSeqArc(NNSSndHandle *handle, int playerNo, int bankNo, int playerPrio, const NNSSndSeqArcSeqInfo *info,
                        const NNSSndSeqArc *seqArc, int seqArcNo, int index) {
    NNSSndSeqPlayer *seqPlayer;
    NNSSndHeapHandle heap;
    SNDBankData *bank;

    seqPlayer = NNSi_SndPlayerAllocSeqPlayer(handle, playerNo, playerPrio);
    if (seqPlayer == NULL) {
        return FALSE;
    }
    heap = NNSi_SndPlayerAllocHeap(playerNo, seqPlayer);
    if (NNSi_SndArcLoadBank(bankNo, NNS_SND_ARC_LOAD_BANK | NNS_SND_ARC_LOAD_WAVE, heap, FALSE, &bank) !=
        NNS_SND_ARC_LOAD_SUCESS) {
        NNSi_SndPlayerFreeSeqPlayer(seqPlayer);
        return FALSE;
    }
    NNSi_SndPlayerStartSeq(seqPlayer, (const u8 *)seqArc + seqArc->baseOffset, info->offset, bank);
    NNS_SndPlayerSetInitialVolume(handle, info->param.volume);
    NNS_SndPlayerSetChannelPriority(handle, info->param.channelPrio);
    NNS_SndPlayerSetSeqArcNo(handle, seqArcNo, index);
    return TRUE;
}
