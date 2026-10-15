#include "nitrowifi/wcm.h"

#include "nitro/mi.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// NitroWiFi's WCM (overlay 11), over NitroSDK's WM. dsd splits this range into 93 pieces; eight Thumb/ARM pairs among
// them are the linker's long-branch veneers to the LTD module (lib/twl's TwlSDK, not yet decompiled past mi.h/pm.h),
// so this file calls their real targets directly instead of writing the veneers out. The ROM names none of this;
// WCM_Init through WCM_SetRecvDCFCallback keep the names lib/dwc/include/dwc/wcm.h already gave them, and the rest
// are named from what they do.

// sys_memcpy_fast and sys_memcpy32_fast: like sys_memcpy and MI_CpuFillFast's sys_memset32_fast, a multiple of 4
// bytes at a time where they can (MIi_CpuCopyFast and MIi_CpuCopy32Fast). Not yet in nitro/mi.h
void sys_memcpy_fast(const void *src, void *dest, u32 size);
void sys_memcpy32_fast(const void *src, void *dest, u32 size);

// The hardware IRQ dispatcher NitroSDK installs, compared here against a callback's address. Not yet in nitro/os.h
void CPU_IRQInvoke(void);

// LTD's WM-adjacent helpers this file reaches through the linker's veneers; LTD itself (lib/twl) is not yet
// decompiled past mi.h/pm.h, so these keep their default names
s32 func_02769930(void);
s32 func_02769a30(void);
s32 func_02769aa4(const u8 *bssid, const u8 *essid, u32 option);
s32 func_02769b80(void);
s32 func_02769c70(void);
s32 func_02769ce8(void);
s32 func_02769dc4(const u8 *dst, const u8 *header, s32 headerLen, const u8 *data, s32 dataLen);
s32 func_02769e54(void);

// NitroSDK's WM internals this file calls that nitro/wm.h does not yet declare
s32 func_02080758(void *buf, u16 dmaNo);
BOOL func_02080908(void);
void *func_02080dbc(void);
int func_02081348(WMCallbackFunc callback);
int func_02081398(WMCallbackFunc callback);
int func_02081b94(WMCallbackFunc callback);
int func_02081b18(const void *header, s32 headerLen, const void *data, s32 dataLen);
int func_02081ab0(WMCallbackFunc callback, void *buffer, u16 bufferSize);
int func_02082470(WMCallbackFunc callback, u8 bssid0, u8 bssid1, void *replyBuf);
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
    u16 rssi; // the beacon or probe response's signal level, WCM_OnBeaconReceived's param
    s32 id; // the AP list header's count when this entry was allocated or reused
    WCMApEntry *prev;
    WCMApEntry *next;
    u8 record[0xc0]; // the raw beacon or probe response header, with its parsed information elements appended
};

// The caller's AP list buffer (WCMConfig.pbdbuffer), read as a header followed by up to apMax WCMApEntry records
typedef struct WCMApListHeader {
    s32 count;
    WCMApEntry *head; // the oldest entry (WCM_GetOldestApEntry's result)
    WCMApEntry *tail; // the newest
} WCMApListHeader;

// WCM's own work area (WCM_WORK_SIZE bytes), zeroed by WCM_Init. Most of the connect and search state machine's
// scratch area is still unmapped
typedef struct WCMWork {
    u8 unk_0000[0x600];
    u8 scanBuffer[0x400]; // 0x600, scan capacity in bytes
    u8 unk_0a00[0x1294 - 0xa00];
    OSAlarm alarm; // 0x1294
    s32 dmaNo; // 0x12c0, from WCMConfig.dmano
    void *apBuffer; // 0x12c4, from WCMConfig.pbdbuffer: a WCMApListHeader followed by its entries
    u32 apBufferWords; // 0x12c8, from WCMConfig.nbdbuffer
    u32 scanMode; // 0x12cc, from WCMConfig.nbdmode
    WCMNotify notify; // 0x12d0
    u32 searchOption; // 0x12d4
    u16 scanChannelTime; // 0x12d8, zero selects the WM default
    s16 pendingNotify; // 0x12da, notification ID consumed by WCM_NotifyResult
    u8 apListLocked; // 0x12dc
    u8 wmIndInstalled; // 0x12dd
    u8 state; // 0x12de
    u8 subState; // 0x12df
    u8 unk_12e0[0x22a0 - 0x12e0]; // BSS data and notification storage
    WMScanExParam scanRequest;    // 0x22a0, including the 16-byte SSID match mask
    u32 scanCount;                // 0x22e4
    u16 unk_22e8;
    u16 unk_22ea;
    u8 unk_22ec[WCM_WORK_SIZE - 0x22ec];
} WCMWork;

// WCM's own small globals, zeroed with the rest of .bss
// data_ov011_0218df74: the anchor WCM_Init and WCM_Finish set and clear
typedef struct WCMAnchor {
    u32 unk_00;
    WCMWork *work; // 0x04
} WCMAnchor;

