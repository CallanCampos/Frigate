// The rest of the wireless manager's calls (libwm, TwlSDK's wm/ARM9/src/wm_etc.c, by its place in the library: the
// file name is a guess): the WEP key, the game's information in the beacon, the beacon indication, the life times of
// the connections and the measure of a channel. Each checks the state and the arguments, registers the callback and
// sends the command to the ARM7. The names are descriptive
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

// WM_SetWEPKey
int func_02082424(WMCallbackFunc callback, u16 wepMode, const void *wepKey) {
    int result = func_02080a84();

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
        cp15_cleanDC(wepKey, 0x50);
    }
    func_02080950(WM_APIID_SET_WEP_KEY, callback);
    result = WMi_SendCommand(WM_APIID_SET_WEP_KEY, 2, wepMode, wepKey);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_SetWEPKeyEx
int func_02082470(WMCallbackFunc callback, u16 wepMode, u16 wepKeyId, const void *wepKey) {
    int result = func_02080a84();

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
        cp15_cleanDC(wepKey, 0x50);
    }
    func_02080950(WM_APIID_SET_WEP_KEY_EX, callback);
    result = WMi_SendCommand(WM_APIID_SET_WEP_KEY_EX, 3, wepMode, wepKey, wepKeyId);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_SetGameInfo: the information is at most WM_SIZE_USER_GAMEINFO bytes
int func_020824bc(WMCallbackFunc callback, const void *userGameInfo, u16 gameInfoSize, u32 ggid, u16 tgid, u8 attr) {
    int result = func_02080ab0(2, WM_STATE_PARENT, WM_STATE_MP_PARENT);

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
    cp15_cleanDC(tmpUserGameInfoBuf, gameInfoSize);
    func_02080950(WM_APIID_SET_GAME_INFO, callback);
    result = WMi_SendCommand(WM_APIID_SET_GAME_INFO, 5, tmpUserGameInfoBuf, gameInfoSize, ggid, tgid, attr);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_SetBeaconIndication: enable is 0 or 1
int func_0208252c(WMCallbackFunc callback, int enable) {
    int result = func_02080a84();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (enable != 0 && enable != 1) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    func_02080950(WM_APIID_SET_BEACON_IND, callback);
    result = WMi_SendCommand(WM_APIID_SET_BEACON_IND, 1, enable);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_SetLifeTime
int func_02082560(WMCallbackFunc callback, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime, u16 mpLifeTime) {
    int result = func_02080a84();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_SET_LIFE_TIME, callback);
    result = WMi_SendCommand(WM_APIID_SET_LIFE_TIME, 4, tableNumber, camLifeTime, frameLifeTime, mpLifeTime);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_MeasureChannel
int func_0208259c(WMCallbackFunc callback, u16 ccaMode, u16 edThreshold, u16 channel, u16 measureTime) {
    int result;
    WmMeasureChannelCommand cmd;

    func_02080a64();
    result = func_02080ab0(1, WM_STATE_IDLE);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    func_02080950(WM_APIID_MEASURE_CHANNEL, callback);
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
