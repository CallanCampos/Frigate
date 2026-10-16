#include "nitro/os.h"
#include "snd_internal.h"

// NitroSystem's sound library setup and its work each frame (NNS_Snd), and the copy of the ARM7 driver's state that
// it reads the driver's info from. The file name is a guess, from NitroSystem's names for its sound files. swan names
// NNS_SndInit and NNS_SndMain NNS_SndInit and NNS_SndMain

// Two copies of the driver's state: one the library reads, the other the ARM7 writes. sCurDriverInfo is the one to
// read, or -1 before the first copy has arrived
static SNDDriverInfo sDriverInfo[2] ATTRIBUTE_ALIGN(32);
static s8 sCurDriverInfo;
static BOOL sDriverInfoFirstFlag;
static u32 sDriverInfoCommandTag;
static PMSleepCallbackInfo sPostSleepCallback;
static PMSleepCallbackInfo sPreSleepCallback;

static void BeginSleep(void *arg);
static void EndSleep(void *arg);

static inline const SNDDriverInfo *GetCurrentDriverInfo(void) {
    if (sCurDriverInfo < 0) {
        return NULL;
    }
    return &sDriverInfo[sCurDriverInfo];
}

void NNS_SndInit(void) {
    static BOOL initialized = FALSE;

    if (initialized) {
        return;
    }
    initialized = TRUE;

    SND_Init();

    PM_SetSleepCallbackInfo(&sPreSleepCallback, BeginSleep, NULL);
    PM_SetSleepCallbackInfo(&sPostSleepCallback, EndSleep, NULL);
    PM_PrependPreSleepCallback(&sPreSleepCallback);
    PM_AppendPostSleepCallback(&sPostSleepCallback);

    NNSi_SndInitResourceMgr();
    NNSi_SndCaptureInit();
    NNSi_SndPlayerInit();

    sCurDriverInfo = -1;
    sDriverInfoFirstFlag = TRUE;
}

void NNS_SndMain(void) {
    while (SND_RecvCommandReply(SND_COMMAND_NOBLOCK)) {
    }

    NNSi_SndPlayerMain();
    NNSi_SndCaptureMain();
    NNSi_SndArcStrmMain();

    SND_FlushCommand(SND_COMMAND_NOBLOCK);
}

void NNS_SndSetMasterVolume(int volume) {
    SND_SetMasterVolume(volume);
}

void NNS_SndSetMonoFlag(BOOL flag) {
    if (flag) {
        SND_SetMasterPan(SND_CHANNEL_PAN_CENTER);
    } else {
        SND_ResetMasterPan();
    }
}

BOOL NNS_SndUpdateDriverInfo(void) {
    if (!sDriverInfoFirstFlag) {
        while (SND_RecvCommandReply(SND_COMMAND_NOBLOCK)) {
        }
        if (!SND_IsFinishedCommandTag(sDriverInfoCommandTag)) {
            return FALSE;
        }

        if (sCurDriverInfo < 0) {
            sCurDriverInfo = 1;
        }
        SND_ReadDriverInfo(&sDriverInfo[sCurDriverInfo]);
        sDriverInfoCommandTag = SND_GetCurrentCommandTag();

        if (sCurDriverInfo == 0) {
            sCurDriverInfo = 1;
        } else {
            sCurDriverInfo = 0;
        }
        DC_InvalidateRange(&sDriverInfo[sCurDriverInfo], sizeof(SNDDriverInfo));

        SND_FlushCommand(SND_COMMAND_NOBLOCK);
        return TRUE;
    }

    SND_ReadDriverInfo(&sDriverInfo[0]);
    sDriverInfoCommandTag = SND_GetCurrentCommandTag();
    sDriverInfoFirstFlag = FALSE;
    return FALSE;
}

BOOL NNSi_SndReadDriverTrackInfo(int playerNo, int trackNo, SNDTrackInfo *trackInfo) {
    const SNDDriverInfo *driverInfo = GetCurrentDriverInfo();

    if (!driverInfo) {
        return FALSE;
    }
    return SND_ReadTrackInfo(driverInfo, playerNo, trackNo, trackInfo);
}

// Before sleep, the capture and every channel stop, and the commands are sent and waited for
static void BeginSleep(void *arg) {
    u32 tag;

    NNSi_SndCaptureBeginSleep();
    SND_StopTimer(0, 0, 0, 0);

    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
}

static void EndSleep(void *arg) {
    NNSi_SndCaptureEndSleep();
}