// data_ov011_0218df7c, right after WCMAnchor: the globals WCM_InitOnce sets up once
typedef struct WCMGlobal {
    u8 macCached; // 0x00, set once WCM_InitOnce has read the hardware's MAC address
    u8 unk_01[3];
    u32 unk_04;
    u32 unk_08;
    OSMutex mutex; // 0x0c
    u16 unk_24;
    void (*recvCallback)(u8 *src, u8 *dst, u8 *data, u32 len); // 0x28
    u8 unk_2c[4];
    u8 ownMac[6]; // 0x30
    u8 unk_36[2];
    u8 rssiSampleCount; // 0x38
    u8 rssiSamples[0x13]; // 0x39, up to 16 used
} WCMGlobal;

static WCMAnchor sWork;
static WCMGlobal sWcm;

s32 WCM_InitCore(void *work, s32 size, BOOL keepBand);

static BOOL WCM_GetRequiredWorkSize(s32 *size, s32 band);
static s32 WCM_SearchAsyncWithParams(const u8 *bssid, const u8 *essid, u32 option);
static s32 WCM_SearchStep1(const u8 *bssid, const u8 *essid, u32 option);
static s32 WCM_OnBeaconReceivedDSi(void *frame);
static void WCM_StoreKnownNetwork(const void *bssDesc, const void *wepDesc);
static void WCM_AddRssiSample(s32 rssi);
static WCMWork *WCM_GetWork(void);
static void WCM_SetConfig(WCMConfig *config, WCMNotify notify);
static void WCM_ResetSearchOption(void);
static u16 WCM_PickChannel(u32 channelIndex);
static void WCM_Notify(s16 notify, s16 result, s32 parameter, void *param1, s32 param2);
static void WCM_NotifyResult(s16 result, void *parameter, u32 detail, u32 line);
static s32 WCM_SetState(s32 newState);
static void WCM_CancelTimeout(void);
static void WCM_TimeoutAlarmHandler(void);
static s32 WCM_DefaultBand(void);
static void WCM_ClearApListBuffer(void);
static void *WCM_PointApListCore(u16 index);
static WCMApEntry *WCM_AllocApEntry(void);
static WCMApEntry *WCM_GetOldestApEntry(void);
static WCMApEntry *WCM_FindApEntryByBssid(const u8 *bssid);
static WCMApEntry *WCM_FindApEntryById(s32 id);
static void WCM_TouchApEntry(WCMApEntry *entry);
static void WCM_FillApEntry(const void *frame, WCMApEntry *entry);
static void *WCM_FindVendorIE(void *frame);
static void *WCM_GetInformationElement(void *frame, u32 tag, void *out);
static void WCM_InitOnce(void);
static WCMGlobal *WCM_GetGlobal(void);
static void WCM_SetMacAddress(const u8 *mac);
static s32 WCM_BuildFrameHeader(s32 kind, void *header, s32 dataLen, s32 ssidLen);
static BOOL WCM_CompareBytes(const void *a, const void *b, u32 size);
static BOOL WCM_CompareBssID(const u8 *a, const u8 *b);
static s32 WCM_UpdateLinkQuality(void);
static s32 WCM_RssiToQuality(s32 rssi);
static s32 WCM_GetAverageRssi(void);
static s32 WCM_StartupStep1(void);
static s32 WCM_StartupStep2(void);
static s32 WCM_ConnectStep1(const u8 *bssid, const u8 *essid, u32 option);
static s32 WCM_ConnectStep2(void);
static s32 WCM_DisconnectStep(void);
static s32 WCM_StartParentStep(void);
static s32 WCM_SendFrame(const u8 *dst, const void *header, s32 headerLen, BOOL viaDSi);
static u16 WCM_ComputeChannelGap(u16 channelMask);
static void WCM_CancelConnection(void);
static void WCM_OnWmIndication(void *ind);
static void WCM_ConnectStep3(void *ind);
static void WCM_ConnectStep4(void *ind);
static void WCM_ConnectStep5(void *ind);
static void WCM_ConnectStep6(void *ind);
static void WCM_ConnectStep7(void *ind);
static void WCM_ConnectStep8(void *ind);
static void WCM_ParentStep2(void *ind);
static void WCM_ParentStep3(void *ind);
static void WCM_BuildScanRequest(const u8 *bssid, const u8 *essid, u32 option);
static void WCM_OnAsyncDone(void *ind);
static void WCM_OnAsyncDoneWithStatus(void *ind);

s32 WCM_Init(void *work, s32 size) {
    return WCM_InitCore(work, size, TRUE);
}

