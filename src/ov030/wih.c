#include "gfl/wih.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/net_whpipe.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// wih.c is named after its embedded string. It is the wireless helper of the GFL net, which grew out of NitroSDK's demo
// wh.c: the names that follow its functions are the ones it seems to be, and the variable and its fields keep the
// look of its names (the string of the assert shows _pWmInfo->sScanCallback). It runs a chain of WM calls, each of
// them with a callback that starts the next, and keeps the state of the chain in sSysState

// The work of the helper, which one allocation holds (this and 0x20 bytes of cache line slack)
typedef struct {
    WMParentParam sParentParam;
    u8 unk_38[8];
    // The buffer that WM works in
    u8 sWmBuf[0xf00];
    // The copy of the beacon that the scan is looking at
    WMBssDesc sBssDesc;
    u8 sScanBuf[0x400];
    WMScanExParam sScanParam;
    WMDataSharingInfo sDataSharing;
    WMDataSet sDataSet;
    // What runs when the start or the end of the wireless is done
    WHCallback sCallback;
    u8 *sSendBuf;
    u8 *sRecvBuf;
    int sSysState;
    // How this machine connects (WH_CONNECTMODE_*)
    int sConnectMode;
    WHReceiver sReceiver;
    NetScanFilter sScanFilter;
    // What runs with the AID of a machine that has disconnected. Nothing sets it
    void (*sDisconnectCallback)(int netId);
    WHConnectCallback sConnectCallback;
    WHScanInfoCallback sScanInfoCallback;
    u32 sSendBufSize;
    u32 sRecvBufSize;
    WHScanCallback sScanCallback;
    // The error that was reported first
    int sLastError;
    u32 sSeed;
    u8 unk_1ea0[8];
    u16 sAid;
    // The AIDs that are connected, a bit each (this machine's is 0)
    u16 sBitmap;
    u16 sChannel;
    // The least busy that a channel was, and the channels that were as little busy
    u16 sMinBusy;
    u16 sBestBitmap;
    // The channel to scan, 0 for all of them in turn
    u16 sScanChannel;
    // Whether to connect to the parent that the scan has found
    u16 sAutoConnect;
    u8 unk_1eb6[4];
    u16 sBeaconCount;
    u16 sScanCount;
    // The frames until the scan starts again
    s16 sScanWait;
    HeapID sHeapId;
    // The frames that it waits to start the scan again
    u16 sScanTime;
    u8 sSsid[0x1a];
    // Whether the scan is not to start again
    u8 sScanPaused;
    // Whether the scan has been asked to end
    u8 sScanEnding;
} WIH;

static void WH_ChangeSysState(int state);
static void WH_ReportError(int errcode);
static BOOL WH_StateInSetParentParam(void);
static void WH_StateOutSetParentParam(void *arg);
static BOOL WH_StateInStartParent(void);
static void WH_StateOutStartParent(void *arg);
static BOOL WH_StateInStartParentMP(void);
static void WH_StateOutStartParentMP(void *arg);
static BOOL WH_StateInEndParentMP(void);
static void WH_StateOutEndParentMP(void *arg);
static BOOL WH_StateInEndParent(void);
static void WH_StateOutEndParent(void *arg);
static BOOL WH_StateInStartScan(void);
static BOOL WH_CheckBssDesc(const WMBssDesc *bssDesc);
static void WH_StateOutStartScan(void *arg);
static BOOL WH_StateInEndScan(void);
static void WH_StateOutEndScan(void *arg);
static BOOL WH_StateInStartChild(void);
static void WH_StateOutStartChild(void *arg);
static BOOL WH_StateInStartChildMP(void);
static void WH_StateOutStartChildMP(void *arg);
static BOOL WH_StateInEndChildMP(void);
static void WH_StateOutEndChildMP(void *arg);
static BOOL WH_StateInEndChild(void);
static void WH_StateOutEndChild(void *arg);
static BOOL WH_StateInReset(void);
static void WH_StateOutReset(void *arg);
static BOOL WH_SendMPData(const void *data, u16 size, WHSendCallback callback);
static void WH_SendMPDataCallback(void *arg);
static void WH_PortReceiveCallback(void *arg);
static void WH_StateOutEnd(void *arg);
static u16 WH_StateInMeasureChannel(u16 channel);
static void WH_StateOutMeasureChannel(void *arg);
static int WH_MeasureChannel(WMCallbackFunc callback, u16 channel);
static s16 WH_ChooseChannel(u16 bitmap);
static void WH_IndicationCallback(void *arg);
static BOOL WH_StateInInitialize(WHCallback callback, u32 extended);
static void WH_StateOutSetLifeTime(void *arg);
static void WH_StateOutInitialize(void *arg);

static WIH *_pWmInfo;

static void WH_ChangeSysState(int state) {
    _pWmInfo->sSysState = state;
}

// Keeps the first error, until the state is that of an error
static void WH_ReportError(int errcode) {
    if ((u32)(_pWmInfo->sSysState - WH_SYSSTATE_ERROR) > 1) {
        _pWmInfo->sLastError = errcode;
    }
}

