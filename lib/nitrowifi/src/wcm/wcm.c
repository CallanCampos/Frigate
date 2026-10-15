#include "nitrowifi/wcm.h"
#include "nitrowifi/wcm_internal.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "nitrowifi/wcm_dsi.h"

// NitroWiFi's WCM (overlay 11), over NitroSDK's WM. dsd splits this range into 93 pieces; eight Thumb/ARM pairs among
// them are the linker's long-branch veneers to the DSi WCM backend physically placed in LTD. This file calls
// their real targets directly instead of writing the veneers out. The ROM names none of this; established WCM
// names are kept where identified. WCM_StopSearchAsync, WCM_RestoreIdleAsync, WCM_GetConnectedApSsid and
// WCM_ReleaseIrqOwnedMutex, along with the private helper/callback names, are descriptive reconstructions.

// sys_memcpy_fast and sys_memcpy32_fast: like sys_memcpy and MI_CpuFillFast's sys_memset32_fast, a multiple of 4
// bytes at a time where they can (MIi_CpuCopyFast and MIi_CpuCopy32Fast). Not yet in nitro/mi.h
void sys_memcpy_fast(const void *src, void *dest, u32 size);
void sys_memcpy32_fast(const void *src, void *dest, u32 size);

// NitroSDK's WM internals this file calls that nitro/wm.h does not yet declare
s32 func_02080758(void *buf, u16 dmaNo);
BOOL func_02080908(void);
void *func_02080dbc(void);
int func_02081348(WMCallbackFunc callback);
int func_02081398(WMCallbackFunc callback);
int func_02081b94(WMCallbackFunc callback);
int func_02081ab0(WMCallbackFunc callback, void *buffer, u16 bufferSize);
int func_02082470(WMCallbackFunc callback, u8 mode, u8 argument, void *data);
int func_020812fc(WMCallbackFunc callback);
int func_02081370(WMCallbackFunc callback);

// NitroSDK's alarm-adjacent OS calls that nitro/os.h does not yet declare
void func_0207bbf4(void);
BOOL func_0207bc18(void);

// NitroWiFi's own IE reader, elsewhere in main; not yet decompiled
void *func_02703250(const void *frame, u32 tag, void *out);

// An access point WCM found during a search, in a doubly linked list off the caller's apBuffer. WCM_PointApList's
// result points at record, past this header
// Nintendo's and Microsoft's OUIs, that WCM_FillApEntry and WCM_FindVendorIE look for among a beacon's vendor
// information elements (Nintendo's own, and WPA's and WMM's)
static const u8 data_ov011_02185004[4];
static const u8 data_ov011_02185008[4];
static const u8 data_ov011_0218500c[8];

typedef struct WCMApEntry WCMApEntry;
struct WCMApEntry {
    u8 inUse;
    u8 unk_01;
    u16 scanLinkLevel; // DS scan link level; the DSi producer's value domain is unconfirmed
    s32 id; // the AP list header's count when this entry was allocated or reused
    WCMApEntry *prev;
    WCMApEntry *next;
    WMBssDesc record; // Retained prefix; bytes 0x40..0xbf hold packed IEs rather than WMGameInfo.
};

// The caller's AP list buffer (WCMConfig.pbdbuffer), read as a header followed by up to apMax WCMApEntry records
typedef struct WCMApListHeader {
    u32 count;
    WCMApEntry *head; // the oldest entry (WCM_GetOldestApEntry's result)
    WCMApEntry *tail; // the newest
} WCMApListHeader;

// Recovered DCF receive-record prefix, named here for its WCM consumer rather than as an established SDK type.
// The payload follows this prefix in the registered receive buffer; its length is supplied separately.
typedef struct WCMReceiveRecord {
    u8 unk_00[6];
    u16 payloadLength; // 0x06
    u8 unk_08[6];
    u16 signalWord; // 0x0e, the high byte is the encoded input to WCM_AddRssiSample
    u8 unk_10[8];
    u8 destination[6]; // 0x18
    u8 source[6];      // 0x1e
    u8 unk_24[8];
    u8 payload[]; // 0x2c
} WCMReceiveRecord;

// Private views of the callback bytes WCM reads; these names do not establish complete SDK callback layouts.
// The existing SDK connection prefix ends before the failure reason supplied to this WCM consumer.
typedef struct WCMConnectStatusView {
    WMStartConnectCallback base;
    u16 unk_0c;
    u16 reason; // 0x0e, retained only when errcode is 1
} WCMConnectStatusView;

typedef struct WCMDcfCallbackView {
    WMCallback base;
    u16 state; // 0x04, start or receive event
    u16 unk_06;
    WCMReceiveRecord *record; // 0x08, borrowed WM buffer, dispatched synchronously
} WCMDcfCallbackView;

// WCM's mapped work prefix follows the DS allocation (WCM_WORK_SIZE bytes). Mode 2 allocates 0x4800 bytes;
// its additional tail and most of the connect/search scratch area remain unmapped.
struct WCMWork {
    u8 unk_0000[0x600];
    union {
        u8 scan[0x400];
        u8 receive[0x620];
    } ioBuffer; // 0x600, the scan and DCF receive operations reuse this storage
    u8 unk_0c20[0x1240 - 0xc20];
    // Descriptive private storage recovered from WCM's copies and WM command arguments.
    struct {
        u8 data[0x50];
        u8 mode;     // Secondary descriptor mode.
        u8 argument; // Legacy key index or variable-copy byte count; mode zero retains the previous byte.
        u8 padding[2];
    } connectionStorage; // 0x1240..0x1293; no original SDK type identity established
    OSAlarm alarm; // 0x1294
    s32 dmaNo; // 0x12c0, from WCMConfig.dmano
    void *apBuffer; // 0x12c4, from WCMConfig.pbdbuffer: a WCMApListHeader followed by its entries
    u32 apBufferBytes; // 0x12c8, from WCMConfig.nbdbuffer
    u32 scanMode; // 0x12cc, from WCMConfig.nbdmode
    WCMNotify notify; // 0x12d0
    u32 searchOption; // 0x12d4
    u16 scanChannelTime; // 0x12d8, zero selects the WM default
    s16 pendingNotify; // 0x12da, notification ID consumed by WCM_NotifyResult
    u8 apListLocked; // 0x12dc
    u8 resetPending; // 0x12dd, set before reset submission; successful reset completion clears it
    u8 state; // 0x12de
    u8 wirelessMode;              // 0x12df, selected by WCM_InitCore: 1 for DS, 2 for DSi
    u8 unk_12e0[0xc0];            // BSS data and notification storage
    u8 startupBuffer[0xf00];      // 0x13a0, passed to WM initialization
    WMScanExParam scanRequest;    // 0x22a0, including the 16-byte SSID match mask
    u32 scanCount;                // 0x22e4
    u16 associationId;            // 0x22e8, from the successful connection callback; cleared on disconnect or reset
    u16 connectionFailureReason;  // 0x22ea, retained across reset for retry and notification
    u8 unk_22ec[WCM_WORK_SIZE - 0x22ec];
};

