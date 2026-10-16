#include "nitro/mi.h"
#include "nitro/os.h"
#include "snd_internal.h"

// NitroSystem's capture (NNSi_SndCapture), which records the mixer's output into a buffer and plays it back on
// channels of its own, for reverb and effects. Only what the sound library's main loop and sleep callbacks call is
// left in the game; the functions that start a capture were stripped. The file name is a guess, from NitroSystem's
// names for its sound files

#define CAPTURE_MSG_NUM 8

static void StopCapture(void);

// MWCC lays these out in reverse order of declaration: the flag first and the capture last. sCaptureReserved and
// sCaptureMsgBuf were used only by the functions that start a capture, which the linker dropped, so their names
// are guesses
static NNSiSndCaptureInfo sCaptureInfo;
static OSMessage sCaptureMsgBuf[CAPTURE_MSG_NUM];
static OSMessageQueue sMsgQ;
static u32 sCaptureReserved;
static BOOL sCaptureThreadFlag;

void NNSi_SndCaptureInit(void) {
    sCaptureThreadFlag = FALSE;
    sCaptureInfo.active = FALSE;
}

void NNSi_SndCaptureMain(void) {
    NNSiSndCaptureInfo *info = &sCaptureInfo;
    NNSSndFader *fader;
    int volume;

    if (!info->active) {
        return;
    }
    if (info->type != NNSi_SND_CAPTURE_TYPE_REVERB) {
        return;
    }

    fader = &info->fader;
    NNSi_SndFaderUpdate(fader);
    if (info->fadeStopFlag && NNSi_SndFaderIsFinished(fader)) {
        StopCapture();
        return;
    }

    volume = NNSi_SndFaderGet(fader) >> 8;
    if (volume != info->volume) {
        SND_SetChannelVolume(info->chBitMask, volume, SND_CHANNEL_DATASHIFT_NONE);
        info->volume = volume;
    }
}

static void StopCapture(void) {
    NNSiSndCaptureInfo *info = &sCaptureInfo;
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
        while (OS_ReceiveMessage(&sMsgQ, NULL, OS_MESSAGE_NOBLOCK)) {
        }
    }

    if (info->capBitMask) {
        NNS_SndUnlockCapture(info->capBitMask);
    }
    if (info->lockChBitFlag) {
        NNS_SndUnlockChannel(info->lockChBitFlag);
    }
    if (alarmFlag) {
        NNSi_SndFreeAlarm(info->alarmNo);
    }
    if (info->type == NNSi_SND_CAPTURE_TYPE_EFFECT) {
        SND_SetOutputSelector(0, 0, 0, 0);
    }
    info->active = FALSE;
}

void NNSi_SndCaptureBeginSleep(void) {
    NNSiSndCaptureInfo *info = &sCaptureInfo;
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
    NNSiSndCaptureInfo *info = &sCaptureInfo;

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
