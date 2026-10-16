// The wireless manager's standard operations (libwm, TwlSDK's wm/ARM9/src/wm_standard.c, by its place in the library:
// the file name is a guess): starting and stopping the manager and its power, setting a parent up and starting it, the
// scan, the connection of a child and its disconnection. Each checks the state and the arguments, registers the
// callback and sends the command to the ARM7. The names are descriptive
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// What the manager sends the ARM7 to start a scan: the fields of WMScanExParam in other places. Its size is 0x3c; the
// last four bytes are not set
typedef struct {
    u16 apiid;
    u16 channelList;
    WMBssDesc *scanBuf;
    u16 scanBufSize;
    u16 maxChannelTime;
    u8 bssid[6];
    u16 scanType;
    u16 ssidLength;
    u8 ssid[32];
    u16 ssidMatchLength;
    u8 unk_38[4];
} WmStartScanExCommand;

// What it sends to start the connection of a child. Its size is 0x28; the halfword at 0x24 is not set
typedef struct {
    u16 apiid;
    u16 unk_02;
    const WMBssDesc *bssDesc;
    u8 ssid[24];
    BOOL powerSave;
    u16 unk_24;
    u16 authMode;
} WmStartConnectCommand;

static int WMi_EnableEx(WMCallbackFunc callback, int flags);
static int WMi_InitializeEx(void *wmSysBuf, WMCallbackFunc callback, u16 dmaNo, int flags);
static BOOL WmCheckParentParameter(const WMParentParam *param);

// WM_Enable
int func_020812fc(WMCallbackFunc callback) {
    return WMi_EnableEx(callback, 0);
}

// WMi_EnableEx: starts the manager's side of the ARM7 in the buffers of the manager
static int WMi_EnableEx(WMCallbackFunc callback, int flags) {
    WMArm9Buf *w9b;
    int result = func_02080ab0(1, WM_STATE_READY);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_ENABLE, callback);
    func_02080e24();
    w9b = func_02080a64();
    result = WMi_SendCommand(WM_APIID_ENABLE, 4, w9b->wm7buf, w9b->status, w9b->indBuf, flags);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_Disable
