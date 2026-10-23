#include "nitro/mi.h"
#include "nitro/os.h"
#include "snd_internal.h"

// NitroSystem's capture (NNSi_SndCapture), which records the mixer's output into a buffer and plays it back on channels
// of its own, for reverb and effects. Only what the sound library's main loop and sleep callbacks call is left in the
// game; the functions that start a capture were stripped. The SDK's file is capture.c, and the names are NitroSystem's,
// as ntrtwl's reconstruction of NitroSystem has them, except sCaptureReserved and sCaptureThreadFlag, which are ours

#define THREAD_MESSAGE_BUF_SIZE 8

static void NNSi_SndCaptureStop(void);

// MWCC lays these out in reverse order of declaration: the flag first and the capture last. sCaptureReserved and
// sMesgBuf were used only by the functions that start a capture, which the linker dropped
static CaptureParam sCapParam;
static OSMessage sMesgBuf[THREAD_MESSAGE_BUF_SIZE];
static OSMessageQueue sMesgQ;
static u32 sCaptureReserved;
static BOOL sCaptureThreadFlag;

void NNSi_SndCaptureInit(void) {
    sCaptureThreadFlag = FALSE;
    sCapParam.active = FALSE;
}

void NNSi_SndCaptureMain(void) {
    CaptureParam *info = &sCapParam;
    NNSSndFader *fader;
    int volume;

    if (!info->active) {
        return;
    }
    if (info->type != NNS_SND_CAPTURE_TYPE_REVERB) {
        return;
    }

    fader = &info->fader;
    NNSi_SndFaderUpdate(fader);
    if (info->fadeStopFlag && NNSi_SndFaderIsFinished(fader)) {
        NNSi_SndCaptureStop();
        return;
    }

    volume = NNSi_SndFaderGet(fader) >> 8;
    if (volume != info->volume) {
        SND_SetChannelVolume(info->chBitMask, volume, SND_CHANNEL_DATASHIFT_NONE);
        info->volume = volume;
    }
}

static void NNSi_SndCaptureStop(void) {
    CaptureParam *info = &sCapParam;
    BOOL alarmFlag;
    u32 tag;

    if (!info->active) {
        return;
    }

    alarmFlag = info->alarmNo >= 0 ? TRUE : FALSE;
    SND_StopTimer(info->chBitMask, info->capBitMask, alarmFlag ? 1 << info->alarmNo : 0, 0);
    if (alarmFlag) {
        tag = SND_GetCurrentCommandTag();
        SND_FlushCommand(SND_COMMAND_BLOCK);
        SND_WaitForCommandProc(tag);
        while (OS_ReceiveMessage(&sMesgQ, NULL, OS_MESSAGE_NOBLOCK)) {
        }
    }

    if (info->capBitMask) {
        NNS_SndUnlockCapture(info->capBitMask);
    }
    if (info->lockChBitFlag) {
        NNS_SndUnlockChannel(info->lockChBitFlag);
    }
    if (alarmFlag) {
        NNS_SndFreeAlarm(info->alarmNo);
    }
    if (info->type == NNS_SND_CAPTURE_TYPE_EFFECT) {
        SND_SetOutputSelector(0, 0, 0, 0);
    }
    info->active = FALSE;
}

void NNSi_SndCaptureBeginSleep(void) {
    CaptureParam *info = &sCapParam;
    u32 tag;

    if (!info->active) {
        return;
    }
    SND_StopTimer(info->chBitMask, info->capBitMask, info->alarmNo >= 0 ? 1 << info->alarmNo : 0, 0);

    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
}

void NNSi_SndCaptureEndSleep(void) {
    CaptureParam *info = &sCapParam;

    if (!info->active) {
        return;
    }
    info->bufPos = 0;
    MI_CpuClear32(info->bufL, info->bufSize);
    MI_CpuClear32(info->bufR, info->bufSize);
    DC_FlushRange(info->bufL, info->bufSize);
    DC_FlushRange(info->bufR, info->bufSize);

    SND_StartTimer(info->chBitMask, info->capBitMask, info->alarmNo >= 0 ? 1 << info->alarmNo : 0, 0);
}
