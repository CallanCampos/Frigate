#include "dwcutil/dwc_mov_wh.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// The DWC utility's wireless helper for DS Download Play: it measures how busy each channel is, one after the other,
// and keeps the ones that were least busy. It is libdwcutil.TWL.HYB.a's dwc_mov_wh.o, as Black 1's decompilation
// (aletheia) has the same functions in the same order, and they have those SDK names (WHi_MeasureChannel and
// SelectChannel among them), which follow overlay 30's WH_* helper (src/ov030/wih.c) state for state.

static void DWCi_MOV_WH_ChangeSysState(WIHState state);
static void DWCi_MOV_WH_SetError(int errcode);
static BOOL DWCi_MOV_WH_StateInSetParentParam(void);
static void DWCi_MOV_WH_StateOutSetParentParam(void *arg);
static BOOL DWCi_MOV_WH_StateInSetParentWEPKey(void);
static void DWCi_MOV_WH_StateOutSetParentWEPKey(void *arg);
static BOOL DWCi_MOV_WH_StateInStartParent(void);
static void DWCi_MOV_WH_StateOutStartParent(void *arg);
static BOOL DWCi_MOV_WH_StateInStartParentMP(void);
static void DWCi_MOV_WH_StateOutStartParentMP(void *arg);
static BOOL DWCi_MOV_WH_StateInStartParentKeyShare(void);
static BOOL DWCi_MOV_WH_StateInEndParentKeyShare(void);
static BOOL DWCi_MOV_WH_StateInEndParentMP(void);
static void DWCi_MOV_WH_StateOutEndParentMP(void *arg);
static BOOL DWCi_MOV_WH_StateInEndParent(void);
static void DWCi_MOV_WH_StateOutEndParent(void *arg);
static BOOL DWCi_MOV_WH_StateInEndChildKeyShare(void);
static BOOL DWCi_MOV_WH_StateInEndChildMP(void);
static void DWCi_MOV_WH_StateOutEndChildMP(void *arg);
static BOOL DWCi_MOV_WH_StateInEndChild(void);
static void DWCi_MOV_WH_StateOutEndChild(void *arg);
static BOOL DWCi_MOV_WH_StateInReset(void);
static void DWCi_MOV_WH_StateOutReset(void *arg);
static void DWCi_MOV_WH_StateOutEnd(void *arg);
static u16 DWCi_MOV_WH_StateInMeasureChannel(u16 channel);
static void DWCi_MOV_WH_StateOutMeasureChannel(void *arg);
static int WHi_MeasureChannel(WMCallbackFunc callback, u16 channel);
static s16 SelectChannel(u16 bitmap);
static void DWCi_MOV_WH_IndicateHandler(void *arg);
static BOOL DWCi_MOV_WH_StateInInitialize(void);
static void DWCi_MOV_WH_StateOutInitialize(void *arg);

// Takes the work that the parent's DWCi_MOV_MBP_MemInit hands over and starts it out at state 0, with the three words
// at the end of it cleared
void DWCi_MOV_WH_MemInit(void *loaderContext) {
    gWork.work = loaderContext;
    gWork.work->sysState = WIH_STATE_STOP;
    gWork.work->unk_13a8 = 0;
    gWork.work->unk_13ac = NULL;
    gWork.work->unk_13b0 = 0;
}

// Moves to the state given, logging the state it moves from and the one it moves to through the work pointer's log,
// which nothing in this build sets
static void DWCi_MOV_WH_ChangeSysState(WIHState state) {
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c18d4, stateNames[gWork.work->sysState]);
    }
    gWork.work->sysState = state;
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c18dc, stateNames[gWork.work->sysState]);
    }
}

// Keeps the first error, until the state is that of an error
static void DWCi_MOV_WH_SetError(int errcode) {
    if ((u32)(gWork.work->sysState - WIH_STATE_ERROR) > 1) {
        gWork.work->lastError = errcode;
    }
}

