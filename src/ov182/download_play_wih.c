#include "app/download_play_wih.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// download_play_wih.c is the wireless helper of the DS Download Play range of overlay 182: it measures how busy each
// channel is, one after the other, and keeps the ones that were least busy

static void func_ov182_021b9be4(int state);
static void func_ov182_021b9c34(int errcode);
static BOOL func_ov182_021b9c48(void);
static void func_ov182_021b9c78(void *arg);
static BOOL func_ov182_021b9cc0(void);
static void func_ov182_021b9d04(void *arg);
static BOOL func_ov182_021b9d28(void);
static void func_ov182_021b9d68(void *arg);
static BOOL func_ov182_021b9e48(void);
static void func_ov182_021b9eb0(void *arg);
static BOOL func_ov182_021b9f80(void);
static BOOL func_ov182_021b9fac(void);
static BOOL func_ov182_021b9fd0(void);
static void func_ov182_021b9ff4(void *arg);
static BOOL func_ov182_021ba02c(void);
static void func_ov182_021ba048(void *arg);
static BOOL func_ov182_021ba060(void);
static BOOL func_ov182_021ba098(void);
static void func_ov182_021ba0bc(void *arg);
static BOOL func_ov182_021ba0e0(void);
static void func_ov182_021ba10c(void *arg);
static BOOL func_ov182_021ba124(void);
static void func_ov182_021ba148(void *arg);
static void func_ov182_021ba168(void *arg);
static u16 func_ov182_021ba238(u16 channel);
static void func_ov182_021ba2a8(void *arg);
static int func_ov182_021ba33c(WMCallbackFunc callback, u16 channel);
static s16 func_ov182_021ba3a0(u16 bitmap);
static void func_ov182_021ba478(void *arg);
static BOOL func_ov182_021ba48c(void);
static void func_ov182_021ba4c0(void *arg);

// Takes the work that the parent's DownloadPlay_SetWork hands over and starts it out at state 0, with the three words
// at the end of it cleared
void func_ov182_021b9bbc(void *loaderContext) {
    gWork.work = loaderContext;
    gWork.work->sysState = WIH_STATE_STOP;
    gWork.work->unk_13a8 = 0;
    gWork.work->unk_13ac = NULL;
    gWork.work->unk_13b0 = 0;
}

// Moves to the state given, logging the state it moves from and the one it moves to through the work pointer's log,
// which nothing in this build sets
static void func_ov182_021b9be4(int state) {
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c18d4, stateNames[gWork.work->sysState]);
    }
    gWork.work->sysState = state;
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c18dc, stateNames[gWork.work->sysState]);
    }
}

// Keeps the first error, until the state is that of an error
static void func_ov182_021b9c34(int errcode) {
    if ((u32)(gWork.work->sysState - WIH_STATE_ERROR) > 1) {
        gWork.work->lastError = errcode;
    }
}

// The parent's parameter setup: what runs when the wireless mode starts as a parent
static BOOL func_ov182_021b9c48(void) {
    int result;

    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = WM_SetParentParameter(func_ov182_021b9c78, &gWork.work->sParentParam);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// What the parameter setup calls: sets the WEP key if the parent gave the work one to set, and goes on to the parent
// start if not
static void func_ov182_021b9c78(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return;
    }
    if (gWork.work->unk_13ac != NULL) {
        if (!func_ov182_021b9cc0()) {
            func_ov182_021b9be4(WIH_STATE_ERROR);
        }
        return;
    }
    if (!func_ov182_021b9d28()) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
    }
}

// Has the parent fill the work's WEP key and sets it, going on to the parent start from its callback
static BOOL func_ov182_021b9cc0(void) {
    u16 wepMode;
    int result;

    func_ov182_021b9be4(WIH_STATE_BUSY);
    wepMode = gWork.work->unk_13ac(gWork.work->wepKey);
    result = WM_SetWEPKey(func_ov182_021b9d04, wepMode, gWork.work->wepKey);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// What setting the WEP key calls: goes on to the parent start
static void func_ov182_021b9d04(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return;
    }
    if (!func_ov182_021b9d28()) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
    }
}

