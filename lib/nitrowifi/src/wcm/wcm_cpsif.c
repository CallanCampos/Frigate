#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/wcm.h"
#include "nitrowifi/wcm_dsi.h"
#include "wcm_internal.h"

// NitroWiFi's WCM (overlay 11): its interface to CPS, the TCP/IP stack, which sends and receives frames over the
// connection (DCF). TwlSDK's libwcm names the object cpsif.o; Black 1's decompilation (aletheia) has the same
// functions, in this order, in its wcm_cpsif.c, and the names are the SDK's. The Thumb/ARM pair after
// WCM_SendDCFDataEx is the linker's long-branch veneer to the DSi backend in the LTD autoload.

// The 802.3 header of a mode 2 frame. Only the length, big-endian, is filled in here
typedef struct WCMEthernetHeader {
    u8 destination[6];
    u8 source[6];
    u16 length;
} WCMEthernetHeader;

static WCMCpsifWork sWcmGlobal;

static s32 WCMi_MergeToBuffer(s32 mode, void *dest, const void *header, u32 headerLen, const void *data, u32 dataLen);

void WCMi_InitCpsif(void) {
    if (!sWcmGlobal.initialized) {
        sWcmGlobal.initialized = TRUE;
        sWcmGlobal.sendStatus = 0;
        MI_CpuFill8(sWcmGlobal.ownMac, 0, 6);
        OS_InitThreadQueue(&sWcmGlobal.sendQueue);
        OS_InitMutex(&sWcmGlobal.mutex);
    }
}

void WCMi_CpsifRecvCallback(const u8 *src, const u8 *dst, const u8 *data, s32 len) {
    if (sWcmGlobal.recvCallback != NULL) {
        sWcmGlobal.recvCallback(src, dst, data, len);
    }
}

WCMCpsifWork *WCMi_GetCpsifWork(void) {
    return &sWcmGlobal;
}

u8 *WCM_GetApMacAddress(void) {
    u8 *mac = NULL;
    WCMWork *work = WCMi_GetSystemWork();
    u32 irq = OS_DisableInterrupts();

    if (work != NULL && work->state == 9 && work->resetPending == 0) {
        mac = work->connectBss.bssid;
    }
    OS_RestoreInterrupts(irq);
    return mac;
}

// The SSID is borrowed from the connected BSS descriptor and is not necessarily NUL-terminated.
u8 *WCM_GetApEssid(u16 *ssidLength) {
    u8 *ssid = NULL;
    u16 length = 0;
    WCMWork *work = WCMi_GetSystemWork();
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
    work = WCMi_GetSystemWork();
    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return -1;
    }
    OS_LockMutex(&sWcmGlobal.mutex);
    work = WCMi_GetSystemWork();
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
    sentLen = WCMi_MergeToBuffer(work->wirelessMode, work, header, headerLen, data, dataLen);
    switch (work->wirelessMode) {
    case 1:
        WCMi_WmifSetDCFData(dst, work, (u16)sentLen, NULL);
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

void WCMi_CpsifUnlockMutexInIRQ(OSMutex *mutex) {
    // The IRQ-handler address is an owner marker, not an OSThread object; this path releases no ownership-list link.
    if (mutex->thread == (OSThread *)OS_IrqHandler) {
        mutex->count--;
        if (mutex->count == 0) {
            mutex->thread = NULL;
            OS_WakeupThread(&mutex->queue);
        }
    }
}

void WCMi_CpsifSetMacAddress(const u8 *mac) {
    MI_CpuCopy(mac, sWcmGlobal.ownMac, 6);
}

void WCM_GetMacAddress(u8 *mac) {
    MI_CpuCopy(sWcmGlobal.ownMac, mac, 6);
}

static s32 WCMi_MergeToBuffer(s32 mode, void *dest, const void *header, u32 headerLen, const void *data, u32 dataLen) {
    void *payload;
    u32 total;

    total = headerLen + dataLen;
    switch (mode) {
    case 1:
    default:
        payload = dest;
        break;
    case 2:
        // The frame starts with an 802.3 header
        payload = (WCMEthernetHeader *)dest + 1;
        ((WCMEthernetHeader *)dest)->length = (u16)((((u16)total & 0xff00U) >> 8) | (((u16)total & 0x00ff) << 8));
        total += sizeof(WCMEthernetHeader);
        break;
    }
    MI_CpuCopy(header, payload, headerLen);
    if (data != NULL && dataLen != 0) {
        MI_CpuCopy(data, (u8 *)payload + headerLen, dataLen);
    }
    return total;
}