// WCM's own small globals, zeroed with the rest of .bss
// data_ov011_0218df74: the anchor WCM_Init and WCM_Finish set and clear
typedef struct WCMAnchor {
    u8 preferredMode; // 0x00, read when the requested mode is zero; no writer identified
    u8 unk_01[3];
    WCMWork *work; // 0x04
} WCMAnchor;

// data_ov011_0218df7c, right after WCMAnchor: the globals WCM_InitOnce sets up once
struct WCMGlobal {
    u8 initialized; // 0x00, set once WCM_InitOnce has initialized the send state
    u8 unk_01[3];
    OSThreadQueue sendQueue; // 0x04
    OSMutex mutex; // 0x0c
    u16 sendStatus; // 0x24
    u8 padding_26[2];
    void (*recvCallback)(u8 *src, u8 *dst, u8 *data, u32 len); // 0x28
    void (*sendCompletion)(s32);                               // 0x2c
    u8 ownMac[6]; // 0x30
    u8 unk_36[2];
    u8 rssiSampleCursor; // 0x38, count until full, then 16 | the next ring index
    u8 rssiSamples[16];  // 0x39
    u8 unk_49[3];
};

static WCMAnchor sWork;
static WCMGlobal sWcm;

static BOOL WCM_GetRequiredWorkSize(s32 *size, s32 mode);
static s32 WCM_SearchAsyncWithParams(const u8 *bssid, const u8 *essid, u32 option);
static void WCM_SetConfig(WCMConfig *config, WCMNotify notify);
static void WCM_ResetSearchOption(void);
static void WCM_TimeoutAlarmHandler(void);
static s32 WCM_GetDefaultWirelessMode(void);
static void *WCM_PointApListCore(u16 index);
static WCMApEntry *WCM_AllocApEntry(void);
static WCMApEntry *WCM_GetOldestApEntry(void);
static WCMApEntry *WCM_FindApEntryByBssid(const u8 *bssid);
static WCMApEntry *WCM_FindApEntryById(s32 id);
static void WCM_TouchApEntry(WCMApEntry *entry);
static void *WCM_FindVendorIE(const WMBssDesc *bss);
static void *WCM_GetInformationElement(const void *frame, u32 tag, void *out);
static void WCM_InitOnce(void);
static s32 WCM_AssembleSendFrame(s32 mode, void *dest, const void *header, s32 headerLen, const void *data,
                                 s32 dataLen);
static BOOL WCM_CompareBytes(const void *a, const void *b, u32 size);
static BOOL WCM_CompareBssID(const u8 *a, const u8 *b);
static s32 WCM_RssiToQuality(u32 rssi);
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
    u32 irq;
    s32 mode = requestedMode;
    s32 requiredSize;
    s32 result;

    irq = CPU_IRQDisable();
    if (sWork.work != NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    if (((u32)work & 0x1f) != 0) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    sys_memset(work, 0, size);
    if (mode == 0) {
        mode = sWork.preferredMode;
        if (mode == 0) {
            mode = WCM_GetDefaultWirelessMode();
        }
    }
    if (!WCM_GetRequiredWorkSize(&requiredSize, mode)) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    if (size < requiredSize) {
        CPU_SetIRQMask(irq);
        return 6;
    }
    sWork.work = work;
    sWork.work->state = 1;
    sWork.work->pendingNotify = 0;
    sWork.work->scanChannelTime = 0;
    sWork.work->apListLocked = 0;
    sWork.work->resetPending = 0;
    sWork.work->wirelessMode = mode;
    WCM_ResetSearchOption();
    WCM_InitOnce();
    if (!clock_is_inited()) {
        clock_init();
    }
    if (!func_0207bc18()) {
        func_0207bbf4();
    }
    func_0207bc24(&sWork.work->alarm);
    CPU_SetIRQMask(irq);
    return 0;
}

void WCM_Finish(void) {
    u32 irq;
    WCMWork *work;

    irq = CPU_IRQDisable();
    work = sWork.work;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return;
    }
    if (work->state != 1) {
        CPU_SetIRQMask(irq);
        return;
    }
    sWork.work = NULL;
    CPU_SetIRQMask(irq);
}

static BOOL WCM_GetRequiredWorkSize(s32 *size, s32 mode) {
    if (mode == 0) {
        mode = WCM_GetDefaultWirelessMode();
    }
    if (mode == 1) {
        *size = WCM_WORK_SIZE;
    } else if (mode == 2) {
        if (!hw_isDSi()) {
            return FALSE;
        }
        *size = 0x4800;
    } else {
        return FALSE;
    }
    return TRUE;
}

s32 WCM_StartupAsync(WCMConfig *config, WCMNotify notify) {
    u32 irq;
    WCMWork *work;
    s32 result;

    irq = CPU_IRQDisable();
    work = sWork.work;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    switch (work->state) {
    case 1:
        WCM_SetConfig(config, notify);
        break;
    case 2:
        CPU_SetIRQMask(irq);
        return 2;
    case 3:
        CPU_SetIRQMask(irq);
        return 0;
    default:
        CPU_SetIRQMask(irq);
        return 1;
    }

    if (!func_020810cc()) {
        CPU_SetIRQMask(irq);
        return 5;
    }
    switch (work->wirelessMode) {
    case 1:
        result = WCM_StartupStep1();
        break;
    case 2:
        if (hw_isDSi()) {
            result = func_02769930();
        } else {
            result = 7;
        }
        break;
    default:
        result = 7;
        break;
    }
    CPU_SetIRQMask(irq);
    return result;
}

s32 WCM_CleanupAsync(void) {
    u32 irq;
    WCMWork *work;
    s32 result;

    irq = CPU_IRQDisable();
    work = sWork.work;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    switch (work->state) {
    case 1:
        CPU_SetIRQMask(irq);
        return 0;
    case 3:
        break;
    case 4:
        CPU_SetIRQMask(irq);
        return 2;
    default:
        CPU_SetIRQMask(irq);
        return 1;
    }

    switch (work->wirelessMode) {
    case 1:
        result = WCM_StartupStep2();
        break;
    case 2:
        if (hw_isDSi()) {
            result = func_02769a30();
        } else {
            result = 7;
        }
        break;
    default:
        result = 7;
        break;
    }
    CPU_SetIRQMask(irq);
    return result;
}