static BOOL WH_StateInSetParentParam(void) {
    int result;

    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    result = func_02081474(WH_StateOutSetParentParam, &_pWmInfo->sParentParam);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutSetParentParam(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return;
    }
    if (!WH_StateInStartParent()) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
    }
}

static BOOL WH_StateInStartParent(void) {
    int result;

    if (_pWmInfo->sSysState == WH_SYSSTATE_CONNECTED || _pWmInfo->sSysState == WH_SYSSTATE_DATASHARING) {
        return TRUE;
    }
    result = func_02081594(WH_StateOutStartParent);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    _pWmInfo->sAid = 0;
    _pWmInfo->sBitmap = 1;
    return TRUE;
}

static void WH_StateOutStartParent(void *arg) {
    WMStartParentCallback *cb = arg;
    BOOL accept = TRUE;
    u16 bit = 1 << cb->aid;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return;
    }
    switch (cb->state) {
    case WM_STATECODE_BEACON_SENT:
        _pWmInfo->sBeaconCount++;
        break;
    case WM_STATECODE_CONNECTED:
        if (_pWmInfo->sScanFilter != NULL) {
            accept = _pWmInfo->sScanFilter(cb, func_02042d94());
        }
        if (!accept) {
            int result = func_02081774(NULL, cb->aid);

            if (result != WM_ERRCODE_OPERATING) {
                WH_ReportError(result);
                WH_ChangeSysState(WH_SYSSTATE_ERROR);
            }
            return;
        }
        _pWmInfo->sBitmap |= bit;
        if (_pWmInfo->sConnectCallback != NULL) {
            _pWmInfo->sConnectCallback(cb->aid);
        }
        break;
    case WM_STATECODE_DISCONNECTED:
    case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
        if (_pWmInfo->sBitmap & (1 << cb->aid)) {
            _pWmInfo->sBitmap &= ~bit;
            if (_pWmInfo->sDisconnectCallback != NULL) {
                _pWmInfo->sDisconnectCallback(cb->aid);
            }
            func_ov030_02174088();
            func_02040a9c(cb->aid);
            func_0204049c(cb->aid);
        }
        break;
    case WM_STATECODE_PARENT_START:
        if (!WH_StateInStartParentMP()) {
            WH_ChangeSysState(WH_SYSSTATE_ERROR);
        }
        break;
    }
}

static BOOL WH_StateInStartParentMP(void) {
    int result;

    if (_pWmInfo->sSysState == WH_SYSSTATE_CONNECTED || _pWmInfo->sSysState == WH_SYSSTATE_DATASHARING) {
        return TRUE;
    }
    WH_ChangeSysState(WH_SYSSTATE_CONNECTED);
    func_02080fbc();
    func_02080f74();
    if (_pWmInfo->sRecvBuf != NULL) {
        func_02042ed0(_pWmInfo->sRecvBuf);
        func_02042ed0(_pWmInfo->sSendBuf);
    }
    _pWmInfo->sRecvBufSize = func_02080fbc();
    _pWmInfo->sSendBufSize = func_02080f74();
    _pWmInfo->sRecvBuf = allocConfigDSSoftwareFeature(_pWmInfo->sHeapId, _pWmInfo->sRecvBufSize + 0x20, "wih.c", 1192);
    _pWmInfo->sSendBuf = allocConfigDSSoftwareFeature(_pWmInfo->sHeapId, _pWmInfo->sSendBufSize + 0x20, "wih.c", 1193);
    result = func_0208195c(WH_StateOutStartParentMP, _pWmInfo->sRecvBuf, _pWmInfo->sRecvBufSize, _pWmInfo->sSendBuf,
                           _pWmInfo->sSendBufSize, func_02042d34() == 0x35 ? 1 : 2);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutStartParentMP(void *arg) {
    WMStartMPCallback *cb = arg;
    u16 aidBitmap;
    int machines;
    int lastAid;
    BOOL doubleMode;
    int result;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return;
    }
    switch (cb->state) {
    case WM_STATECODE_MP_START:
        if (_pWmInfo->sConnectMode == WH_CONNECTMODE_DS_PARENT) {
            machines = func_02042de8();
            lastAid = func_02042dc0() - 1;
            aidBitmap = (1 << (lastAid + 1)) - 1;
            doubleMode = TRUE;

            if (func_02042d34() == 0x35) {
                doubleMode = FALSE;
            }
            result = func_02081bd8(&_pWmInfo->sDataSharing, 13, aidBitmap, machines, doubleMode);
            if (result != 0) {
                WH_ReportError(result);
                WH_ChangeSysState(WH_SYSSTATE_ERROR);
                return;
            }
            WH_ChangeSysState(WH_SYSSTATE_DATASHARING);
        } else {
            WH_ChangeSysState(WH_SYSSTATE_CONNECTED);
        }
        break;
    case WM_STATECODE_MPEND_IND:
    case WM_STATECODE_MP_IND:
    case WM_STATECODE_MPACK_IND:
        break;
    }
}

