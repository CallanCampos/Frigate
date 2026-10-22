#include "nitrowifi/wcm.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "nitrowifi/wcm_dsi.h"
#include "nitrowifi/wcm_internal.h"
#include "twl/nwm.h"
#include <stddef.h>

// NitroWiFi's WCM (overlay 11), over NitroSDK's WM. dsd splits this range into 93 pieces; eight Thumb/ARM pairs among
// them are the linker's long-branch veneers to the DSi WCM backend physically placed in LTD. This file calls
// their real targets directly instead of writing the veneers out. The ROM names none of this; established WCM
// names are kept where identified. WCM_StopSearchAsync, WCM_RestoreIdleAsync, WCM_GetConnectedApSsid and
// WCM_ReleaseIrqOwnedMutex, along with the private helper/callback names, are descriptive reconstructions.

// A vendor OUI and the type byte after it, which WCM_FindVendorIE copies as a whole
typedef struct WCMVendorOui {
    u8 bytes[4];
} WCMVendorOui;

// Vendor OUIs, each followed by a type byte, that WCM_FillApEntry and WCM_FindVendorIE look for among a beacon's
// vendor-specific information elements: Nintendo's (00:09:BF), and Microsoft's (00:50:F2) WPA (type 1) and WMM
// (type 2, subtype 0). Defined with their bytes after WCMiAnyEssid, at the end of the file
static const WCMVendorOui sNintendoOui;
static const u8 sWpaOui[4];
static const u8 sWmmOui[8];

// An access point WCM found during a search, in a doubly linked list off the caller's apBuffer. WCM_PointApList's
// result points at data, past this header
typedef struct WCMApEntry WCMApEntry;
struct WCMApEntry {
    u8 inUse;
    u8 unk_01;
    u16 scanLinkLevel; // DS scan link level; the DSi producer's value domain is unconfirmed
    u32 id;            // the AP list header's count when this entry was allocated or reused
    WCMApEntry *prev;
    WCMApEntry *next;
    u32 data[WCM_APLIST_SIZE / sizeof(u32)]; // a WMBssDesc's prefix; bytes 0x40..0xbf hold packed IEs, not WMGameInfo
};

// The vendor-specific information element that carries a DSi beacon's game info, which starts with the info's magic
// number
typedef struct WCMVendorIE {
    u8 id;
    u8 length;
    u8 unk_02[8];
    u16 magicNumber;
} WCMVendorIE;

// The caller's AP list buffer (WCMConfig.pbdbuffer), read as a header followed by up to apMax WCMApEntry records
typedef struct WCMApListHeader {
    u32 count;
    WCMApEntry *head; // the oldest entry (WCM_GetOldestApEntry's result)
    WCMApEntry *tail; // the newest
    WCMApEntry entries[];
} WCMApListHeader;

// The 802.3 header of a mode 2 frame. Only the length, big-endian, is filled in here
typedef struct WCMEthernetHeader {
    u8 destination[6];
    u8 source[6];
    u16 length;
} WCMEthernetHeader;

// WCM's own small globals, zeroed with the rest of .bss
// sWcmAnchor: the anchor WCM_Init and WCM_Finish set and clear
typedef struct WCMAnchor {
    u8 preferredMode; // 0x00, read when the requested mode is zero; no writer identified
    u8 unk_01[3];
    WCMWork *work; // 0x04
} WCMAnchor;

static WCMAnchor sWcmAnchor;
static WCMGlobal sWcmGlobal;
// The last 16 signal samples, a ring, and the count of samples until it is full, then 16 plus the next index
static u8 sWcmRssiSamples[16];
static u8 sWcmRssiCursor = 0;

static BOOL WCM_GetRequiredWorkSize(s32 *size, s32 mode);
static s32 WCM_SearchAsyncWithParams(const u8 *bssid, const u8 *essid, u32 option);
static void WCM_SetConfig(WCMConfig *config, WCMNotify notify);
static void WCM_ResetSearchOption(void);
static void WCM_TimeoutAlarmHandler(void);
static s32 WCM_GetDefaultWirelessMode(void);
static WMBssDesc *WCM_PointApListCore(u16 index);
static WCMApEntry *WCM_AllocApEntry(void);
static WCMApEntry *WCM_GetOldestApEntry(void);
static WCMApEntry *WCM_FindApEntryByBssid(const u8 *bssid);
static WCMApEntry *WCM_FindApEntryById(u32 id);
static void WCM_TouchApEntry(WCMApEntry *entry);
static void *WCM_FindVendorIE(const WMBssDesc *bss);
static void *WCM_GetInformationElement(const void *frame, u32 tag, const u8 *oui);
static void WCM_InitOnce(void);
static s32 WCM_AssembleSendFrame(s32 mode, void *dest, const void *header, s32 headerLen, const void *data,
                                 s32 dataLen);
static BOOL WCM_CompareBytes(const void *a, const void *b, u32 size);
static BOOL WCM_CompareBssID(const u8 *a, const u8 *b);
static WMLinkLevel WCM_RssiToQuality(u32 rssi);
static u8 WCM_GetAverageRssi(void);
static s32 WCM_StartupStep1(void);
static s32 WCM_StartupStep2(void);
static s32 WCM_StartSearchStep(const u8 *bssid, const u8 *essid, u32 option);
static s32 WCM_StartConnectStep(void);
static s32 WCM_StartDisconnectStep(void);
static s32 WCM_RestoreIdleStep(void);
static s32 WCM_SendFrame(const u8 *dst, const void *buffer, u16 byteLen, void (*completion)(s32));
static u16 WCM_GetDsAllowedChannelSpanMask(void);
static void WCM_RequestReset(void);
static void WCM_OnWmIndication(void *ind);
static void WCM_OnWmControlComplete(void *ind);
static void WCM_ConnectStep4(void *ind);
static void WCM_ConnectStep5(void *ind);
static void WCM_OnConnectStatus(void *ind);
static void WCM_OnDisconnectComplete(void *ind);
static void WCM_OnDcfReceive(void *ind);
static void WCM_OnDisconnectPreparationComplete(void *ind);
static void WCM_OnResetComplete(void *ind);
static void WCM_BuildScanRequest(const u8 *bssid, const u8 *essid, u32 option);
static void WCM_OnSendComplete(void *ind);
static void WCM_OnSendCompleteWithCallback(void *ind);

s32 WCM_Init(void *work, s32 size) {
    return WCM_InitCore(work, size, 1);
}

