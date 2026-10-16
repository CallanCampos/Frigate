#include "snd_internal.h"

// NitroSystem's sound resource manager (NNS_Snd), which keeps the bit masks of the channels, capture channels and
// alarms that the library has taken from the driver. The file name is a guess, from NitroSystem's names for its sound
// files. NNS_SndLockChannel and NNS_SndUnlockChannel are swan's names for NNS_SndLockChannel and NNS_SndUnlockChannel,
// and NNSi_SndAllocAlarm and NNSi_SndFreeAlarm are NNSi_SndAllocAlarm and NNSi_SndFreeAlarm by their code

#define SND_ALARM_NUM 8

static u32 sLockChannel;
static u32 sLockCapture;
static u32 sAllocatedAlarm;

BOOL NNS_SndLockChannel(u32 chBitMask) {
    if (chBitMask == 0) {
        return TRUE;
    }
    if (chBitMask & sLockChannel) {
        return FALSE;
    }
    SND_LockChannel(chBitMask, 0);
    sLockChannel |= chBitMask;
    return TRUE;
}

void NNS_SndUnlockChannel(u32 chBitMask) {
    if (chBitMask == 0) {
        return;
    }
    SND_UnlockChannel(chBitMask, 0);
    sLockChannel &= ~chBitMask;
}

void NNS_SndUnlockCapture(u32 capBitFlag) {
    sLockCapture &= ~capBitFlag;
}

int NNSi_SndAllocAlarm(void) {
    int alarmNo;
    u32 bitMask = 1;

    for (alarmNo = 0; alarmNo < SND_ALARM_NUM; alarmNo++) {
        if (!(sAllocatedAlarm & bitMask)) {
            sAllocatedAlarm |= bitMask;
            return alarmNo;
        }
        bitMask <<= 1;
    }
    return -1;
}

void NNSi_SndFreeAlarm(int alarmNo) {
    sAllocatedAlarm &= ~(1 << alarmNo);
}

void NNSi_SndInitResourceMgr(void) {
    sLockChannel = 0;
    sLockCapture = 0;
    sAllocatedAlarm = 0;
}
