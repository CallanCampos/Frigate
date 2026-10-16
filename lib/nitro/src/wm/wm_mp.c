// The wireless manager's multi-point mode (libwm, TwlSDK's wm/ARM9/src/wm_mp.c, by its place in the library: the file
// name is a guess): the start of the mode, in a round of which a parent sends to all of its children and each child
// answers, the data that a port sends in it, and its end. The public functions have the SDK's names, from what
// they do; the static ones' names are ours
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// What the ARM7 takes of the multi-point mode's parameters: the bits of the mask say which ones. Its size is 0x10
typedef struct {
    u32 mask;
    u16 parentFreq;
    u16 childFreq;
    u16 fixedFreq;
    u16 defaultRetryCount;
    u8 minPollBmpMode;
    u8 singlePacketMode;
    u8 ignoreFatalError;
    u8 unk_0f;
} WmMpParam;

#define WM_MP_PARAM_MASK_FREQ 0x0003          // parentFreq and childFreq
#define WM_MP_PARAM_MASK_FIXED_FREQ 0x0004    // fixedFreq
#define WM_MP_PARAM_MASK_ALL_BUT_FIXED 0x1e03 // the frequencies and the four more that follow fixedFreq

// What the manager sends the ARM7 to start the mode. Its size is 0x40; the buffers are given by their addresses and
// sizes, the receive buffer's in halfwords
typedef struct {
    u16 apiid;
    u16 unk_02;
    void *recvBuf;
    u32 recvBufHalfwords;
    void *sendBuf;
    u32 sendBufSize;
    u8 unk_14[0x1c];
    WmMpParam param;
} WmStartMpCommand;

static int WMi_StartMP(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize, void *sendBuf, u16 sendBufSize,
                       const WmMpParam *mpParam);

// WMi_StartMP: checks the state, that a child may start the mode and that the buffers are the right sizes, and sends
// the command
static int WMi_StartMP(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize, void *sendBuf, u16 sendBufSize,
                       const WmMpParam *mpParam) {
    WmStartMpCommand cmd;
    WMStatus *status = WMi_GetSystemWork()->status;
    int result = WMi_CheckStateEx(2, WM_STATE_PARENT, WM_STATE_CHILD);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(&status->aid, 2);
    DC_InvalidateRange(&status->childMpFlag, 2);
    if (status->aid != 0 && status->childMpFlag != 1) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    DC_InvalidateRange(&status->mpFlag, 4);
    if (status->mpFlag == TRUE) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (recvBufSize & 0x3f) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (sendBufSize & 0x1f) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    DC_InvalidateRange(&status->mpSizeCheckOff, 2);
    if (status->mpSizeCheckOff == 0) {
        if (recvBufSize < WM_GetMPReceiveBufferSize()) {
            return WM_ERRCODE_INVALID_PARAM;
        }
        if (sendBufSize < WM_GetMPSendBufferSize()) {
            return WM_ERRCODE_INVALID_PARAM;
        }
    }

    WMi_SetCallbackTable(WM_APIID_START_MP, callback);
    MIi_CpuClear32(0, &cmd, sizeof(cmd));
    cmd.apiid = WM_APIID_START_MP;
    cmd.recvBufHalfwords = recvBufSize / 2;
    cmd.recvBuf = recvBuf;
    cmd.sendBuf = sendBuf;
    cmd.sendBufSize = sendBufSize;
    MIi_CpuClear32(0, cmd.unk_14, sizeof(cmd.unk_14));
    MIi_CpuCopy32(mpParam, &cmd.param, sizeof(cmd.param));
    result = WMi_SendCommandDirect(&cmd, sizeof(cmd));
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_StartMPEx: the same frequency for the parent and the children, and the retries, the polling and the fatal errors
// as the game wants them. A frequency that is fixed is also given to the ARM7 as such
int WM_StartMPEx(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize, void *sendBuf, u16 sendBufSize, u16 mpFreq,
                 u16 defaultRetryCount, BOOL minPollBmpMode, BOOL singlePacketMode, BOOL fixFreqMode,
                 BOOL ignoreFatalError) {
    WmMpParam param;

    MIi_CpuClear32(0, &param, sizeof(param));
    param.mask = WM_MP_PARAM_MASK_ALL_BUT_FIXED;
    param.parentFreq = mpFreq;
    param.childFreq = mpFreq;
    param.defaultRetryCount = defaultRetryCount;
    param.minPollBmpMode = minPollBmpMode;
    param.singlePacketMode = singlePacketMode;
    param.ignoreFatalError = ignoreFatalError;
    if (fixFreqMode && mpFreq) {
        param.mask |= WM_MP_PARAM_MASK_FIXED_FREQ;
        param.fixedFreq = mpFreq;
    }
    return WMi_StartMP(callback, recvBuf, recvBufSize, sendBuf, sendBufSize, &param);
}

int WM_StartMP(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize, void *sendBuf, u16 sendBufSize, u16 mpFreq) {
    WmMpParam param;

    MIi_CpuClear32(0, &param, sizeof(param));
    param.mask = WM_MP_PARAM_MASK_FREQ;
    param.parentFreq = mpFreq;
    param.childFreq = mpFreq;
    return WMi_StartMP(callback, recvBuf, recvBufSize, sendBuf, sendBufSize, &param);
}

// WM_SetMPDataToPortEx: sends the data to the ports of the children in the bitmap in the next round; the callback and
// its argument come back with the result
int WM_SetMPDataToPortEx(WMCallbackFunc callback, void *arg, const void *sendData, u16 sendDataSize, u16 destBitmap,
                         u16 port, u16 prio) {
    u16 childBitmap = 1;
    WMStatus *status = WMi_GetSystemWork()->status;
    int result = WMi_CheckStateEx(2, WM_STATE_MP_PARENT, WM_STATE_MP_CHILD);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(&status->aid, 2);
    if (status->aid == 0) {
        DC_InvalidateRange(&status->childBitmap, 2);
        childBitmap = status->childBitmap;
        DC_InvalidateRange(&status->unk_086, 2);
    }
    if (sendData == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (childBitmap == 0) {
        return WM_ERRCODE_NO_CHILD;
    }
    DC_InvalidateRange(&status->mpSendBuf, 2);
    if (sendData == status->mpSendBuf) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (sendDataSize > WM_SIZE_MP_DATA_MAX) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (sendDataSize == 0) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    DC_StoreRange(sendData, sendDataSize);
    result = WMi_SendCommand(WM_APIID_SET_MP_DATA, 7, sendData, sendDataSize, destBitmap, port, prio, callback, arg);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_EndMP(WMCallbackFunc callback) {
    WMArm9Buf *w9b = WMi_GetSystemWork();
    int result = WMi_CheckStateEx(2, WM_STATE_MP_PARENT, WM_STATE_MP_CHILD);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(&w9b->status->mpFlag, 4);
    if (w9b->status->mpFlag == FALSE) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    WMi_SetCallbackTable(WM_APIID_END_MP, callback);
    result = WMi_SendCommand(WM_APIID_END_MP, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}