s32 WCM_SearchAsync(const u8 *bssid, const u8 *essid, u32 option) {
    if (bssid == NULL || essid == NULL) {
        return WCM_StopSearchAsync();
    }
    return WCM_SearchAsyncWithParams(bssid, essid, option);
}

static s32 WCM_SearchAsyncWithParams(const u8 *bssid, const u8 *essid, u32 option) {
    u32 irq;
    WCMWork *work;
    s32 result;

    irq = CPU_IRQDisable();
    work = sWork.work;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    switch (work->wirelessMode) {
    case 1:
        result = WCM_StartSearchStep(bssid, essid, option);
        break;
    case 2:
        if (hw_isDSi()) {
            result = func_02769aa4(bssid, essid, option);
        } else {
            result = 7;
        }
        break;
    default:
        result = 7;
        break;
    }
    CPU_SetIRQMask(irq);
    return result;
}

s32 WCM_StopSearchAsync(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = sWork.work;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    switch (work->state) {
    case 3:
        CPU_SetIRQMask(irq);
        return 0;
    case 6:
        WCM_SetState(7);
        sWork.work->pendingNotify = WCM_NOTIFY_END_SEARCH;
        CPU_SetIRQMask(irq);
        return 3;
    case 7:
        CPU_SetIRQMask(irq);
        return 2;
    default:
        CPU_SetIRQMask(irq);
        return 1;
    }
}

s32 WCM_ConnectAsync(const void *bssDesc, const void *wepDesc, u32 option) {
    u32 irq;
    WCMWork *work;
    s32 result;

    irq = CPU_IRQDisable();
    work = WCM_GetWork();
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    switch (work->state) {
    case 3:
        if (bssDesc == NULL) {
            CPU_SetIRQMask(irq);
            return 1;
        }
        if (((const WMBssDesc *)bssDesc)->gameInfoLength != 0) {
            CPU_SetIRQMask(irq);
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
                CPU_SetIRQMask(irq);
                return 1;
            }
            work->connectionStorage.mode = descriptor[0];
            if (descriptor[0] == 0) {
                sys_memset(work->connectionStorage.data, 0, sizeof(work->connectionStorage.data));
            } else if (descriptor[0] <= 3) {
                work->connectionStorage.argument = descriptor[1];
                sys_memcpy(descriptor + 2, work->connectionStorage.data, sizeof(work->connectionStorage.data));
            } else {
                work->connectionStorage.argument = descriptor[1];
                sys_memcpy(descriptor + 2, work->connectionStorage.data, work->connectionStorage.argument);
            }
        } else {
            sys_memset(&work->connectionStorage, 0, sizeof(work->connectionStorage));
        }
        sys_memcpy(bssDesc, work->unk_12e0, sizeof(work->unk_12e0));
        WCM_SetSearchOption(option);
        break;
    case 8:
        CPU_SetIRQMask(irq);
        return 2;
    case 9:
        CPU_SetIRQMask(irq);
        return 0;
    default:
        CPU_SetIRQMask(irq);
        return 1;
    }

    switch (work->wirelessMode) {
    case 1:
        result = WCM_StartConnectStep();
        break;
    case 2:
        if (hw_isDSi()) {
            result = func_02769b80();
        } else {
            result = 7;
        }
        break;
    default:
        result = 7;
        break;
    }
    CPU_SetIRQMask(irq);
    return result;
}

s32 WCM_DisconnectAsync(void) {
    u32 irq;
    WCMWork *work;
    s32 result = 0;

    irq = CPU_IRQDisable();
    work = sWork.work;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    switch (work->state) {
    case 3:
        CPU_SetIRQMask(irq);
        return 0;
    case 9:
        break;
    case 10:
        CPU_SetIRQMask(irq);
        return 2;
    default:
        CPU_SetIRQMask(irq);
        return 1;
    }

    if (work->resetPending == 1) {
        WCM_SetState(10);
        sWork.work->pendingNotify = 6;
    } else {
        switch (work->wirelessMode) {
        case 1:
            result = WCM_StartDisconnectStep();
            break;
        case 2:
            if (hw_isDSi()) {
                result = func_02769c70();
            } else {
                result = 7;
            }
            break;
        default:
            result = 7;
            break;
        }
    }
    CPU_SetIRQMask(irq);
    return result;
}

s32 WCM_RestoreIdleAsync(void) {
    u32 irq;
    WCMWork *work;
    s32 result = 0;

    irq = CPU_IRQDisable();
    work = WCM_GetWork();
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    switch (work->state) {
    case 1:
        CPU_SetIRQMask(irq);
        return 0;
    case 13:
        CPU_SetIRQMask(irq);
        return 2;
    case 6:
        WCM_SetState(13);
        work->pendingNotify = 9;
        CPU_SetIRQMask(irq);
        return 3;
    case 3:
    case 9:
    case 12:
        break;
    default:
        CPU_SetIRQMask(irq);
        return 1;
    }

    if (work->resetPending == 1) {
        WCM_SetState(13);
        work->pendingNotify = 9;
    } else {
        switch (work->wirelessMode) {
        case 1:
            result = WCM_RestoreIdleStep();
            break;
        case 2:
            if (hw_isDSi()) {
                result = func_02769ce8();
            } else {
                result = 7;
            }
            break;
        default:
            result = 7;
            break;
        }
    }
    CPU_SetIRQMask(irq);
    return result;
}

s32 WCM_GetState(void) {
    u32 irq;
    WCMWork *work;
    s32 state;

    irq = CPU_IRQDisable();
    work = sWork.work;
    state = 0;
    if (work != NULL) {
        state = work->state;
    }
    CPU_SetIRQMask(irq);
    return state;
}

s32 WCM_SetSearchOption(u32 option) {
    u32 irq;
    WCMWork *work;
    u32 mask = 0;
    u32 old;

    irq = CPU_IRQDisable();
    work = sWork.work;
    // BUG: The original reads the search option before checking whether WCM has been initialized.
#ifdef BUGFIX
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 0;
    }
    old = work->searchOption;
#else
    old = work->searchOption;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
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
    work->searchOption = (old & ~mask) | option;
    CPU_SetIRQMask(irq);
    return old;
}

s32 WCM_GetChannelMask(void) {
    WCMWork *work = WCM_GetWork();

    if (work->state < 3 || work->state > 10) {
        return 0;
    }
    switch (work->wirelessMode) {
    case 1:
        return WCM_GetDsAllowedChannelSpanMask();
    case 2:
        if (hw_isDSi()) {
            return func_02769e54();
        }
        break;
    default:
        break;
    }
    return 0;
}

