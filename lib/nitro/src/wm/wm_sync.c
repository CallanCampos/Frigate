// The wireless manager's functions that give their answer at once (libwm, TwlSDK's wm/ARM9/src/wm_sync.c, by its place
// in the library: the file name is a guess): setting the callbacks of indications and ports, reading the status and
// the data of the multi-point mode, the sizes of its buffers, the channels and the link level, the dispersion of the
// beacon and the scan, the elements of a beacon and the next TGID. The public functions have the SDK's names, from what
// they do; the static ones' names are ours
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nitro/wm.h"

int WM_SetIndCallback(WMCallbackFunc callback) {
    u32 irq = OS_DisableInterrupts();
    int result = WMi_CheckInitialized();

    if (result != WM_ERRCODE_SUCCESS) {
        OS_RestoreInterrupts(irq);
        return result;
    }
    WMi_GetSystemWork()->indCallback = callback;
    OS_RestoreInterrupts(irq);
    return WM_ERRCODE_SUCCESS;
}

int WM_SetPortCallback(u16 port, WMCallbackFunc callback, void *arg) {
    WMPortRecvCallback cb;
    u32 irq;
    int result;
    WMArm9Buf *w9b;

    if (callback != NULL) {
        MI_CpuFill8(&cb, 0, sizeof(cb));
        cb.apiid = WM_APIID_PORT_RECV;
        cb.errcode = WM_ERRCODE_SUCCESS;
        cb.state = WM_STATECODE_PORT_INIT;
        cb.port = port;
        cb.recvBuf = NULL;
        cb.data = NULL;
        cb.length = 0;
        cb.seqNo = 0xffff;
        cb.arg = arg;
        cb.aid = 0;
        OS_GetMacAddress(cb.macAddress);
    }

    irq = OS_DisableInterrupts();
    result = WMi_CheckInitialized();
    if (result != WM_ERRCODE_SUCCESS) {
        OS_RestoreInterrupts(irq);
        return result;
    }

    w9b = WMi_GetSystemWork();
    w9b->portCallbackTable[port] = callback;
    w9b->portCallbackArgTable[port] = arg;
    if (callback != NULL) {
        cb.connectedAidBitmap = WMi_GetConnectedAidBitmap();
        cb.myAid = WMi_GetMyAid();
        callback(&cb);
    }
    OS_RestoreInterrupts(irq);
    return WM_ERRCODE_SUCCESS;
}