static BOOL WH_StateInEndParentMP(void) {
    int result;

    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    result = func_02081a68(WH_StateOutEndParentMP);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutEndParentMP(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_Reset();
        return;
    }
    if (!WH_StateInEndParent()) {
        WH_Reset();
    }
}

static BOOL WH_StateInEndParent(void) {
    int result = func_020815a0(WH_StateOutEndParent);

    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutEndParent(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        return;
    }
    WH_ChangeSysState(WH_SYSSTATE_IDLE);
}

// WH_ChildConnectAuto: scans for the parent of the MAC address, and connects to it
BOOL WH_ChildConnectAuto(u16 mode, const u8 *mac, u16 channel) {
    const u16 *bssid = (const u16 *)mac;

    WH_ChangeSysState(WH_SYSSTATE_SCANNING);
    if (mac != NULL) {
        _pWmInfo->sScanParam.bssid[2] = bssid[2];
        _pWmInfo->sScanParam.bssid[1] = bssid[1];
        _pWmInfo->sScanParam.bssid[0] = bssid[0];
    } else {
        sys_memset(_pWmInfo->sScanParam.bssid, 0xff, 6);
    }
    _pWmInfo->sConnectMode = mode;
    _pWmInfo->sScanCallback = NULL;
    _pWmInfo->sScanChannel = channel;
    _pWmInfo->sScanParam.channelList = 1;
    _pWmInfo->sAutoConnect = 1;
    if (!WH_StateInStartScan()) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// WH_StartScan: scans for parents, giving the callback each one it finds
BOOL WH_StartScan(WHScanCallback callback, const u8 *mac, u16 channel) {
    const u16 *bssid = (const u16 *)mac;

    WH_ChangeSysState(WH_SYSSTATE_SCANNING);
    _pWmInfo->sScanCallback = callback;
    _pWmInfo->sScanChannel = channel;
    _pWmInfo->sScanParam.channelList = 1;
    _pWmInfo->sAutoConnect = 0;
    _pWmInfo->sScanCount = 0;
    if (mac != NULL) {
        _pWmInfo->sScanParam.bssid[2] = bssid[2];
        _pWmInfo->sScanParam.bssid[1] = bssid[1];
        _pWmInfo->sScanParam.bssid[0] = bssid[0];
    } else {
        sys_memset(_pWmInfo->sScanParam.bssid, 0xff, 6);
    }
    if (!WH_StateInStartScan()) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// Starts a scan of the next channel, or of the one the channel given
static BOOL WH_StateInStartScan(void) {
    u16 allowed = func_020810cc();
    int result;

    if (allowed == 0x8000) {
        WH_ReportError(3);
        return FALSE;
    }
    if (allowed == 0) {
        WH_ReportError(0x16);
        return FALSE;
    }
    if (_pWmInfo->sScanChannel == 0) {
        int i;
        u32 list;
        u32 channel;
        int bit;

        list = _pWmInfo->sScanParam.channelList;
        for (i = 0; i < 32; i++) {
            if (list & 1) {
                break;
            }
            list >>= 1;
        }
        channel = i + 1;
        do {
            channel++;
            if (channel > 16) {
                channel = 1;
            }
            bit = 1 << (channel - 1);
        } while (!(allowed & bit));
        _pWmInfo->sScanParam.channelList = bit;
    } else {
        _pWmInfo->sScanParam.channelList = 1 << (_pWmInfo->sScanChannel - 1);
    }
    _pWmInfo->sScanParam.maxChannelTime = func_020811a4();
    _pWmInfo->sScanParam.scanBuf = (WMBssDesc *)_pWmInfo->sScanBuf;
    _pWmInfo->sScanParam.scanBufSize = sizeof(_pWmInfo->sScanBuf);
    _pWmInfo->sScanParam.scanType = 1;
    _pWmInfo->sScanParam.ssidLength = 0;
    result = func_020815c8(WH_StateOutStartScan, &_pWmInfo->sScanParam);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

// WH_SetScanTarget: sets the channel and the MAC address that the scan looks for
void WH_SetScanTarget(u16 channel, const u8 *mac) {
    _pWmInfo->sScanChannel = channel;
    sys_memcpy(mac, _pWmInfo->sScanParam.bssid, 6);
}

// Whether the beacon is a valid one of this game's, from the length of its data, its magic number, its GGID and its
// attributes
static BOOL WH_CheckBssDesc(const WMBssDesc *bssDesc) {
    u16 length = bssDesc->gameInfoLength;

    if (length < 0x10 || length > 0x80 || length != bssDesc->gameInfo.userGameInfoLength + 0x10
        || bssDesc->gameInfo.magicNumber != 1 || length < 8 || bssDesc->gameInfo.ggid != _pWmInfo->sParentParam.ggid) {
        return FALSE;
    }
    if ((bssDesc->gameInfo.gameNameCount_attribute & 3) == 1) {
        return TRUE;
    }
    return FALSE;
}

static void WH_StateOutStartScan(void *arg) {
    WMStartScanExCallback *cb = arg;
    u16 state = cb->state;
    BOOL found;
    int i;

    _pWmInfo->sScanCount++;
    _pWmInfo->sScanWait = 0;
    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return;
    }
    if (_pWmInfo->sSysState != WH_SYSSTATE_SCANNING) {
        _pWmInfo->sAutoConnect = 0;
        if (!WH_StateInEndScan()) {
            WH_ChangeSysState(WH_SYSSTATE_ERROR);
        }
        return;
    }
    _pWmInfo->sScanWait = _pWmInfo->sScanTime;
    switch (state) {
    case WM_STATECODE_PARENT_NOT_FOUND:
        func_02012edc();
        return;
    case WM_STATECODE_PARENT_FOUND:
        break;
    default:
        return;
    }
    if (cb->bssDescCount != 0) {
        cp15_invalidateDC(_pWmInfo->sScanBuf, sizeof(_pWmInfo->sScanBuf));
    }
    found = FALSE;
    for (i = 0; i < cb->bssDescCount; i++) {
        WMBssDesc *bssDesc = &_pWmInfo->sBssDesc;

        sys_memcpy(cb->bssDesc[i], bssDesc, sizeof(WMBssDesc));
        func_02012ebc(cb->linkLevel[i]);
        if (_pWmInfo->sScanInfoCallback != NULL) {
            _pWmInfo->sScanInfoCallback(bssDesc, func_02042d94(), cb->linkLevel[i]);
        }
        if (WH_CheckBssDesc(bssDesc)) {
            GFL_ASSERT(_pWmInfo->sScanCallback);
            if (_pWmInfo->sScanCallback(bssDesc)) {
                found = TRUE;
            }
        }
    }
    if (_pWmInfo->sAutoConnect != 0 && found) {
        _pWmInfo->sScanWait = 0;
        if (!WH_StateInEndScan()) {
            WH_ChangeSysState(WH_SYSSTATE_ERROR);
        }
    }
}

// WH_EndScan: stops the scan, once the one under way is done. Whether there was one
BOOL WH_EndScan(void) {
    u32 irq;
    BOOL stopped = FALSE;

    irq = CPU_IRQDisable();

    if (_pWmInfo->sSysState == WH_SYSSTATE_SCANNING) {
        if (_pWmInfo->sScanWait != 0) {
            if (!WH_StateInStartScan()) {
                WH_ChangeSysState(WH_SYSSTATE_ERROR);
            }
        }
        _pWmInfo->sScanWait = -1;
        _pWmInfo->sAutoConnect = 0;
        stopped = TRUE;
        _pWmInfo->sScanEnding = TRUE;
        WH_ChangeSysState(WH_SYSSTATE_BUSY);
    }
    CPU_SetIRQMask(irq);
    return stopped;
}

static BOOL WH_StateInEndScan(void) {
    int result = func_020816b4(WH_StateOutEndScan);

    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutEndScan(void *arg) {
    WMCallback *cb = arg;

    _pWmInfo->sScanEnding = FALSE;
    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        return;
    }
    WH_ChangeSysState(WH_SYSSTATE_IDLE);
    if (_pWmInfo->sAutoConnect != 0) {
        _pWmInfo->sAutoConnect = 0;
        if (!WH_StateInStartChild()) {
            WH_ChangeSysState(WH_SYSSTATE_ERROR);
        }
    }
}

static BOOL WH_StateInStartChild(void) {
    int result;

    if (_pWmInfo->sSysState == WH_SYSSTATE_CONNECTED || _pWmInfo->sSysState == WH_SYSSTATE_DATASHARING) {
        return TRUE;
    }
    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    result = func_020816dc(WH_StateOutStartChild, &_pWmInfo->sBssDesc, _pWmInfo->sSsid, TRUE, 0);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutStartChild(void *arg) {
    WMStartConnectCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        if (cb->errcode == 0xc) {
            WH_ChangeSysState(WH_SYSSTATE_ERROR);
        } else if (cb->errcode == 0xb) {
            WH_ChangeSysState(WH_SYSSTATE_ERROR);
        } else if (cb->errcode == 1) {
            WH_ChangeSysState(WH_SYSSTATE_CONNECT_FAIL);
        } else {
            WH_ChangeSysState(WH_SYSSTATE_ERROR);
        }
        return;
    }
    if (cb->state == 8) {
        return;
    }
    if (cb->state == WM_STATECODE_CONNECTED) {
        WH_ChangeSysState(WH_SYSSTATE_CONNECTED);
        if (!WH_StateInStartChildMP()) {
            WH_ChangeSysState(WH_SYSSTATE_BUSY);
            return;
        }
        _pWmInfo->sAid = cb->aid;
        if (_pWmInfo->sConnectCallback != NULL) {
            _pWmInfo->sConnectCallback(cb->aid);
        }
    } else if (cb->state == 6) {
        return;
    } else if (cb->state == WM_STATECODE_DISCONNECTED) {
        WH_ReportError(0x14);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
    } else if (cb->state == WM_STATECODE_DISCONNECTED_FROM_MYSELF) {
        return;
    } else {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
    }
}

static BOOL WH_StateInStartChildMP(void) {
    int result;

    func_02080fbc();
    func_02080f74();
    if (_pWmInfo->sRecvBuf != NULL) {
        func_02042ed0(_pWmInfo->sRecvBuf);
        func_02042ed0(_pWmInfo->sSendBuf);
    }
    _pWmInfo->sRecvBufSize = func_02080fbc();
    _pWmInfo->sSendBufSize = func_02080f74();
    _pWmInfo->sRecvBuf = allocConfigDSSoftwareFeature(_pWmInfo->sHeapId, _pWmInfo->sRecvBufSize + 0x20, "wih.c", 2043);
    _pWmInfo->sSendBuf = allocConfigDSSoftwareFeature(_pWmInfo->sHeapId, _pWmInfo->sSendBufSize + 0x20, "wih.c", 2044);
    result = func_0208195c(WH_StateOutStartChildMP, _pWmInfo->sRecvBuf, _pWmInfo->sRecvBufSize, _pWmInfo->sSendBuf,
                           _pWmInfo->sSendBufSize, func_02042d34() == 0x35 ? 1 : 2);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutStartChildMP(void *arg) {
    WMStartMPCallback *cb = arg;
    u16 aidBitmap;
    int machines;
    int lastAid;
    BOOL doubleMode;
    int result;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        if (cb->errcode == 0xf || cb->errcode == 9 || cb->errcode == 0xd) {
            return;
        }
        WH_ReportError(cb->errcode);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return;
    }
    switch (cb->state) {
    case WM_STATECODE_MP_START:
        if (_pWmInfo->sConnectMode == WH_CONNECTMODE_DS_CHILD) {
            machines = func_02042de8();
            lastAid = func_02042dc0() - 1;
            aidBitmap = (1 << (lastAid + 1)) - 1;
            doubleMode = TRUE;

            if (func_02042d34() == 0x35) {
                doubleMode = FALSE;
            }
            result = func_02081bd8(&_pWmInfo->sDataSharing, 13, aidBitmap, machines, doubleMode);
            if (result != 0) {
                WH_ReportError(result);
                WH_Finalize();
                return;
            }
            WH_ChangeSysState(WH_SYSSTATE_DATASHARING);
        } else {
            WH_ChangeSysState(WH_SYSSTATE_CONNECTED);
        }
        break;
    case WM_STATECODE_MPEND_IND:
    case WM_STATECODE_MP_IND:
    case WM_STATECODE_MPACK_IND:
        break;
    }
}

static BOOL WH_StateInEndChildMP(void) {
    int result;

    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    result = func_02081a68(WH_StateOutEndChildMP);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutEndChildMP(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_Finalize();
        return;
    }
    if (!WH_StateInEndChild()) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
    }
}

static BOOL WH_StateInEndChild(void) {
    int result;

    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    result = func_02081774(WH_StateOutEndChild, 0);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        WH_Reset();
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutEndChild(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        return;
    }
    WH_ChangeSysState(WH_SYSSTATE_IDLE);
}

static BOOL WH_StateInReset(void) {
    int result;

    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    result = func_02081424(WH_StateOutReset);
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutReset(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        WH_ReportError(cb->errcode);
        return;
    }
    WH_ChangeSysState(WH_SYSSTATE_IDLE);
}

// Sends the data to every machine on the port, and calls back once it is gone
static BOOL WH_SendMPData(const void *data, u16 size, WHSendCallback callback) {
    cp15_flushDC(_pWmInfo->sSendBuf, _pWmInfo->sSendBufSize);
    if (func_02081998(WH_SendMPDataCallback, callback, data, size, 0xffff, 14, 2) == WM_ERRCODE_OPERATING) {
        return TRUE;
    }
    return FALSE;
}

static void WH_SendMPDataCallback(void *arg) {
    WMPortSendCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS && cb->errcode != 0xf) {
        WH_ReportError(cb->errcode);
        return;
    }
    if (cb->arg != NULL) {
        ((WHSendCallback)cb->arg)(cb->errcode == WM_ERRCODE_SUCCESS);
    }
}

