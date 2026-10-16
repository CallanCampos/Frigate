#include "nitro/os.h"
#include "snd_internal.h"

// NitroSystem's streams (NNS_SndStrm): PCM played in a loop from a buffer on locked channels, with an alarm per stream
// that calls back for each block of the buffer as it is played, to fill it again. Streams stop timers before the
// system sleeps and start them after it wakes. swan calls none of them by name; the file name is a guess

// Each locked channel's part of the buffer and its own volume, added to the stream's
typedef struct StrmChannelInfo {
    void *buffer;
    int volume;
} StrmChannelInfo;

static void ForceStop(NNSSndStrm *stream);
static void Shutdown(NNSSndStrm *stream);
static void AlarmCallback(void *arg);
static void StrmCallback(NNSSndStrm *stream, NNSSndStrmCallbackStatus status);
static void StrmBeginSleep(void *arg);
static void StrmEndSleep(void *arg);

static BOOL sInitialized;
static NNSFndList sStrmList;
static void *sBufPtr[NNS_SND_STRM_CHANNEL_MAX];
static StrmChannelInfo sChInfo[NNS_SND_STRM_CHANNEL_MAX];

void NNS_SndStrmInit(NNSSndStrm *stream) {
    if (!sInitialized) {
        NNS_FndInitList(&sStrmList, 0);
        sInitialized = TRUE;
    }

    PM_SetSleepCallbackInfo(&stream->preSleepInfo, StrmBeginSleep, stream);
    PM_SetSleepCallbackInfo(&stream->postSleepInfo, StrmEndSleep, stream);
    stream->chBitMask = 0;
    stream->numChannels = 0;
    stream->active = FALSE;
    stream->started = FALSE;
}

BOOL NNS_SndStrmAllocChannel(NNSSndStrm *stream, int numChannels, const u8 chNoList[]) {
    u32 chBitMask = 0;
    int i;

    for (i = 0; i < numChannels; i++) {
        stream->chNo[i] = chNoList[i];
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
    u32 oldIntr;

    if (stream->active) {
        NNS_SndStrmStop(stream);
    }

    // Each channel's part is a whole number of 32-byte units per block
    stream->bufSize = len / (interval * 32 * stream->numChannels);
    stream->bufSize = stream->bufSize * interval * 32;
    samples = stream->bufSize;
    if (format == NNS_SND_STRM_FORMAT_PCM16) {
        samples /= 2;
    }
    alarmPeriod = timer * samples / interval;

    stream->alarmNo = NNSi_SndAllocAlarm();
    if (stream->alarmNo < 0) {
        return FALSE;
    }

    for (i = 0; i < stream->numChannels; i++) {
        int chNo = stream->chNo[i];

        sChInfo[chNo].buffer = (u8 *)buffer + stream->bufSize * i;
        sChInfo[chNo].volume = 0;
        SND_SetupChannelPcm(chNo, format, sChInfo[chNo].buffer, SND_CHANNEL_LOOP_REPEAT, 0,
                            stream->bufSize / sizeof(u32), SND_CHANNEL_VOLUME_MAX, 0, timer * 32,
                            SND_CHANNEL_PAN_CENTER);
    }

    SND_SetupAlarm(stream->alarmNo, alarmPeriod, alarmPeriod, AlarmCallback, stream);
    NNS_FndAppendListObject(&sStrmList, stream);

    stream->format = format;
    stream->interval = interval;
    stream->callback = callback;
    stream->arg = arg;
    stream->curBlock = 0;
    stream->volume = 0;
    stream->active = TRUE;

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

    if (!stream->started) {
        PM_PrependPreSleepCallback(&stream->preSleepInfo);
        PM_AppendPostSleepCallback(&stream->postSleepInfo);
        stream->started = TRUE;
    }
}

void NNS_SndStrmStop(NNSSndStrm *stream) {
    if (stream->active) {
        ForceStop(stream);
    }
}

void NNS_SndStrmSetVolume(NNSSndStrm *stream, int volume) {
    int i;

    stream->volume = volume;

    for (i = 0; i < stream->numChannels; i++) {
        int chNo = stream->chNo[i];
        u16 chVolume = SND_CalcChannelVolume(stream->volume + sChInfo[chNo].volume);

        SND_SetChannelVolume(1 << chNo, SND_CHANNEL_VOLUME(chVolume), SND_CHANNEL_DATASHIFT(chVolume));
    }
}

void NNS_SndStrmSetChannelPan(NNSSndStrm *stream, int index, int pan) {
    if (index <= stream->numChannels - 1) {
        SND_SetChannelPan(1 << stream->chNo[index], pan);
    }
}

static void ForceStop(NNSSndStrm *stream) {
    if (stream->started) {
        u32 tag;

        SND_StopTimer(stream->chBitMask, 0, 1 << stream->alarmNo, 0);
        PM_DeletePreSleepCallback(&stream->preSleepInfo);
        PM_DeletePostSleepCallback(&stream->postSleepInfo);
        stream->started = FALSE;

        tag = SND_GetCurrentCommandTag();
        SND_FlushCommand(SND_COMMAND_BLOCK);
        SND_WaitForCommandProc(tag);
    }

    Shutdown(stream);
}

static void Shutdown(NNSSndStrm *stream) {
    NNSi_SndFreeAlarm(stream->alarmNo);
    NNS_FndRemoveListObject(&sStrmList, stream);
    stream->active = FALSE;
}

static void AlarmCallback(void *arg) {
    StrmCallback(arg, NNS_SND_STRM_CALLBACK_INTERVAL);
}

// A pointer moved on by a byte offset, an inline in the style of NitroSystem's FND; the name is a guess
static inline void *AddU32ToPtr(void *ptr, u32 val) {
    return (void *)((u32)ptr + val);
}

static void StrmCallback(NNSSndStrm *stream, NNSSndStrmCallbackStatus status) {
    u32 len = stream->bufSize / stream->interval;
    u32 offset = len * stream->curBlock;
    int i;

    for (i = 0; i < stream->numChannels; i++) {
        sBufPtr[i] = AddU32ToPtr(sChInfo[stream->chNo[i]].buffer, offset);
    }

    stream->callback(status, stream->numChannels, sBufPtr, len, stream->format, stream->arg);

    if (++stream->curBlock >= stream->interval) {
        stream->curBlock = 0;
    }
}

static void StrmBeginSleep(void *arg) {
    NNSSndStrm *stream = arg;
    u32 tag;

    if (stream->started) {
        SND_StopTimer(stream->chBitMask, 0, 1 << stream->alarmNo, 0);
        tag = SND_GetCurrentCommandTag();
        SND_FlushCommand(SND_COMMAND_BLOCK);
        SND_WaitForCommandProc(tag);
    }
}

// The buffer is filled up to its end again before the channels start over from its start
static void StrmEndSleep(void *arg) {
    NNSSndStrm *stream = arg;

    if (stream->started) {
        while (stream->curBlock != 0) {
            u32 oldIntr = OS_DisableInterrupts();
            StrmCallback(stream, NNS_SND_STRM_CALLBACK_INTERVAL);
            OS_RestoreInterrupts(oldIntr);
        }

        SND_StartTimer(stream->chBitMask, 0, 1 << stream->alarmNo, 0);
    }
}
