// The core of the wireless manager (libwm, TwlSDK's wm/ARM9/src/wm_system.c, by its place in the library; ntrtwl's
// reconstruction of NitroSDK has the same file). It sets the manager up in a buffer of the game's, keeps the queue of
// buffers for the commands to the ARM7, sends a command and receives every callback and indication that the ARM7 sends
// back, and checks the manager's state for the functions of the other files. The names are NitroSDK's, as that
// reconstruction has them, where it has the code; the DSi's sleep callback and wireless check
// (WMi_RegisterSleepCallback, WMi_DeleteSleepCallback, WmSleepCallback and WMi_IsWirelessAvailable) have no counterpart
// there, and their names are ours
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/wm.h"
#include "twl/nwm.h"
#include <stdarg.h>

// The buffers for commands and callbacks, and the queue that hands them out: a free buffer is in it
static WMCommandBuf fifoBuf[WM_BUF_MSG_NUM] ATTRIBUTE_ALIGN(8);
static WMPortRecvCallback cb_Port;
static OSMessage bufMsg[WM_BUF_MSG_NUM];
static OSMessageQueue bufMsgQ;

// The manager's state: whether WM_Init has run, the buffer it was given and the callback that ends the game if the
// console sleeps while the manager is on
static struct {
    u16 initialized;
    WMArm9Buf *wm9buf;
    PMSleepCallbackInfo sleepCallbackInfo;
} sWm;

static int WmInitCore(void *wmSysBuf, u16 dmaNo, u32 size);
static void *WmGetCommandBuffer4Arm7(void);
static void WmReceiveFifo(PXIFifoTag tag, u32 data, BOOL err);
static void WmClearFifoRecvFlag(void);
static void WmSleepCallback(void *arg);

WMErrCode WM_Init(void *wmSysBuf, u16 dmaNo) {
    int result = WmInitCore(wmSysBuf, dmaNo, WM_SYSTEM_BUF_SIZE);

    if (result == WM_ERRCODE_SUCCESS) {
        sWm.wm9buf->scanOnlyFlag = 0;
    }
    return result;
}

// Sets the manager up in the buffer of size bytes
static int WmInitCore(void *wmSysBuf, u16 dmaNo, u32 size) {
    OSIntrMode irq = OS_DisableInterrupts();
    int i;

    if (OS_IsRunOnTwl() && NWM_CheckInitialized() != NWM_RETCODE_ILLEGAL_STATE) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (sWm.initialized) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (!wmSysBuf) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (OSi_IsAddressInVideoMemory((u32)wmSysBuf) || (OS_IsRunOnTwl() && OSi_IsAddressInMainMemoryEx((u32)wmSysBuf))) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (dmaNo > 3) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }
    if ((u32)wmSysBuf & 0x1f) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }

    PXI_Init();
    if (!PXI_IsCallbackReady(PXI_FIFO_TAG_WM, PXI_PROC_ARM7)) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_WM_DISABLE;
    }

    DC_InvalidateRange(wmSysBuf, size);
    MIi_DmaFill32(dmaNo, wmSysBuf, 0, size, TRUE);

    sWm.wm9buf = wmSysBuf;
    sWm.wm9buf->WM7 = (u8 *)wmSysBuf + sizeof(WMArm9Buf);
    sWm.wm9buf->status = (WMStatus *)((u8 *)sWm.wm9buf->WM7 + WM_ARM7WM_BUF_SIZE);
    sWm.wm9buf->fifo9to7 = (u8 *)sWm.wm9buf->status + WM_STATUS_BUF_SIZE;
    sWm.wm9buf->fifo7to9 = (WMCommandBuf *)((u8 *)sWm.wm9buf->fifo9to7 + WM_FIFO_BUF_SIZE);
    WmClearFifoRecvFlag();
    sWm.wm9buf->dmaNo = dmaNo;
    sWm.wm9buf->connectedAidBitmap = 0;
    sWm.wm9buf->myAid = 0;
    for (i = 0; i < WM_NUM_OF_PORT; i++) {
        sWm.wm9buf->portCallbackTable[i] = NULL;
        sWm.wm9buf->portCallbackArgument[i] = NULL;
    }

    OS_InitMessageQueue(&bufMsgQ, bufMsg, WM_BUF_MSG_NUM);
    for (i = 0; i < WM_BUF_MSG_NUM; i++) {
        fifoBuf[i].apiid = WM_API_REQUEST_ACCEPTED;
        DC_StoreRange(&fifoBuf[i], 2);
        OS_SendMessage(&bufMsgQ, &fifoBuf[i], OS_MESSAGE_BLOCK);
    }

    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_WM, WmReceiveFifo);
    sWm.initialized = TRUE;
    OS_RestoreInterrupts(irq);
    return WM_ERRCODE_SUCCESS;
}