static void WH_PortReceiveCallback(void *arg) {
    WMPortRecvCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        return;
    }
    if (_pWmInfo->sReceiver != NULL) {
        if (cb->state == WM_STATECODE_PORT_RECV) {
            _pWmInfo->sReceiver(cb->aid, cb->data, cb->length);
        } else if (cb->state == WM_STATECODE_DISCONNECTED) {
            _pWmInfo->sReceiver(cb->aid, NULL, 0);
        }
    }
}

static void WH_StateOutEnd(void *arg) {
    WMCallback *cb = arg;

    if (_pWmInfo->sCallback != NULL) {
        _pWmInfo->sCallback(cb->errcode == WM_ERRCODE_SUCCESS);
    }
    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ChangeSysState(WH_SYSSTATE_FATAL);
    } else {
        WH_ChangeSysState(WH_SYSSTATE_STOP);
    }
}

// WH_SetGgid
void WH_SetGgid(u32 ggid) {
    _pWmInfo->sParentParam.ggid = ggid;
}

// WH_SetUserGameInfo
void WH_SetUserGameInfo(const void *data, u16 size) {
    _pWmInfo->sParentParam.userGameInfo = data;
    _pWmInfo->sParentParam.userGameInfoLength = size;
}

// WH_GetBitmap
u16 WH_GetBitmap(void) {
    return _pWmInfo->sBitmap;
}

