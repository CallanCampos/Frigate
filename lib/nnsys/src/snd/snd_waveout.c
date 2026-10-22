#include "snd_internal.h"

// NitroSystem's wave output (NNS_SndWaveOut), which plays raw samples on a channel locked for it. The file name is a
// guess, from NitroSystem's names for its sound files. swan names NNS_SndWaveOutAllocChannel,
// NNS_SndWaveOutFreeChannel, NNS_SndWaveOutStart, NNS_SndWaveOutStop and NNS_SndWaveOutIsPlaying
// NNS_SndWaveOutAllocChannel, NNS_SndWaveOutFreeChannel, NNS_SndWaveOutStart, NNS_SndWaveOutStop and
// NNS_SndWaveOutIsPlaying, and NNS_SndWaveOutSetVolume and NNS_SndWaveOutSetSpeed NNS_SndWaveOutSetVolume and
// NNS_SndWaveOutSetSpeed

#define NNS_SND_WAVE_FORMAT_PCM8 0
#define NNS_SND_WAVE_FORMAT_ADPCM 2

// startFlag is set once the driver has started the channel, after the command tagged commandTag
typedef struct NNSSndWaveOut {
    int chNo;
    int sampleRate;
    BOOL playFlag;
    BOOL startFlag;
    u32 commandTag;
} NNSSndWaveOut;

static NNSSndWaveOut sWaveOut[SND_CHANNEL_NUM];

// The channel timer that plays sampleRate samples a second at speed, a ratio with 0x8000 for 1
static inline int CalcTimer(int sampleRate, int speed) {
    u64 timer = (u64)SND_TIMER_CLOCK * 0x8000 / sampleRate / speed;

    if (timer < SND_CHANNEL_TIMER_MIN) {
        timer = SND_CHANNEL_TIMER_MIN;
    } else if (timer > SND_CHANNEL_TIMER_MAX) {
        timer = SND_CHANNEL_TIMER_MAX;
    }
    return timer;
}

NNSSndWaveOutHandle NNS_SndWaveOutAllocChannel(int chNo) {
    NNSSndWaveOut *waveOut;

    if (!NNS_SndLockChannel(1 << chNo)) {
        return NULL;
    }
    waveOut = &sWaveOut[chNo];
    waveOut->chNo = chNo;
    waveOut->playFlag = FALSE;
    return waveOut;
}

void NNS_SndWaveOutFreeChannel(NNSSndWaveOutHandle handle) {
    NNSSndWaveOut *waveOut = handle;

    NNS_SndUnlockChannel(1 << waveOut->chNo);
}

BOOL NNS_SndWaveOutStart(NNSSndWaveOutHandle handle, NNSSndWaveFormat format, const void *data, BOOL loop,
                         int loopStartSample, int samples, int sampleRate, int volume, int speed, int pan) {
    NNSSndWaveOut *waveOut = handle;
    int loopStart;
    int loopLen;
    int timer;

    switch (format) {
    case NNS_SND_WAVE_FORMAT_PCM8:
        loopStart = loopStartSample >> 2;
        loopLen = (samples >> 2) - loopStart;
        break;
    case NNS_SND_WAVE_FORMAT_PCM16:
        loopStart = loopStartSample >> 1;
        loopLen = (samples >> 1) - loopStart;
        break;
    case NNS_SND_WAVE_FORMAT_ADPCM:
        loopStart = loopStartSample >> 3;
        loopLen = (samples >> 3) - loopStart;
        break;
    }

    timer = CalcTimer(sampleRate, speed);
    SND_SetupChannelPcm(waveOut->chNo, format, data, loop ? SND_CHANNEL_LOOP_REPEAT : SND_CHANNEL_LOOP_1SHOT, loopStart,
                        loopLen, volume, SND_CHANNEL_DATASHIFT_NONE, timer, pan);
    SND_StartTimer(1 << waveOut->chNo, 0, 0, 0);

    waveOut->playFlag = TRUE;
    waveOut->startFlag = FALSE;
    waveOut->commandTag = SND_GetCurrentCommandTag();
    waveOut->sampleRate = sampleRate;
    return TRUE;
}

void NNS_SndWaveOutStop(NNSSndWaveOutHandle handle) {
    NNSSndWaveOut *waveOut = handle;

    if (waveOut->playFlag) {
        SND_StopTimer(1 << waveOut->chNo, 0, 0, 0);
        waveOut->playFlag = FALSE;
    }
}

void NNS_SndWaveOutSetVolume(NNSSndWaveOutHandle handle, int volume) {
    NNSSndWaveOut *waveOut = handle;

    if (waveOut->playFlag) {
        SND_SetChannelVolume(1 << waveOut->chNo, volume, SND_CHANNEL_DATASHIFT_NONE);
    }
}

void NNS_SndWaveOutSetSpeed(NNSSndWaveOutHandle handle, int speed) {
    NNSSndWaveOut *waveOut = handle;

    if (waveOut->playFlag) {
        SND_SetChannelTimer(1 << waveOut->chNo, CalcTimer(waveOut->sampleRate, speed));
    }
}

BOOL NNS_SndWaveOutIsPlaying(NNSSndWaveOutHandle handle) {
    NNSSndWaveOut *waveOut = handle;

    if (!waveOut->playFlag) {
        return FALSE;
    }
    if (!waveOut->startFlag) {
        if (!SND_IsFinishedCommandTag(waveOut->commandTag)) {
            return TRUE;
        }
        waveOut->startFlag = TRUE;
    }
    if (SND_GetChannelStatus() & (1 << waveOut->chNo)) {
        return TRUE;
    }
    waveOut->playFlag = FALSE;
    return FALSE;
}

void NNS_SndWaveOutWaitForChannelStop(NNSSndWaveOutHandle handle) {
    NNSSndWaveOut *waveOut = handle;
    u32 tag;

    if (!waveOut->playFlag) {
        if (SND_GetChannelStatus() & (1 << waveOut->chNo)) {
            tag = SND_GetCurrentCommandTag();
            SND_FlushCommand(SND_COMMAND_BLOCK);
            SND_WaitForCommandProc(tag);
        }
        return;
    }
    if (!waveOut->startFlag) {
        SND_WaitForCommandProc(waveOut->commandTag);
        waveOut->startFlag = TRUE;
    }
    while (SND_GetChannelStatus() & (1 << waveOut->chNo)) {
    }
}