s32 WCM_InitCore(void *work, s32 size, s32 requestedMode) {
    OSIntrMode irq = OS_DisableInterrupts();
    s32 requiredSize = 0;

    if (sWcmAnchor.work != NULL) {
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
        requestedMode = sWcmAnchor.preferredMode;
        if (requestedMode == 0) {
            requestedMode = WCM_GetDefaultWirelessMode();
        }
    }
    if (!WCM_GetRequiredWorkSize(&requiredSize, requestedMode)) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    if (size < requiredSize) {
        OS_RestoreInterrupts(irq);
        return 6;
    }
    sWcmAnchor.work = work;
    sWcmAnchor.work->state = 1;
    sWcmAnchor.work->pendingNotify = 0;
    sWcmAnchor.work->scanChannelTime = 0;
    sWcmAnchor.work->apListLocked = 0;
    sWcmAnchor.work->resetPending = 0;
    sWcmAnchor.work->wirelessMode = requestedMode;
    WCM_ResetSearchOption();
    WCM_InitOnce();
    if (!OS_IsTickAvailable()) {
        OS_InitTick();
    }
    if (!OS_IsAlarmAvailable()) {
        OS_InitAlarm();
    }
    OS_CreateAlarm(&sWcmAnchor.work->alarm);
    OS_RestoreInterrupts(irq);
    return 0;
}