int WM_ReadStatus(WMStatus *statusBuf) {
    WMArm9Buf *w9b = WMi_GetSystemWork();
    int result = WMi_CheckInitialized();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (statusBuf == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    DC_InvalidateRange(w9b->status, sizeof(WMStatus));
    MIi_CpuCopyFast(w9b->status, statusBuf, sizeof(WMStatus));
    return WM_ERRCODE_SUCCESS;
}

int WM_GetMPSendBufferSize(void) {
    WMArm9Buf *w9b = WMi_GetSystemWork();

    if (WMi_CheckStateEx(2, WM_STATE_PARENT, WM_STATE_CHILD) != WM_ERRCODE_SUCCESS) {
        return 0;
    }
    DC_InvalidateRange(&w9b->status->mpFlag, 4);
    if (w9b->status->mpFlag == TRUE) {
        return 0;
    }
    DC_InvalidateRange(&w9b->status->mpSendSize, 4);
    return (w9b->status->mpSendSize + 0x1f) & ~0x1f;
}

int WM_GetMPReceiveBufferSize(void) {
    u32 size;
    BOOL isParent;
    WMArm9Buf *w9b = WMi_GetSystemWork();

    if (WMi_CheckStateEx(2, WM_STATE_PARENT, WM_STATE_CHILD) != WM_ERRCODE_SUCCESS) {
        return 0;
    }
    DC_InvalidateRange(&w9b->status->mpFlag, 4);
    if (w9b->status->mpFlag == TRUE) {
        return 0;
    }
    DC_InvalidateRange(&w9b->status->aid, 2);
    isParent = TRUE;
    if (w9b->status->aid != 0) {
        isParent = FALSE;
    }
    DC_InvalidateRange(&w9b->status->mpRecvSize, 2);
    size = w9b->status->mpRecvSize;
    if (isParent == TRUE) {
        DC_InvalidateRange(&w9b->status->maxEntry, 2);
        size += 0xc;
        return ((size * w9b->status->maxEntry + 0x29) & ~0x1f) * 2;
    }
    return ((size + 0x51) & ~0x1f) * 2;
}

// The data that the child with the AID sent in a round of the multi-point mode, or NULL if it sent none
const WMMpRecvData *WM_ReadMPData(const WMMpRecvBuf *recvBuf, u16 aid) {
    WMArm9Buf *w9b = WMi_GetSystemWork();
    // BUG: when the buffer has 15 entries and none is the child's, the loop writes datap[15], past the array and over a
    // saved register
#ifdef BUGFIX
    const WMMpRecvData *datap[16];
#else
    const WMMpRecvData *datap[15];
#endif
    int i;

    if (WMi_CheckInitialized() != WM_ERRCODE_SUCCESS) {
        return NULL;
    }
    if (aid < 1 || aid > 15) {
        return NULL;
    }
    DC_InvalidateRange(&w9b->status->childBitmap, 2);
    if (!(w9b->status->childBitmap & (1 << aid))) {
        return NULL;
    }
    if (recvBuf->count == 0 || recvBuf->count > 15) {
        return NULL;
    }

    datap[0] = (const WMMpRecvData *)recvBuf->entries;
    i = 0;
    do {
        if (aid == datap[i]->aid) {
            return datap[i];
        }
        i++;
        datap[i] = (const WMMpRecvData *)((const u8 *)datap[i - 1] + recvBuf->entrySize);
    } while (i < recvBuf->count);
    return NULL;
}

// The channels that the console may use, one bit each
u16 WM_GetAllowedChannel(void) {
    if (OS_IsRunOnTwl() && !WMi_IsWirelessAvailable()) {
        return 0;
    }
    if (WMi_CheckInitialized() != WM_ERRCODE_SUCCESS) {
        return 0x8000;
    }
    return *(u16 *)HW_WM_ALLOWED_CHANNEL_BUF;
}

WMLinkLevel WM_GetLinkLevel(void) {
    WMArm9Buf *w9b = WMi_GetSystemWork();

    if (WMi_CheckInitialized() != WM_ERRCODE_SUCCESS) {
        return WM_LINK_LEVEL_0;
    }
    DC_InvalidateRange(w9b->status, 2);
    switch (w9b->status->state) {
    case WM_STATE_MP_PARENT:
        DC_InvalidateRange(&w9b->status->childBitmap, 2);
        if (w9b->status->childBitmap == 0) {
            return WM_LINK_LEVEL_0;
        }
        // fall through
    case WM_STATE_MP_CHILD:
    case WM_STATE_DCF_CHILD:
        DC_InvalidateRange(&w9b->status->linkLevel, 2);
        return w9b->status->linkLevel;
    }
    return WM_LINK_LEVEL_0;
}

// WM_GetDispersionBeaconPeriod: the period of a parent's beacon in milliseconds, 200 to 219, from the console's
// address and the time
u16 WM_GetDispersionBeaconPeriod(void) {
    u8 mac[6];
    u16 seed;
    int i;

    OS_GetMacAddress(mac);
    for (i = 0, seed = 0; i < 6; i++) {
        seed += mac[i];
    }
    seed += (u16)OS_GetVBlankCount();
    return 200 + (u16)(seed * 7) % 20;
}

// How long to stay on a channel while scanning, 30 to 39 milliseconds
u16 WM_GetDispersionScanPeriod(void) {
    u8 mac[6];
    u16 seed;
    int i;

    OS_GetMacAddress(mac);
    for (i = 0, seed = 0; i < 6; i++) {
        seed += mac[i];
    }
    seed += (u16)OS_GetVBlankCount();
    return 30 + (u16)(seed * 13) % 10;
}

// The elements that follow the beacon's header, if there is no game information
WMOtherElements WM_GetOtherElements(const WMBssDesc *bssDesc) {
    WMOtherElements elems;
    const u8 *p;
    int i;
    u16 size;
    u16 total;

    if (bssDesc->gameInfoLength != 0) {
        elems.count = 0;
        return elems;
    }

    elems.count = bssDesc->otherElementCount;
    if (elems.count == 0) {
        return elems;
    }
    if (elems.count > 16) {
        elems.count = 16;
    }

    p = (const u8 *)&bssDesc->gameInfo;
    size = bssDesc->length * 2 - 0x40;
    total = 0;
    for (i = 0; i < elems.count; i++) {
        u16 elementSize;

        elems.element[i].id = p[0];
        elems.element[i].length = p[1];
        elems.element[i].data = (u8 *)p + 2;
        elementSize = elems.element[i].length + 2;
        total += elementSize;
        if (total > size) {
            elems.count = 0;
            return elems;
        }
        p += elementSize;
    }
    return elems;
}

// The TGID for a parent's next beacon, counting up from the minute and second at the first call
u16 WM_GetNextTgid(void) {
    static u32 tgid_bak = 0x10000;
    RTCTime time;

    if (tgid_bak == 0x10000) {
        RTC_Init();
        if (RTC_GetTime(&time) == 0) {
            tgid_bak = (u16)(time.second + (time.minute << 8));
        }
    }
    tgid_bak = (u16)(tgid_bak + 1);
    return tgid_bak;
}
