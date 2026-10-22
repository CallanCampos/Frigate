// NitroSDK's SND (snd_work.c, a guess from the SDK's file names): the work that the ARM7's sound driver shares with the
// ARM9, and the functions that read what the driver reports in it. The driver writes it, so each read first discards
// the data cache's copy of the word.
#include "nitro/os.h"
#include "nitro/snd.h"

SNDSharedWork *SNDi_SharedWork;

// NitroSDK's SND_GetPlayerStatus: a bit for each player that is at work
u32 SND_GetPlayerStatus(void) {
    DC_InvalidateRange(&SNDi_SharedWork->playerStatus, sizeof(SNDi_SharedWork->playerStatus));
    return SNDi_SharedWork->playerStatus;
}

// NitroSDK's SND_GetChannelStatus: a bit for each channel that is at work
u32 SND_GetChannelStatus(void) {
    DC_InvalidateRange(&SNDi_SharedWork->channelStatus, sizeof(SNDi_SharedWork->channelStatus));
    return SNDi_SharedWork->channelStatus;
}

// NitroSDK's SND_GetPlayerTickCounter
u32 SND_GetPlayerTickCounter(int playerNo) {
    DC_InvalidateRange(&SNDi_SharedWork->player[playerNo].tickCounter, sizeof(u32));
    return SNDi_SharedWork->player[playerNo].tickCounter;
}

// The links of the driver's channels are ARM7 addresses. This turns the one that is stored at link into the address of
// the channel in the copy of the state, or NULL for the end of the list
static inline const SNDDriverChannel *ToArm9(const SNDDriverInfo *driverInfo, const SNDDriverChannel *const *link) {
    u32 base = driverInfo->workAddress;
    const SNDDriverChannel *channel = *link;

    if (channel == NULL) {
        return NULL;
    }
    return (const SNDDriverChannel *)((const u8 *)driverInfo + ((u32)channel - base));
}

// NitroSDK's SND_ReadTrackInfo: reads a track of a player from a copy of the driver's state. FALSE when the numbers are
// out of range or the player has no such track
BOOL SND_ReadTrackInfo(const SNDDriverInfo *driverInfo, int playerNo, int trackNo, SNDTrackInfo *trackInfo) {
    const SNDTrack *track;
    const SNDDriverChannel *channel;
    u8 trackIndex;

    if (playerNo < 0 || playerNo > SND_PLAYER_NUM - 1) {
        return FALSE;
    }
    if (trackNo < 0 || trackNo > SND_TRACK_NUM_PER_PLAYER - 1) {
        return FALSE;
    }
    trackIndex = driverInfo->player[playerNo].tracks[trackNo];
    if (trackIndex == SND_INVALID_TRACK_INDEX) {
        return FALSE;
    }

    track = &driverInfo->track[trackIndex];
    trackInfo->prgNo = track->prgNo;
    trackInfo->volume = track->volume;
    trackInfo->volume2 = track->volume2;
    trackInfo->pitchBend = track->pitch_bend;
    trackInfo->bendRange = track->bend_range;
    trackInfo->pan = track->pan + SND_CHANNEL_PAN_CENTER;
    trackInfo->transpose = track->transpose;

    // The channels that the track plays are a list of ARM7 addresses
    channel = ToArm9(driverInfo, &track->channel_list);
    trackInfo->chCount = 0;
    while (channel != NULL) {
        trackInfo->channel[trackInfo->chCount] = channel->myNo;
        trackInfo->chCount++;
        channel = ToArm9(driverInfo, &channel->next);
    }
    return TRUE;
}

// NitroSDK's SNDi_GetFinishedCommandTag: the number of packets that the ARM7 has dealt with
u32 SNDi_GetFinishedCommandTag(void) {
    DC_InvalidateRange(SNDi_SharedWork, sizeof(SNDi_SharedWork->finishCommandTag));
    return SNDi_SharedWork->finishCommandTag;
}

// NitroSDK's SNDi_InitSharedWork: nothing plays, and no variable of a player has a value
void SNDi_InitSharedWork(SNDSharedWork *work) {
    int i;
    int j;

    work->playerStatus = 0;
    work->channelStatus = 0;
    work->captureStatus = 0;
    work->finishCommandTag = 0;
    for (i = 0; i < SND_PLAYER_NUM; i++) {
        work->player[i].tickCounter = 0;
        for (j = 0; j < SND_PLAYER_VARIABLE_MAX; j++) {
            work->player[i].variable[j] = SND_DEFAULT_VARIABLE;
        }
    }
    for (i = 0; i < SND_PLAYER_VARIABLE_MAX; i++) {
        work->globalVariable[i] = SND_DEFAULT_VARIABLE;
    }
    DC_FlushRange(work, sizeof(SNDSharedWork));
}