// The parent's parameter setup: what runs when the wireless mode starts as a parent
static BOOL DWCi_MOV_WH_StateInSetParentParam(void) {
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = WM_SetParentParameter(DWCi_MOV_WH_StateOutSetParentParam, &gWork.work->sParentParam);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// What the parameter setup calls: sets the WEP key if the parent gave the work one to set, and goes on to the parent
// start if not
static void DWCi_MOV_WH_StateOutSetParentParam(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return;
    }
    if (gWork.work->unk_13ac != NULL) {
        if (!DWCi_MOV_WH_StateInSetParentWEPKey()) {
            DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        }
        return;
    }
    if (!DWCi_MOV_WH_StateInStartParent()) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
    }
}

// Has the parent fill the work's WEP key and sets it, going on to the parent start from its callback
static BOOL DWCi_MOV_WH_StateInSetParentWEPKey(void) {
    u16 wepMode;
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    wepMode = gWork.work->unk_13ac(gWork.work->wepKey, &gWork.work->sParentParam);
    result = WM_SetWEPKey(DWCi_MOV_WH_StateOutSetParentWEPKey, wepMode, gWork.work->wepKey);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// What setting the WEP key calls: goes on to the parent start
static void DWCi_MOV_WH_StateOutSetParentWEPKey(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return;
    }
    if (!DWCi_MOV_WH_StateInStartParent()) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
    }
}

// The parent start. A machine that is already connected, sharing data or sharing keys needs no start
static BOOL DWCi_MOV_WH_StateInStartParent(void) {
    WMErrCode result;

    if (gWork.work->sysState == WIH_STATE_CONNECTED || gWork.work->sysState == WIH_STATE_DATASHARING ||
        gWork.work->sysState == WIH_STATE_KEYSHARING) {
        return TRUE;
    }
    result = WM_StartParent(DWCi_MOV_WH_StateOutStartParent);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    gWork.work->sAid = 0;
    gWork.work->sBitmap = 1;
    return TRUE;
}