s32 WCM_Finish(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (sWcmAnchor.work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    if (sWcmAnchor.work->state != 1) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    sWcmAnchor.work = NULL;
    OS_RestoreInterrupts(irq);
    return 0;
}

static BOOL WCM_GetRequiredWorkSize(s32 *size, s32 mode) {
    if (mode == 0) {
        mode = WCM_GetDefaultWirelessMode();
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
    if (sWcmAnchor.work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (sWcmAnchor.work->state) {
    case 1:
        WCM_SetConfig(config, notify);
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
    switch (sWcmAnchor.work->wirelessMode) {
    case 1:
        result = WCM_StartupStep1();
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
    work = sWcmAnchor.work;
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
        result = WCM_StartupStep2();
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
        return WCM_StopSearchAsync();
    }
    return WCM_SearchAsyncWithParams(bssid, essid, option);
}

static s32 WCM_SearchAsyncWithParams(const u8 *bssid, const u8 *essid, u32 option) {
    OSIntrMode irq;
    WCMWork *work;
    s32 result;

    irq = OS_DisableInterrupts();
    work = sWcmAnchor.work;
    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (work->wirelessMode) {
    case 1:
        result = WCM_StartSearchStep(bssid, essid, option);
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

s32 WCM_StopSearchAsync(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (sWcmAnchor.work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (sWcmAnchor.work->state) {
    case 6:
        WCM_SetState(7);
        sWcmAnchor.work->pendingNotify = WCM_NOTIFY_END_SEARCH;
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
    work = WCM_GetWork();
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
        WCM_SetSearchOption(option);
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
        result = WCM_StartConnectStep();
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
    WCMWork *work = sWcmAnchor.work;

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
        WCM_SetState(10);
        sWcmAnchor.work->pendingNotify = 6;
    } else {
        switch (work->wirelessMode) {
        case 1:
            result = WCM_StartDisconnectStep();
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

s32 WCM_RestoreIdleAsync(void) {
    u32 irq = OS_DisableInterrupts();
    s32 result = 0;

    if (sWcmAnchor.work == NULL) {
        OS_RestoreInterrupts(irq);
        return 1;
    }
    switch (sWcmAnchor.work->state) {
    case 13:
        OS_RestoreInterrupts(irq);
        return 2;
    case 1:
        OS_RestoreInterrupts(irq);
        return 0;
    case 6:
        WCM_SetState(13);
        sWcmAnchor.work->pendingNotify = 9;
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

    if (sWcmAnchor.work->resetPending == 1) {
        WCM_SetState(13);
        sWcmAnchor.work->pendingNotify = 9;
    } else {
        switch (sWcmAnchor.work->wirelessMode) {
        case 1:
            result = WCM_RestoreIdleStep();
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

s32 WCM_GetState(void) {
    u32 irq;
    WCMWork *work;
    s32 state;

    irq = OS_DisableInterrupts();
    work = sWcmAnchor.work;
    state = 0;
    if (work != NULL) {
        state = work->state;
    }
    OS_RestoreInterrupts(irq);
    return state;
}

u32 WCM_SetSearchOption(u32 option) {
    u32 irq = OS_DisableInterrupts();
    u32 mask = 0;
    // BUG: The original reads the search option before checking whether WCM has been initialized.
#ifdef BUGFIX
    u32 old;

    if (sWcmAnchor.work == NULL) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
    old = sWcmAnchor.work->searchOption;
#else
    u32 old = sWcmAnchor.work->searchOption;

    if (sWcmAnchor.work == NULL) {
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
    sWcmAnchor.work->searchOption = (old & ~mask) | option;
    OS_RestoreInterrupts(irq);
    return old;
}

s32 WCM_GetChannelMask(void) {
    WCMWork *work = WCM_GetWork();

    if (work->state >= 3 && work->state <= 10) {
        switch (work->wirelessMode) {
        case 1:
            return WCM_GetDsAllowedChannelSpanMask();
        case 2:
            if (OS_IsRunOnTwl()) {
                return func_02769e54();
            }
            break;
        }
    }
    return 0;
}

WCMWork *WCM_GetWork(void) {
    return sWcmAnchor.work;
}

static void WCM_SetConfig(WCMConfig *config, WCMNotify notify) {
    if (config == NULL) {
        sWcmAnchor.work->dmaNo = 3;
        sWcmAnchor.work->apBuffer = NULL;
        sWcmAnchor.work->apBufferBytes = 0;
        sWcmAnchor.work->scanMode = 0;
    } else {
        sWcmAnchor.work->dmaNo = config->dmano & 3;
        if (((4 - ((u32)config->pbdbuffer & 3)) % 4) + sizeof(WCMApListHeader) > config->nbdbuffer) {
            sWcmAnchor.work->apBuffer = NULL;
            sWcmAnchor.work->apBufferBytes = 0;
        } else {
            sWcmAnchor.work->apBuffer = (void *)(((u32)config->pbdbuffer + 3) & ~3);
            sWcmAnchor.work->apBufferBytes = config->nbdbuffer - (4 - ((u32)config->pbdbuffer & 3)) % 4;
            MI_CpuFill8(sWcmAnchor.work->apBuffer, 0, sWcmAnchor.work->apBufferBytes);
        }
        sWcmAnchor.work->scanMode = config->nbdmode;
    }
    sWcmAnchor.work->notify = notify;
}

static void WCM_ResetSearchOption(void) {
    sWcmAnchor.work->searchOption = 0xaaa082;
}

u16 WCM_PickChannel(s32 channelIndex) {
    s32 i;
    s32 channel;

    for (i = 0; i < 13; i++) {
        channel = (channelIndex + i) % 13;
        if (sWcmAnchor.work->searchOption & (1 << (channel + 1))) {
            if (WCM_GetChannelMask() & (1 << channel)) {
                break;
            }
        }
    }
    return (u16)((channelIndex + i) % 13 + 1);
}

void WCM_NotifyResult(s16 result, void *parameter, u32 detail, u32 line) {
    WCMWork *work = sWcmAnchor.work;
    s16 notify = work->pendingNotify;

    work->pendingNotify = 0;
    // The SDK transports mixed pointer/integer payloads as 32-bit words; a host port must widen this representation.
    WCM_Notify(notify, result, (s32)parameter, (void *)detail, line);
}

void WCM_Notify(s16 notify, s16 result, s32 parameter, void *param1, s32 param2) {
    WCMWork *work = sWcmAnchor.work;
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

void WCM_SetState(s32 newState) {
    u32 irq = OS_DisableInterrupts();

    if (sWcmAnchor.work->state == 9 && newState != 9) {
        OS_CancelAlarm(&sWcmAnchor.work->alarm);
    }
    if (sWcmAnchor.work->state != 11) {
        sWcmAnchor.work->state = newState;
    }
    if (newState == 9) {
        OS_CancelAlarm(&sWcmAnchor.work->alarm);
        OS_SetAlarm(&sWcmAnchor.work->alarm, 0x22f5341, (void (*)(void *))WCM_TimeoutAlarmHandler, NULL);
    }
    OS_RestoreInterrupts(irq);
}

void WCM_CancelTimeout(void) {
    u32 irq = OS_DisableInterrupts();

    OS_CancelAlarm(&sWcmAnchor.work->alarm);
    if (sWcmAnchor.work->state == 9) {
        OS_SetAlarm(&sWcmAnchor.work->alarm, 0x22f5341, (void (*)(void *))WCM_TimeoutAlarmHandler, NULL);
    }
    OS_RestoreInterrupts(irq);
}

static void WCM_TimeoutAlarmHandler(void) {
    WCM_CancelTimeout();
}

static s32 WCM_GetDefaultWirelessMode(void) {
    return OS_IsRunOnTwl() ? 2 : 1;
}

void WCM_ClearApListBuffer(void) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCM_GetWork();

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return;
    }
    if (work->apBuffer != NULL && (s32)work->apBufferBytes > 0) {
        MI_CpuFill8(work->apBuffer, 0, work->apBufferBytes);
    }
    OS_RestoreInterrupts(irq);
}

s32 WCM_CountApList(void) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCM_GetWork();
    s32 count = 0;
    WCMApListHeader *header;

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
    header = (WCMApListHeader *)work->apBuffer;
    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        count = header->count;
    }
    OS_RestoreInterrupts(irq);
    return count;
}

s32 WCM_LockApList(s32 lock) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCM_GetWork();

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
    if (lock != 0) {
        lock = work->apListLocked ? 1 : 0;
        work->apListLocked = 1;
    } else {
        lock = work->apListLocked ? 1 : 0;
        work->apListLocked = 0;
    }
    OS_RestoreInterrupts(irq);
    return lock;
}

WMBssDesc *WCM_PointApList(u16 index) {
    return WCM_PointApListCore(index);
}

static WMBssDesc *WCM_PointApListCore(u16 index) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCM_GetWork();
    WCMApEntry *entry;

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return NULL;
    }
    entry = WCM_FindApEntryById(index);
    if (entry == NULL) {
        OS_RestoreInterrupts(irq);
        return NULL;
    }
    OS_RestoreInterrupts(irq);
    return (WMBssDesc *)entry->data;
}

void WCM_OnBeaconReceived(const WMBssDesc *bss, u16 scanLinkLevel) {
    WCMWork *work = WCM_GetWork();
    WCMApEntry *entry;

    if (work == NULL || work->apListLocked != 0) {
        return;
    }
    if (OS_IsRunOnTwl() && work->wirelessMode == 2 && WCM_FindVendorIE(bss) != NULL) {
        return;
    }
    if (work->wirelessMode == 1 && bss->gameInfoLength != 0) {
        return;
    }
    entry = WCM_FindApEntryByBssid(bss->bssid);
    if (entry == NULL) {
        entry = WCM_AllocApEntry();
    }
    if (entry == NULL && work->scanMode == 1) {
        entry = WCM_GetOldestApEntry();
    }
    if (entry != NULL) {
        WCM_FillApEntry(bss, (WMBssDesc *)entry->data);
        entry->scanLinkLevel = scanLinkLevel;
        WCM_TouchApEntry(entry);
    }
}

static WCMApEntry *WCM_AllocApEntry(void) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    s32 i;
    WCMApEntry *entries;
    WCMApEntry *entry = NULL;

    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        u32 max = (work->apBufferBytes - sizeof(WCMApListHeader)) / sizeof(WCMApEntry);

        if (max > 0) {
            if (max > header->count) {
                entries = header->entries;
                for (i = 0; i < max; i++) {
                    entry = &entries[i];
                    if (!entry->inUse) {
                        break;
                    }
                }
                if (i < max) {
                    entry->inUse = TRUE;
                    entry->id = header->count;
                    entry->next = NULL;
                    entry->prev = header->tail;
                    header->tail = entry;
                    if (entry->prev != NULL) {
                        entry->prev->next = entry;
                    } else {
                        header->head = entry;
                    }
                    header->count++;
                }
            }
        }
    }
    return entry;
}

static WCMApEntry *WCM_GetOldestApEntry(void) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;

    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        return header->head;
    }
    return NULL;
}

static WCMApEntry *WCM_FindApEntryByBssid(const u8 *bssid) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *entry = NULL;

    if (bssid == NULL) {
        return NULL;
    }
    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        for (entry = header->head; entry != NULL; entry = entry->next) {
            if (WCM_CompareBssID(((WMBssDesc *)entry->data)->bssid, bssid)) {
                break;
            }
        }
    }
    return entry;
}

static WCMApEntry *WCM_FindApEntryById(u32 id) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *entry = NULL;

    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        for (entry = header->head; entry != NULL; entry = entry->next) {
            if (entry->id == id) {
                break;
            }
        }
    }
    return entry;
}

static void WCM_TouchApEntry(WCMApEntry *entry) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *walk;

    if (entry == NULL) {
        return;
    }
    if (header == NULL || work->apBufferBytes <= sizeof(WCMApListHeader)) {
        return;
    }
    walk = header->head;
    if (walk != NULL) {
        do {
            if (walk == entry) {
                if (walk->prev != NULL) {
                    walk->prev->next = walk->next;
                } else {
                    header->head = walk->next;
                }
                if (walk->next != NULL) {
                    walk->next->prev = walk->prev;
                } else {
                    header->tail = walk->prev;
                }
                break;
            }
            walk = walk->next;
        } while (walk != NULL);
    }
    // Append (or re-append) at the tail
    entry->next = NULL;
    entry->prev = header->tail;
    header->tail = entry;
    if (entry->prev != NULL) {
        entry->prev->next = entry;
    } else {
        header->head = entry;
    }
    // A fresh entry, never in the list, gets a new id; one found and moved keeps its id
    if (walk == NULL) {
        entry->id = header->count;
        header->count++;
    }
}