// WH_GetSystemState
int WH_GetSystemState(void) {
    if (_pWmInfo == NULL) {
        return WH_SYSSTATE_STOP;
    }
    return _pWmInfo->sSysState;
}

// WH_GetLastError
int WH_GetLastError(void) {
    if (_pWmInfo != NULL) {
        return _pWmInfo->sLastError;
    }
    return 0;
}

// WH_StartMeasureChannel: measures how busy each channel is, one after the other
BOOL WH_StartMeasureChannel(void) {
    u16 mac[3];
    u16 result;

    func_0207c33c((u8 *)mac);
    _pWmInfo->sSeed = OS_GetVBlankCount() + mac[0] + mac[1] + mac[2];
    _pWmInfo->sSeed = _pWmInfo->sSeed * 69069 + 12345;
    _pWmInfo->sChannel = 0;
    _pWmInfo->sMinBusy = 101;
    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    result = WH_StateInMeasureChannel(1);
    if (result == 0x18) {
        WH_ReportError(0x18);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return FALSE;
    }
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// Measures the next channel that is allowed, from the one given. 0x18 once there are none left
static u16 WH_StateInMeasureChannel(u16 channel) {
    u16 allowed = func_020810cc();

    if (allowed == 0x8000) {
        WH_ReportError(3);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return 3;
    }
    if (allowed == 0) {
        WH_ReportError(0x16);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return 0x18;
    }
    if (!((1 << (channel - 1)) & allowed)) {
        do {
            channel++;
            if (channel > 16) {
                return 0x18;
            }
        } while (!((1 << (channel - 1)) & allowed));
    }
    return WH_MeasureChannel(WH_StateOutMeasureChannel, channel);
}

static void WH_StateOutMeasureChannel(void *arg) {
    WMMeasureChannelCallback *cb = arg;
    u16 channel;
    u16 busy;
    u16 next;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return;
    }
    channel = cb->channel;
    busy = cb->ccaBusyRatio;
    if (_pWmInfo->sMinBusy > busy) {
        _pWmInfo->sMinBusy = busy;
        _pWmInfo->sBestBitmap = 1 << (channel - 1);
    } else if (_pWmInfo->sMinBusy == busy) {
        _pWmInfo->sBestBitmap |= 1 << (channel - 1);
    }
    next = WH_StateInMeasureChannel(channel + 1);
    if (next == 0x18 && _pWmInfo->sSysState != WH_SYSSTATE_ERROR) {
        WH_ChangeSysState(WH_SYSSTATE_MEASURECHANNEL);
        return;
    }
    if (next != WM_ERRCODE_OPERATING) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
    }
}