// The parent start. A machine that is already connected, sharing data or sharing keys needs no start
static BOOL func_ov182_021b9d28(void) {
    int result;

    if (gWork.work->sysState == WIH_STATE_CONNECTED || gWork.work->sysState == WIH_STATE_DATASHARING ||
        gWork.work->sysState == WIH_STATE_KEYSHARING) {
        return TRUE;
    }
    result = WM_StartParent(func_ov182_021b9d68);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    gWork.work->sAid = 0;
    gWork.work->sBitmap = 1;
    return TRUE;
}

// What the parent start calls with each change of who is connected
static void func_ov182_021b9d68(void *arg) {
    WMStartParentCallback *cb = arg;
    u16 aid;
    u16 bit;

    aid = cb->aid;
    bit = 1 << aid;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return;
    }
    switch (cb->state) {
    case WM_STATECODE_CONNECTED:
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c18e0, aid);
        }
        if (gWork.work->sScanFilter != NULL && !gWork.work->sScanFilter(cb)) {
            int result = WM_Disconnect(NULL, cb->aid);

            if (result != WM_ERRCODE_OPERATING) {
                func_ov182_021b9c34(result);
                func_ov182_021b9be4(WIH_STATE_ERROR);
            }
            return;
        }
        gWork.work->sBitmap |= bit;
        return;
    case WM_STATECODE_DISCONNECTED:
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c190c, aid);
        }
        gWork.work->sBitmap &= ~bit;
        return;
    case WM_STATECODE_PARENT_START:
        if (!func_ov182_021b9e48()) {
            func_ov182_021b9be4(WIH_STATE_ERROR);
        }
        return;
    case WM_STATECODE_BEACON_SENT:
        return;
    }
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c1938, cb->state);
    }
}

// The multi-point parent start
static BOOL func_ov182_021b9e48(void) {
    int result;

    if (gWork.work->sysState == WIH_STATE_CONNECTED || gWork.work->sysState == WIH_STATE_DATASHARING ||
        gWork.work->sysState == WIH_STATE_KEYSHARING) {
        return TRUE;
    }
    func_ov182_021b9be4(WIH_STATE_CONNECTED);
    result = WM_StartMP(func_ov182_021b9eb0, (u16 *)gWork.work->recvBuf, gWork.work->recvBufSize,
                        (u16 *)gWork.work->sendBuf, gWork.work->sendBufSize, 1);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// What the multi-point start calls
static void func_ov182_021b9eb0(void *arg) {
    WMStartMPCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return;
    }
    switch (cb->state) {
    case WM_STATECODE_MP_START:
        if (gWork.work->sConnectMode == WIH_CONNECTMODE_KS_PARENT) {
            if (gWork.work->sysState == WIH_STATE_CONNECTED) {
                if (func_ov182_021b9f80()) {
                    return;
                }
                if (gWork.logPrint != NULL) {
                    gWork.logPrint(0x08000000, data_ov182_021c1958);
                }
                func_ov182_021b9be4(WIH_STATE_ERROR);
                return;
            }
            if (gWork.work->sysState == WIH_STATE_KEYSHARING) {
                return;
            }
        } else if (gWork.work->sConnectMode == WIH_CONNECTMODE_DS_PARENT) {
            int result = WM_StartDataSharing(&gWork.work->dataSharing, 13, 7, 0x44, TRUE);

            if (result != WM_ERRCODE_SUCCESS) {
                func_ov182_021b9c34(result);
                func_ov182_021b9be4(WIH_STATE_ERROR);
                return;
            }
            func_ov182_021b9be4(WIH_STATE_DATASHARING);
            return;
        }
        func_ov182_021b9be4(WIH_STATE_CONNECTED);
        return;
    case WM_STATECODE_MPEND_IND:
        break;
    case WM_STATECODE_MP_IND:
    case WM_STATECODE_MPACK_IND:
    default:
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c1938, cb->state);
        }
        break;
    }
}