// Copies the descriptor prefix and retains selected IEs in the destination's raw 0x40..0xbf tail.
void WCM_FillApEntry(const WMBssDesc *source, WMBssDesc *destination) {
    WCMWork *work = WCM_GetWork();
    const u8 *ie = (const u8 *)&source->gameInfo;
    u8 *output = (u8 *)&destination->gameInfo;
    u8 total = 0;
    u16 n;

    MIi_CpuClearFast(0, destination, sizeof(*destination));
    MIi_CpuCopyFast(source, destination, offsetof(WMBssDesc, gameInfo));
    destination->otherElementCount = 0;
    destination->length = work->wirelessMode == 1 ? 0x20 : 0x40;
    for (n = 0; n < source->otherElementCount; n++) {
        u8 tag = ie[0];
        u16 ieLen = ie[1] + 2;
        BOOL retain = tag == 0x32 || tag == 0x2a || tag == 0x2f || tag == 7 || tag == 0x30;

        if (tag == 0xdd) {
            retain = MI_CpuComp8(ie + 2, sWpaOui, 4) == 0 || MI_CpuComp8(ie + 2, sWmmOui, 5) == 0;
        }
        if (retain) {
            MI_CpuCopy(ie, output, (u8)ieLen);
            output += ieLen;
            total += (u8)ieLen;
            destination->otherElementCount++;
        }
        ie += ieLen;
    }
    destination->length += total;
}

static void *WCM_FindVendorIE(const WMBssDesc *bss) {
    WCMWork *work = WCM_GetWork();
    WCMVendorOui oui = sNintendoOui;

    if (bss == NULL) {
        return NULL;
    }
    if (OS_IsRunOnTwl()) {
        if (work->wirelessMode == 2) {
            WCMVendorIE *ie = NULL;

            if (bss->otherElementCount != 0) {
                ie = WCM_GetInformationElement(bss, 0xdd, oui.bytes);
            }
            if (ie != NULL) {
                const WMGameInfo *info = (const WMGameInfo *)&ie->magicNumber;

                if (info->magicNumber == 1) {
                    return (void *)info;
                }
            }
        }
    } else if (work->wirelessMode == 1) {
        u16 len = bss->gameInfoLength;

        if (len != 0) {
            BOOL valid = FALSE;

            if (len >= 0x10 && len <= 0x80 && len == bss->gameInfo.userGameInfoLength + 0x10 &&
                bss->gameInfo.magicNumber == 1 && bss->length == (len + 1 + offsetof(WMBssDesc, gameInfo)) / 2) {
                valid = TRUE;
            }
            if (valid == TRUE) {
                return (void *)&bss->gameInfo;
            }
        }
    }
    return NULL;
}

static void *WCM_GetInformationElement(const void *frame, u32 tag, const u8 *oui) {
    return func_02703250((NWMBssDesc *)frame, tag, oui);
}

static void WCM_InitOnce(void) {
    if (!sWcmGlobal.initialized) {
        sWcmGlobal.initialized = TRUE;
        sWcmGlobal.sendStatus = 0;
        MI_CpuFill8(sWcmGlobal.ownMac, 0, 6);
        OS_InitThreadQueue(&sWcmGlobal.sendQueue);
        OS_InitMutex(&sWcmGlobal.mutex);
    }
}

void WCM_DispatchRecvCallback(const u8 *src, const u8 *dst, const u8 *data, s32 len) {
    if (sWcmGlobal.recvCallback != NULL) {
        sWcmGlobal.recvCallback(src, dst, data, len);
    }
}

WCMGlobal *WCM_GetGlobal(void) {
    return &sWcmGlobal;
}

u8 *WCM_GetApMacAddress(void) {
    u8 *mac = NULL;
    WCMWork *work = WCM_GetWork();
    u32 irq = OS_DisableInterrupts();

    if (work != NULL && work->state == 9 && work->resetPending == 0) {
        mac = work->connectBss.bssid;
    }
    OS_RestoreInterrupts(irq);
    return mac;
}

// The SSID is borrowed from the connected BSS descriptor and is not necessarily NUL-terminated.
u8 *WCM_GetConnectedApSsid(u16 *ssidLength) {
    u8 *ssid = NULL;
    u16 length = 0;
    WCMWork *work = WCM_GetWork();
    u32 irq = OS_DisableInterrupts();

    if (work != NULL) {
        if (work->state == 9 && work->resetPending == 0) {
            ssid = work->connectBss.ssid;
            length = work->connectBss.ssidLength;
        }
    }
    OS_RestoreInterrupts(irq);
    if (ssidLength != NULL) {
        *ssidLength = length;
    }
    return ssid;
}

void WCM_SetRecvDCFCallback(WCMRecvInd callback) {
    u32 irq = OS_DisableInterrupts();
    sWcmGlobal.recvCallback = callback;
    OS_RestoreInterrupts(irq);
}

s32 WCM_SendDCFDataEx(const u8 *dst, const u8 *header, s32 headerLen, const u8 *data, s32 dataLen) {
    u32 irq;
    WCMWork *work;
    s32 sentLen;

    irq = OS_DisableInterrupts();
    work = WCM_GetWork();
    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return -1;
    }
    OS_LockMutex(&sWcmGlobal.mutex);
    work = WCM_GetWork();
    if (work == NULL) {
        OS_UnlockMutex(&sWcmGlobal.mutex);
        OS_RestoreInterrupts(irq);
        return -1;
    }
    if (work->state != 9 || work->resetPending == 1) {
        OS_UnlockMutex(&sWcmGlobal.mutex);
        OS_RestoreInterrupts(irq);
        return -4;
    }
    sentLen = WCM_AssembleSendFrame(work->wirelessMode, work, header, headerLen, data, dataLen);
    switch (work->wirelessMode) {
    case 1:
        WCM_SendFrame(dst, work, (u16)sentLen, NULL);
        break;
    case 2:
        if (OS_IsRunOnTwl()) {
            func_02769dc4(dst, work, (u16)sentLen, NULL);
        }
        break;
    default:
        break;
    }
    OS_UnlockMutex(&sWcmGlobal.mutex);
    OS_RestoreInterrupts(irq);
    return headerLen + dataLen;
}