static int WH_MeasureChannel(WMCallbackFunc callback, u16 channel) {
    return func_0208259c(callback, 3, 0x11, channel, 30);
}

// WH_GetMeasureChannel: the channel to use, one of the least busy ones, chosen at random
u16 WH_GetMeasureChannel(void) {
    WH_ChangeSysState(WH_SYSSTATE_IDLE);
    _pWmInfo->sChannel = WH_ChooseChannel(_pWmInfo->sBestBitmap);
    return _pWmInfo->sChannel;
}

static s16 WH_ChooseChannel(u16 bitmap) {
    s16 channel = 0;
    s16 i;
    u16 count = 0;
    u16 pick;

    for (i = 0; i < 16; i++) {
        if (bitmap & (1 << i)) {
            channel = i + 1;
            count++;
        }
    }
    if (count > 1) {
        _pWmInfo->sSeed = _pWmInfo->sSeed * 69069 + 12345;
        pick = (count * (u8)_pWmInfo->sSeed) >> 8;
        for (i = 0; i < 16; i++) {
            if (bitmap & 1) {
                if (pick == 0) {
                    return i + 1;
                }
                pick--;
            }
            bitmap >>= 1;
        }
        return 0;
    }
    return channel;
}

// WH_Initialize: allocates the work and starts the wireless
BOOL WH_Initialize(HeapID heapId, WHCallback callback, u32 extended) {
    if (_pWmInfo != NULL) {
        return FALSE;
    }
    _pWmInfo = allocConfigDSSoftwareFeature(heapId, sizeof(WIH) + 0x20, "wih.c", 3124);
    sys_memset(_pWmInfo, 0, sizeof(WIH));
    _pWmInfo->sScanTime = 1;
    _pWmInfo->sBitmap = 1;
    _pWmInfo->sLastError = 0;
    _pWmInfo->sHeapId = heapId;
    if (!WH_StateInInitialize(callback, extended)) {
        return FALSE;
    }
    return TRUE;
}