// What the parent start calls with each change of who is connected
static void DWCi_MOV_WH_StateOutStartParent(void *arg) {
    WMStartParentCallback *cb = arg;
    u16 aid;
    u16 bit;

    aid = cb->aid;
    bit = 1 << aid;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return;
    }
    switch (cb->state) {
    case WM_STATECODE_CONNECTED:
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c18e0, aid);
        }
        if (gWork.work->sScanFilter != NULL && !gWork.work->sScanFilter(cb)) {
            WMErrCode result = WM_Disconnect(NULL, cb->aid);

            if (result != WM_ERRCODE_OPERATING) {
                DWCi_MOV_WH_SetError(result);
                DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
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
        if (!DWCi_MOV_WH_StateInStartParentMP()) {
            DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
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
static BOOL DWCi_MOV_WH_StateInStartParentMP(void) {
    WMErrCode result;

    if (gWork.work->sysState == WIH_STATE_CONNECTED || gWork.work->sysState == WIH_STATE_DATASHARING ||
        gWork.work->sysState == WIH_STATE_KEYSHARING) {
        return TRUE;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_CONNECTED);
    result = WM_StartMP(DWCi_MOV_WH_StateOutStartParentMP, (u16 *)gWork.work->recvBuf, gWork.work->recvBufSize,
                        (u16 *)gWork.work->sendBuf, gWork.work->sendBufSize, 1);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// What the multi-point start calls
static void DWCi_MOV_WH_StateOutStartParentMP(void *arg) {
    WMStartMPCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return;
    }
    switch (cb->state) {
    case WM_STATECODE_MP_START:
        if (gWork.work->sConnectMode == WIH_CONNECTMODE_KS_PARENT) {
            if (gWork.work->sysState == WIH_STATE_CONNECTED) {
                if (DWCi_MOV_WH_StateInStartParentKeyShare()) {
                    return;
                }
                if (gWork.logPrint != NULL) {
                    gWork.logPrint(0x08000000, data_ov182_021c1958);
                }
                DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
                return;
            }
            if (gWork.work->sysState == WIH_STATE_KEYSHARING) {
                return;
            }
        } else if (gWork.work->sConnectMode == WIH_CONNECTMODE_DS_PARENT) {
            WMErrCode result = WM_StartDataSharing(&gWork.work->dataSharing, 13, 7, 0x44, TRUE);

            if (result != WM_ERRCODE_SUCCESS) {
                DWCi_MOV_WH_SetError(result);
                DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
                return;
            }
            DWCi_MOV_WH_ChangeSysState(WIH_STATE_DATASHARING);
            return;
        }
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_CONNECTED);
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
static BOOL DWCi_MOV_WH_StateInStartParentKeyShare(void) {
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_KEYSHARING);
    result = WM_StartKeySharing(&gWork.work->keySharing, 13);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// The key sharing end
static BOOL DWCi_MOV_WH_StateInEndParentKeyShare(void) {
    WMErrCode result = WM_EndKeySharing(&gWork.work->keySharing);

    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// The end of the parent mode
static BOOL DWCi_MOV_WH_StateInEndParentMP(void) {
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = WM_EndMP(DWCi_MOV_WH_StateOutEndParentMP);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// What the end of the parent mode calls
static void DWCi_MOV_WH_StateOutEndParentMP(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_Reset();
        return;
    }
    if (!DWCi_MOV_WH_StateInEndParent()) {
        if (gWork.logPrint != NULL) {
            gWork.logPrint(0x08000000, data_ov182_021c1988);
        }
        DWCi_MOV_WH_Reset();
    }
}

// The end of the parent itself
static BOOL DWCi_MOV_WH_StateInEndParent(void) {
    WMErrCode result = WM_EndParent(DWCi_MOV_WH_StateOutEndParent);

    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// What the end of the parent calls
static void DWCi_MOV_WH_StateOutEndParent(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        return;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_IDLE);
}

// The key sharing end, only when the helper is sharing keys
static BOOL DWCi_MOV_WH_StateInEndChildKeyShare(void) {
    WMErrCode result;

    if (gWork.work->sysState != WIH_STATE_KEYSHARING) {
        return FALSE;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = WM_EndKeySharing(&gWork.work->keySharing);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// The end of the multi-point mode of a child
static BOOL DWCi_MOV_WH_StateInEndChildMP(void) {
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = WM_EndMP(DWCi_MOV_WH_StateOutEndChildMP);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// What the end of the multi-point mode of a child calls
static void DWCi_MOV_WH_StateOutEndChildMP(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_Finalize();
        return;
    }
    if (!DWCi_MOV_WH_StateInEndChild()) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
    }
}

// The disconnect of a child
static BOOL DWCi_MOV_WH_StateInEndChild(void) {
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = WM_Disconnect(DWCi_MOV_WH_StateOutEndChild, 0);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        DWCi_MOV_WH_Reset();
        return FALSE;
    }
    return TRUE;
}

// What the disconnect of a child calls
static void DWCi_MOV_WH_StateOutEndChild(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        return;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_IDLE);
}

// The wireless reset
static BOOL DWCi_MOV_WH_StateInReset(void) {
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = WM_Reset(DWCi_MOV_WH_StateOutReset);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// What the wireless reset calls
static void DWCi_MOV_WH_StateOutReset(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        DWCi_MOV_WH_SetError(cb->errcode);
        return;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_IDLE);
}

// What the end of the wireless calls: the state that it ends in says whether that succeeded
static void DWCi_MOV_WH_StateOutEnd(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_FATAL);
        return;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_STOP);
}

// WH_SetGgid
void DWCi_MOV_WH_SetGgid(u32 ggid) {
    gWork.work->sParentParam.ggid = ggid;
}

// WH_GetBitmap
u16 DWCi_MOV_WH_GetBitmap(void) {
    return gWork.work->sBitmap;
}

// WH_GetSystemState
int DWCi_MOV_WH_GetSystemState(void) {
    return gWork.work->sysState;
}

// WH_StartMeasureChannel: measures how busy each channel is, one after the other
BOOL DWCi_MOV_WH_StartMeasureChannel(void) {
    u16 mac[3];
    u16 result;

    OS_GetMacAddress((u8 *)mac);
    gWork.work->seed = OS_GetVBlankCount() + mac[0] + mac[1] + mac[2];
    gWork.work->seed = gWork.work->seed * 69069 + 12345;
    gWork.work->channel = 0;
    gWork.work->minBusy = 101;
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = DWCi_MOV_WH_StateInMeasureChannel(1);
    if (result == WIH_ERRCODE_NOMORE_CHANNEL) {
        DWCi_MOV_WH_SetError(WIH_ERRCODE_NOMORE_CHANNEL);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return FALSE;
    }
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// Measures the next channel that is allowed, from the one given. WIH_ERRCODE_NOMORE_CHANNEL once there are none left
static u16 DWCi_MOV_WH_StateInMeasureChannel(u16 channel) {
    u16 allowed = WM_GetAllowedChannel();

    if (allowed == 0x8000) {
        DWCi_MOV_WH_SetError(WM_ERRCODE_ILLEGAL_STATE);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (allowed == 0) {
        DWCi_MOV_WH_SetError(WIH_ERRCODE_NO_RADIO);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
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
    return WHi_MeasureChannel(DWCi_MOV_WH_StateOutMeasureChannel, channel);
}

// What the measure of a channel calls: logs how busy it was, keeps the least busy channel, and goes on to the next one
static void DWCi_MOV_WH_StateOutMeasureChannel(void *arg) {
    WMMeasureChannelCallback *cb = arg;
    u16 channel;
    u16 next;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
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
    next = DWCi_MOV_WH_StateInMeasureChannel(channel + 1);
    if (next == WIH_ERRCODE_NOMORE_CHANNEL) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_MEASURECHANNEL);
    } else if (next != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
    }
}

static int WHi_MeasureChannel(WMCallbackFunc callback, u16 channel) {
    return WM_MeasureChannel(callback, 3, 0x11, channel, 30);
}

// WH_GetMeasureChannel: the channel to use, one of the least busy ones, chosen at random
u16 DWCi_MOV_WH_GetMeasureChannel(void) {
    if (gWork.work->sysState != WIH_STATE_MEASURECHANNEL) {
        OS_Terminate();
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_IDLE);
    gWork.work->channel = SelectChannel(gWork.work->bestBitmap);
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c19f8, gWork.work->channel);
    }
    return gWork.work->channel;
}

static s16 SelectChannel(u16 bitmap) {
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
BOOL DWCi_MOV_WH_Initialize(void) {
    gWork.work->recvBufSize = 0;
    gWork.work->sendBufSize = 0;
    gWork.work->unk_48 = 0;
    gWork.work->sAid = 0;
    gWork.work->sBitmap = 1;
    gWork.work->lastError = WM_ERRCODE_SUCCESS;
    gWork.work->sParentParam.userGameInfo = NULL;
    gWork.work->sParentParam.userGameInfoLength = 0;
    gWork.work->sScanFilter = NULL;
    switch (DWCi_MOV_WH_StateInInitialize()) {
    case FALSE:
        return FALSE;
    default:
        return TRUE;
    }
}

// The wireless indication callback: an error that it cannot recover from ends the game
static void DWCi_MOV_WH_IndicateHandler(void *arg) {
    WMCallback *cb = arg;

    if (cb->errcode == WM_ERRCODE_FIFO_ERROR) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        OS_Terminate();
    }
}

// The wireless start
static BOOL DWCi_MOV_WH_StateInInitialize(void) {
    WMErrCode result;

    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    result = WM_Initialize(gWork.work->wmBuf, DWCi_MOV_WH_StateOutInitialize, 2);
    if (result != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_SetError(result);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_FATAL);
        return FALSE;
    }
    return TRUE;
}