void WCM_ReleaseIrqOwnedMutex(OSMutex *mutex) {
    // The IRQ-handler address is an owner marker, not an OSThread object; this path releases no ownership-list link.
    if (mutex->thread == (OSThread *)OS_IrqHandler) {
        mutex->count--;
        if (mutex->count == 0) {
            mutex->thread = NULL;
            OS_WakeupThread(&mutex->queue);
        }
    }
}

void WCM_SetMacAddress(const u8 *mac) {
    MI_CpuCopy(mac, sWcmGlobal.ownMac, 6);
}

void WCM_GetMacAddress(u8 *mac) {
    MI_CpuCopy(sWcmGlobal.ownMac, mac, 6);
}

static s32 WCM_AssembleSendFrame(s32 mode, void *dest, const void *header, s32 headerLen, const void *data,
                                 s32 dataLen) {
    u8 *payload;
    s32 total = headerLen + dataLen;

    switch (mode) {
    case 1:
    default:
        payload = dest;
        break;
    case 2: {
        WCMEthernetHeader *ether = dest;
        u16 length = (u16)total;

        ether->length = (u16)(((length & 0xff00) >> 8) | ((length & 0x00ff) << 8));
        payload = (u8 *)dest + sizeof(*ether);
        total += sizeof(*ether);
        break;
    }
    }
    MI_CpuCopy(header, payload, headerLen);
    if (data != NULL && dataLen != 0) {
        MI_CpuCopy(data, payload + headerLen, dataLen);
    }
    return total;
}