WCMWork *WCM_GetWork(void) {
    return sWork.work;
}

static void WCM_SetConfig(WCMConfig *config, WCMNotify notify) {
    WCMWork *work = sWork.work;

    if (config == NULL) {
        work->dmaNo = 3;
        work->apBuffer = NULL;
        work->apBufferBytes = 0;
        work->scanMode = 0;
    } else {
        u32 alignmentDelta = (4 - ((u32)config->pbdbuffer & 3)) & 3;

        work->dmaNo = config->dmano & 3;
        if ((u32)config->nbdbuffer < alignmentDelta + sizeof(WCMApListHeader)) {
            work->apBuffer = NULL;
            work->apBufferBytes = 0;
        } else {
            u32 align = ((u32)config->pbdbuffer + 3) & ~3u;
            u32 byteCount = (u32)config->nbdbuffer - alignmentDelta;
            work->apBuffer = (void *)align;
            work->apBufferBytes = byteCount;
            sys_memset(work->apBuffer, 0, byteCount);
        }
        work->scanMode = config->nbdmode;
    }
    work->notify = notify;
}

static void WCM_ResetSearchOption(void) {
    WCM_GetWork()->searchOption = 0xaaa082;
}

u16 WCM_PickChannel(s32 channelIndex) {
    s32 i;
    s32 channel;

    for (i = 0; i < 13; i++) {
        channel = (channelIndex + i) % 13;
        if (sWork.work->searchOption & (1 << (channel + 1))) {
            if (WCM_GetChannelMask() & (1 << channel)) {
                break;
            }
        }
    }
    return (u16)((channelIndex + i) % 13 + 1);
}

void WCM_NotifyResult(s16 result, void *parameter, u32 detail, u32 line) {
    WCMWork *work = sWork.work;
    s16 notify = work->pendingNotify;

    work->pendingNotify = 0;
    // The SDK transports mixed pointer/integer payloads as 32-bit words; a host port must widen this representation.
    WCM_Notify(notify, result, (s32)parameter, (void *)detail, line);
}

void WCM_Notify(s16 notify, s16 result, s32 parameter, void *param1, s32 param2) {
    WCMWork *work = sWork.work;
    struct {
        WCMNotice notice;
        void *param1;
        s32 param2;
    } buf;

    if (work->notify == NULL) {
        return;
    }
    buf.notice.notify = notify;
    buf.notice.result = result;
    buf.notice.parameter = parameter;
    buf.param1 = param1;
    buf.param2 = param2;
    work->notify(&buf.notice);
}

void WCM_SetState(s32 newState) {
    u32 irq = CPU_IRQDisable();

    if (sWork.work->state == 9 && newState != 9) {
        func_0207bd3c(&sWork.work->alarm);
    }
    if (sWork.work->state != 11) {
        sWork.work->state = newState;
    }
    if (newState == 9) {
        func_0207bd3c(&sWork.work->alarm);
        func_0207bcf4(&sWork.work->alarm, 0x22f5341, (void (*)(void *))WCM_TimeoutAlarmHandler, NULL);
    }
    CPU_SetIRQMask(irq);
}

void WCM_CancelTimeout(void) {
    WCMWork *work = sWork.work;
    u32 irq = CPU_IRQDisable();

    func_0207bd3c(&work->alarm);
    if (work->state == 9) {
        func_0207bcf4(&work->alarm, 0x22f5341, (void (*)(void *))WCM_TimeoutAlarmHandler, NULL);
    }
    CPU_SetIRQMask(irq);
}

static void WCM_TimeoutAlarmHandler(void) {
    WCM_CancelTimeout();
}

static s32 WCM_GetDefaultWirelessMode(void) {
    return hw_isDSi() ? 2 : 1;
}

void WCM_ClearApListBuffer(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();

    if (work != NULL && work->apBuffer != NULL && (s32)work->apBufferBytes > 0) {
        sys_memset(work->apBuffer, 0, work->apBufferBytes);
    }
    CPU_SetIRQMask(irq);
}

s32 WCM_CountApList(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    s32 count = 0;

    if (work != NULL) {
        WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
        if (header != NULL && work->apBufferBytes > 0xc) {
            count = header->count;
        }
    }
    CPU_SetIRQMask(irq);
    return count;
}

s32 WCM_LockApList(s32 lock) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    s32 wasLocked;

    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return 0;
    }
    wasLocked = (work->apListLocked != 0);
    work->apListLocked = (lock != 0);
    CPU_SetIRQMask(irq);
    return wasLocked;
}

void *WCM_PointApList(u16 index) {
    return WCM_PointApListCore(index);
}

static void *WCM_PointApListCore(u16 index) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    WCMApEntry *entry;

    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return NULL;
    }
    entry = WCM_FindApEntryById(index);
    if (entry == NULL) {
        CPU_SetIRQMask(irq);
        return NULL;
    }
    CPU_SetIRQMask(irq);
    return &entry->record;
}

void WCM_OnBeaconReceived(const WMBssDesc *bss, u16 scanLinkLevel) {
    WCMWork *work = WCM_GetWork();
    WCMApEntry *entry;

    if (work == NULL || work->apListLocked != 0) {
        return;
    }
    if (hw_isDSi() && work->wirelessMode == 2 && WCM_FindVendorIE(bss) != NULL) {
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
        WCM_FillApEntry(bss, &entry->record);
        entry->scanLinkLevel = scanLinkLevel;
        WCM_TouchApEntry(entry);
    }
}

static WCMApEntry *WCM_AllocApEntry(void) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *entries;
    u32 max;
    u32 i;
    WCMApEntry *entry = NULL;

    if (header == NULL) {
        return NULL;
    }
    if (work->apBufferBytes <= 0xc) {
        return NULL;
    }
    max = (work->apBufferBytes - 0xc) / sizeof(WCMApEntry);
    if (max == 0 || header->count >= max) {
        return NULL;
    }
    entries = (WCMApEntry *)((u8 *)header + 0xc);
    for (i = 0; i < max; i++) {
        entry = &entries[i];
        if (!entry->inUse) {
            break;
        }
    }
    if (i >= max) {
        return entry;
    }
    entry->inUse = TRUE;
    entry->id = header->count;
    entry->prev = header->tail;
    entry->next = NULL;
    header->tail = entry;
    if (entry->prev != NULL) {
        entry->prev->next = entry;
    } else {
        header->head = entry;
    }
    header->count++;
    return entry;
}