// What the wireless start calls
static void DWCi_MOV_WH_StateOutInitialize(void *arg) {
    WMCallback *cb = arg;
    WMErrCode result;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(cb->errcode);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_FATAL);
        return;
    }
    result = WM_SetIndCallback(DWCi_MOV_WH_IndicateHandler);
    if (result != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(result);
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_FATAL);
        return;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_IDLE);
}

// WH_ParentConnect: starts as a parent, in the mode of the connection (the multi-point mode, the keys or the data
// shared)
BOOL DWCi_MOV_WH_ParentConnect(u16 mode, u16 tgid, u16 channel) {
    if (gWork.work->sysState != WIH_STATE_IDLE) {
        OS_Terminate();
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
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
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
        return DWCi_MOV_WH_StateInSetParentParam();
    }
    if (gWork.logPrint != NULL) {
        gWork.logPrint(0x08000000, data_ov182_021c1a10, mode);
    }
    return FALSE;
}

// WH_SetScanFilter
void DWCi_MOV_WH_SetJudgeAcceptFunc(WHScanFilter filter) {
    gWork.work->sScanFilter = filter;
}

// WH_GetSharedDataAdr: the data of the machine of the AID, from the last step of the data sharing
u8 *DWCi_MOV_WH_GetSharedDataAdr(u16 netId) {
    return (u8 *)WM_GetSharedDataAddress(&gWork.work->dataSharing, &gWork.work->dataSet, netId);
}