// WH_Release: frees the work and its buffers
void WH_Release(void) {
    if (_pWmInfo != NULL) {
        func_02042ed0(_pWmInfo->sRecvBuf);
        func_02042ed0(_pWmInfo->sSendBuf);
        func_02042ed0(_pWmInfo);
        _pWmInfo = NULL;
    }
}

static void WH_IndicationCallback(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode == 8) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        WH_ReportError(0x19);
    }
}

// Starts WM. Its first argument is not used
static BOOL WH_StateInInitialize(WHCallback callback, u32 extended) {
    int result;

    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    if (extended == 0) {
        result = func_020813c4(_pWmInfo->sWmBuf, WH_StateOutInitialize, 2);
    } else {
        result = func_020813d0(_pWmInfo->sWmBuf, WH_StateOutInitialize, 2, 0);
    }
    if (result != WM_ERRCODE_OPERATING) {
        WH_ReportError(result);
        WH_ChangeSysState(WH_SYSSTATE_FATAL);
        return FALSE;
    }
    return TRUE;
}

static void WH_StateOutSetLifeTime(void *arg) {
    WH_ChangeSysState(WH_SYSSTATE_IDLE);
}

static void WH_StateOutInitialize(void *arg) {
    WMCallback *cb = arg;
    int result;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        WH_ReportError(cb->errcode);
        WH_ChangeSysState(WH_SYSSTATE_FATAL);
        if (_pWmInfo->sCallback != NULL) {
            _pWmInfo->sCallback(FALSE);
        }
        return;
    }
    result = func_02080e7c(WH_IndicationCallback);
    if (result != WM_ERRCODE_SUCCESS) {
        WH_ReportError(result);
        WH_ChangeSysState(WH_SYSSTATE_FATAL);
        if (_pWmInfo->sCallback != NULL) {
            _pWmInfo->sCallback(FALSE);
        }
        return;
    }
    WH_ChangeSysState(WH_SYSSTATE_IDLE);
    func_02082560(WH_StateOutSetLifeTime, 0xffff, 40, 5, 40);
    if (_pWmInfo->sCallback != NULL) {
        _pWmInfo->sCallback(TRUE);
    }
}

// WH_ParentConnect: starts as a parent, in the mode of the connection (the multi-point mode, the keys or the data shared)
BOOL WH_ParentConnect(u16 mode, u16 tgid, u16 channel, u16 maxEntry) {
    _pWmInfo->sBeaconCount = 0;
    _pWmInfo->sConnectMode = mode;
    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    _pWmInfo->sParentParam.tgid = tgid;
    _pWmInfo->sParentParam.channel = channel;
    _pWmInfo->sParentParam.beaconPeriod = func_0208115c();
    if (func_02042df4() != 0) {
        _pWmInfo->sParentParam.parentMaxSize = func_02042df4();
    } else {
        _pWmInfo->sParentParam.parentMaxSize = func_02042de8() * func_02042dc0() + 4;
    }
    _pWmInfo->sParentParam.childMaxSize = func_02042de8();
    _pWmInfo->sParentParam.maxEntry = maxEntry;
    _pWmInfo->sParentParam.CS_Flag = 0;
    _pWmInfo->sParentParam.multiBootFlag = 0;
    _pWmInfo->sParentParam.entryFlag = 1;
    _pWmInfo->sParentParam.KS_Flag = mode == WH_CONNECTMODE_KS_PARENT;
    if (mode == WH_CONNECTMODE_MP_PARENT || mode == WH_CONNECTMODE_KS_PARENT || mode == WH_CONNECTMODE_DS_PARENT) {
        return WH_StateInSetParentParam();
    }
    return FALSE;
}

// WH_SetScanFilter
void WH_SetScanFilter(NetScanFilter filter) {
    _pWmInfo->sScanFilter = filter;
}

// WH_SetReceiver
void WH_SetReceiver(WHReceiver callback) {
    _pWmInfo->sReceiver = callback;
    if (func_02080eac(14, WH_PortReceiveCallback, NULL) != WM_ERRCODE_SUCCESS) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
    }
}

// WH_SendData: sends the data to every machine, unless this is the parent and no child is there
BOOL WH_SendData(u8 *data, u16 size, WHSendCallback callback) {
    if (WH_GetAid() == 0 && (WH_GetBitmap() & 0xfe) == 0) {
        return FALSE;
    }
    return WH_SendMPData(data, size, callback);
}

// WH_GetSharedDataAdr: the data of the machine of the AID, from the last step of the data sharing
u8 *WH_GetSharedDataAdr(u16 netId) {
    return func_020823ac(&_pWmInfo->sDataSharing, &_pWmInfo->sDataSet, netId);
}

// WH_StepDS: shares the data given, for the machines to read the next step
BOOL WH_StepDS(u8 *data) {
    int result = func_02081df0(&_pWmInfo->sDataSharing, data, &_pWmInfo->sDataSet);

    if (result == 7) {
        return FALSE;
    }
    if (result == 5) {
        return FALSE;
    }
    if (result != WM_ERRCODE_SUCCESS) {
        WH_ReportError(result);
        return FALSE;
    }
    return TRUE;
}