// The key sharing start, which this build's parent path has no call for
static BOOL func_ov182_021b9f80(void) {
    int result;

    func_ov182_021b9be4(WIH_STATE_KEYSHARING);
    result = WM_StartKeySharing(&gWork.work->keySharing, 13);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// The key sharing end
static BOOL func_ov182_021b9fac(void) {
    int result = WM_EndKeySharing(&gWork.work->keySharing);

    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// The end of the parent mode
static BOOL func_ov182_021b9fd0(void) {
    int result;

    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = WM_EndMP(func_ov182_021b9ff4);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// What the end of the parent mode calls
static void func_ov182_021b9ff4(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021ba668();
        return;
    }
    if (!func_ov182_021ba02c()) {
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c1988);
        }
        func_ov182_021ba668();
    }
}

// The end of the parent itself
static BOOL func_ov182_021ba02c(void) {
    int result = WM_EndParent(func_ov182_021ba048);

    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// What the end of the parent calls
static void func_ov182_021ba048(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        return;
    }
    func_ov182_021b9be4(WIH_STATE_IDLE);
}

// The key sharing end, only when the helper is sharing keys
static BOOL func_ov182_021ba060(void) {
    int result;

    if (gWork.work->sysState != WIH_STATE_KEYSHARING) {
        return FALSE;
    }
    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = WM_EndKeySharing(&gWork.work->keySharing);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// The end of the multi-point mode of a child
static BOOL func_ov182_021ba098(void) {
    int result;

    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = WM_EndMP(func_ov182_021ba0bc);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// What the end of the multi-point mode of a child calls
static void func_ov182_021ba0bc(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021ba67c();
        return;
    }
    if (!func_ov182_021ba0e0()) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
    }
}

// The disconnect of a child
static BOOL func_ov182_021ba0e0(void) {
    int result;

    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = WM_Disconnect(func_ov182_021ba10c, 0);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        func_ov182_021ba668();
        return FALSE;
    }
    return TRUE;
}

// What the disconnect of a child calls
static void func_ov182_021ba10c(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        return;
    }
    func_ov182_021b9be4(WIH_STATE_IDLE);
}

// The wireless reset
static BOOL func_ov182_021ba124(void) {
    int result;

    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = WM_Reset(func_ov182_021ba148);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// What the wireless reset calls
static void func_ov182_021ba148(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
        func_ov182_021b9c34(cb->errcode);
        return;
    }
    func_ov182_021b9be4(WIH_STATE_IDLE);
}

// What the end of the wireless calls: the state that it ends in says whether that succeeded
static void func_ov182_021ba168(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9be4(WIH_STATE_FATAL);
        return;
    }
    func_ov182_021b9be4(WIH_STATE_STOP);
}

// WH_SetGgid
void func_ov182_021ba180(u32 ggid) {
    gWork.work->sParentParam.ggid = ggid;
}

// WH_GetBitmap
u16 func_ov182_021ba18c(void) {
    return gWork.work->sBitmap;
}

// WH_GetSystemState
int func_ov182_021ba19c(void) {
    return gWork.work->sysState;
}

// WH_StartMeasureChannel: measures how busy each channel is, one after the other
BOOL func_ov182_021ba1a8(void) {
    u16 mac[3];
    u16 result;

    OS_GetMacAddress((u8 *)mac);
    gWork.work->seed = OS_GetVBlankCount() + mac[0] + mac[1] + mac[2];
    gWork.work->seed = gWork.work->seed * 69069 + 12345;
    gWork.work->channel = 0;
    gWork.work->minBusy = 101;
    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = func_ov182_021ba238(1);
    if (result == WIH_ERRCODE_NOMORE_CHANNEL) {
        func_ov182_021b9c34(WIH_ERRCODE_NOMORE_CHANNEL);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return FALSE;
    }
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// Measures the next channel that is allowed, from the one given. WIH_ERRCODE_NOMORE_CHANNEL once there are none left
static u16 func_ov182_021ba238(u16 channel) {
    u16 allowed = WM_GetAllowedChannel();

    if (allowed == 0x8000) {
        func_ov182_021b9c34(WM_ERRCODE_ILLEGAL_STATE);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (allowed == 0) {
        func_ov182_021b9c34(WIH_ERRCODE_NO_RADIO);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return WIH_ERRCODE_NOMORE_CHANNEL;
    }
    if (!((1 << (channel - 1)) & allowed)) {
        do {
            channel++;
            if (channel > 16) {
                return WIH_ERRCODE_NOMORE_CHANNEL;
            }
        } while (!((1 << (channel - 1)) & allowed));
    }
    return func_ov182_021ba33c(func_ov182_021ba2a8, channel);
}

// What the measure of a channel calls: logs how busy it was, keeps the least busy channel, and goes on to the next one
static void func_ov182_021ba2a8(void *arg) {
    WMMeasureChannelCallback *cb = arg;
    u16 channel;
    u16 next;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return;
    }
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c19e0, cb->channel, cb->ccaBusyRatio);
    }
    channel = cb->channel;
    if (gWork.work->minBusy > cb->ccaBusyRatio) {
        gWork.work->minBusy = cb->ccaBusyRatio;
        gWork.work->bestBitmap = 1 << (channel - 1);
    } else if (gWork.work->minBusy == cb->ccaBusyRatio) {
        gWork.work->bestBitmap |= 1 << (channel - 1);
    }
    next = func_ov182_021ba238(channel + 1);
    if (next == WIH_ERRCODE_NOMORE_CHANNEL) {
        func_ov182_021b9be4(WIH_STATE_MEASURECHANNEL);
    } else if (next != WM_ERRCODE_OPERATING) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
    }
}