// WH_StepDS: shares the data given, for the machines to read the next step
BOOL DWCi_MOV_WH_StepDS(u8 *data) {
    WMErrCode result = WM_StepDataSharing(&gWork.work->dataSharing, (const u16 *)data, &gWork.work->dataSet);

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
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    if (result != WM_ERRCODE_SUCCESS) {
        DWCi_MOV_WH_SetError(result);
        return FALSE;
    }
    return TRUE;
}

// WH_Reset: resets the wireless
void DWCi_MOV_WH_Reset(void) {
    if (!DWCi_MOV_WH_StateInReset()) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_FATAL);
    }
}

// WH_Finalize: ends whatever the machine is doing, back to idle
void DWCi_MOV_WH_Finalize(void) {
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
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
        DWCi_MOV_WH_Reset();
        return;
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    switch (gWork.work->sConnectMode) {
    case WIH_CONNECTMODE_KS_CHILD:
        if (!DWCi_MOV_WH_StateInEndChildKeyShare()) {
            DWCi_MOV_WH_Reset();
        }
        return;
    case WIH_CONNECTMODE_MP_CHILD:
    case WIH_CONNECTMODE_DS_CHILD:
        if (!DWCi_MOV_WH_StateInEndChildMP()) {
            DWCi_MOV_WH_Reset();
        }
        return;
    case WIH_CONNECTMODE_KS_PARENT:
        if (!DWCi_MOV_WH_StateInEndParentKeyShare()) {
            DWCi_MOV_WH_Reset();
        }
        return;
    case WIH_CONNECTMODE_MP_PARENT:
    case WIH_CONNECTMODE_DS_PARENT:
        if (!DWCi_MOV_WH_StateInEndParentMP()) {
            DWCi_MOV_WH_Reset();
        }
        return;
    }
}

// WH_End: ends the wireless
BOOL DWCi_MOV_WH_End(void) {
    if (gWork.work->sysState != WIH_STATE_IDLE) {
        OS_Terminate();
    }
    DWCi_MOV_WH_ChangeSysState(WIH_STATE_BUSY);
    if (WM_End(DWCi_MOV_WH_StateOutEnd) != WM_ERRCODE_OPERATING) {
        DWCi_MOV_WH_ChangeSysState(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}