static BOOL WCM_CompareBytes(const void *a, const void *b, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (((const u8 *)a)[i] != ((const u8 *)b)[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL WCM_CompareBssID(const u8 *a, const u8 *b) {
    return WCM_CompareBytes(a, b, 6);
}

WMLinkLevel WCM_UpdateLinkQuality(void) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCM_GetWork();
    WMLinkLevel quality = WM_LINK_LEVEL_0;

    if (work != NULL && work->state == 9) {
        quality = WCM_RssiToQuality(WCM_GetAverageRssi());
    }
    OS_RestoreInterrupts(irq);
    return quality;
}

static WMLinkLevel WCM_RssiToQuality(u32 rssi) {
    switch (WCM_GetWork()->wirelessMode) {
    case 1:
    default:
        if (rssi >= 0x1c) {
            return WM_LINK_LEVEL_3;
        }
        if (rssi >= 0x16) {
            return WM_LINK_LEVEL_2;
        }
        if (rssi >= 0x10) {
            return WM_LINK_LEVEL_1;
        }
        return WM_LINK_LEVEL_0;
    case 2:
        if (rssi >= 0x16) {
            return WM_LINK_LEVEL_3;
        }
        if (rssi >= 0x11) {
            return WM_LINK_LEVEL_2;
        }
        if (rssi >= 0xc) {
            return WM_LINK_LEVEL_1;
        }
        return WM_LINK_LEVEL_0;
    }
}

static u8 WCM_GetAverageRssi(void) {
    s32 sum = 0;
    s32 i;

    if (sWcmRssiCursor > 16) {
        for (i = 0; i < 16; i++) {
            sum += sWcmRssiSamples[i];
        }
        sum /= 16;
    } else if (sWcmRssiCursor > 0) {
        for (i = 0; i < sWcmRssiCursor; i++) {
            sum += sWcmRssiSamples[i];
        }
        sum /= sWcmRssiCursor;
    }
    return (u8)sum;
}

// Adds a signal-level sample to the running RSSI average's circular buffer
// Decodes the receive record's high signal byte. The original arithmetic is mode-dependent;
// no physical units for the encoded input or the resulting sample have been established.
void WCM_AddRssiSample(s32 rssi) {
    switch (WCM_GetWork()->wirelessMode) {
    case 2:
        break;
    case 1:
    default:
        if (rssi & 2) {
            rssi = (u8)(rssi >> 2);
        } else {
            rssi = (u8)(25 + (rssi >> 2));
        }
        break;
    }
    sWcmRssiSamples[sWcmRssiCursor % 16] = rssi;
    if (sWcmRssiCursor >= 16) {
        sWcmRssiCursor = (u8)(((1 + sWcmRssiCursor) % 16) + 16);
    } else {
        sWcmRssiCursor++;
    }
}

// The startup FSM's WM_Initialize-adjacent first step: brings up the hardware and reads its MAC address
static s32 WCM_StartupStep1(void) {
    WCMWork *work = WCM_GetWork();

    switch (WM_Init(work->startupBuffer, (u16)work->dmaNo)) {
    case 0:
        break;
    case 3:
        WCM_SetState(11);
        return 7;
    case 4:
        return 5;
    // WM_ERRCODE_INVALID_PARAM, which the SDK lists with the default
    case 6:
    default:
        WCM_SetState(11);
        return 7;
    }
    if (WM_SetIndCallback((WMCallbackFunc)WCM_OnWmIndication) != 0) {
        WCM_SetState(11);
        return 7;
    }
    {
        u8 mac[6];

        OS_GetMacAddress(mac);
        WCM_SetMacAddress(mac);
    }
    switch (WM_Enable((WMCallbackFunc)WCM_OnWmControlComplete)) {
    case 2:
        WCM_SetState(2);
        work->pendingNotify = 1;
        break;
    case 8:
        WCM_SetState(12);
        return 1;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCM_SetState(11);
        return 7;
    }
    return 3;
}

static s32 WCM_StartupStep2(void) {
    WCMWork *work = WCM_GetWork();

    switch (WM_PowerOff((WMCallbackFunc)WCM_OnWmControlComplete)) {
    case 2:
        WCM_SetState(4);
        work->pendingNotify = 2;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCM_SetState(0xb);
        return 7;
    }
    return 3;
}

static s32 WCM_StartSearchStep(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCM_GetWork();

    switch (work->state) {
    case 5:
        WCM_BuildScanRequest(bssid, essid, option);
        return 2;
    case 6:
        WCM_BuildScanRequest(bssid, essid, option);
        return 0;
    case 3:
        break;
    default:
        return 1;
    }
    WCM_BuildScanRequest(bssid, essid, option);
    DC_InvalidateRange(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
    work->scanCount++;
    switch (WM_StartScanEx((WMCallbackFunc)WCM_ConnectStep4, &work->scanRequest)) {
    case 2:
        WCM_SetState(5);
        work->pendingNotify = 3;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCM_SetState(0xb);
        return 7;
    }
    return 3;
}

static s32 WCM_StartConnectStep(void) {
    WCMWork *work = WCM_GetWork();

    if (work->state == 3) {
        work->connectBss.rateSetSupport = work->connectBss.rateSetBasic | 3;
    }
    switch (WM_SetLifeTime((WMCallbackFunc)WCM_OnWmControlComplete, 0xffff, 0x50, 0xffff, 0xffff)) {
    case 2:
        WCM_SetState(8);
        work->pendingNotify = 5;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCM_SetState(0xb);
        return 7;
    }
    return 3;
}

static s32 WCM_StartDisconnectStep(void) {
    WCMWork *work = WCM_GetWork();

    switch (WM_EndDCF((WMCallbackFunc)WCM_OnDisconnectPreparationComplete)) {
    case 2:
        WCM_SetState(0xa);
        work->pendingNotify = 6;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCM_SetState(0xb);
        return 7;
    }
    return 3;
}

static s32 WCM_RestoreIdleStep(void) {
    WCMWork *work = WCM_GetWork();
    const WMStatus *status = WMi_GetStatusAddress();
    s32 result;

    DC_InvalidateRange((void *)status, 2);
    switch (status->state) {
    case 0:
        result = WM_Finish();
        if (result == 0) {
            WCM_SetState(1);
            work->pendingNotify = 0;
            return 0;
        }
        break;
    case 1:
        result = WM_Disable((WMCallbackFunc)WCM_OnWmControlComplete);
        break;
    case 2:
        result = WM_PowerOff((WMCallbackFunc)WCM_OnWmControlComplete);
        break;
    default:
        work->resetPending = 1;
        result = WM_Reset((WMCallbackFunc)WCM_OnResetComplete);
        break;
    }
    switch (result) {
    case 2:
        WCM_SetState(0xd);
        work->pendingNotify = 9;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCM_SetState(0xb);
        return 7;
    }
    return 3;
}

static s32 WCM_SendFrame(const u8 *dst, const void *buffer, u16 byteLen, void (*completion)(s32)) {
    WCMGlobal *global = WCM_GetGlobal();
    WMCallbackFunc callback;

    if (completion == NULL) {
        callback = WCM_OnSendComplete;
    } else {
        global->sendCompletion = completion;
        callback = WCM_OnSendCompleteWithCallback;
    }
    switch (WM_SetDCFData(callback, dst, buffer, byteLen)) {
    case 2:
        break;
    // WM_ERRCODE_ILLEGAL_STATE, WM_ERRCODE_INVALID_PARAM and WM_ERRCODE_FIFO_ERROR, which the SDK lists with the
    // default
    case 3:
    case 6:
    case 8:
    default:
        return -5;
    }
    if (completion == NULL) {
        OS_SleepThread(&global->sendQueue);
        switch (global->sendStatus) {
        case 0:
            break;
        // WM_ERRCODE_FAILED, which the SDK lists with the default
        case 1:
        default:
            return -5;
        }
    }
    return byteLen;
}

static u16 WCM_GetDsAllowedChannelSpanMask(void) {
    u16 allowed = WM_GetAllowedChannel();
    u16 lowerMask = 1;
    u16 upperMask = 0x8000;

    if (allowed == 0) {
        return 0;
    }
    // Keep the inclusive span, including any gaps in the hardware's allowed-channel mask.
    while (!(upperMask & allowed)) {
        upperMask = (upperMask >> 1) | 0x8000;
    }
    while (!(lowerMask & allowed)) {
        lowerMask = (lowerMask << 1) | 1;
    }
    upperMask <<= 1;
    lowerMask >>= 1;
    return (u16) ~(upperMask | lowerMask);
}

static void WCM_RequestReset(void) {
    WCMWork *work = WCM_GetWork();

    if (work->resetPending == 0) {
        work->resetPending = 1;
        if (WM_Reset((WMCallbackFunc)WCM_OnResetComplete) != 2) {
            WCM_SetState(0xb);
            WCM_NotifyResult(7, NULL, 0, 0x24f);
        }
    }
}

// WM's indication callback, installed in WCM_StartupStep1: dispatches disconnect indications by the current state
static void WCM_OnWmIndication(void *ind) {
    const WMIndCallback *indication = ind;
    WCMWork *work = WCM_GetWork();

    if (indication->errcode == 8) {
        if (indication->state == 0x16 && indication->unk_06 == 0x25) {
            switch (work->state) {
            case 8:
                WCM_SetState(0xc);
                break;
            case 9:
            case 12:
                WCM_RequestReset();
                break;
            case 10:
                WCM_SetState(0xc);
                break;
            }
        }
    }
}

static void WCM_OnWmControlComplete(void *ind) {
    const WMCallback *callback = (const WMCallback *)ind;
    int result = 20;
    WCMWork *work = WCM_GetWork();

    switch (callback->errcode) {
    case 0:
        switch (callback->apiid) {
        case 3:
            result = WM_PowerOn((WMCallbackFunc)WCM_OnWmControlComplete);
            break;
        case 4:
            result = WM_Finish();
            switch (result) {
            case 0:
                WCM_SetState(1);
                WCM_NotifyResult(0, NULL, 0, 0x2a4);
                break;
            // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
            case 4:
            default:
                WCM_SetState(11);
                WCM_NotifyResult(7, NULL, 0, 0x2ad);
                break;
            }
            return;
        case 5:
            WCM_SetState(3);
            WCM_NotifyResult(0, NULL, 0, 0x2b4);
            return;
        case 6:
            result = WM_Disable((WMCallbackFunc)WCM_OnWmControlComplete);
            break;
        case 29:
            result = WM_SetBeaconIndication((WMCallbackFunc)WCM_OnWmControlComplete, 0);
            break;
        case 25:
            result = WM_SetWEPKeyEx((WMCallbackFunc)WCM_OnWmControlComplete, work->connectionStorage.mode,
                                    work->connectionStorage.argument, work->connectionStorage.data);
            break;
        case 39:
            result = WM_StartConnectEx(
                (WMCallbackFunc)WCM_OnConnectStatus, &work->connectBss, NULL,
                (work->searchOption & WCM_OPTION_POWER_ACTIVE) == WCM_OPTION_POWER_ACTIVE ? FALSE : TRUE,
                (work->searchOption & WCM_OPTION_AUTH_SHARED) == WCM_OPTION_AUTH_SHARED ? (u16)1 : (u16)0);
            break;
        }
        switch (result) {
        case 2:
            break;
        case 8:
            WCM_SetState(12);
            WCM_NotifyResult(1, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x2f0);
            break;
        // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
        case 3:
        default:
            WCM_SetState(11);
            WCM_NotifyResult(7, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x2f9);
            break;
        }
        break;
    case 1:
        WCM_SetState(12);
        WCM_NotifyResult(1, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x31f);
        break;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x329);
        break;
    }
}

static void WCM_ConnectStep4(void *ind) {
    const WMStartScanExCallback *callback = ind;
    int result = 0x14;
    WCMWork *work = WCM_GetWork();

    switch (callback->errcode) {
    case 0:
        if (work->state == 5) {
            WCM_SetState(6);
            WCM_NotifyResult(0, NULL, 0, 0x346);
        }
        switch (work->state) {
        case 6:
            work->pendingNotify = 7;
            if (callback->state == WM_STATECODE_PARENT_FOUND) {
                int i;

                DC_InvalidateRange(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
                for (i = 0; i < callback->bssDescCount; i++) {
                    WCM_OnBeaconReceived(callback->bssDesc[i], callback->linkLevel[i]);
                    WCM_Notify(7, 0, (s32)callback->bssDesc[i], ind, 0x35c);
                }
            }
            if ((work->searchOption & 0xc00000) == 0xc00000) {
                u32 count = MATH_CountPopulation(work->searchOption & 0x3ffe);

                if (count > 0) {
                    if (work->scanCount % count == 0) {
                        WCM_Notify(8, 0, work->scanCount, NULL, 0x36a);
                    }
                }
            }
            work->scanRequest.channelList =
                (u16)((1 << WCM_PickChannel((u16)(32 - MATH_CountLeadingZerosFunc(callback->channel)))) >> 1);
            DC_StoreRange(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
            work->scanCount++;
            result = WM_StartScanEx((WMCallbackFunc)WCM_ConnectStep4, &work->scanRequest);
            break;
        case 7:
            result = WM_EndScan((WMCallbackFunc)WCM_ConnectStep5);
            break;
        case 0xd:
            WCM_RequestReset();
            return;
        }
        switch (result) {
        case 2:
            break;
        case 8:
            WCM_SetState(0xc);
            WCM_NotifyResult(1, NULL, 0, 0x395);
            break;
        // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
        case 3:
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, NULL, 0, 0x39e);
            break;
        }
        break;
    case 1:
        WCM_RequestReset();
        break;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, 0, 0x3af);
        break;
    }
}

static void WCM_ConnectStep5(void *ind) {
    switch (((const WMCallback *)ind)->errcode) {
    case 0:
        WCM_SetState(3);
        WCM_NotifyResult(0, NULL, 0, 0x3c5);
        return;
    case 1:
        WCM_RequestReset();
        return;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, NULL, 0, 0x3d5);
        return;
    }
}

static void WCM_OnConnectStatus(void *ind) {
    const WMStartConnectCallback *callback = ind;
    WCMWork *work = WCM_GetWork();
    int result;

    switch (callback->errcode) {
    case 0:
        switch (callback->state) {
        case 8:
        case 9:
            switch (work->state) {
            case 10:
                work->associationId = 0;
            case 8:
                WCM_SetState(0xc);
                break;
            case 9:
                work->associationId = 0;
                work->pendingNotify = 6;
            case 12:
                WCM_RequestReset();
                break;
            default:
                break;
            }
            break;
        case 6:
        case 0x1a:
            break;
        case 7:
            if (work->state == 0xc) {
                WCM_SetState(8);
                WCM_RequestReset();
            } else if (callback->aid >= 1 && callback->aid <= 0x7d7) {
                work->associationId = callback->aid;
                result = WM_StartDCF((WMCallbackFunc)WCM_OnDcfReceive, (WMDcfRecvBuf *)work->ioBuffer.receive,
                                     sizeof(work->ioBuffer.receive));
                switch (result) {
                case 2:
                    break;
                case 8:
                    WCM_SetState(0xc);
                    WCM_NotifyResult(1, &work->connectBss, 0, 0x430);
                    break;
                // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
                case 3:
                default:
                    WCM_SetState(0xb);
                    WCM_NotifyResult(7, &work->connectBss, 0, 0x439);
                    break;
                }
            } else {
                WCM_RequestReset();
            }
            break;
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, &work->connectBss, callback->state, 0x447);
            break;
        }
        break;
    case 1:
        work->connectionFailureReason = callback->wlStatus;
    case 6:
    case 11:
    case 12:
        WCM_SetState(8);
        WCM_RequestReset();
        break;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, &work->connectBss, 0, 0x45e);
        break;
    }
}

