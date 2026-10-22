// The rest of the wireless manager's calls (libwm, TwlSDK's wm/ARM9/src/wm_etc.c, by its place in the library; ntrtwl's
// reconstruction of NitroSDK has the same file): the WEP key, the game's information in the beacon, the beacon
// indication, the life times of the connections and the measure of a channel. Each checks the state and the arguments,
// registers the callback and sends the command to the ARM7. The names are NitroSDK's, as that reconstruction has them
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// What it sends the ARM7 to measure a channel. Its size is 10
typedef struct {
    u16 apiid;
    u16 ccaMode;
    u16 edThreshold;
    u16 channel;
    u16 measureTime;
} WMMeasureChannelReq;

// The game's information in the beacon is copied here, where the ARM7 reads it. The cache is cleaned in whole lines of
// 32 bytes, so the buffer's size is WM_SIZE_USER_GAMEINFO (0x70) rounded up
static u8 tmpUserGameInfoBuf[0x80] ATTRIBUTE_ALIGN(32);

WMErrCode WM_SetWEPKey(WMCallbackFunc callback, u16 wepMode, const u16 *wepKey) {
    int result = WMi_CheckIdle();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (wepMode > 3) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (wepMode != 0) {
        if (wepKey == NULL) {
            return WM_ERRCODE_INVALID_PARAM;
        }
        DC_StoreRange(wepKey, 0x50);
    }
    WMi_SetCallbackTable(WM_APIID_SET_WEPKEY, callback);
    result = WMi_SendCommand(WM_APIID_SET_WEPKEY, 2, wepMode, wepKey);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

WMErrCode WM_SetWEPKeyEx(WMCallbackFunc callback, u16 wepMode, u16 wepKeyId, const u8 *wepKey) {
    int result = WMi_CheckIdle();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (wepMode > 3) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (wepMode != 0) {
        if (wepKey == NULL) {
            return WM_ERRCODE_INVALID_PARAM;
        }
        DC_StoreRange(wepKey, 0x50);
    }
    WMi_SetCallbackTable(WM_APIID_SET_WEPKEY_EX, callback);
    result = WMi_SendCommand(WM_APIID_SET_WEPKEY_EX, 3, wepMode, wepKey, wepKeyId);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// The information is at most WM_SIZE_USER_GAMEINFO bytes
WMErrCode WM_SetGameInfo(WMCallbackFunc callback, const u16 *userGameInfo, u16 gameInfoSize, u32 ggid, u16 tgid,
                         u8 attr) {
    int result = WMi_CheckStateEx(2, WM_STATE_PARENT, WM_STATE_MP_PARENT);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (!userGameInfo) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (gameInfoSize > WM_SIZE_USER_GAMEINFO) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    MI_CpuCopy16(userGameInfo, tmpUserGameInfoBuf, gameInfoSize);
    DC_StoreRange(tmpUserGameInfoBuf, gameInfoSize);
    WMi_SetCallbackTable(WM_APIID_SET_GAMEINFO, callback);
    result = WMi_SendCommand(WM_APIID_SET_GAMEINFO, 5, tmpUserGameInfoBuf, gameInfoSize, ggid, tgid, attr);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// Enable is 0 or 1
WMErrCode WM_SetBeaconIndication(WMCallbackFunc callback, u16 enable) {
    int result = WMi_CheckIdle();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (enable != 0 && enable != 1) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    WMi_SetCallbackTable(WM_APIID_SET_BEACON_IND, callback);
    result = WMi_SendCommand(WM_APIID_SET_BEACON_IND, 1, enable);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

WMErrCode WM_SetLifeTime(WMCallbackFunc callback, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime, u16 mpLifeTime) {
    int result = WMi_CheckIdle();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_SET_LIFETIME, callback);
    result = WMi_SendCommand(WM_APIID_SET_LIFETIME, 4, tableNumber, camLifeTime, frameLifeTime, mpLifeTime);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

WMErrCode WM_MeasureChannel(WMCallbackFunc callback, u16 ccaMode, u16 edThreshold, u16 channel, u16 measureTime) {
    int result;
    WMMeasureChannelReq cmd;

    // The SDK fetches the system work here and doesn't use it
    WMi_GetSystemWork();
    result = WMi_CheckStateEx(1, WM_STATE_IDLE);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_MEASURE_CHANNEL, callback);
    cmd.apiid = WM_APIID_MEASURE_CHANNEL;
    cmd.ccaMode = ccaMode;
    cmd.edThreshold = edThreshold;
    cmd.channel = channel;
    cmd.measureTime = measureTime;
    result = WMi_SendCommandDirect(&cmd, sizeof(cmd));
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}
