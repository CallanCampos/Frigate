#include "nitro/os.h"
#include "snd_internal.h"

// NitroSystem's streams (NNS_SndStrm): PCM played in a loop from a buffer on locked channels, with an alarm per stream
// that calls back for each block of the buffer as it is played, to fill it again. Streams stop timers before the system
// sleeps and start them after it wakes. swan calls none of them by name. The SDK's file is stream.c, and the names are
// NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them, except sBufPtr and sInitialized, which are ours
// for statics that NitroSystem declares inside its functions

// Each locked channel's part of the buffer and its own volume, added to the stream's
typedef struct NNSSndStrmChannel {
    void *buffer;
    int volume;
} NNSSndStrmChannel;

static void ForceStopStrm(NNSSndStrm *stream);
static void ShutdownStrm(NNSSndStrm *stream);
static void AlarmCallback(void *arg);
static void StrmCallback(NNSSndStrm *stream, NNSSndStrmCallbackStatus status);
static void BeginSleep(void *arg);
static void EndSleep(void *arg);

static BOOL sInitialized;
static NNSFndList sStrmList;
static void *sBufPtr[NNS_SND_STRM_CHANNEL_MAX];
static NNSSndStrmChannel sStrmChannel[NNS_SND_STRM_CHANNEL_MAX];

void NNS_SndStrmInit(NNSSndStrm *stream) {
    if (!sInitialized) {
        NNS_FndInitList(&sStrmList, 0);
        sInitialized = TRUE;
    }

    PM_SetSleepCallbackInfo(&stream->preSleepInfo, BeginSleep, stream);
    PM_SetSleepCallbackInfo(&stream->postSleepInfo, EndSleep, stream);
    stream->chBitMask = 0;
    stream->numChannels = 0;
    stream->activeFlag = FALSE;
    stream->startFlag = FALSE;
}

BOOL NNS_SndStrmAllocChannel(NNSSndStrm *stream, int numChannels, const u8 chNoList[]) {
    u32 chBitMask = 0;
    int i;

    for (i = 0; i < numChannels; i++) {
        stream->channelNo[i] = chNoList[i];
        chBitMask |= 1 << chNoList[i];
    }

    if (!NNS_SndLockChannel(chBitMask)) {
        return FALSE;
    }

    stream->numChannels = numChannels;
    stream->chBitMask = chBitMask;
    return TRUE;
}

void NNS_SndStrmFreeChannel(NNSSndStrm *stream) {
    if (stream->chBitMask != 0) {
        NNS_SndUnlockChannel(stream->chBitMask);
        stream->chBitMask = 0;
        stream->numChannels = 0;
    }
}

BOOL NNS_SndStrmSetup(NNSSndStrm *stream, NNSSndStrmFormat format, void *buffer, u32 len, int timer, int interval,
                      NNSSndStrmCallback callback, void *arg) {
    u32 samples;
    u32 alarmPeriod;
    int i;
    OSIntrMode oldIntr;

    if (stream->activeFlag) {
        NNS_SndStrmStop(stream);
    }

    // Each channel's part is a whole number of 32-byte units per block
    stream->chBufLen = len / (interval * 32 * stream->numChannels);
    stream->chBufLen = stream->chBufLen * interval * 32;
    samples = stream->chBufLen;
    if (format == NNS_SND_STRM_FORMAT_PCM16) {
        samples /= 2;
    }
    alarmPeriod = timer * samples / interval;

    stream->alarmNo = NNS_SndAllocAlarm();
    if (stream->alarmNo < 0) {
        return FALSE;
    }

    for (i = 0; i < stream->numChannels; i++) {
        int chNo = stream->channelNo[i];

        sStrmChannel[chNo].buffer = (u8 *)buffer + stream->chBufLen * i;
        sStrmChannel[chNo].volume = 0;
        SND_SetupChannelPcm(chNo, format, sStrmChannel[chNo].buffer, SND_CHANNEL_LOOP_REPEAT, 0,
                            stream->chBufLen / sizeof(u32), SND_CHANNEL_VOLUME_MAX, 0, timer * 32,
                            SND_CHANNEL_PAN_CENTER);
    }

    SND_SetupAlarm(stream->alarmNo, alarmPeriod, alarmPeriod, AlarmCallback, stream);
    NNS_FndAppendListObject(&sStrmList, stream);

    stream->format = format;
    stream->interval = interval;
    stream->callback = callback;
    stream->callbackArg = arg;
    stream->curBuffer = 0;
    stream->volume = 0;
    stream->activeFlag = TRUE;

    // The first call fills the whole buffer, as one block
    oldIntr = OS_DisableInterrupts();
    stream->interval = 1;
    StrmCallback(stream, NNS_SND_STRM_CALLBACK_SETUP);
    stream->interval = interval;
    OS_RestoreInterrupts(oldIntr);

    return TRUE;
}

