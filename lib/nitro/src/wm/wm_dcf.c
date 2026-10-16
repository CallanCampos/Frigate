// The wireless manager's DCF mode (libwm, TwlSDK's wm/ARM9/src/wm_dcf.c, by its place in the library: the file name is
// a guess): the mode in which a child, connected to an ordinary access point, sends and receives frames of the
// distributed coordination function. The names are descriptive
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// Starts the mode with the buffer that the frames are received into
int WM_StartDCF(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize) {
    WMArm9Buf *w9b = WMi_GetSystemWork();
    int result = WMi_CheckStateEx(1, WM_STATE_CHILD);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(&w9b->status->dcfFlag, 4);
    if (w9b->status->dcfFlag == TRUE) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (recvBufSize < 0x10) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (recvBuf == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    DC_StoreRange(recvBuf, recvBufSize);
    WMi_SetCallbackTable(WM_APIID_START_DCF, callback);
    result = WMi_SendCommand(WM_APIID_START_DCF, 2, recvBuf, recvBufSize);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

// WM_SetDCFData: sends the frame in the buffer to the six-byte address. The mode must have been started and the frame
// is 0x5e4 bytes at most
int WM_SetDCFData(WMCallbackFunc callback, const u8 *dst, const void *buffer, u16 byteLength) {
    WMArm9Buf *w9b = WMi_GetSystemWork();
    int result = WMi_CheckStateEx(1, WM_STATE_DCF_CHILD);
    u32 dstAddr[2];

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(&w9b->status->dcfFlag, 4);
    if (w9b->status->dcfFlag == FALSE) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (byteLength > 0x5e4) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    DC_StoreRange(buffer, byteLength);
    WMi_SetCallbackTable(WM_APIID_SET_DCF_DATA, callback);
    MI_CpuCopy8(dst, dstAddr, 6);
    result = WMi_SendCommand(WM_APIID_SET_DCF_DATA, 4, dstAddr[0], dstAddr[1], buffer, byteLength);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}

int WM_EndDCF(WMCallbackFunc callback) {
    WMArm9Buf *w9b = WMi_GetSystemWork();
    int result = WMi_CheckStateEx(1, WM_STATE_DCF_CHILD);

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(&w9b->status->dcfFlag, 4);
    if (w9b->status->dcfFlag == FALSE) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    WMi_SetCallbackTable(WM_APIID_END_DCF, callback);
    result = WMi_SendCommand(WM_APIID_END_DCF, 0);
    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    return WM_ERRCODE_OPERATING;
}