static void WCM_OnDisconnectComplete(void *ind) {
    const WMCallback *callback = ind;
    WCMWork *work = WCM_GetWork();

    switch (callback->errcode) {
    case 0:
        if (work->state == 0xc) {
            WCM_SetState(0xa);
            WCM_RequestReset();
        } else {
            work->associationId = 0;
            WCM_SetState(3);
            WCM_NotifyResult(0, &work->connectBss, 0, 0x481);
        }
        break;
    case 1:
    case 3:
        WCM_SetState(0xa);
        WCM_RequestReset();
        break;
    // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
    case 4:
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, &work->connectBss, 0, 0x493);
        break;
    }
}

static void WCM_OnDcfReceive(void *ind) {
    const WMStartDCFCallback *callback = ind;
    WCMWork *work = WCM_GetWork();

    switch (callback->errcode) {
    case 0:
        switch (callback->state) {
        case 0xe:
            if (work->state == 0xc) {
                WCM_SetState(8);
                WCM_RequestReset();
            } else {
                WCM_SetState(9);
                WCM_NotifyResult(0, &work->connectBss, 0, 0x4b7);
            }
            break;
        case 0xf:
            WCM_AddRssiSample((u8)(callback->recvBuf->rate_rssi >> 8));
            DC_InvalidateRange(callback->recvBuf, 0x620);
            WCM_DispatchRecvCallback(callback->recvBuf->srcAdrs, callback->recvBuf->destAdrs,
                                     (u8 *)callback->recvBuf->data, callback->recvBuf->length);
            break;
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, &work->connectBss, callback->state, 0x4c6);
            break;
        }
        break;
    // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
    case 4:
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, &work->connectBss, 0, 0x4d1);
        break;
    }
}

