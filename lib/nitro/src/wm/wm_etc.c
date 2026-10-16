// The rest of the wireless manager's calls (libwm, TwlSDK's wm/ARM9/src/wm_etc.c, by its place in the library: the
// file name is a guess): the WEP key, the game's information in the beacon, the beacon indication, the life times of
// the connections and the measure of a channel. Each checks the state and the arguments, registers the callback and
// sends the command to the ARM7. The public functions have the SDK's names, from what
// they do; the static ones' names are ours
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
} WmMeasureChannelCommand;

// The game's information in the beacon is copied here, where the ARM7 reads it. The cache is cleaned in whole lines of
// 32 bytes, so the buffer's size is WM_SIZE_USER_GAMEINFO (0x70) rounded up
static u8 tmpUserGameInfoBuf[0x80] ATTRIBUTE_ALIGN(32);

int WM_SetWEPKey(WMCallbackFunc callback, u16 wepMode, const void *wepKey) {
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
    WMi_SetCallbackTable(WM_APIID_SET_WEP_KEY, callback);
    result = WMi_SendCommand(WM_APIID_SET_WEP_KEY, 2, wepMode, wepKey);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_SetWEPKeyEx(WMCallbackFunc callback, u16 wepMode, u16 wepKeyId, const void *wepKey) {
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
    WMi_SetCallbackTable(WM_APIID_SET_WEP_KEY_EX, callback);
    result = WMi_SendCommand(WM_APIID_SET_WEP_KEY_EX, 3, wepMode, wepKey, wepKeyId);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// The information is at most WM_SIZE_USER_GAMEINFO bytes
int WM_SetGameInfo(WMCallbackFunc callback, const void *userGameInfo, u16 gameInfoSize, u32 ggid, u16 tgid, u8 attr) {
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
    WMi_SetCallbackTable(WM_APIID_SET_GAME_INFO, callback);
    result = WMi_SendCommand(WM_APIID_SET_GAME_INFO, 5, tmpUserGameInfoBuf, gameInfoSize, ggid, tgid, attr);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// Enable is 0 or 1
int WM_SetBeaconIndication(WMCallbackFunc callback, int enable) {
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

int WM_SetLifeTime(WMCallbackFunc callback, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime, u16 mpLifeTime) {
    int result = WMi_CheckIdle();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    WMi_SetCallbackTable(WM_APIID_SET_LIFE_TIME, callback);
    result = WMi_SendCommand(WM_APIID_SET_LIFE_TIME, 4, tableNumber, camLifeTime, frameLifeTime, mpLifeTime);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_MeasureChannel(WMCallbackFunc callback, u16 ccaMode, u16 edThreshold, u16 channel, u16 measureTime) {
    int result;
    WmMeasureChannelCommand cmd;

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
