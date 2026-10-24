#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "nitrowifi/wcm.h"
#include "nitrowifi/wcm_dsi.h"
#include "wcm_internal.h"

// NitroWiFi's WCM (overlay 11): its initialization, its public operations and their state machine, which hands each
// step to the DS's backend over NitroSDK's WM (wcm_wmif.c) or the DSi's over TwlSDK's NWM (wcm_dsi.c, in the LTD
// autoload). TwlSDK's libwcm names the object system.o; Black 1's decompilation (aletheia) has the same functions, in
// this order, in its wcm_system.c, and the names are the SDK's. The seven Thumb/ARM pairs after the operations that
// call the DSi backend are the linker's long-branch veneers to the LTD autoload, which the C leaves to the linker.

// The "any" BSSID and ESSID that a search takes when the caller gives none
const u8 WCM_Bssid_Any[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 };
const u8 WCM_Essid_Any[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

// The device ID (1 for the DS's wireless, 2 for the DSi's) that WCM_InitEx takes when its caller gives 0, ahead of the
// hardware's default; nothing in the ROM sets it
static u8 WCMi_OverridedDeviceId = 0;
// WCM's work, which WCM_InitEx sets and WCM_Finish clears. The two are initialized statics, which MWCC reaches from the
// first one's address
static WCMWork *wcmw = NULL;

static BOOL WCM_GetWorkSize(s32 *size, s32 mode);
static s32 WCM_BeginSearchAsync(const u8 *bssid, const u8 *essid, u32 option);
static void WcmConfigure(WCMConfig *config, WCMNotify notify);
static void WcmInitOption(void);
static void WcmKeepAliveAlarm(void);
static s32 WCMi_GetDefaultDeviceId(void);

s32 WCM_Init(void *work, s32 size) {
    return WCM_InitEx(work, size, 1);
}

s32 WCM_InitEx(void *work, s32 size, s32 requestedMode) {
    OSIntrMode irq = OS_DisableInterrupts();
    s32 requiredSize = 0;

    if (wcmw != NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    if (((u32)work & 0x1f) != 0) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    MI_CpuFill8(work, 0, size);
    if (requestedMode == 0) {
        requestedMode = WCMi_OverridedDeviceId;
        if (requestedMode == 0) {
            requestedMode = WCMi_GetDefaultDeviceId();
        }
    }
    if (!WCM_GetWorkSize(&requiredSize, requestedMode)) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    if (size < requiredSize) {
        OS_RestoreInterrupts(irq);
        return 6;
    }
    wcmw = work;
    wcmw->state = 1;
    wcmw->pendingNotify = 0;
    wcmw->scanChannelTime = 0;
    wcmw->apListLocked = 0;
    wcmw->resetPending = 0;
    wcmw->wirelessMode = requestedMode;
    WcmInitOption();
    WCMi_InitCpsif();
    if (!OS_IsTickAvailable()) {
        OS_InitTick();
    }
    if (!OS_IsAlarmAvailable()) {
        OS_InitAlarm();
    }
    OS_CreateAlarm(&wcmw->alarm);
    OS_RestoreInterrupts(irq);
    return 0;
}

s32 WCM_Finish(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (wcmw == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    if (wcmw->state != 1) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    wcmw = NULL;
    OS_RestoreInterrupts(irq);
    return 0;
}

static BOOL WCM_GetWorkSize(s32 *size, s32 mode) {
    if (mode == 0) {
        mode = WCMi_GetDefaultDeviceId();
    }
    switch (mode) {
    case 1:
        *size = WCM_WORK_SIZE;
        break;
    case 2:
        if (!OS_IsRunOnTwl()) {
            return FALSE;
        }
        *size = 0x4800;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

s32 WCM_StartupAsync(WCMConfig *config, WCMNotify notify) {
    OSIntrMode irq;
    s32 result;

    irq = OS_DisableInterrupts();
    if (wcmw == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (wcmw->state) {
    case 1:
        WcmConfigure(config, notify);
        break;
    case 2:
        OS_RestoreInterrupts(irq);
        return 2;
    case 3:
        OS_RestoreInterrupts(irq);
        return 0;
    default:
        OS_RestoreInterrupts(irq);
        return 1;
    }

    if (!WM_GetAllowedChannel()) {
        // BUG: Returns with interrupts still disabled
#ifdef BUGFIX
        OS_RestoreInterrupts(irq);
#endif
        return 5;
    }
    switch (wcmw->wirelessMode) {
    case 1:
        result = WCMi_WmifStartupAsync();
        break;
    case 2:
        if (OS_IsRunOnTwl()) {
            result = func_02769930();
            break;
        }
        // Mode 2 without DSi hardware is unsupported, like any other mode
    default:
        result = 7;
        break;
    }
    OS_RestoreInterrupts(irq);
    return result;
}

s32 WCM_CleanupAsync(void) {
    OSIntrMode irq;
    WCMWork *work;
    s32 result;

    irq = OS_DisableInterrupts();
    work = wcmw;
    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (work->state) {
    case 3:
        break;
    case 4:
        OS_RestoreInterrupts(irq);
        return 2;
    case 1:
        OS_RestoreInterrupts(irq);
        return 0;
    default:
        OS_RestoreInterrupts(irq);
        return 1;
    }

    switch (work->wirelessMode) {
    case 1:
        result = WCMi_WmifCleanupAsync();
        break;
    case 2:
        if (OS_IsRunOnTwl()) {
            result = func_02769a30();
            break;
        }
        // Mode 2 without DSi hardware is unsupported, like any other mode
    default:
        result = 7;
        break;
    }
    OS_RestoreInterrupts(irq);
    return result;
}

s32 WCM_SearchAsync(void *bssid, void *essid, u32 option) {
    if (bssid == NULL || essid == NULL) {
        return WCM_EndSearchAsync();
    }
    return WCM_BeginSearchAsync(bssid, essid, option);
}

static s32 WCM_BeginSearchAsync(const u8 *bssid, const u8 *essid, u32 option) {
    OSIntrMode irq;
    WCMWork *work;
    s32 result;

    irq = OS_DisableInterrupts();
    work = wcmw;
    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (work->wirelessMode) {
    case 1:
        result = WCMi_WmifBeginSearchAsync(bssid, essid, option);
        break;
    case 2:
        if (OS_IsRunOnTwl()) {
            result = func_02769aa4(bssid, essid, option);
            break;
        }
        // Mode 2 without DSi hardware is unsupported, like any other mode
    default:
        result = 7;
        break;
    }
    OS_RestoreInterrupts(irq);
    return result;
}

s32 WCM_EndSearchAsync(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (wcmw == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (wcmw->state) {
    case 6:
        WCMi_SetPhase(7);
        wcmw->pendingNotify = WCM_NOTIFY_END_SEARCH;
        break;
    case 7:
        OS_RestoreInterrupts(irq);
        return 2;
    case 3:
        OS_RestoreInterrupts(irq);
        return 0;
    default:
        OS_RestoreInterrupts(irq);
        return 1;
    }
    OS_RestoreInterrupts(irq);
    return 3;
}

s32 WCM_ConnectAsync(void *bssDesc, void *wepDesc, u32 option) {
    u32 irq;
    WCMWork *work;
    s32 result;

    irq = OS_DisableInterrupts();
    work = WCMi_GetSystemWork();
    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (work->state) {
    case 3:
        if (bssDesc == NULL) {
            OS_RestoreInterrupts(irq);
            return 1;
        }
        if (((const WMBssDesc *)bssDesc)->gameInfoLength != 0) {
            OS_RestoreInterrupts(irq);
            return 1;
        }
        if (wepDesc != NULL) {
            const u8 *descriptor = wepDesc;

            // BUG: The original checks the previous argument byte, allowing a new length beyond the apparent 0x40
            // limit.
#ifdef BUGFIX
            if ((descriptor[0] <= 3 && descriptor[1] >= 4) || descriptor[1] > 0x40) {
#else
            if ((descriptor[0] <= 3 && descriptor[1] >= 4) || work->connectionStorage.argument > 0x40) {
#endif
                OS_RestoreInterrupts(irq);
                return 1;
            }
            work->connectionStorage.mode = descriptor[0];
            if (descriptor[0] == 0) {
                MI_CpuFill8(work->connectionStorage.data, 0, sizeof(work->connectionStorage.data));
            } else if (descriptor[0] <= 3) {
                work->connectionStorage.argument = descriptor[1];
                MI_CpuCopy8(descriptor + 2, work->connectionStorage.data, sizeof(work->connectionStorage.data));
            } else {
                work->connectionStorage.argument = descriptor[1];
                MI_CpuCopy8(descriptor + 2, work->connectionStorage.data, work->connectionStorage.argument);
            }
        } else {
            MI_CpuFill8(&work->connectionStorage, 0, sizeof(work->connectionStorage));
        }
        MI_CpuCopy8(bssDesc, &work->connectBss, sizeof(work->connectBss));
        WCM_UpdateOption(option);
        break;
    case 8:
        OS_RestoreInterrupts(irq);
        return 2;
    case 9:
        OS_RestoreInterrupts(irq);
        return 0;
    default:
        OS_RestoreInterrupts(irq);
        return 1;
    }

    switch (work->wirelessMode) {
    case 1:
        result = WCMi_WmifConnectAsync();
        break;
    case 2:
        if (OS_IsRunOnTwl()) {
            result = func_02769b80();
            break;
        }
        // Mode 2 without DSi hardware is unsupported, like any other mode
    default:
        result = 7;
        break;
    }
    OS_RestoreInterrupts(irq);
    return result;
}

s32 WCM_DisconnectAsync(void) {
    u32 irq = OS_DisableInterrupts();
    s32 result = 0;
    WCMWork *work = wcmw;

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (work->state) {
    case 9:
        break;
    case 10:
        OS_RestoreInterrupts(irq);
        return 2;
    case 3:
        OS_RestoreInterrupts(irq);
        return 0;
    default:
        OS_RestoreInterrupts(irq);
        return 1;
    }

    if (work->resetPending == 1) {
        WCMi_SetPhase(10);
        wcmw->pendingNotify = 6;
    } else {
        switch (work->wirelessMode) {
        case 1:
            result = WCMi_WmifDisconnectAsync();
            break;
        case 2:
            if (OS_IsRunOnTwl()) {
                result = func_02769c70();
                break;
            }
            // Mode 2 without DSi hardware is unsupported, like any other mode
        default:
            result = 7;
            break;
        }
    }
    OS_RestoreInterrupts(irq);
    return result;
}

s32 WCM_TerminateAsync(void) {
    u32 irq = OS_DisableInterrupts();
    s32 result = 0;

    if (wcmw == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (wcmw->state) {
    case 13:
        OS_RestoreInterrupts(irq);
        return 2;
    case 1:
        OS_RestoreInterrupts(irq);
        return 0;
    case 6:
        WCMi_SetPhase(13);
        wcmw->pendingNotify = 9;
        OS_RestoreInterrupts(irq);
        return 3;
    case 3:
    case 9:
    case 12:
        break;
    default:
        OS_RestoreInterrupts(irq);
        return 1;
    }

    if (wcmw->resetPending == 1) {
        WCMi_SetPhase(13);
        wcmw->pendingNotify = 9;
    } else {
        switch (wcmw->wirelessMode) {
        case 1:
            result = WCMi_WmifTerminateAsync();
            break;
        case 2:
            if (OS_IsRunOnTwl()) {
                result = func_02769ce8();
                break;
            }
            // Mode 2 without DSi hardware is unsupported, like any other mode
        default:
            result = 7;
            break;
        }
    }
    OS_RestoreInterrupts(irq);
    return result;
}

s32 WCM_GetPhase(void) {
    u32 irq;
    WCMWork *work;
    s32 state;

    irq = OS_DisableInterrupts();
    work = wcmw;
    state = 0;
    if (work != NULL) {
        state = work->state;
    }
    OS_RestoreInterrupts(irq);
    return state;
}

u32 WCM_UpdateOption(u32 option) {
    u32 irq = OS_DisableInterrupts();
    u32 mask = 0;
    // BUG: The original reads the search option before checking whether WCM has been initialized.
#ifdef BUGFIX
    u32 old;

    if (wcmw == NULL) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
    old = wcmw->searchOption;
#else
    u32 old = wcmw->searchOption;

    if (wcmw == NULL) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
#endif
    if (option & 0x8000) {
        mask |= 0x3ffe;
        if (!(option & 0x3ffe)) {
            option |= 0xa082;
        }
    }
    if (option & 0x20000) {
        mask |= 0x10000;
    }
    if (option & 0x80000) {
        mask |= 0x40000;
    }
    if (option & 0x200000) {
        mask |= 0x100000;
    }
    if (option & 0x800000) {
        mask |= 0x400000;
    }
    wcmw->searchOption = (old & ~mask) | option;
    OS_RestoreInterrupts(irq);
    return old;
}

s32 WCM_GetAllowedChannel(void) {
    WCMWork *work = WCMi_GetSystemWork();

    if (work->state >= 3 && work->state <= 10) {
        switch (work->wirelessMode) {
        case 1:
            return WCMi_WmifGetAllowedChannel();
        case 2:
            if (OS_IsRunOnTwl()) {
                return func_02769e54();
            }
            break;
        }
    }
    return 0;
}

WCMWork *WCMi_GetSystemWork(void) {
    return wcmw;
}

static void WcmConfigure(WCMConfig *config, WCMNotify notify) {
    if (config == NULL) {
        wcmw->dmaNo = 3;
        wcmw->apBuffer = NULL;
        wcmw->apBufferBytes = 0;
        wcmw->scanMode = 0;
    } else {
        wcmw->dmaNo = config->dmano & 3;
        if (((4 - ((u32)config->pbdbuffer & 3)) % 4) + sizeof(WCMApListHeader) > config->nbdbuffer) {
            wcmw->apBuffer = NULL;
            wcmw->apBufferBytes = 0;
        } else {
            wcmw->apBuffer = (void *)(((u32)config->pbdbuffer + 3) & ~3);
            wcmw->apBufferBytes = config->nbdbuffer - (4 - ((u32)config->pbdbuffer & 3)) % 4;
            MI_CpuFill8(wcmw->apBuffer, 0, wcmw->apBufferBytes);
        }
        wcmw->scanMode = config->nbdmode;
    }
    wcmw->notify = notify;
}

static void WcmInitOption(void) {
    wcmw->searchOption = 0xaaa082;
}

u16 WCMi_GetNextScanChannel(s32 channelIndex) {
    s32 i;
    s32 channel;

    for (i = 0; i < 13; i++) {
        channel = (channelIndex + i) % 13;
        if (wcmw->searchOption & (1 << (channel + 1))) {
            if (WCM_GetAllowedChannel() & (1 << channel)) {
                break;
            }
        }
    }
    return (u16)((channelIndex + i) % 13 + 1);
}

void WCMi_Notify(s16 result, void *parameter, u32 detail, u32 line) {
    WCMWork *work = wcmw;
    s16 notify = work->pendingNotify;

    work->pendingNotify = 0;
    // The SDK transports mixed pointer/integer payloads as 32-bit words; a host port must widen this representation.
    WCMi_NotifyEx(notify, result, (s32)parameter, (void *)detail, line);
}

void WCMi_NotifyEx(s16 notify, s16 result, s32 parameter, void *param1, s32 param2) {
    WCMWork *work = wcmw;
    WCMNotice notice;

    if (work->notify == NULL) {
        return;
    }
    notice.notify = notify;
    notice.result = result;
    notice.parameter[0].n = parameter;
    notice.parameter[1].p = param1;
    notice.parameter[2].n = param2;
    work->notify(&notice);
}

void WCMi_SetPhase(s32 newState) {
    u32 irq = OS_DisableInterrupts();

    if (wcmw->state == 9 && newState != 9) {
        OS_CancelAlarm(&wcmw->alarm);
    }
    if (wcmw->state != 11) {
        wcmw->state = newState;
    }
    if (newState == 9) {
        OS_CancelAlarm(&wcmw->alarm);
        OS_SetAlarm(&wcmw->alarm, 0x22f5341, (void (*)(void *))WcmKeepAliveAlarm, NULL);
    }
    OS_RestoreInterrupts(irq);
}

void WCMi_ResetKeepAliveAlarm(void) {
    u32 irq = OS_DisableInterrupts();

    OS_CancelAlarm(&wcmw->alarm);
    if (wcmw->state == 9) {
        OS_SetAlarm(&wcmw->alarm, 0x22f5341, (void (*)(void *))WcmKeepAliveAlarm, NULL);
    }
    OS_RestoreInterrupts(irq);
}

static void WcmKeepAliveAlarm(void) {
    WCMi_ResetKeepAliveAlarm();
}

static s32 WCMi_GetDefaultDeviceId(void) {
    return OS_IsRunOnTwl() ? 2 : 1;
}