s32 WCM_InitCore(void *work, s32 size, BOOL keepBand) {
    u32 irq;
    s32 band;
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
    if (!keepBand && !sWcm.macCached) {
        band = WCM_DefaultBand();
    } else {
        band = keepBand;
    }
    if (!WCM_GetRequiredWorkSize(&requiredSize, band)) {
        CPU_SetIRQMask(irq);
        return 1;
    }
    if (size < requiredSize) {
        CPU_SetIRQMask(irq);
        return 6;
    }
    sWork.work = work;
    ((WCMWork *)work)->state = 1;
    ((WCMWork *)work)->subState = 0;
    ((WCMWork *)work)->apListLocked = 0;
    ((WCMWork *)work)->wmIndInstalled = 0;
    WCM_InitOnce();
    if (!clock_is_inited()) {
        clock_init();
    }
    if (!func_0207bc18()) {
        func_0207bbf4();
    }
    func_0207bc24((OSAlarm *)((u8 *)work + 0x1294));
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

static BOOL WCM_GetRequiredWorkSize(s32 *size, s32 band) {
    if (band == 1) {
        *size = 0x23 << 8;
    } else if (band == 2) {
        if (!hw_isDSi()) {
            return FALSE;
        }
        *size = 0x12 << 10;
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
    switch (work->subState) {
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

    switch (work->subState) {
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
        return WCM_ContinueSearchAsync();
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
    switch (work->subState) {
    case 1:
        result = WCM_SearchStep1(bssid, essid, option);
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

s32 WCM_ContinueSearchAsync(void) {
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
    case 3:
        CPU_SetIRQMask(irq);
        return 2;
    case 6:
        CPU_SetIRQMask(irq);
        return 0;
    case 7:
        CPU_SetIRQMask(irq);
        return 1;
    default:
        break;
    }

    func_02081424((WMCallbackFunc)WCM_OnWmIndication);
    WCM_SetState(7);
    WCM_NotifyResult(WCM_NOTIFY_COMMON, NULL, 0, 0);
    CPU_SetIRQMask(irq);
    return 3;
}

s32 WCM_ConnectAsync(const void *bssDesc, const void *wepDesc, u32 wepMode) {
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
        if (((const u16 *)bssDesc)[0] != 0) {
            CPU_SetIRQMask(irq);
            return 1;
        }
        WCM_StoreKnownNetwork(bssDesc, wepDesc);
        WCM_BuildScanRequest((void *)bssDesc, (void *)wepDesc, wepMode);
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

    switch (work->unk_12e0[0]) {
    case 1:
        result = WCM_ConnectStep2();
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
    work = WCM_GetWork();
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

    switch (work->unk_12e0[1]) {
    case 1:
        result = WCM_DisconnectStep();
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
    CPU_SetIRQMask(irq);
    return result;
}

s32 WCM_StartParentAsync(void) {
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
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 11:
        CPU_SetIRQMask(irq);
        return 1;
    case 0:
        CPU_SetIRQMask(irq);
        return 2;
    case 13:
        WCM_SetState(13);
        WCM_NotifyResult(9, NULL, 2, 0);
        CPU_SetIRQMask(irq);
        return 3;
    default:
        break;
    }

    switch (work->unk_12e0[1]) {
    case 1:
        result = WCM_StartParentStep();
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
    old = work->searchOption;
    if (work == NULL) {
        CPU_SetIRQMask(irq);
        return old;
    }
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
    work->searchOption = (option & mask) | (old & ~mask);
    CPU_SetIRQMask(irq);
    return old;
}

s32 WCM_GetChannelMask(void) {
    WCMWork *work = WCM_GetWork();

    if (work->state < 3 || work->state > 10) {
        return 0;
    }
    switch (work->subState) {
    case 1:
        return WCM_ComputeChannelGap(0);
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

static WCMWork *WCM_GetWork(void) {
    return sWork.work;
}

static void WCM_SetConfig(WCMConfig *config, WCMNotify notify) {
    WCMWork *work = sWork.work;

    if (config == NULL) {
        work->dmaNo = 3;
        work->apBuffer = NULL;
        work->apBufferWords = 0;
        work->scanMode = 0;
    } else {
        work->dmaNo = config->dmano & 3;
        if ((config->nbdbuffer - (4 - (s32)config->pbdbuffer & 3)) <= 0xc) {
            work->apBuffer = NULL;
            work->apBufferWords = 0;
        } else {
            u32 align = ((u32)config->pbdbuffer + 3) & ~3u;
            u32 words = config->nbdbuffer - ((4 - ((u32)config->pbdbuffer & 3)) & 3);
            work->apBuffer = (void *)align;
            work->apBufferWords = words;
            sys_memset(work->apBuffer, 0, words);
        }
        work->scanMode = config->nbdmode;
    }
    work->notify = notify;
}

static void WCM_ResetSearchOption(void) {
    WCM_GetWork()->searchOption = 0xaaa082;
}

static u16 WCM_PickChannel(u32 channelIndex) {
    WCMWork *work = sWork.work;
    u32 i;
    u32 channel;

    for (i = channelIndex; i < 13; i++) {
        channel = i % 13;
        if (work->searchOption & (1 << (channel + 1))) {
            if (WCM_GetChannelMask() & (1 << channel)) {
                break;
            }
        }
    }
    channel = i % 13;
    return (u16)(channel + 1);
}

static void WCM_NotifyResult(s16 result, void *parameter, u32 detail, u32 line) {
    WCMWork *work = sWork.work;
    s16 notify = work->pendingNotify;

    work->pendingNotify = 0;
    // The SDK transports mixed pointer/integer payloads as 32-bit words; a host port must widen this representation.
    WCM_Notify(notify, result, (s32)parameter, (void *)detail, line);
}

static void WCM_Notify(s16 notify, s16 result, s32 parameter, void *param1, s32 param2) {
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

static s32 WCM_SetState(s32 newState) {
    WCMWork *work = sWork.work;

    if (work->state == 9 && newState != 9) {
        func_0207bd3c(&work->alarm);
    }
    if (work->state != 11) {
        work->state = newState;
    }
    if (newState == 9) {
        func_0207bd3c(&work->alarm);
        func_0207bcf4(&work->alarm, 0x22f5341, (void (*)(void *))WCM_TimeoutAlarmHandler, NULL);
    }
    return 0;
}

static void WCM_CancelTimeout(void) {
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

static s32 WCM_DefaultBand(void) {
    return hw_isDSi() ? 2 : 1;
}

static void WCM_ClearApListBuffer(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();

    if (work != NULL && work->apBuffer != NULL && (s32)work->apBufferWords > 0) {
        sys_memset(work->apBuffer, 0, work->apBufferWords);
    }
    CPU_SetIRQMask(irq);
}

s32 WCM_CountApList(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    s32 count = 0;

    if (work != NULL) {
        WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
        if (header != NULL && work->apBufferWords > 0xc) {
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
    return entry->record;
}

s32 WCM_OnBeaconReceived(void *frame, u16 param) {
    WCMWork *work = WCM_GetWork();
    void *vendorIE;
    WCMApEntry *entry = NULL;

    if (work == NULL) {
        return 0;
    }
    if (work->apListLocked != 0) {
        return 0;
    }
    if (hw_isDSi() && work->unk_12e0[3] == 2) {
        if (WCM_OnBeaconReceivedDSi(frame) != 0) {
            return 0;
        }
    }
    if (work->subState == 1 && ((u16 *)frame)[0x1e] != 0) {
        return 0;
    }
    entry = WCM_FindApEntryByBssid((const u8 *)frame + 4);
    if (!entry) {
        entry = WCM_AllocApEntry();
    }
    if (!entry && work->scanMode == 1) {
        entry = WCM_GetOldestApEntry();
    }
    if (entry != NULL) {
        WCM_FillApEntry(frame, entry);
        ((u16 *)entry->record)[1] = param;
        WCM_TouchApEntry(entry);
    }
    return 0;
}

static WCMApEntry *WCM_AllocApEntry(void) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *entries;
    s32 max;
    s32 i;
    WCMApEntry *entry = NULL;

    if (header == NULL) {
        return NULL;
    }
    if (work->apBufferWords <= 0xc) {
        return NULL;
    }
    max = (work->apBufferWords - 0xc) / sizeof(WCMApEntry);
    if (header->count >= max) {
        return NULL;
    }
    entries = (WCMApEntry *)((u8 *)header + 0xc);
    for (i = 0; i < max; i++) {
        if (!entries[i].inUse) {
            break;
        }
    }
    if (i >= max) {
        return NULL;
    }
    entry = &entries[i];
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

    if (header != NULL && work->apBufferWords > 0xc) {
        return header->head;
    }
    return NULL;
}

static WCMApEntry *WCM_FindApEntryById(s32 id) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *entry = NULL;

    if (header != NULL && work->apBufferWords > 0xc) {
        for (entry = header->head; entry != NULL; entry = entry->next) {
            if (entry->id == id) {
                break;
            }
        }
    }
    return entry;
}

static WCMApEntry *WCM_FindApEntryByBssid(const u8 *bssid) {
    WCMWork *work = WCM_GetWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApEntry *entry = NULL;

    if (bssid == NULL) {
        return NULL;
    }
    if (header != NULL && work->apBufferWords > 0xc) {
        for (entry = header->head; entry != NULL; entry = entry->next) {
            u8 *record = entry->record;
            if (WCM_CompareBssID(record + 4, bssid)) {
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
    if (header == NULL || work->apBufferWords <= 0xc) {
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

// Clears an entry's record, copies the raw beacon or probe response header in, and appends its parsed SSID and
// security information elements after it
static void WCM_FillApEntry(const void *frame, WCMApEntry *entry) {
    WCMWork *work = WCM_GetWork();
    u8 *record = entry->record;
    const u8 *ie;
    u16 ieCount;
    u16 ieLen;
    s32 total = 0;
    u16 n = 0;

    sys_memset32_fast(0, record, 0xc0);
    sys_memcpy32_fast(frame, record, 0x40);
    ((u16 *)record)[0x1f] = 0;
    record[0] = (work->subState == 1) ? 0x20 : 0x40;

    ie = (const u8 *)frame + 0x40;
    ieCount = ((const u16 *)frame)[0x1f];
    while (n < ieCount) {
        u8 tag = ie[0];
        u8 len = ie[1];
        if (tag == 0x32 || tag == 0x2a || tag == 0x2f || tag == 7 || tag == 0x30) {
            ieLen = len + 2;
            sys_memcpy_fast(ie, record + 0x40 + total, ieLen);
            total += ieLen;
            ((u16 *)(record + 0x3e))[0]++;
        } else if (tag == 0xdd) {
            if (sys_memcmp(ie + 2, data_ov011_02185008, 4) == 0 || sys_memcmp(ie + 2, data_ov011_0218500c, 5) == 0) {
                ieLen = len + 2;
                sys_memcpy_fast(ie, record + 0x40 + total, ieLen);
                total += ieLen;
                ((u16 *)(record + 0x3e))[0]++;
            }
        }
        ie += len + 2;
        n++;
    }
    ((u16 *)record)[0] += (u8)total;
}

static void *WCM_FindVendorIE(void *frame) {
    WCMWork *work = WCM_GetWork();
    u8 buf[4];

    sys_memcpy_fast(data_ov011_02185004, buf, 4);
    if (frame == NULL) {
        return NULL;
    }
    if (hw_isDSi() && work->subState == 2) {
        if (((u16 *)frame)[0x1f] != 0) {
            void *ie = WCM_GetInformationElement(frame, 0xdd, buf);
            if (ie != NULL && ((u16 *)ie)[5] == 1) {
                return (u8 *)ie + 5;
            }
        }
        return NULL;
    }
    if (work->subState == 1) {
        u16 len = ((u16 *)frame)[0x1e];
        if (len != 0 && len >= 0x10 && len <= 0x80) {
            if (len == ((u8 *)frame)[0x4a] + 0x10 && ((u16 *)frame)[0x20] == 1 && ((u16 *)frame)[0] == (len + 1 + 0x40) / 2) {
                return (u8 *)frame + 0x40;
            }
        }
    }
    return NULL;
}

static void *WCM_GetInformationElement(void *frame, u32 tag, void *out) {
    return func_02703250(frame, tag, out);
}

static void WCM_InitOnce(void) {
    if (!sWcm.macCached) {
        sWcm.macCached = TRUE;
        sWcm.unk_24 = 0;
        sys_memset(sWcm.ownMac, 0, 6);
        sWcm.unk_04 = 0;
        OS_InitMutex(&sWcm.mutex);
        sWcm.unk_08 = 0;
    }
}

static WCMGlobal *WCM_GetGlobal(void) {
    return &sWcm;
}

void WCM_DispatchRecvCallback(u8 *src, u8 *dst, u8 *data, u32 len) {
    if (sWcm.recvCallback != NULL) {
        sWcm.recvCallback(src, dst, data, len);
    }
}

const u8 *WCM_GetApMacAddress(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    const u8 *mac = NULL;

    if (work != NULL && work->state == 9 && work->wmIndInstalled == 0) {
        mac = work->unk_12e0 + 4;
    }
    CPU_SetIRQMask(irq);
    return mac;
}

s32 WCM_GetConnectedApInfo(u16 *channel) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    const u8 *bssid = NULL;
    u16 ch = 0;

    if (work != NULL && work->state == 9 && work->wmIndInstalled == 0) {
        ch = *(u16 *)(work->unk_12e0 + 0xc);
        bssid = work->unk_12e0 + 0xe;
    }
    CPU_SetIRQMask(irq);
    if (channel != NULL) {
        *channel = ch;
    }
    return (s32)bssid;
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
    if (work->state == 9 || work->subState == 1) {
        OS_UnlockMutex(&sWcm.mutex);
        CPU_SetIRQMask(irq);
        return -4;
    }
    sentLen = WCM_BuildFrameHeader(work->subState + 1, (void *)header, headerLen, dataLen);
    switch (work->subState + 1) {
    case 1:
        WCM_SendFrame((const u8 *)header, work, sentLen, FALSE);
        break;
    case 2:
        if (hw_isDSi()) {
            func_02769dc4(header, header, sentLen, data, dataLen);
        }
        break;
    default:
        break;
    }
    OS_UnlockMutex(&sWcm.mutex);
    CPU_SetIRQMask(irq);
    return dst - header + headerLen;
}

void WCM_OnDmaDone(void *info) {
    if (*(void **)((u8 *)info + 8) == (void *)CPU_IRQInvoke) {
        s32 *remaining = (s32 *)((u8 *)info + 0xc);
        (*remaining)--;
        if (*remaining == 0) {
            *(void **)((u8 *)info + 8) = NULL;
            OS_WakeupThread((OSThreadQueue *)info);
        }
    }
}

static void WCM_SetMacAddress(const u8 *mac) {
    sys_memcpy_fast(mac, sWcm.ownMac, 6);
}

void WCM_GetMacAddress(u8 *mac) {
    sys_memcpy_fast(sWcm.ownMac, mac, 6);
}

static s32 WCM_BuildFrameHeader(s32 kind, void *header, s32 dataLen, s32 ssidLen) {
    u8 *h = (u8 *)header;
    u16 total = ssidLen;

    if (kind == 1 || kind == 2) {
        h = (u8 *)header + 0xe;
        total = ((ssidLen & 0xff00) >> 8) | ((ssidLen << 8) & 0xff00);
    }
    sys_memcpy_fast(header, h, dataLen);
    if (ssidLen != 0 && dataLen != 0) {
        sys_memcpy_fast((u8 *)header + dataLen, h + dataLen, 0);
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

static s32 WCM_UpdateLinkQuality(void) {
    u32 irq = CPU_IRQDisable();
    WCMWork *work = WCM_GetWork();
    s32 quality = 0;

    if (work != NULL && work->state == 9) {
        WCM_AddRssiSample(0);
        quality = WCM_RssiToQuality(WCM_GetAverageRssi());
    }
    CPU_SetIRQMask(irq);
    return quality;
}

static s32 WCM_RssiToQuality(s32 rssi) {
    WCMWork *work = WCM_GetWork();

    if (work->subState == 1 || work->subState == 2) {
        if (work->subState == 1) {
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
    return 0;
}

// Adds a signal-level sample to the running RSSI average's circular buffer
static void WCM_AddRssiSample(s32 rssi) {
    u8 count = sWcm.rssiSampleCount;

    sWcm.rssiSamples[count % 0x10] = (u8)rssi;
    sWcm.rssiSampleCount = (u8)(count + 1);
}

static s32 WCM_GetAverageRssi(void) {
    u32 i;
    s32 sum = 0;
    u8 count = sWcm.rssiSampleCount;

    if (count > 0x10) {
        for (i = 0; i < 0x10; i++) {
            sum += sWcm.rssiSamples[i];
        }
        return (sum + (sum >> 0x1c)) >> 4;
    }
    if (count == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        sum += sWcm.rssiSamples[i];
    }
    return sum / count;
}

// The startup FSM's WM_Initialize-adjacent first step: brings up the hardware and reads its MAC address
static s32 WCM_StartupStep1(void) {
    WCMWork *work = WCM_GetWork();
    s32 result;

    result = func_02080758(work, (u16)work->dmaNo);
    switch (result) {
    case 0:
        if (func_02080e7c((WMCallbackFunc)WCM_OnWmIndication) != 0) {
            WCM_SetState(0xb);
            return 7;
        }
        {
            u8 mac[6];
            func_0207c33c(mac);
            WCM_SetMacAddress(mac);
        }
        switch (func_020812fc((WMCallbackFunc)WCM_ConnectStep3)) {
        case 2:
            WCM_SetState(2);
            work->pendingNotify = 1;
            return 3;
        case 8:
            return 1;
        default:
            WCM_SetState(0xb);
            return 7;
        }
    case 4:
        return 5;
    default:
        WCM_SetState(0xb);
        return 7;
    }
}

static s32 WCM_StartupStep2(void) {
    WCMWork *work = WCM_GetWork();

    switch (func_02081398((WMCallbackFunc)WCM_ConnectStep3)) {
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

static s32 WCM_ConnectStep1(const u8 *bssid, const u8 *essid, u32 option) {
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

static s32 WCM_ConnectStep2(void) {
    WCMWork *work = WCM_GetWork();

    switch (func_02081348((WMCallbackFunc)WCM_ConnectStep4)) {
    case 2:
        WCM_SetState(0xd);
        work->pendingNotify = 9;
        return 3;
    case 8:
        return 4;
    default:
        WCM_SetState(0xb);
        return 7;
    }
}

static s32 WCM_DisconnectStep(void) {
    WCMWork *work = WCM_GetWork();

    switch (func_02081b94((WMCallbackFunc)WCM_ParentStep3)) {
    case 2:
        WCM_SetState(0xa);
        work->pendingNotify = 6;
        return 3;
    case 8:
        return 4;
    default:
        WCM_SetState(0xb);
        return 7;
    }
}

static s32 WCM_StartParentStep(void) {
    WCMWork *work = WCM_GetWork();
    WMBssDesc *info;

    cp15_invalidateDC(func_02080dbc(), 2);
    info = func_02080dbc();
    switch (((u16 *)info)[0]) {
    case 0:
        if (func_02080908() != 0) {
            return 4;
        }
        WCM_SetState(1);
        work->pendingNotify = 0;
        return 1;
    case 1:
        return func_02081348((WMCallbackFunc)WCM_ConnectStep4);
    case 2:
        return func_02081398((WMCallbackFunc)WCM_ConnectStep3);
    default:
        work->wmIndInstalled = 1;
        return func_02081424((WMCallbackFunc)WCM_ConnectStep3);
    }
}

static s32 WCM_SendFrame(const u8 *dst, const void *header, s32 headerLen, BOOL viaDSi) {
    WCMWork *work = WCM_GetWork();
    WCMGlobal *global = WCM_GetGlobal();
    s32 result;

    if (!viaDSi) {
        global->unk_04 = (u32)WCM_OnAsyncDone;
    } else {
        global->unk_04 = (u32)WCM_OnAsyncDoneWithStatus;
    }
    result = func_02081b18(dst, headerLen, header, headerLen);
    switch (result) {
    case 2:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        return -4;
    default:
        break;
    }
    if (result == 2) {
        if (!viaDSi) {
            scheduler_yield((OSThreadQueue *)((u8 *)global + 4));
            if (global->unk_24 == 1) {
                return -4;
            }
        }
        return headerLen;
    }
    return headerLen;
}

static u16 WCM_ComputeChannelGap(u16 channelMask) {
    u32 i;
    u32 bit = 0x8000;
    u32 lowBit = 1;
    u32 mask = channelMask;

    if (mask == 0) {
        return 0;
    }
    while (!(bit & mask)) {
        bit >>= 1;
    }
    while (!(lowBit & mask)) {
        lowBit <<= 1;
        lowBit |= 1;
    }
    return (u16)~((bit << 1) | (lowBit >> 1));
}

static void WCM_CancelConnection(void) {
    WCMWork *work = WCM_GetWork();

    if (work->wmIndInstalled == 0) {
        work->wmIndInstalled = 1;
        if (func_02081424((WMCallbackFunc)WCM_ConnectStep3) != 2) {
            WCM_SetState(0xb);
            WCM_NotifyResult(7, NULL, 0, 0);
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
        WCM_CancelConnection();
        break;
    case 2:
        WCM_SetState(0xc);
        break;
    default:
        break;
    }
}

static void WCM_ConnectStep3(void *ind) {
    WCMWork *work = WCM_GetWork();
    s32 result = (s32)ind;

    switch (((u16 *)ind)[1]) {
    case 0:
        switch (((u16 *)ind)[0]) {
        case 3:
            func_02081370(ind);
            break;
        case 0x1a:
            func_02081348(0);
            break;
        case 0x1d:
            func_0208252c(ind, 0);
            break;
        case 0x27: {
            u32 flags = *(u32 *)((u8 *)work + 0x12d4);
            BOOL powerSave = (flags & 0xc0000) == 0xc0000;
            BOOL authMode = (flags & 0x30000) != 0x30000;
            result = func_020816dc(ind, (const WMBssDesc *)work->unk_12e0, NULL, powerSave, authMode);
            break;
        }
        default:
            if (((u16 *)ind)[0] <= 0x19 && ((u16 *)ind)[0] >= 6) {
                switch (((u16 *)ind)[0]) {
                case 6:
                    result = func_02081348(ind);
                    break;
                default:
                    break;
                }
            }
            break;
        }
        break;
    case 1:
    case 3:
        WCM_SetState(0xc);
        WCM_NotifyResult(7, work->unk_12e0 + 6, result, 0);
        return;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(1, work->unk_12e0 + 6, result, 0);
        return;
    }
    switch (result) {
    case 2:
        return;
    case 3:
        WCM_SetState(1);
        WCM_NotifyResult(0, NULL, 0, 0);
        return;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0 + 6, result, 0);
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
            WCM_CancelConnection();
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
        WCM_CancelConnection();
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, 0, 0x3af);
        break;
    }
}

static void WCM_ConnectStep5(void *ind) {
    switch (((u16 *)ind)[1]) {
    case 0:
        WCM_SetState(3);
        func_020811a4();
        break;
    case 1:
        WCM_CancelConnection();
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, 0, 0);
        break;
    }
}

static void WCM_ConnectStep6(void *ind) {
    WCMWork *work = WCM_GetWork();
    int result;

    switch (((u16 *)ind)[1]) {
    case 0:
        switch (((u16 *)ind)[4]) {
        case 8:
        case 9:
            switch (work->state) {
            case 10:
                work->unk_22e8 = 0;
            case 8:
                WCM_SetState(0xc);
                break;
            case 9:
                work->unk_22e8 = 0;
                work->pendingNotify = 6;
            case 12:
                WCM_CancelConnection();
                break;
            default:
                break;
            }
            break;
        case 7:
            if (work->state == 0xc) {
                WCM_SetState(8);
                WCM_CancelConnection();
                break;
            }
            if (((u16 *)ind)[5] < 1 || ((u16 *)ind)[5] > 0x7d7) {
                WCM_CancelConnection();
                break;
            }
            work->unk_22e8 = ((u16 *)ind)[5];
            result = func_02081ab0((WMCallbackFunc)WCM_ConnectStep8, work->scanBuffer, 0x620);
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
            WCM_NotifyResult(7, work->unk_12e0, ((u16 *)ind)[4], 0x447);
            break;
        }
        break;
    case 1:
        work->unk_22ea = ((u16 *)ind)[7];
    case 6:
    case 11:
    case 12:
        WCM_SetState(8);
        WCM_CancelConnection();
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0x45e);
        break;
    }
}

static void WCM_ConnectStep7(void *ind) {
    WCMWork *work = WCM_GetWork();

    switch (((u16 *)ind)[1]) {
    case 0:
        if (((u8 *)work)[0x12de] == 0xc) {
            WCM_SetState(0xa);
            WCM_CancelConnection();
        } else {
            work->unk_22e8 = 0;
            WCM_SetState(3);
            WCM_NotifyResult(0, work->unk_12e0, 0, 0x481);
        }
        break;
    case 1:
    case 3:
        WCM_SetState(0xa);
        WCM_CancelConnection();
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0x493);
        break;
    }
}

static void WCM_ConnectStep8(void *ind) {
    WCMWork *work = WCM_GetWork();

    switch (((u16 *)ind)[1]) {
    case 0:
        switch (((u16 *)ind)[2]) {
        case 0xe:
            if (((u8 *)work)[0x12de] == 0xc) {
                WCM_SetState(8);
                WCM_CancelConnection();
            } else {
                WCM_SetState(9);
                WCM_NotifyResult(0, work->unk_12e0, 0, 0);
            }
            break;
        case 0xf: {
            WMBssDesc *bssDesc = *(WMBssDesc **)((u8 *)ind + 8);
            u8 rssi = (u8)(((u16 *)bssDesc)[7] >> 8);
            WCM_AddRssiSample(rssi);
            cp15_invalidateDC(bssDesc, 0x620);
            WCM_DispatchRecvCallback((u8 *)bssDesc + 0x1e, (u8 *)bssDesc + 0x18, (u8 *)bssDesc + 0x2c, ((u16 *)bssDesc)[3]);
            break;
        }
        default:
            WCM_SetState(0xb);
            WCM_NotifyResult(7, work->unk_12e0, ((u16 *)ind)[2], 0);
            break;
        }
        break;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0);
        break;
    }
}

static void WCM_ParentStep2(void *ind) {
    WCMWork *work = WCM_GetWork();

    if (((u8 *)work)[0x12de] == 0xc) {
        WCM_SetState(0xa);
        WCM_CancelConnection();
        return;
    }
    switch (func_02081774(0, 0)) {
    case 2:
    case 0x8:
        return;
    case 3:
        WCM_SetState(0xa);
        WCM_CancelConnection();
        return;
    default:
        WCM_SetState(0xb);
        WCM_NotifyResult(7, work->unk_12e0, 0, 0);
        return;
    }
}

static void WCM_ParentStep3(void *ind) {
    WCMWork *work = WCM_GetWork();
    u16 reason;
    BOOL authMode;
    BOOL powerSave;
    int result;

    if (((u16 *)ind)[1] != 0) {
        WCM_SetState(0xb);
        WCM_NotifyResult(7, NULL, 0, 0x5a7);
        return;
    }
    work->wmIndInstalled = 0;
    work->unk_22e8 = 0;
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
        reason = work->unk_22ea;
        work->unk_22ea = 0;
        if (reason == 0x12 && (*(u16 *)((u8 *)work + 0x1310) & 0x24) != 0x24) {
            *(u16 *)((u8 *)work + 0x1310) |= 0x24;
            authMode = ((work->searchOption & 0xc0000) == 0xc0000);
            powerSave = ((work->searchOption & 0x30000) != 0x30000);
            result = func_020816dc((WMCallbackFunc)WCM_ConnectStep6, (const WMBssDesc *)work->unk_12e0, NULL, powerSave,
                                   (u16)authMode);
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
        result = func_02081398((WMCallbackFunc)WCM_ConnectStep3);
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
    work->scanRequest.scanBuf = (WMBssDesc *)work->scanBuffer;
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

static void WCM_OnAsyncDone(void *ind) {
    WCMGlobal *global = WCM_GetGlobal();

    if (((u16 *)ind)[0] != 0x12) {
        return;
    }
    global->unk_24 = ((u16 *)ind)[1];
    if (global->unk_24 == 0) {
        WCM_CancelTimeout();
    }
    OS_WakeupThread((OSThreadQueue *)((u8 *)global + 4));
}

static void WCM_OnAsyncDoneWithStatus(void *ind) {
    WCMGlobal *global = WCM_GetGlobal();
    s32 status;

    if (((u16 *)ind)[0] != 0x12) {
        return;
    }
    global->unk_24 = ((u16 *)ind)[1];
    if (global->unk_24 == 0) {
        WCM_CancelTimeout();
    }
    status = (global->unk_24 == 0) ? 0 : -4;
    {
        void (*callback)(s32) = (void (*)(s32))global->unk_04;

        global->unk_04 = 0;
        WCM_OnDmaDone((u8 *)global + 0xc);
        if (callback != NULL) {
            callback(status);
        }
    }
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