int func_02081348(WMCallbackFunc callback) {
    int result = func_02080ab0(1, WM_STATE_STOP);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_DISABLE, callback);
    result = WMi_SendCommand(WM_APIID_DISABLE, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_PowerOn
int func_02081370(WMCallbackFunc callback) {
    int result = func_02080ab0(1, WM_STATE_STOP);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_POWER_ON, callback);
    result = WMi_SendCommand(WM_APIID_POWER_ON, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_PowerOff
int func_02081398(WMCallbackFunc callback) {
    int result = func_02080ab0(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_POWER_OFF, callback);
    result = WMi_SendCommand(WM_APIID_POWER_OFF, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_Initialize
int func_020813c4(void *wmSysBuf, WMCallbackFunc callback, u16 dmaNo) {
    return WMi_InitializeEx(wmSysBuf, callback, dmaNo, 0);
}

// Like WM_Initialize, with a fourth argument: the flags to the ARM7 are bit 0, and bit 1 as well if the argument is 0
int func_020813d0(void *wmSysBuf, WMCallbackFunc callback, u16 dmaNo, int extra) {
    int flags = 1;

    if (extra == 0) {
        flags |= 2;
    }
    return WMi_InitializeEx(wmSysBuf, callback, dmaNo, flags);
}

// WMi_InitializeEx: sets the manager up in the buffer and starts the ARM7's side of it
static int WMi_InitializeEx(void *wmSysBuf, WMCallbackFunc callback, u16 dmaNo, int flags) {
    WMArm9Buf *w9b;
    int result = func_02080758(wmSysBuf, dmaNo);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_INITIALIZE, callback);
    func_02080e24();
    w9b = func_02080a64();
    result = WMi_SendCommand(WM_APIID_INITIALIZE, 4, w9b->wm7buf, w9b->status, w9b->indBuf, flags);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_Reset
int func_02081424(WMCallbackFunc callback) {
    int result = func_02080a84();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_RESET, callback);
    result = WMi_SendCommand(WM_APIID_RESET, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_End
int func_02081448(WMCallbackFunc callback) {
    int result = func_02080ab0(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_END, callback);
    result = WMi_SendCommand(WM_APIID_END, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_SetParentParameter
int func_02081474(WMCallbackFunc callback, WMParentParam *param) {
    int result = func_02080ab0(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (hw_isDSi() && !func_02080e60()) {
        return WM_ERRCODE_WM_DISABLE;
    }
    if (param == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (param->userGameInfoLength != 0 && param->userGameInfo == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    // The most that the parent and the children send in a round includes the key sharing's data, if it is on
    if (param->parentMaxSize + (param->KS_Flag ? 0x2a : 0) > WM_SIZE_MP_DATA_MAX ||
        param->childMaxSize + (param->KS_Flag ? 6 : 0) > WM_SIZE_MP_DATA_MAX) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    // BUG: the result of the check is not used, so a bad parameter goes to the ARM7
#ifdef BUGFIX
    if (!WmCheckParentParameter(param)) {
        return WM_ERRCODE_INVALID_PARAM;
    }
#else
    WmCheckParentParameter(param);
#endif
    func_02080950(WM_APIID_SET_P_PARAM, callback);
    cp15_cleanDC(param, 0x40); // two cache lines
    if (param->userGameInfoLength != 0) {
        cp15_cleanDC(param->userGameInfo, param->userGameInfoLength);
    }
    result = WMi_SendCommand(WM_APIID_SET_P_PARAM, 1, param);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// Whether the parent's parameters are in range: the game information, the beacon's period in milliseconds and the
// channel, 1 to 14
static BOOL WmCheckParentParameter(const WMParentParam *param) {
    if (param->userGameInfoLength > WM_SIZE_USER_GAMEINFO) {
        return FALSE;
    }
    if (param->beaconPeriod < 10 || param->beaconPeriod > 1000) {
        return FALSE;
    }
    if (param->channel < 1 || param->channel > 14) {
        return FALSE;
    }
    return TRUE;
}

// WMi_StartParentEx: starts the parent; the flag goes to the ARM7 as the argument of the command
int func_02081544(WMCallbackFunc callback, int enable) {
    WMArm9Buf *w9b;
    int result = func_02080ab0(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (hw_isDSi() && !func_02080e60()) {
        return WM_ERRCODE_WM_DISABLE;
    }
    w9b = func_02080a64();
    w9b->myAid = 0;
    w9b->connectedAidBitmap = 0;
    func_02080950(WM_APIID_START_PARENT, callback);
    result = WMi_SendCommand(WM_APIID_START_PARENT, 1, enable);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_StartParent
int func_02081594(WMCallbackFunc callback) {
    return func_02081544(callback, TRUE);
}

// WM_EndParent
int func_020815a0(WMCallbackFunc callback) {
    int result = func_02080ab0(1, WM_STATE_PARENT);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_END_PARENT, callback);
    result = WMi_SendCommand(WM_APIID_END_PARENT, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_StartScanEx: scans the channels of the list for parents. The scan type is 0 to 3, 2 and 3 matching the SSID
int func_020815c8(WMCallbackFunc callback, WMScanExParam *param) {
    WmStartScanExCommand cmd;
    int result = func_02080ab0(3, WM_STATE_IDLE, WM_STATE_CLASS1, WM_STATE_SCAN);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (hw_isDSi() && !func_02080e60()) {
        return WM_ERRCODE_WM_DISABLE;
    }
    if (param == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (param->scanBuf == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (param->scanBufSize > 0x400) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (param->ssidLength > sizeof(param->ssid)) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (param->scanType != 0 && param->scanType != 1 && param->scanType != 2 && param->scanType != 3) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if ((param->scanType == 2 || param->scanType == 3) && param->ssidMatchLength > sizeof(param->ssid)) {
        return WM_ERRCODE_INVALID_PARAM;
    }

    func_02080950(WM_APIID_START_SCAN_EX, callback);
    cmd.apiid = WM_APIID_START_SCAN_EX;
    cmd.channelList = param->channelList;
    cmd.scanBuf = param->scanBuf;
    cmd.scanBufSize = param->scanBufSize;
    cmd.maxChannelTime = param->maxChannelTime;
    sys_memcpy(param->bssid, cmd.bssid, sizeof(cmd.bssid));
    cmd.scanType = param->scanType;
    cmd.ssidMatchLength = param->ssidMatchLength;
    cmd.ssidLength = param->ssidLength;
    sys_memcpy(param->ssid, cmd.ssid, sizeof(cmd.ssid));
    result = WMi_SendCommandDirect(&cmd, sizeof(cmd));
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_EndScan
int func_020816b4(WMCallbackFunc callback) {
    int result = func_02080ab0(1, WM_STATE_SCAN);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_END_SCAN, callback);
    result = WMi_SendCommand(WM_APIID_END_SCAN, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_StartConnectEx: connects to the parent that the scan found; the SSID is the 24 bytes that the child gives, or none
int func_020816dc(WMCallbackFunc callback, const WMBssDesc *bssDesc, const u8 *ssid, BOOL powerSave, u16 authMode) {
    WmStartConnectCommand cmd;
    WMArm9Buf *w9b;
    int result = func_02080ab0(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (hw_isDSi() && !func_02080e60()) {
        return WM_ERRCODE_WM_DISABLE;
    }
    if (bssDesc == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    cp15_cleanDC(bssDesc, bssDesc->length * 2);
    w9b = func_02080a64();
    w9b->myAid = 0;
    w9b->connectedAidBitmap = 0;
    func_02080950(WM_APIID_START_CONNECT, callback);

    cmd.apiid = WM_APIID_START_CONNECT;
    cmd.bssDesc = bssDesc;
    if (ssid != NULL) {
        sys_memcpy(ssid, cmd.ssid, sizeof(cmd.ssid));
    } else {
        sys_memset(cmd.ssid, 0, sizeof(cmd.ssid));
    }
    cmd.powerSave = powerSave;
    cmd.authMode = authMode;
    result = WMi_SendCommandDirect(&cmd, sizeof(cmd));
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_Disconnect: a child leaves its parent with AID 0; a parent drops the child with the AID, which must be connected
int func_02081774(WMCallbackFunc callback, u16 aid) {
    WMArm9Buf *w9b = func_02080a64();
    int result =
        func_02080ab0(5, WM_STATE_PARENT, WM_STATE_MP_PARENT, WM_STATE_CHILD, WM_STATE_MP_CHILD, WM_STATE_DCF_CHILD);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (w9b->status->state == WM_STATE_PARENT || w9b->status->state == WM_STATE_MP_PARENT) {
        if (aid < 1 || aid > 15) {
            return WM_ERRCODE_INVALID_PARAM;
        }
        cp15_invalidateDC(&w9b->status->childBitmap, 2);
        if (!(w9b->status->childBitmap & (1 << aid))) {
            return WM_ERRCODE_NO_CHILD;
        }
    } else if (aid != 0) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    func_02080950(WM_APIID_DISCONNECT, callback);
    result = WMi_SendCommand(WM_APIID_DISCONNECT, 1, 1 << aid);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}