static void WCM_OnDisconnectPreparationComplete(void *ind) {
    const WMCallback *callback = ind;
    WCMWork *work = WCM_GetWork();

    switch (callback->errcode) {
    case 0:
        if (work->state == 0xc) {
            WCM_SetState(0xa);
            WCM_RequestReset();
        } else {
            switch (WM_Disconnect((WMCallbackFunc)WCM_OnDisconnectComplete, 0)) {
            case 2:
                break;
            case 8:
                WCM_SetState(0xc);
                WCM_NotifyResult(1, &work->connectBss, 0, 0x4fa);
                break;
            case 3:
                WCM_SetState(0xa);
                WCM_RequestReset();
                break;
            default:
                WCM_SetState(0xb);
                WCM_NotifyResult(7, &work->connectBss, 0, 0x506);
                break;
            }
        }
        break;
    case 1:
    case 3:
        WCM_SetState(0xa);
        WCM_RequestReset();
        break;
    // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
    case 4:
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, &work->connectBss, 0, 0x519);
        break;
    }
}

static void WCM_OnResetComplete(void *ind) {
    const WMCallback *callback = ind;
    WCMWork *work = WCM_GetWork();
    int result;

    switch (callback->errcode) {
    case 0:
        work->resetPending = 0;
        work->associationId = 0;
        switch (work->state) {
        case 5:
        case 6:
            WCM_SetState(3);
            WCM_NotifyResult(1, NULL, 0, 0x53c);
            break;
        case 7:
            WCM_SetState(3);
            WCM_NotifyResult(0, NULL, 0, 0x542);
            break;
        case 8: {
            u16 reason = work->connectionFailureReason;

            work->connectionFailureReason = 0;
            if (reason == 0x12) {
                if ((work->connectBss.rateSetSupport & 0x24) != 0x24) {
                    work->connectBss.rateSetSupport |= 0x24;
                    result = WM_StartConnectEx(
                        (WMCallbackFunc)WCM_OnConnectStatus, &work->connectBss, NULL,
                        (work->searchOption & WCM_OPTION_POWER_ACTIVE) == WCM_OPTION_POWER_ACTIVE ? FALSE : TRUE,
                        (work->searchOption & WCM_OPTION_AUTH_SHARED) == WCM_OPTION_AUTH_SHARED ? (u16)1 : (u16)0);
                    switch (result) {
                    case 2:
                        break;
                    case 8:
                        WCM_SetState(0xc);
                        WCM_NotifyResult(1, &work->connectBss, reason, 0x563);
                        break;
                    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
                    case 3:
                    default:
                        WCM_SetState(0xb);
                        WCM_NotifyResult(7, &work->connectBss, reason, 0x56c);
                        break;
                    }
                    return;
                }
            }
            WCM_SetState(3);
            WCM_NotifyResult(1, &work->connectBss, reason, 0x574);
            break;
        }
        case 9:
        case 12:
            WCM_SetState(3);
            WCM_NotifyResult(0, &work->connectBss, 1, 0x57c);
            break;
        case 10:
            WCM_SetState(3);
            WCM_NotifyResult(0, &work->connectBss, 0, 0x583);
            break;
        case 13:
            result = WM_PowerOff((WMCallbackFunc)WCM_OnWmControlComplete);
            switch (result) {
            case 2:
                break;
            case 8:
                WCM_SetState(0xc);
                WCM_NotifyResult(1, NULL, 0, 0x591);
                break;
            // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
            case 3:
            default:
                WCM_SetState(0xb);
                WCM_NotifyResult(7, NULL, 0, 0x59a);
                break;
            }
            break;
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, NULL, work->state, 0x5a0);
            break;
        }
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, 0, 0x5a7);
        break;
    }
}

static void WCM_BuildScanRequest(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCM_GetWork();
    u16 channelTime;

    WCM_SetSearchOption(option);
    work->scanRequest.scanBuf = (WMBssDesc *)work->ioBuffer.scan;
    work->scanRequest.scanBufSize = 0x400;
    work->scanRequest.channelList = (u16)((1 << WCM_PickChannel(0)) >> 1);
    channelTime = work->scanChannelTime;
    if (channelTime == 0) {
        channelTime = WM_GetDispersionScanPeriod();
    }
    work->scanRequest.maxChannelTime = channelTime;
    work->scanRequest.scanType = ((work->searchOption & 0x300000) != 0x300000);
    if (bssid == NULL) {
        MI_CpuCopy8(WCMiAnyBssid, work->scanRequest.bssid, 6);
    } else {
        MI_CpuCopy8(bssid, work->scanRequest.bssid, 6);
    }
    if (essid == NULL || essid == WCMiAnyEssid) {
        MI_CpuCopy8(WCMiAnyEssid, work->scanRequest.ssid, 0x20);
        work->scanRequest.ssidLength = 0;
    } else {
        s32 i;

        MI_CpuCopy8(essid, work->scanRequest.ssid, 0x20);
        for (i = 0; i < 0x20; i++) {
            if (essid[i] == 0) {
                break;
            }
        }
        work->scanRequest.ssidLength = (u16)i;
    }
    work->scanCount = 0;
}

static void WCM_OnSendComplete(void *ind) {
    WCMGlobal *global = WCM_GetGlobal();
    const WMCallback *result = ind;

    if (result->apiid != 0x12) {
        return;
    }
    global->sendStatus = result->errcode;
    if (result->errcode == 0) {
        WCM_CancelTimeout();
    }
    OS_WakeupThread(&global->sendQueue);
}

static void WCM_OnSendCompleteWithCallback(void *ind) {
    WCMGlobal *global = WCM_GetGlobal();
    const WMCallback *result = ind;
    s32 status;
    void (*completion)(s32);

    if (result->apiid != 0x12) {
        return;
    }
    global->sendStatus = result->errcode;
    if (result->errcode == 0) {
        WCM_CancelTimeout();
    }
    switch (global->sendStatus) {
    case 0:
        status = 0;
        break;
    // WM_ERRCODE_FAILED, which the SDK lists with the default
    case 1:
    default:
        status = -5;
        break;
    }
    completion = global->sendCompletion;
    global->sendCompletion = NULL;
    WCM_ReleaseIrqOwnedMutex(&global->mutex);
    completion(status);
}

// The "any" BSSID and ESSID that a search takes when the caller gives none
const u8 WCMiAnyBssid[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 };
const u8 WCMiAnyEssid[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
static const WCMVendorOui sNintendoOui = { { 0x00, 0x09, 0xbf, 0x00 } };
static const u8 sWpaOui[] = { 0x00, 0x50, 0xf2, 0x01 };
static const u8 sWmmOui[] = { 0x00, 0x50, 0xf2, 0x02, 0x00, 0x00, 0x00, 0x00 };
