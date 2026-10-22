#include "snd_internal.h"

// NitroSystem's sound resource manager (NNS_Snd), which keeps the bit masks of the channels, capture channels and
// alarms that the library has taken from the driver. The SDK's file is resource_mgr.c, and the names are NitroSystem's,
// as ntrtwl's reconstruction of NitroSystem has them. swan names NNS_SndLockChannel and NNS_SndUnlockChannel
// sndEnableChannels and sndDisableChannels

static u32 sChannelLock;
static u32 sCaptureLock;
static u32 sAlarmLock;

BOOL NNS_SndLockChannel(u32 chBitMask) {
    if (chBitMask == 0) {
        return TRUE;
    }
    if (chBitMask & sChannelLock) {
        return FALSE;
    }
    SND_LockChannel(chBitMask, 0);
    sChannelLock |= chBitMask;
    return TRUE;
}

void NNS_SndUnlockChannel(u32 chBitMask) {
    if (chBitMask == 0) {
        return;
    }
    SND_UnlockChannel(chBitMask, 0);
    sChannelLock &= ~chBitMask;
}

void NNS_SndUnlockCapture(u32 capBitFlag) {
    sCaptureLock &= ~capBitFlag;
}

int NNS_SndAllocAlarm(void) {
    int alarmNo;
    u32 bitMask = 1;

    for (alarmNo = 0; alarmNo < SND_ALARM_NUM; alarmNo++) {
        if (!(sAlarmLock & bitMask)) {
            sAlarmLock |= bitMask;
            return alarmNo;
        }
        bitMask <<= 1;
    }
    return -1;
}

void NNS_SndFreeAlarm(int alarmNo) {
    sAlarmLock &= ~(1 << alarmNo);
}

void NNSi_SndInitResourceMgr(void) {
    sChannelLock = 0;
    sCaptureLock = 0;
    sAlarmLock = 0;
}