static int func_ov182_021ba33c(WMCallbackFunc callback, u16 channel) {
    return WM_MeasureChannel(callback, 3, 0x11, channel, 30);
}

// WH_GetMeasureChannel: the channel to use, one of the least busy ones, chosen at random
u16 func_ov182_021ba350(void) {
    if (gWork.work->sysState != WIH_STATE_MEASURECHANNEL) {
        OS_Terminate();
    }
    func_ov182_021b9be4(WIH_STATE_IDLE);
    gWork.work->channel = func_ov182_021ba3a0(gWork.work->bestBitmap);
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c19f8, gWork.work->channel);
    }
    return gWork.work->channel;
}

static s16 func_ov182_021ba3a0(u16 bitmap) {
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
        gWork.work->seed = gWork.work->seed * 69069 + 12345;
        pick = (count * (u8)gWork.work->seed) >> 8;
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

// WH_Initialize: starts the wireless, which the parent has already given the work to
BOOL func_ov182_021ba42c(void) {
    gWork.work->recvBufSize = 0;
    gWork.work->sendBufSize = 0;
    gWork.work->unk_48 = 0;
    gWork.work->sAid = 0;
    gWork.work->sBitmap = 1;
    gWork.work->lastError = WM_ERRCODE_SUCCESS;
    gWork.work->sParentParam.userGameInfo = NULL;
    gWork.work->sParentParam.userGameInfoLength = 0;
    gWork.work->sScanFilter = NULL;
    if (func_ov182_021ba48c()) {
        return TRUE;
    }
    return FALSE;
}

// The wireless indication callback: an error that it cannot recover from ends the game
static void func_ov182_021ba478(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode == WM_ERRCODE_FIFO_ERROR) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
        OS_Terminate();
    }
}

// The wireless start
static BOOL func_ov182_021ba48c(void) {
    int result;

    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = WM_Initialize(gWork.work->wmBuf, func_ov182_021ba4c0, 2);
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        func_ov182_021b9be4(WIH_STATE_FATAL);
        return FALSE;
    }
    return TRUE;
}

// What the wireless start calls
static void func_ov182_021ba4c0(void *arg) {
    WMCallback *cb = arg;
    int result;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021b9be4(WIH_STATE_FATAL);
        return;
    }
    result = WM_SetIndCallback(func_ov182_021ba478);
    if (result != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(result);
        func_ov182_021b9be4(WIH_STATE_FATAL);
        return;
    }
    func_ov182_021b9be4(WIH_STATE_IDLE);
}