WMErrCode WM_Finish(void) {
    OSIntrMode irq = OS_DisableInterrupts();
    int result;

    if (WMi_CheckInitialized() != WM_ERRCODE_SUCCESS) {
        OS_RestoreInterrupts(irq);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    result = WMi_CheckStateEx(1, WM_STATE_READY);
    if (result != WM_ERRCODE_SUCCESS) {
        // BUG: the interrupts stay disabled
#ifdef BUGFIX
        OS_RestoreInterrupts(irq);
#endif
        return result;
    }

    WmClearFifoRecvFlag();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_WM, NULL);
    sWm.wm9buf = NULL;
    sWm.initialized = FALSE;
    OS_RestoreInterrupts(irq);
    return WM_ERRCODE_SUCCESS;
}

void WMi_SetCallbackTable(WMApiid apiid, WMCallbackFunc callback) {
    sWm.wm9buf->CallbackTable[apiid] = callback;
}

// Takes a free buffer from the queue, or NULL if there is none
static void *WmGetCommandBuffer4Arm7(void) {
    WMCommandBuf *cmd;

    if (!OS_ReceiveMessage(&bufMsgQ, (OSMessage *)&cmd, OS_MESSAGE_NOBLOCK)) {
        return NULL;
    }
    DC_InvalidateRange(cmd, 2);
    if (!(cmd->apiid & WM_API_REQUEST_ACCEPTED)) {
        OS_JamMessage(&bufMsgQ, cmd, OS_MESSAGE_BLOCK);
        return NULL;
    }
    return cmd;
}

// WMi_SendCommand
WMErrCode WMi_SendCommand(WMApiid apiid, u16 paramNum, ...) {
    WMCommandBuf *cmd = WmGetCommandBuffer4Arm7();
    va_list vl;
    int i;
    int fifoResult;

    if (cmd == NULL) {
        return WM_ERRCODE_FIFO_ERROR;
    }
    cmd->apiid = apiid;
    va_start(vl, paramNum);
    for (i = 0; i < paramNum; i++) {
        cmd->param[i] = va_arg(vl, u32);
    }
    va_end(vl);

    DC_StoreRange(cmd, WM_FIFO_BUF_SIZE);
    fifoResult = PXI_SendWordByFifo(PXI_FIFO_TAG_WM, (u32)cmd, FALSE);
    OS_SendMessage(&bufMsgQ, cmd, OS_MESSAGE_BLOCK);
    return fifoResult < 0 ? WM_ERRCODE_FIFO_ERROR : WM_ERRCODE_OPERATING;
}

// WMi_SendCommandDirect
WMErrCode WMi_SendCommandDirect(void *cmd, u32 size) {
    WMCommandBuf *buf = WmGetCommandBuffer4Arm7();
    int fifoResult;

    if (buf == NULL) {
        return WM_ERRCODE_FIFO_ERROR;
    }
    MI_CpuCopy8(cmd, buf, size);
    DC_StoreRange(buf, size);
    fifoResult = PXI_SendWordByFifo(PXI_FIFO_TAG_WM, (u32)buf, FALSE);
    OS_SendMessage(&bufMsgQ, buf, OS_MESSAGE_BLOCK);
    return fifoResult < 0 ? WM_ERRCODE_FIFO_ERROR : WM_ERRCODE_OPERATING;
}

WMArm9Buf *WMi_GetSystemWork(void) {
    return sWm.wm9buf;
}

WMErrCode WMi_CheckInitialized(void) {
    if (sWm.initialized) {
        return WM_ERRCODE_SUCCESS;
    }
    return WM_ERRCODE_ILLEGAL_STATE;
}

WMErrCode WMi_CheckIdle(void) {
    int result = WMi_CheckInitialized();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(sWm.wm9buf->status, 2);
    if (sWm.wm9buf->status->state <= WM_STATE_STOP) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    return WM_ERRCODE_SUCCESS;
}

WMErrCode WMi_CheckStateEx(s32 paramNum, ...) {
    int result = WMi_CheckInitialized();
    va_list vl;
    u16 state;

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    DC_InvalidateRange(sWm.wm9buf->status, 2);
    state = sWm.wm9buf->status->state;
    result = WM_ERRCODE_ILLEGAL_STATE;
    va_start(vl, paramNum);
    for (; paramNum != 0; paramNum--) {
        if (va_arg(vl, int) == state) {
            result = WM_ERRCODE_SUCCESS;
        }
    }
    va_end(vl);
    return result;
}