void NNS_SndStrmStart(NNSSndStrm *stream) {
    SND_StartTimer(stream->chBitMask, 0, 1 << stream->alarmNo, 0);

    if (!stream->startFlag) {
        PM_PrependPreSleepCallback(&stream->preSleepInfo);
        PM_AppendPostSleepCallback(&stream->postSleepInfo);
        stream->startFlag = TRUE;
    }
}

void NNS_SndStrmStop(NNSSndStrm *stream) {
    if (stream->activeFlag) {
        ForceStopStrm(stream);
    }
}

void NNS_SndStrmSetVolume(NNSSndStrm *stream, int volume) {
    int i;

    stream->volume = volume;

    for (i = 0; i < stream->numChannels; i++) {
        int chNo = stream->channelNo[i];
        u16 chVolume = SND_CalcChannelVolume(stream->volume + sStrmChannel[chNo].volume);

        SND_SetChannelVolume(1 << chNo, SND_CHANNEL_VOLUME(chVolume), SND_CHANNEL_DATASHIFT(chVolume));
    }
}

void NNS_SndStrmSetChannelPan(NNSSndStrm *stream, int index, int pan) {
    if (index <= stream->numChannels - 1) {
        SND_SetChannelPan(1 << stream->channelNo[index], pan);
    }
}

static void ForceStopStrm(NNSSndStrm *stream) {
    if (stream->startFlag) {
        u32 tag;

        SND_StopTimer(stream->chBitMask, 0, 1 << stream->alarmNo, 0);
        PM_DeletePreSleepCallback(&stream->preSleepInfo);
        PM_DeletePostSleepCallback(&stream->postSleepInfo);
        stream->startFlag = FALSE;

        tag = SND_GetCurrentCommandTag();
        SND_FlushCommand(SND_COMMAND_BLOCK);
        SND_WaitForCommandProc(tag);
    }

    ShutdownStrm(stream);
}

static void ShutdownStrm(NNSSndStrm *stream) {
    NNS_SndFreeAlarm(stream->alarmNo);
    NNS_FndRemoveListObject(&sStrmList, stream);
    stream->activeFlag = FALSE;
}

static void AlarmCallback(void *arg) {
    StrmCallback(arg, NNS_SND_STRM_CALLBACK_INTERVAL);
}

// A pointer moved on by a byte offset, an inline like those of NitroSystem's FND
static inline void *AddU32ToPtr(void *ptr, u32 val) {
    return (void *)((uptr)ptr + val);
}

static void StrmCallback(NNSSndStrm *stream, NNSSndStrmCallbackStatus status) {
    u32 len = stream->chBufLen / stream->interval;
    u32 offset = len * stream->curBuffer;
    int i;

    for (i = 0; i < stream->numChannels; i++) {
        sBufPtr[i] = AddU32ToPtr(sStrmChannel[stream->channelNo[i]].buffer, offset);
    }

    stream->callback(status, stream->numChannels, sBufPtr, len, stream->format, stream->callbackArg);

    if (++stream->curBuffer >= stream->interval) {
        stream->curBuffer = 0;
    }
}

static void BeginSleep(void *arg) {
    NNSSndStrm *stream = arg;
    u32 tag;

    if (stream->startFlag) {
        SND_StopTimer(stream->chBitMask, 0, 1 << stream->alarmNo, 0);
        tag = SND_GetCurrentCommandTag();
        SND_FlushCommand(SND_COMMAND_BLOCK);
        SND_WaitForCommandProc(tag);
    }
}

// The buffer is filled up to its end again before the channels start over from its start
static void EndSleep(void *arg) {
    NNSSndStrm *stream = arg;

    if (stream->startFlag) {
        while (stream->curBuffer != 0) {
            OSIntrMode oldIntr = OS_DisableInterrupts();
            StrmCallback(stream, NNS_SND_STRM_CALLBACK_INTERVAL);
            OS_RestoreInterrupts(oldIntr);
        }

        SND_StartTimer(stream->chBitMask, 0, 1 << stream->alarmNo, 0);
    }
}