// WH_ParentConnect: starts as a parent, in the mode of the connection (the multi-point mode, the keys or the data
// shared)
BOOL func_ov182_021ba4f8(u16 mode, u16 tgid, u16 channel) {
    if (gWork.work->sysState != WIH_STATE_IDLE) {
        OS_Terminate();
        return FALSE;
    }
    gWork.work->recvBufSize = 0x180;
    gWork.work->sendBufSize = 0xe0;
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c19b0, gWork.work->recvBufSize);
    }
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c19c8, gWork.work->sendBufSize);
    }
    gWork.work->sConnectMode = mode;
    func_ov182_021b9be4(WIH_STATE_BUSY);
    gWork.work->sParentParam.tgid = tgid;
    gWork.work->sParentParam.channel = channel;
    gWork.work->sParentParam.beaconPeriod = WM_GetDispersionBeaconPeriod();
    gWork.work->sParentParam.parentMaxSize = 0xd0;
    gWork.work->sParentParam.childMaxSize = 0x44;
    gWork.work->sParentParam.maxEntry = 2;
    gWork.work->sParentParam.CS_Flag = 0;
    gWork.work->sParentParam.multiBootFlag = 0;
    gWork.work->sParentParam.entryFlag = 1;
    gWork.work->sParentParam.KS_Flag = mode == WIH_CONNECTMODE_KS_PARENT;
    if (mode == WIH_CONNECTMODE_MP_PARENT || mode == WIH_CONNECTMODE_KS_PARENT || mode == WIH_CONNECTMODE_DS_PARENT) {
        return func_ov182_021b9c48();
    }
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c1a10, mode);
    }
    return FALSE;
}

// WH_SetScanFilter
void func_ov182_021ba5d0(WHScanFilter filter) {
    gWork.work->sScanFilter = filter;
}

// WH_GetSharedDataAdr: the data of the machine of the AID, from the last step of the data sharing
u8 *func_ov182_021ba5dc(u16 netId) {
    return (u8 *)WM_GetSharedDataAddress(&gWork.work->dataSharing, &gWork.work->dataSet, netId);
}

// WH_StepDS: shares the data given, for the machines to read the next step
BOOL func_ov182_021ba5fc(u8 *data) {
    int result = WM_StepDataSharing(&gWork.work->dataSharing, (const u16 *)data, &gWork.work->dataSet);

    if (result == WM_ERRCODE_NO_CHILD) {
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c1a2c);
        }
        return FALSE;
    }
    if (result == WM_ERRCODE_NO_DATASET) {
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c1a5c);
        }
        func_ov182_021b9c34(result);
        return FALSE;
    }
    if (result != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(result);
        return FALSE;
    }
    return TRUE;
}

// WH_Reset: resets the wireless
void func_ov182_021ba668(void) {
    if (!func_ov182_021ba124()) {
        func_ov182_021b9be4(WIH_STATE_FATAL);
    }
}

// WH_Finalize: ends whatever the machine is doing, back to idle
void func_ov182_021ba67c(void) {
    if (gWork.work->sysState == WIH_STATE_IDLE) {
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c1a90);
        }
        return;
    }
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c1ab4, gWork.work->sysState);
    }
    if (gWork.work->sysState != WIH_STATE_KEYSHARING && gWork.work->sysState != WIH_STATE_DATASHARING &&
        gWork.work->sysState != WIH_STATE_CONNECTED) {
        func_ov182_021b9be4(WIH_STATE_BUSY);
        func_ov182_021ba668();
        return;
    }
    func_ov182_021b9be4(WIH_STATE_BUSY);
    switch (gWork.work->sConnectMode) {
    case WIH_CONNECTMODE_KS_CHILD:
        if (!func_ov182_021ba060()) {
            func_ov182_021ba668();
        }
        return;
    case WIH_CONNECTMODE_MP_CHILD:
    case WIH_CONNECTMODE_DS_CHILD:
        if (!func_ov182_021ba098()) {
            func_ov182_021ba668();
        }
        return;
    case WIH_CONNECTMODE_KS_PARENT:
        if (!func_ov182_021b9fac()) {
            func_ov182_021ba668();
        }
        return;
    case WIH_CONNECTMODE_MP_PARENT:
    case WIH_CONNECTMODE_DS_PARENT:
        if (!func_ov182_021b9fd0()) {
            func_ov182_021ba668();
        }
        return;
    }
}

// WH_End: ends the wireless
BOOL func_ov182_021ba730(void) {
    if (gWork.work->sysState != WIH_STATE_IDLE) {
        OS_Terminate();
    }
    func_ov182_021b9be4(WIH_STATE_BUSY);
    if (WM_End(func_ov182_021ba168) != WM_ERRCODE_OPERATING) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}