// Receives what the ARM7 sends through the FIFO: a pointer to a callback (of a command) or an indication. It does
// what the callback's kind asks, calls the game's callback for it and makes the buffer free again
static void WmReceiveFifo(PXIFifoTag tag, u32 data, BOOL err) {
    WMArm9Buf *w9b = sWm.wm9buf;
    WMCallback *cb = (WMCallback *)data;
    WMCallbackFunc callback;

    if (err) {
        return;
    }

    DC_InvalidateRange(w9b->fifo7to9, WM_FIFO_BUF_SIZE);
    if (w9b->scanOnlyFlag == 0) {
        DC_InvalidateRange(w9b->status, WM_STATUS_BUF_SIZE);
    }
    if ((void *)cb != w9b->fifo7to9) {
        DC_InvalidateRange(cb, WM_FIFO_BUF_SIZE);
    }

    if (cb->apiid >= WM_NUM_OF_CALLBACK) {
        if (cb->apiid == WM_APIID_INDICATION) {
            if (cb->errcode == WM_ERRCODE_FLASH_ERROR) {
                OS_Terminate();
            }
            if (w9b->indCallback != NULL) {
                w9b->indCallback(cb);
            }
        } else if (cb->apiid == WM_APIID_PORT_RECV) {
            WMPortRecvCallback *portCb = (WMPortRecvCallback *)cb;

            if (w9b->portCallbackTable[portCb->port] != NULL) {
                portCb->arg = w9b->portCallbackArgument[portCb->port];
                portCb->connectedAidBitmap = w9b->connectedAidBitmap;
                DC_InvalidateRange(portCb->recvBuf, w9b->status->mp_recvBufSize);
                w9b->portCallbackTable[portCb->port](portCb);
            }
        } else if (cb->apiid == WM_APIID_PORT_SEND) {
            WMPortSendCallback *sendCb = (WMPortSendCallback *)cb;

            sendCb->apiid = WM_APIID_SET_MP_DATA;
            if (sendCb->callback != NULL) {
                sendCb->callback(sendCb);
            }
        }
    } else {
        if (cb->apiid == WM_APIID_START_MP) {
            WMStartMPCallback *mpCb = (WMStartMPCallback *)cb;

            if ((mpCb->state == WM_STATECODE_MPEND_IND || mpCb->state == WM_STATECODE_MP_IND) && mpCb->errcode == 0) {
                DC_InvalidateRange(mpCb->recvBuf, w9b->status->mp_recvBufSize);
            }
        }

        if (((cb->apiid == WM_APIID_DISABLE || cb->apiid == WM_APIID_END) && cb->errcode == WM_ERRCODE_SUCCESS) ||
            ((cb->apiid == WM_APIID_ENABLE || cb->apiid == WM_APIID_INITIALIZE) && cb->errcode != WM_ERRCODE_SUCCESS)) {
            WMi_DeleteSleepCallback();
        }

        if (cb->apiid == WM_APIID_END && cb->errcode == WM_ERRCODE_SUCCESS) {
            callback = w9b->CallbackTable[cb->apiid];
            WM_Finish();
            if (callback != NULL) {
                callback(cb);
            }
            return;
        }

        callback = w9b->CallbackTable[cb->apiid];
        if (callback != NULL) {
            callback(cb);
            if (!sWm.initialized) {
                return;
            }
        }

        if (cb->apiid == WM_APIID_START_PARENT || cb->apiid == WM_APIID_START_CONNECT) {
            u16 state;
            u16 myAid;
            u16 port;
            u16 aid;
            u16 reason;
            u8 *macAddress;
            u8 *ssid;
            u16 parentSize;
            u16 childSize;

            if (cb->apiid == WM_APIID_START_PARENT) {
                WMStartParentCallback *parentCb = (WMStartParentCallback *)cb;

                state = parentCb->state;
                aid = parentCb->aid;
                macAddress = parentCb->macAddress;
                ssid = parentCb->ssid;
                myAid = 0;
                reason = parentCb->reason;
                parentSize = parentCb->parentSize;
                childSize = parentCb->childSize;
            } else if (cb->apiid == WM_APIID_START_CONNECT) {
                WMStartConnectCallback *connectCb = (WMStartConnectCallback *)cb;

                state = connectCb->state;
                aid = 0;
                macAddress = connectCb->macAddress;
                ssid = NULL;
                myAid = connectCb->aid;
                reason = connectCb->reason;
                parentSize = connectCb->parentSize;
                childSize = connectCb->childSize;
            }

            if (state == WM_STATECODE_CONNECTED || state == WM_STATECODE_DISCONNECTED ||
                state == WM_STATECODE_DISCONNECTED_FROM_MYSELF) {
                if (state == WM_STATECODE_CONNECTED) {
                    w9b->connectedAidBitmap |= 1 << aid;
                } else {
                    w9b->connectedAidBitmap &= ~(1 << aid);
                }
                w9b->myAid = myAid;

                // Tells the ports that someone connected or disconnected
                MI_CpuFill8(&cb_Port, 0, sizeof(cb_Port));
                cb_Port.apiid = WM_APIID_PORT_RECV;
                cb_Port.errcode = WM_ERRCODE_SUCCESS;
                cb_Port.state = state;
                cb_Port.recvBuf = NULL;
                cb_Port.data = NULL;
                cb_Port.length = 0;
                cb_Port.aid = aid;
                cb_Port.myAid = myAid;
                cb_Port.connectedAidBitmap = w9b->connectedAidBitmap;
                cb_Port.seqNo = 0xffff;
                cb_Port.reason = reason;
                MI_CpuCopy8(macAddress, cb_Port.macAddress, sizeof(cb_Port.macAddress));
                if (ssid != NULL) {
                    MIi_CpuCopy16(ssid, cb_Port.ssid, sizeof(cb_Port.ssid));
                } else {
                    MIi_CpuClear16(0, cb_Port.ssid, sizeof(cb_Port.ssid));
                }
                cb_Port.maxSendDataSize = myAid == 0 ? parentSize : childSize;
                if (myAid != 0) {
                    childSize = parentSize;
                }
                cb_Port.maxRecvDataSize = childSize;

                for (port = 0; port < WM_NUM_OF_PORT; port++) {
                    cb_Port.port = port;
                    if (w9b->portCallbackTable[port] != NULL) {
                        cb_Port.arg = w9b->portCallbackArgument[port];
                        w9b->portCallbackTable[port](&cb_Port);
                    }
                }
            }
        }
    }

    DC_InvalidateRange(w9b->fifo7to9, WM_FIFO_BUF_SIZE);
    WmClearFifoRecvFlag();
    if ((void *)cb != w9b->fifo7to9) {
        cb->apiid |= WM_API_REQUEST_ACCEPTED;
        DC_StoreRange(cb, WM_FIFO_BUF_SIZE);
    }
}

