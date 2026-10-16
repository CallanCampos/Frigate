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

int WM_Enable(WMCallbackFunc callback) {
    return WMi_EnableEx(callback, 0);
}

// Starts the manager's side of the ARM7 in the buffers of the manager
static int WMi_EnableEx(WMCallbackFunc callback, int flags) {
    WMArm9Buf *w9b;
    int result = WMi_CheckStateEx(1, WM_STATE_READY);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_ENABLE, callback);
    WMi_RegisterSleepCallback();
    w9b = WMi_GetSystemWork();
    result = WMi_SendCommand(WM_APIID_ENABLE, 4, w9b->wm7buf, w9b->status, w9b->indBuf, flags);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_Disable(WMCallbackFunc callback) {
    int result = WMi_CheckStateEx(1, WM_STATE_STOP);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_DISABLE, callback);
    result = WMi_SendCommand(WM_APIID_DISABLE, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_PowerOn(WMCallbackFunc callback) {
    int result = WMi_CheckStateEx(1, WM_STATE_STOP);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_POWER_ON, callback);
    result = WMi_SendCommand(WM_APIID_POWER_ON, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_PowerOff(WMCallbackFunc callback) {
    int result = WMi_CheckStateEx(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_POWER_OFF, callback);
    result = WMi_SendCommand(WM_APIID_POWER_OFF, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_Initialize(void *wmSysBuf, WMCallbackFunc callback, u16 dmaNo) {
    return WMi_InitializeEx(wmSysBuf, callback, dmaNo, 0);
}

// Like WM_Initialize, with a fourth argument: the flags to the ARM7 are bit 0, and bit 1 as well if the argument is 0
int WM_InitializeForListening(void *wmSysBuf, WMCallbackFunc callback, u16 dmaNo, int extra) {
    int flags = 1;

    if (extra == 0) {
        flags |= 2;
    }
    return WMi_InitializeEx(wmSysBuf, callback, dmaNo, flags);
}

// Sets the manager up in the buffer and starts the ARM7's side of it
static int WMi_InitializeEx(void *wmSysBuf, WMCallbackFunc callback, u16 dmaNo, int flags) {
    WMArm9Buf *w9b;
    int result = WM_Init(wmSysBuf, dmaNo);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_INITIALIZE, callback);
    WMi_RegisterSleepCallback();
    w9b = WMi_GetSystemWork();
    result = WMi_SendCommand(WM_APIID_INITIALIZE, 4, w9b->wm7buf, w9b->status, w9b->indBuf, flags);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_Reset(WMCallbackFunc callback) {
    int result = WMi_CheckIdle();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_RESET, callback);
    result = WMi_SendCommand(WM_APIID_RESET, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_End(WMCallbackFunc callback) {
    int result = WMi_CheckStateEx(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_END, callback);
    result = WMi_SendCommand(WM_APIID_END, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_SetParentParameter(WMCallbackFunc callback, WMParentParam *param) {
    int result = WMi_CheckStateEx(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (OS_IsRunOnTwl() && !WMi_IsWirelessAvailable()) {
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
    WMi_SetCallbackTable(WM_APIID_SET_P_PARAM, callback);
    DC_StoreRange(param, 0x40); // two cache lines
    if (param->userGameInfoLength != 0) {
        DC_StoreRange(param->userGameInfo, param->userGameInfoLength);
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

// Starts the parent; the flag goes to the ARM7 as the argument of the command
int WMi_StartParentEx(WMCallbackFunc callback, int enable) {
    WMArm9Buf *w9b;
    int result = WMi_CheckStateEx(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (OS_IsRunOnTwl() && !WMi_IsWirelessAvailable()) {
        return WM_ERRCODE_WM_DISABLE;
    }
    w9b = WMi_GetSystemWork();
    w9b->myAid = 0;
    w9b->connectedAidBitmap = 0;
    WMi_SetCallbackTable(WM_APIID_START_PARENT, callback);
    result = WMi_SendCommand(WM_APIID_START_PARENT, 1, enable);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_StartParent(WMCallbackFunc callback) {
    return WMi_StartParentEx(callback, TRUE);
}

int WM_EndParent(WMCallbackFunc callback) {
    int result = WMi_CheckStateEx(1, WM_STATE_PARENT);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_END_PARENT, callback);
    result = WMi_SendCommand(WM_APIID_END_PARENT, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// Scans the channels of the list for parents. The scan type is 0 to 3, 2 and 3 matching the SSID
int WM_StartScanEx(WMCallbackFunc callback, WMScanExParam *param) {
    WmStartScanExCommand cmd;
    int result = WMi_CheckStateEx(3, WM_STATE_IDLE, WM_STATE_CLASS1, WM_STATE_SCAN);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (OS_IsRunOnTwl() && !WMi_IsWirelessAvailable()) {
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

    WMi_SetCallbackTable(WM_APIID_START_SCAN_EX, callback);
    cmd.apiid = WM_APIID_START_SCAN_EX;
    cmd.channelList = param->channelList;
    cmd.scanBuf = param->scanBuf;
    cmd.scanBufSize = param->scanBufSize;
    cmd.maxChannelTime = param->maxChannelTime;
    MI_CpuCopy8(param->bssid, cmd.bssid, sizeof(cmd.bssid));
    cmd.scanType = param->scanType;
    cmd.ssidMatchLength = param->ssidMatchLength;
    cmd.ssidLength = param->ssidLength;
    MI_CpuCopy8(param->ssid, cmd.ssid, sizeof(cmd.ssid));
    result = WMi_SendCommandDirect(&cmd, sizeof(cmd));
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_EndScan(WMCallbackFunc callback) {
    int result = WMi_CheckStateEx(1, WM_STATE_SCAN);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_END_SCAN, callback);
    result = WMi_SendCommand(WM_APIID_END_SCAN, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// Connects to the parent that the scan found; the SSID is the 24 bytes that the child gives, or none
int WM_StartConnectEx(WMCallbackFunc callback, const WMBssDesc *bssDesc, const u8 *ssid, BOOL powerSave, u16 authMode) {
    WmStartConnectCommand cmd;
    WMArm9Buf *w9b;
    int result = WMi_CheckStateEx(1, WM_STATE_IDLE);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (OS_IsRunOnTwl() && !WMi_IsWirelessAvailable()) {
        return WM_ERRCODE_WM_DISABLE;
    }
    if (bssDesc == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    DC_StoreRange(bssDesc, bssDesc->length * 2);
    w9b = WMi_GetSystemWork();
    w9b->myAid = 0;
    w9b->connectedAidBitmap = 0;
    WMi_SetCallbackTable(WM_APIID_START_CONNECT, callback);

    cmd.apiid = WM_APIID_START_CONNECT;
    cmd.bssDesc = bssDesc;
    if (ssid != NULL) {
        MI_CpuCopy8(ssid, cmd.ssid, sizeof(cmd.ssid));
    } else {
        MI_CpuFill8(cmd.ssid, 0, sizeof(cmd.ssid));
    }
    cmd.powerSave = powerSave;
    cmd.authMode = authMode;
    result = WMi_SendCommandDirect(&cmd, sizeof(cmd));
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// A child leaves its parent with AID 0; a parent drops the child with the AID, which must be connected
int WM_Disconnect(WMCallbackFunc callback, u16 aid) {
    WMArm9Buf *w9b = WMi_GetSystemWork();
    int result =
        WMi_CheckStateEx(5, WM_STATE_PARENT, WM_STATE_MP_PARENT, WM_STATE_CHILD, WM_STATE_MP_CHILD, WM_STATE_DCF_CHILD);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (w9b->status->state == WM_STATE_PARENT || w9b->status->state == WM_STATE_MP_PARENT) {
        if (aid < 1 || aid > 15) {
            return WM_ERRCODE_INVALID_PARAM;
        }
        DC_InvalidateRange(&w9b->status->childBitmap, 2);
        if (!(w9b->status->childBitmap & (1 << aid))) {
            return WM_ERRCODE_NO_CHILD;
        }
    } else if (aid != 0) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    WMi_SetCallbackTable(WM_APIID_DISCONNECT, callback);
    result = WMi_SendCommand(WM_APIID_DISCONNECT, 1, 1 << aid);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}