static WCMApEntry *WCM_GetOldestApEntry(void) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;

    if (header != NULL && work->apBufferBytes > 0xc) {
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
    if (header != NULL && work->apBufferBytes > 0xc) {
        for (entry = header->head; entry != NULL; entry = entry->next) {
            u8 *record = (u8 *)&entry->record;
            if (WCM_CompareBssID(record + 4, bssid)) {
                break;
            }
        }
    }
    return entry;
}

static WCMApEntry *WCM_FindApEntryById(s32 id) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *entry = NULL;

    if (header != NULL && work->apBufferBytes > 0xc) {
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
    if (header == NULL || work->apBufferBytes <= 0xc) {
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
    const u8 *ie = (const u8 *)source + 0x40;
    u8 *output = (u8 *)destination + 0x40;
    u8 total = 0;
    u16 n;

    sys_memset32_fast(0, destination, sizeof(*destination));
    sys_memcpy32_fast(source, destination, 0x40);
    destination->otherElementCount = 0;
    destination->length = work->wirelessMode == 1 ? 0x20 : 0x40;
    for (n = 0; n < source->otherElementCount; n++) {
        u8 tag = ie[0];
        u16 ieLen = ie[1] + 2;
        BOOL retain = tag == 0x32 || tag == 0x2a || tag == 0x2f || tag == 7 || tag == 0x30;

        if (tag == 0xdd) {
            retain = sys_memcmp(ie + 2, data_ov011_02185008, 4) == 0 ||
                     sys_memcmp(ie + 2, data_ov011_0218500c, 5) == 0;
        }
        if (retain) {
            sys_memcpy_fast(ie, output, (u8)ieLen);
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
    const u8 *bytes = (const u8 *)bss;
    u8 buf[4];

    sys_memcpy_fast(data_ov011_02185004, buf, 4);
    if (bss == NULL) {
        return NULL;
    }
    if (hw_isDSi()) {
        if (work->wirelessMode == 2 && bss->otherElementCount != 0) {
            void *ie = WCM_GetInformationElement(bss, 0xdd, buf);
            if (ie != NULL && *(u16 *)((u8 *)ie + 0xa) == 1) {
                return (u8 *)ie + 0xa;
            }
        }
        return NULL;
    }
    if (work->wirelessMode == 1) {
        u16 len = bss->gameInfoLength;
        if (len != 0 && len >= 0x10 && len <= 0x80) {
            if (len == bytes[0x4a] + 0x10 && *(const u16 *)(bytes + 0x40) == 1 &&
                bss->length == (len + 1 + 0x40) / 2) {
                return (u8 *)bss + 0x40;
            }
        }
    }
    return NULL;
}

static void *WCM_GetInformationElement(const void *frame, u32 tag, void *out) {
    return func_02703250(frame, tag, out);
}

static void WCM_InitOnce(void) {
    if (!sWcm.initialized) {
        sWcm.initialized = TRUE;
        sWcm.sendStatus = 0;
        sys_memset(sWcm.ownMac, 0, 6);
        OS_InitThreadQueue(&sWcm.sendQueue);
        OS_InitMutex(&sWcm.mutex);
    }
}

void WCM_DispatchRecvCallback(u8 *src, u8 *dst, u8 *data, u32 len) {
    if (sWcm.recvCallback != NULL) {
        sWcm.recvCallback(src, dst, data, len);
    }
}

WCMGlobal *WCM_GetGlobal(void) {
    return &sWcm;
}

const u8 *WCM_GetApMacAddress(void) {
    const u8 *mac = NULL;
    WCMWork *work = WCM_GetWork();
    u32 irq = CPU_IRQDisable();

    if (work != NULL && work->state == 9 && work->resetPending == 0) {
        mac = work->unk_12e0 + 4;
    }
    CPU_SetIRQMask(irq);
    return mac;
}

// The SSID is borrowed from the connected BSS descriptor and is not necessarily NUL-terminated.
const u8 *WCM_GetConnectedApSsid(u16 *ssidLength) {
    const u8 *ssid = NULL;
    u16 length = 0;
    WCMWork *work = WCM_GetWork();
    u32 irq = CPU_IRQDisable();

    if (work != NULL && work->state == 9 && work->resetPending == 0) {
        const WMBssDesc *bss = (const WMBssDesc *)work->unk_12e0;
        length = bss->ssidLength;
        ssid = bss->ssid;
    }
    CPU_SetIRQMask(irq);
    if (ssidLength != NULL) {
        *ssidLength = length;
    }
    return ssid;
}

void WCM_SetRecvDCFCallback(void (*callback)(u8 *src, u8 *dst, u8 *data, u32 len)) {
    u32 irq = CPU_IRQDisable();
    sWcm.recvCallback = callback;
    CPU_SetIRQMask(irq);
}

s32 WCM_SendDCFDataEx(const u8 *dst, const u8 *header, s32 headerLen, const u8 *data, s32 dataLen) {
    u32 irq;
    WCMWork *work;
    s32 sentLen;

    irq = CPU_IRQDisable();
    work = WCM_GetWork();
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return -1;
    }
    OS_LockMutex(&sWcm.mutex);
    work = WCM_GetWork();
    if (work == NULL) {
        OS_UnlockMutex(&sWcm.mutex);
        CPU_SetIRQMask(irq);
        return -1;
    }
    if (work->state != 9 || work->resetPending == 1) {
        OS_UnlockMutex(&sWcm.mutex);
        CPU_SetIRQMask(irq);
        return -4;
    }
    sentLen = WCM_AssembleSendFrame(work->wirelessMode, work, header, headerLen, data, dataLen);
    switch (work->wirelessMode) {
    case 1:
        WCM_SendFrame(dst, work, (u16)sentLen, NULL);
        break;
    case 2:
        if (hw_isDSi()) {
            func_02769dc4(dst, work, (u16)sentLen, NULL);
        }
        break;
    default:
        break;
    }
    OS_UnlockMutex(&sWcm.mutex);
    CPU_SetIRQMask(irq);
    return headerLen + dataLen;
}

void WCM_ReleaseIrqOwnedMutex(OSMutex *mutex) {
    // The IRQ-handler address is an owner marker, not an OSThread object; this path releases no ownership-list link.
    if (mutex->thread == (OSThread *)CPU_IRQInvoke) {
        mutex->count--;
        if (mutex->count == 0) {
            mutex->thread = NULL;
            OS_WakeupThread(&mutex->queue);
        }
    }
}

void WCM_SetMacAddress(const u8 *mac) {
    sys_memcpy_fast(mac, sWcm.ownMac, 6);
}

void WCM_GetMacAddress(u8 *mac) {
    sys_memcpy_fast(sWcm.ownMac, mac, 6);
}

static s32 WCM_AssembleSendFrame(s32 mode, void *dest, const void *header, s32 headerLen, const void *data,
                                 s32 dataLen) {
    u8 *payload = dest;
    s32 total = headerLen + dataLen;

    if (mode == 2) {
        u16 length = (u16)total;
        *(u16 *)((u8 *)dest + 0xc) = (u16)((length >> 8) | (length << 8));
        payload += 0xe;
        total += 0xe;
    }
    sys_memcpy_fast(header, payload, headerLen);
    if (data != NULL && dataLen != 0) {
        sys_memcpy_fast(data, payload + headerLen, dataLen);
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

s32 WCM_UpdateLinkQuality(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    s32 quality = 0;

    if (work != NULL && work->state == 9) {
        quality = WCM_RssiToQuality(WCM_GetAverageRssi());
    }
    CPU_SetIRQMask(irq);
    return quality;
}

static s32 WCM_RssiToQuality(u32 rssi) {
    WCMWork *work = WCM_GetWork();

    if (work->wirelessMode == 2) {
        if (rssi >= 0x16) {
            return 3;
        }
        if (rssi >= 0x11) {
            return 2;
        }
        if (rssi >= 0xc) {
            return 1;
        }
        return 0;
    }
    if (rssi >= 0x1c) {
        return 3;
    }
    if (rssi >= 0x16) {
        return 2;
    }
    if (rssi >= 0x10) {
        return 1;
    }
    return 0;
}

static u8 WCM_GetAverageRssi(void) {
    s32 sum = 0;
    u8 count = sWcm.rssiSampleCursor;
    int i;

    if (count > 0x10) {
        for (i = 0; i < 0x10; i++) {
            sum += sWcm.rssiSamples[i];
        }
        sum /= 0x10;
    } else if (count != 0) {
        for (i = 0; i < count; i++) {
            sum += sWcm.rssiSamples[i];
        }
        sum /= count;
    }
    return sum;
}

// Adds a signal-level sample to the running RSSI average's circular buffer
// Decodes the receive record's high signal byte. The original arithmetic is mode-dependent;
// no physical units for the encoded input or the resulting sample have been established.
void WCM_AddRssiSample(s32 rawSample) {
    u8 sample;
    u8 cursor;

    if (WCM_GetWork()->wirelessMode == 2) {
        sample = (u8)rawSample;
    } else if (rawSample & 2) {
        sample = (u8)(rawSample << 2);
    } else {
        sample = (u8)((rawSample >> 2) + 25);
    }
    cursor = sWcm.rssiSampleCursor;
    sWcm.rssiSamples[cursor % 16] = sample;
    if (cursor < 16) {
        sWcm.rssiSampleCursor = cursor + 1;
    } else {
        sWcm.rssiSampleCursor = 16 + ((cursor + 1) % 16);
    }
}

// The startup FSM's WM_Initialize-adjacent first step: brings up the hardware and reads its MAC address
static s32 WCM_StartupStep1(void) {
    WCMWork *work = WCM_GetWork();
    s32 result = func_02080758(work->startupBuffer, (u16)work->dmaNo);
    switch (result) {
    case 0:
        if (func_02080e7c((WMCallbackFunc)WCM_OnWmIndication) != 0) {
            WCM_SetState(11);
            return 7;
        }
        {
            u8 mac[6];
            func_0207c33c(mac);
            WCM_SetMacAddress(mac);
        }
        switch (func_020812fc((WMCallbackFunc)WCM_OnWmControlComplete)) {
        case 2:
            WCM_SetState(2);
            work->pendingNotify = 1;
            return 3;
        case 8:
            WCM_SetState(12);
            return 1;
        default:
            WCM_SetState(11);
            return 7;
        }
    case 4:
        return 5;
    default:
        WCM_SetState(11);
        return 7;
    }
}

static s32 WCM_StartupStep2(void) {
    WCMWork *work = WCM_GetWork();

    switch (func_02081398((WMCallbackFunc)WCM_OnWmControlComplete)) {
    case 2:
        WCM_SetState(4);
        work->pendingNotify = 2;
        return 3;
    case 8:
        return 4;
    default:
        WCM_SetState(0xb);
        return 7;
    }
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
    cp15_invalidateDC(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
    work->scanCount++;
    switch (func_020815c8((WMCallbackFunc)WCM_ConnectStep4, &work->scanRequest)) {
    case 2:
        WCM_SetState(5);
        work->pendingNotify = 3;
        return 3;
    case 8:
        return 4;
    default:
        WCM_SetState(0xb);
        return 7;
    }
}

static s32 WCM_StartConnectStep(void) {
    WCMWork *work = WCM_GetWork();

    if (work->state == 3) {
        WMBssDesc *bss = (WMBssDesc *)work->unk_12e0;
        bss->rateSetSupport = bss->rateSetBasic | 3;
    }
    switch (func_02082560((WMCallbackFunc)WCM_OnWmControlComplete, 0xffff, 0x50, 0xffff, 0xffff)) {
    case 2:
        WCM_SetState(8);
        work->pendingNotify = 5;
        break;
    case 8:
        return 4;
    default:
        WCM_SetState(0xb);
        return 7;
    }
    return 3;
}

static s32 WCM_StartDisconnectStep(void) {
    WCMWork *work = WCM_GetWork();

    switch (func_02081b94((WMCallbackFunc)WCM_OnDisconnectPreparationComplete)) {
    case 2:
        WCM_SetState(0xa);
        work->pendingNotify = 6;
        break;
    case 8:
        return 4;
    default:
        WCM_SetState(0xb);
        return 7;
    }
    return 3;
}

static s32 WCM_RestoreIdleStep(void) {
    WCMWork *work = WCM_GetWork();
    const u16 *status = func_02080dbc();
    s32 result;

    cp15_invalidateDC(status, 2);
    switch (*status) {
    case 0:
        result = func_02080908();
        if (result == 0) {
            WCM_SetState(1);
            work->pendingNotify = 0;
            return 0;
        }
        break;
    case 1:
        result = func_02081348((WMCallbackFunc)WCM_OnWmControlComplete);
        break;
    case 2:
        result = func_02081398((WMCallbackFunc)WCM_OnWmControlComplete);
        break;
    default:
        work->resetPending = 1;
        result = func_02081424((WMCallbackFunc)WCM_OnResetComplete);
        break;
    }
    switch (result) {
    case 2:
        WCM_SetState(0xd);
        work->pendingNotify = 9;
        break;
    case 8:
        return 4;
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
    if (func_02081b18(callback, dst, buffer, byteLen) != 2) {
        return -5;
    }
    if (completion == NULL) {
        scheduler_yield(&global->sendQueue);
        if (global->sendStatus != 0) {
            return -5;
        }
    }
    return byteLen;
}

static u16 WCM_GetDsAllowedChannelSpanMask(void) {
    u16 allowed = func_020810cc();
    u16 upperMask = 0x8000;
    u16 lowerMask = 1;

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
    return (u16) ~((upperMask << 1) | (lowerMask >> 1));
}

static void WCM_RequestReset(void) {
    WCMWork *work = WCM_GetWork();

    if (work->resetPending == 0) {
        work->resetPending = 1;
        if (func_02081424((WMCallbackFunc)WCM_OnResetComplete) != 2) {
            WCM_SetState(0xb);
            WCM_NotifyResult(7, NULL, 0, 0x24f);
        }
    }
}

// WM's indication callback, installed in WCM_StartupStep1: dispatches disconnect indications by the current state
static void WCM_OnWmIndication(void *ind) {
    u16 type = ((u16 *)ind)[1];
    WCMWork *work;

    if (type != 8 || ((u16 *)ind)[2] != 0x16 || ((u16 *)ind)[3] != 0x25) {
        return;
    }
    work = WCM_GetWork();
    switch (work->state - 8) {
    case 0:
        WCM_SetState(0xc);
        break;
    case 1:
    case 4:
        WCM_RequestReset();
        break;
    case 2:
        WCM_SetState(0xc);
        break;
    default:
        break;
    }
}

static void WCM_OnWmControlComplete(void *ind) {
    const WMCallback *callback = (const WMCallback *)ind;
    WCMWork *work = WCM_GetWork();
    int result = 20;
    void *parameter;
    switch (callback->errcode) {
    case 0:
        switch (callback->apiid) {
        case 3:
            result = func_02081370((WMCallbackFunc)WCM_OnWmControlComplete);
            break;
        case 4:
            if (func_02080908() == 0) {
                WCM_SetState(1);
                WCM_NotifyResult(0, NULL, 0, 0x2a4);
            } else {
                WCM_SetState(11);
                WCM_NotifyResult(7, NULL, 0, 0x2ad);
            }
            return;
        case 5:
            WCM_SetState(3);
            WCM_NotifyResult(0, NULL, 0, 0x2b4);
            return;
        case 6:
            result = func_02081348((WMCallbackFunc)WCM_OnWmControlComplete);
            break;
        case 25:
            result = func_02082470((WMCallbackFunc)WCM_OnWmControlComplete, work->connectionStorage.mode,
                                   work->connectionStorage.argument, work->connectionStorage.data);
            break;
        case 29:
            result = func_0208252c((WMCallbackFunc)WCM_OnWmControlComplete, 0);
            break;
        case 39: {
            u32 options = work->searchOption;
            u16 option = (options & 0xc0000) == 0xc0000;
            BOOL mode = (options & 0x30000) != 0x30000;
            result = func_020816dc((WMCallbackFunc)WCM_OnConnectStatus, (const WMBssDesc *)work->unk_12e0, NULL, mode,
                                   option);
            break;
        }
        }
        switch (result) {
        case 2:
            return;
        case 8:
            WCM_SetState(12);
            parameter = work->pendingNotify == WCM_NOTIFY_CONNECT ? work->unk_12e0 : NULL;
            WCM_NotifyResult(1, parameter, 0, 0x2f0);
            return;
        default:
            WCM_SetState(11);
            parameter = work->pendingNotify == WCM_NOTIFY_CONNECT ? work->unk_12e0 : NULL;
            WCM_NotifyResult(7, parameter, 0, 0x2f9);
            return;
        }
    case 1:
        WCM_SetState(12);
        parameter = work->pendingNotify == WCM_NOTIFY_CONNECT ? work->unk_12e0 : NULL;
        WCM_NotifyResult(1, parameter, 0, 0x31f);
        return;
    default:
        WCM_SetState(11);
        parameter = work->pendingNotify == WCM_NOTIFY_CONNECT ? work->unk_12e0 : NULL;
        WCM_NotifyResult(7, parameter, 0, 0x329);
        return;
    }
}

static void WCM_ConnectStep4(void *ind) {
    WCMWork *work = WCM_GetWork();
    int result = 0x14;
    int i;
    u32 count;

    switch (((u16 *)ind)[1]) {
    case 0:
        if (work->state == 5) {
            WCM_SetState(6);
            WCM_NotifyResult(0, NULL, 0, 0x346);
        }
        switch (work->state) {
        case 6:
            work->pendingNotify = 7;
            if (((u16 *)ind)[4] == 5) {
                cp15_invalidateDC(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
                for (i = 0; i < ((u16 *)ind)[7]; i++) {
                    void *bssDesc = ((void **)ind)[4 + i];
                    WCM_OnBeaconReceived(bssDesc, ((u16 *)((u8 *)ind + 0x50))[i]);
                    WCM_Notify(7, 0, (s32)bssDesc, ind, 0x35c);
                }
            }
            if ((work->searchOption & 0xc00000) == 0xc00000) {
                count = MATH_CountPopulation(work->searchOption & 0x3ffe);
                if (count != 0 && work->scanCount % count == 0) {
                    WCM_Notify(8, 0, work->scanCount, NULL, 0x36a);
                }
            }
            work->scanRequest.channelList =
                (u16)((1 << WCM_PickChannel((u16)(32 - MATH_CountLeadingZerosFunc(((u16 *)ind)[5])))) >> 1);
            cp15_cleanDC(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
            work->scanCount++;
            result = func_020815c8((WMCallbackFunc)WCM_ConnectStep4, &work->scanRequest);
            break;
        case 7:
            result = func_020816b4((WMCallbackFunc)WCM_ConnectStep5);
            break;
        case 0xd:
            WCM_RequestReset();
            return;
        default:
            break;
        }
        if (result == 2) {
            return;
        }
        if (result == 8) {
            WCM_SetState(0xc);
            WCM_NotifyResult(1, NULL, 0, 0x395);
        } else {
            WCM_SetState(0xb);
            WCM_NotifyResult(7, NULL, 0, 0x39e);
        }
        break;
    case 1:
        WCM_RequestReset();
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, 0, 0x3af);
        break;
    }
}

static void WCM_ConnectStep5(void *ind) {
    switch (((const u16 *)ind)[1]) {
    case 0:
        WCM_SetState(3);
        WCM_NotifyResult(0, NULL, 0, 0x3c5);
        return;
    case 1:
        WCM_RequestReset();
        return;
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, NULL, 0, 0x3d5);
        return;
    }
}

static void WCM_OnConnectStatus(void *ind) {
    const WCMConnectStatusView *callback = ind;
    WCMWork *work = WCM_GetWork();
    int result;

    switch (callback->base.errcode) {
    case 0:
        switch (callback->base.state) {
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
        case 7:
            if (work->state == 0xc) {
                WCM_SetState(8);
                WCM_RequestReset();
                break;
            }
            if (callback->base.aid < 1 || callback->base.aid > 0x7d7) {
                WCM_RequestReset();
                break;
            }
            work->associationId = callback->base.aid;
            result =
                func_02081ab0((WMCallbackFunc)WCM_OnDcfReceive, work->ioBuffer.receive, sizeof(work->ioBuffer.receive));
            if (result == 2) {
                break;
            }
            if (result == 8) {
                WCM_SetState(0xc);
                WCM_NotifyResult(1, work->unk_12e0, 0, 0x430);
            } else {
                WCM_SetState(0xb);
                WCM_NotifyResult(7, work->unk_12e0, 0, 0x439);
            }
            break;
        case 6:
        case 0x1a:
            break;
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, work->unk_12e0, callback->base.state, 0x447);
            break;
        }
        break;
    case 1:
        work->connectionFailureReason = callback->reason;
    case 6:
    case 11:
    case 12:
        WCM_SetState(8);
        WCM_RequestReset();
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0x45e);
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
            WCM_NotifyResult(0, work->unk_12e0, 0, 0x481);
        }
        break;
    case 1:
    case 3:
        WCM_SetState(0xa);
        WCM_RequestReset();
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0x493);
        break;
    }
}

static void WCM_OnDcfReceive(void *ind) {
    const WCMDcfCallbackView *callback = ind;
    WCMWork *work = WCM_GetWork();

    switch (callback->base.errcode) {
    case 0:
        switch (callback->state) {
        case 0xe:
            if (work->state == 0xc) {
                WCM_SetState(8);
                WCM_RequestReset();
            } else {
                WCM_SetState(9);
                WCM_NotifyResult(0, work->unk_12e0, 0, 0x4b7);
            }
            break;
        case 0xf: {
            WCMReceiveRecord *record = callback->record;
            u8 rawSample = (u8)(record->signalWord >> 8);
            WCM_AddRssiSample(rawSample);
            cp15_invalidateDC(record, 0x620);
            WCM_DispatchRecvCallback(record->source, record->destination, record->payload, record->payloadLength);
            break;
        }
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, work->unk_12e0, callback->state, 0x4c6);
            break;
        }
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0x4d1);
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
            return;
        }
        switch (func_02081774((WMCallbackFunc)WCM_OnDisconnectComplete, 0)) {
        case 2:
            return;
        case 3:
            WCM_SetState(0xa);
            WCM_RequestReset();
            return;
        case 8:
            WCM_SetState(0xc);
            WCM_NotifyResult(1, work->unk_12e0, 0, 0x4fa);
            return;
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, work->unk_12e0, 0, 0x506);
            return;
        }
    case 1:
    case 3:
        WCM_SetState(0xa);
        WCM_RequestReset();
        return;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0x519);
        return;
    }
}