// Clears bit 0 of the pool of signal strengths that the ARM7 writes
static void WmClearFifoRecvFlag(void) {
    u16 pool = *(vu16 *)HW_WM_CALLBACK_CONTROL;

    if (pool & 1) {
        *(vu16 *)HW_WM_CALLBACK_CONTROL = pool & 0xfffe;
    }
}

const WMStatus *WMi_GetStatusAddress(void) {
    if (WMi_CheckInitialized() != WM_ERRCODE_SUCCESS) {
        return NULL;
    }
    return sWm.wm9buf->status;
}

u16 WM_GetAID(void) {
    OSIntrMode irq = OS_DisableInterrupts();
    u16 aid;

    if (sWm.wm9buf != NULL) {
        aid = sWm.wm9buf->myAid;
    } else {
        aid = 0;
    }
    OS_RestoreInterrupts(irq);
    return aid;
}

u16 WM_GetConnectedAIDs(void) {
    OSIntrMode irq = OS_DisableInterrupts();
    u32 bitmap;

    if (sWm.wm9buf != NULL) {
        bitmap = sWm.wm9buf->connectedAidBitmap;
    } else {
        bitmap = 0;
    }
    OS_RestoreInterrupts(irq);
    return bitmap;
}

void WMi_RegisterSleepCallback(void) {
    PM_SetSleepCallbackInfo(&sWm.sleepCallbackInfo, WmSleepCallback, NULL);
    PM_InsertPreSleepCallback(&sWm.sleepCallbackInfo, 1000);
}

void WMi_DeleteSleepCallback(void) {
    PM_DeletePreSleepCallback(&sWm.sleepCallbackInfo);
}

// The game ends if the console goes to sleep while the manager is on
static void WmSleepCallback(void *arg) {
    OS_Terminate();
}

// Whether wireless is usable
BOOL WMi_IsWirelessAvailable(void) {
    if (OS_IsAvailableWireless() == TRUE && !OS_IsForceDisableWireless()) {
        return TRUE;
    }
    return FALSE;
}