// Run each frame: starts the scan again once its frames to wait have gone
void WH_UpdateScan(void) {
    if (_pWmInfo != NULL && _pWmInfo->sScanWait > 0) {
        if (_pWmInfo->sSysState != WH_SYSSTATE_SCANNING) {
            _pWmInfo->sScanWait = 0;
        } else {
            _pWmInfo->sScanWait = _pWmInfo->sScanWait - 1;
        }
        if (_pWmInfo->sScanWait == 0) {
            if (_pWmInfo->sSysState == WH_SYSSTATE_SCANNING && _pWmInfo->sScanPaused == 0) {
                if (!WH_StateInStartScan()) {
                    WH_ChangeSysState(WH_SYSSTATE_ERROR);
                }
                return;
            }
            _pWmInfo->sScanWait = 1;
        }
    }
}

// WH_Reset: resets the wireless, ending the data sharing first
void WH_Reset(void) {
    if (_pWmInfo->sSysState == WH_SYSSTATE_DATASHARING) {
        int result = func_02081dbc(&_pWmInfo->sDataSharing);

        if (result != WM_ERRCODE_SUCCESS) {
            WH_ReportError(result);
        }
    }
    if (!WH_StateInReset()) {
        GFL_NetErrAbort();
    }
}

// WH_Finalize: ends whatever the machine is doing, back to idle. Whether it has
BOOL WH_Finalize(void) {
    int state;

    if (_pWmInfo == NULL || _pWmInfo->sSysState == WH_SYSSTATE_IDLE) {
        return TRUE;
    }
    state = _pWmInfo->sSysState;
    if (_pWmInfo->sLastError != 0) {
        WH_Reset();
        return TRUE;
    }
    if (state == WH_SYSSTATE_BUSY) {
        return FALSE;
    }
    if (state == WH_SYSSTATE_SCANNING) {
        if (!WH_EndScan()) {
            WH_Reset();
        }
        return TRUE;
    }
    if (_pWmInfo->sScanEnding != 0) {
        return TRUE;
    }
    if (state != WH_SYSSTATE_DATASHARING && state != WH_SYSSTATE_CONNECTED) {
        WH_ChangeSysState(WH_SYSSTATE_BUSY);
        WH_Reset();
        return TRUE;
    }
    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    switch (_pWmInfo->sConnectMode) {
    case WH_CONNECTMODE_DS_CHILD:
        if (func_02081dbc(&_pWmInfo->sDataSharing) != 0) {
            WH_Reset();
            break;
        }
    case WH_CONNECTMODE_MP_CHILD:
        if (!WH_StateInEndChildMP()) {
            WH_Reset();
        }
        break;
    case WH_CONNECTMODE_DS_PARENT:
        if (func_02081dbc(&_pWmInfo->sDataSharing) != 0) {
            WH_Reset();
            break;
        }
    case WH_CONNECTMODE_MP_PARENT:
        if (!WH_StateInEndParentMP()) {
            WH_Reset();
        }
        break;
    }
    return TRUE;
}

// WH_End: ends the wireless, and calls the callback with whether it did
BOOL WH_End(WHCallback callback) {
    _pWmInfo->sCallback = callback;
    WH_ChangeSysState(WH_SYSSTATE_BUSY);
    if (func_02081448(WH_StateOutEnd) != WM_ERRCODE_OPERATING) {
        WH_ChangeSysState(WH_SYSSTATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// WH_GetAid
u16 WH_GetAid(void) {
    if (_pWmInfo != NULL) {
        return _pWmInfo->sAid;
    }
    return 0;
}

// WH_SetScanCallback
void WH_SetScanCallback(WHScanCallback callback) {
    _pWmInfo->sScanCallback = callback;
}

// WH_SetGameInfo: the parent's beacon, once the machine is connected
void WH_SetGameInfo(const void *data, int size, u32 ggid, int tgid) {
    if (_pWmInfo != NULL && (_pWmInfo->sSysState == WH_SYSSTATE_CONNECTED || _pWmInfo->sSysState == WH_SYSSTATE_DATASHARING)) {
        func_020824bc(NULL, data, size, ggid, tgid, 1);
    }
}

// WH_GetBeaconCount: the beacons that the parent has sent
u16 WH_GetBeaconCount(void) {
    return _pWmInfo->sBeaconCount;
}

// WH_SetConnectCallback
void WH_SetConnectCallback(WHConnectCallback callback) {
    _pWmInfo->sConnectCallback = callback;
}

// WH_SetScanInfoCallback
void WH_SetScanInfoCallback(WHScanInfoCallback callback) {
    _pWmInfo->sScanInfoCallback = callback;
}

// WH_SetScanTime
void WH_SetScanTime(u16 time) {
    _pWmInfo->sScanTime = time;
}

// WH_SetScanPaused
void WH_SetScanPaused(BOOL paused) {
    _pWmInfo->sScanPaused = paused;
}