static void WCM_OnResetComplete(void *ind) {
    const WMCallback *callback = ind;
    WCMWork *work = WCM_GetWork();
    u16 reason;
    BOOL authMode;
    BOOL powerSave;
    int result;

    if (callback->errcode != 0) {
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, 0, 0x5a7);
        return;
    }
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
    case 8:
        reason = work->connectionFailureReason;
        work->connectionFailureReason = 0;
        if (reason == 0x12 && (*(u16 *)((u8 *)work + 0x1310) & 0x24) != 0x24) {
            *(u16 *)((u8 *)work + 0x1310) |= 0x24;
            authMode = ((work->searchOption & 0xc0000) == 0xc0000);
            powerSave = ((work->searchOption & 0x30000) != 0x30000);
            result = func_020816dc((WMCallbackFunc)WCM_OnConnectStatus, (const WMBssDesc *)work->unk_12e0, NULL,
                                   powerSave, (u16)authMode);
            if (result == 2) {
                break;
            }
            if (result == 8) {
                WCM_SetState(0xc);
                WCM_NotifyResult(1, work->unk_12e0, reason, 0x563);
            } else {
                WCM_SetState(0xb);
                WCM_NotifyResult(7, work->unk_12e0, reason, 0x56c);
            }
        } else {
            WCM_SetState(3);
            WCM_NotifyResult(1, work->unk_12e0, reason, 0x574);
        }
        break;
    case 9:
    case 12:
        WCM_SetState(3);
        WCM_NotifyResult(0, work->unk_12e0, 1, 0x57c);
        break;
    case 10:
        WCM_SetState(3);
        WCM_NotifyResult(0, work->unk_12e0, 0, 0x583);
        break;
    case 13:
        result = func_02081398((WMCallbackFunc)WCM_OnWmControlComplete);
        if (result == 2) {
            break;
        }
        if (result == 8) {
            WCM_SetState(0xc);
            WCM_NotifyResult(1, NULL, 0, 0x591);
        } else {
            WCM_SetState(0xb);
            WCM_NotifyResult(7, NULL, 0, 0x59a);
        }
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, work->state, 0x5a0);
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
        channelTime = func_020811a4();
    }
    work->scanRequest.maxChannelTime = channelTime;
    work->scanRequest.scanType = ((work->searchOption & 0x300000) != 0x300000);
    if (bssid != NULL) {
        sys_memcpy(bssid, work->scanRequest.bssid, 6);
    } else {
        sys_memcpy(WCMiAnyBssid, work->scanRequest.bssid, 6);
    }
    if (essid == NULL || essid == WCMiAnyEssid) {
        sys_memcpy(WCMiAnyEssid, work->scanRequest.ssid, 0x20);
        work->scanRequest.ssidLength = 0;
    } else {
        s32 i;

        sys_memcpy(essid, work->scanRequest.ssid, 0x20);
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
    status = (global->sendStatus == 0) ? 0 : -5;
    completion = global->sendCompletion;
    global->sendCompletion = NULL;
    WCM_ReleaseIrqOwnedMutex(&global->mutex);
    completion(status);
}

// The "any" BSSID and ESSID that a search takes when the caller gives none
const u8 WCMiAnyBssid[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00};
const u8 WCMiAnyEssid[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
static const u8 data_ov011_02185004[] = {0x00, 0x09, 0xbf, 0x00};
static const u8 data_ov011_02185008[] = {0x00, 0x50, 0xf2, 0x01};
static const u8 data_ov011_0218500c[] = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x00, 0x00, 0x00};
