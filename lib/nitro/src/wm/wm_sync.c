// The wireless manager's functions that give their answer at once (libwm, TwlSDK's wm/ARM9/src/wm_sync.c, by its place
// in the library: the file name is a guess): setting the callbacks of indications and ports, reading the status and
// the data of the multi-point mode, the sizes of its buffers, the channels and the link level, the dispersion of the
// beacon and the scan, the elements of a beacon and the next TGID. The names are descriptive
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nitro/wm.h"

// WM_SetIndCallback
int func_02080e7c(WMCallbackFunc callback) {
    u32 irq = CPU_IRQDisable();
    int result = func_02080a70();

    if (result != WM_ERRCODE_SUCCESS) {
        CPU_SetIRQMask(irq);
        return result;
    }
    func_02080a64()->indCallback = callback;
    CPU_SetIRQMask(irq);
    return WM_ERRCODE_SUCCESS;
}

// WM_SetPortCallback
int func_02080eac(u16 port, WMCallbackFunc callback, void *arg) {
    WMPortRecvCallback cb;
    u32 irq;
    int result;
    WMArm9Buf *w9b;

    if (callback != NULL) {
        sys_memset(&cb, 0, sizeof(cb));
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
        func_0207c33c(cb.macAddress);
    }

    irq = CPU_IRQDisable();
    result = func_02080a70();
    if (result != WM_ERRCODE_SUCCESS) {
        CPU_SetIRQMask(irq);
        return result;
    }

    w9b = func_02080a64();
    w9b->portCallbackTable[port] = callback;
    w9b->portCallbackArgTable[port] = arg;
    if (callback != NULL) {
        cb.connectedAidBitmap = func_02080dfc();
        cb.myAid = func_02080dd8();
        callback(&cb);
    }
    CPU_SetIRQMask(irq);
    return WM_ERRCODE_SUCCESS;
}

// WM_ReadStatus
int func_02080f40(WMStatus *statusBuf) {
    WMArm9Buf *w9b = func_02080a64();
    int result = func_02080a70();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (statusBuf == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    cp15_invalidateDC(w9b->status, sizeof(WMStatus));
    sys_memcpy32_fast(w9b->status, statusBuf, sizeof(WMStatus));
    return WM_ERRCODE_SUCCESS;
}

// WM_GetMPSendBufferSize
int func_02080f74(void) {
    WMArm9Buf *w9b = func_02080a64();

    if (func_02080ab0(2, WM_STATE_PARENT, WM_STATE_CHILD) != WM_ERRCODE_SUCCESS) {
        return 0;
    }
    cp15_invalidateDC(&w9b->status->mpFlag, 4);
    if (w9b->status->mpFlag == TRUE) {
        return 0;
    }
    cp15_invalidateDC(&w9b->status->mpSendSize, 4);
    return (w9b->status->mpSendSize + 0x1f) & ~0x1f;
}

// WM_GetMPReceiveBufferSize
int func_02080fbc(void) {
    u32 size;
    BOOL isParent;
    WMArm9Buf *w9b = func_02080a64();

    if (func_02080ab0(2, WM_STATE_PARENT, WM_STATE_CHILD) != WM_ERRCODE_SUCCESS) {
        return 0;
    }
    cp15_invalidateDC(&w9b->status->mpFlag, 4);
    if (w9b->status->mpFlag == TRUE) {
        return 0;
    }
    cp15_invalidateDC(&w9b->status->aid, 2);
    isParent = TRUE;
    if (w9b->status->aid != 0) {
        isParent = FALSE;
    }
    cp15_invalidateDC(&w9b->status->mpRecvSize, 2);
    size = w9b->status->mpRecvSize;
    if (isParent == TRUE) {
        cp15_invalidateDC(&w9b->status->maxEntry, 2);
        size += 0xc;
        return ((size * w9b->status->maxEntry + 0x29) & ~0x1f) * 2;
    }
    return ((size + 0x51) & ~0x1f) * 2;
}

// WM_ReadMPData: the data that the child with the AID sent in a round of the multi-point mode, or NULL if it sent none
const WMMpRecvData *func_02081040(const WMMpRecvBuf *recvBuf, u16 aid) {
    WMArm9Buf *w9b = func_02080a64();
    // BUG: when the buffer has 15 entries and none is the child's, the loop writes datap[15], past the array and over a
    // saved register
#ifdef BUGFIX
    const WMMpRecvData *datap[16];
#else
    const WMMpRecvData *datap[15];
#endif
    int i;

    if (func_02080a70() != WM_ERRCODE_SUCCESS) {
        return NULL;
    }
    if (aid < 1 || aid > 15) {
        return NULL;
    }
    cp15_invalidateDC(&w9b->status->childBitmap, 2);
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

// WM_GetAllowedChannel: the channels that the console may use, one bit each
u16 func_020810cc(void) {
    if (hw_isDSi() && !func_02080e60()) {
        return 0;
    }
    if (func_02080a70() != WM_ERRCODE_SUCCESS) {
        return 0x8000;
    }
    return *(u16 *)HW_WM_ALLOWED_CHANNEL_BUF;
}

// WM_GetLinkLevel
WMLinkLevel func_020810fc(void) {
    WMArm9Buf *w9b = func_02080a64();

    if (func_02080a70() != WM_ERRCODE_SUCCESS) {
        return WM_LINK_LEVEL_0;
    }
    cp15_invalidateDC(w9b->status, 2);
    switch (w9b->status->state) {
    case WM_STATE_MP_PARENT:
        cp15_invalidateDC(&w9b->status->childBitmap, 2);
        if (w9b->status->childBitmap == 0) {
            return WM_LINK_LEVEL_0;
        }
        // fall through
    case WM_STATE_MP_CHILD:
    case WM_STATE_DCF_CHILD:
        cp15_invalidateDC(&w9b->status->linkLevel, 2);
        return w9b->status->linkLevel;
    }
    return WM_LINK_LEVEL_0;
}

// WM_GetDispersionBeaconPeriod: the period of a parent's beacon in milliseconds, 200 to 219, from the console's
// address and the time
u16 func_0208115c(void) {
    u8 mac[6];
    u16 seed;
    int i;

    func_0207c33c(mac);
    for (i = 0, seed = 0; i < 6; i++) {
        seed += mac[i];
    }
    seed += (u16)OS_GetVBlankCount();
    return 200 + (u16)(seed * 7) % 20;
}

// WM_GetDispersionScanPeriod: how long to stay on a channel while scanning, 30 to 39 milliseconds
u16 func_020811a4(void) {
    u8 mac[6];
    u16 seed;
    int i;

    func_0207c33c(mac);
    for (i = 0, seed = 0; i < 6; i++) {
        seed += mac[i];
    }
    seed += (u16)OS_GetVBlankCount();
    return 30 + (u16)(seed * 13) % 10;
}

// WM_GetOtherElements: the elements that follow the beacon's header, if there is no game information
WMOtherElements func_020811ec(const WMBssDesc *bssDesc) {
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

// WM_GetNextTgid: the TGID for a parent's next beacon, counting up from the minute and second at the first call
u16 func_020812b8(void) {
    static u32 tgid_bak = 0x10000;
    RTCTime time;

    if (tgid_bak == 0x10000) {
        func_0207cb88();
        if (func_0207cc80(&time) == 0) {
            tgid_bak = (u16)(time.second + (time.minute << 8));
        }
    }
    tgid_bak = (u16)(tgid_bak + 1);
    return tgid_bak;
}
