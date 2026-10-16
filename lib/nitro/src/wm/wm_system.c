// The core of the wireless manager (libwm, TwlSDK's wm/ARM9/src/wm_system.c, by its place in the library: the file name
// is a guess). It sets the manager up in a buffer of the game's, keeps the queue of buffers for the commands to the
// ARM7, sends a command and receives every callback and indication that the ARM7 sends back, and checks the manager's
// state for the functions of the other files. The names are descriptive, except swan's WMi_*
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/wm.h"
#include <stdarg.h>

// The buffers for commands and callbacks, and the queue that hands them out: a free buffer is in it
static WMCommandBuf sCommandBufs[WM_NUM_OF_COMMAND_BUFS] ATTRIBUTE_ALIGN(8);
static WMPortRecvCallback sPortRecvCallback;
static OSMessage sCommandMessages[WM_NUM_OF_COMMAND_BUFS];
static OSMessageQueue sCommandQueue;

// The manager's state: whether WM_Init has run, the buffer it was given and the callback that ends the game if the
// console sleeps while the manager is on
static struct {
    u16 initialized;
    WMArm9Buf *wm9buf;
    PMSleepCallbackInfo sleepCallbackInfo;
} sWm;

// What WmReceiveFifo reads of the callback of a child's connection past the fields of WMStartConnectCallback
typedef struct {
    WMStartConnectCallback base;
    u16 unk_0c; // Given to the ports as the reason
    u16 unk_0e;
    u8 macAddress[6];
    u16 parentSize;
    u16 childSize;
} WmStartConnectCallbackView;

static int WmInit(void *wmSysBuf, u16 dmaNo, u32 size);
static void *WmGetCommandBuffer4Arm7(void);
static void WmReceiveFifo(u32 tag, u32 data, BOOL err);
static void WmResetRssiPool(void);
static void WmSleepCallback(void *arg);

// WM_Init
int func_02080758(void *wmSysBuf, u16 dmaNo) {
    int result = WmInit(wmSysBuf, dmaNo, WM_SYSTEM_BUF_SIZE);

    if (result == WM_ERRCODE_SUCCESS) {
        sWm.wm9buf->unk_16 = 0;
    }
    return result;
}

// Sets the manager up in the buffer of size bytes
static int WmInit(void *wmSysBuf, u16 dmaNo, u32 size) {
    u32 irq = CPU_IRQDisable();
    int i;

    if (hw_isDSi() && func_020825ec() != 3) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (sWm.initialized) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    if (!wmSysBuf) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (func_0207b780((u32)wmSysBuf) || (hw_isDSi() && func_0207b744((u32)wmSysBuf))) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (dmaNo > 3) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }
    if ((u32)wmSysBuf & 0x1f) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_INVALID_PARAM;
    }

    PXI_Init();
    if (!func_0207ca48(PXI_FIFO_TAG_WM, 1)) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_WM_DISABLE;
    }

    cp15_invalidateDC(wmSysBuf, size);
    dma_fill(dmaNo, wmSysBuf, 0, size, TRUE);

    sWm.wm9buf = wmSysBuf;
    sWm.wm9buf->wm7buf = (u8 *)wmSysBuf + sizeof(WMArm9Buf);
    sWm.wm9buf->status = (WMStatus *)((u8 *)sWm.wm9buf->wm7buf + WM_ARM7_BUF_SIZE);
    sWm.wm9buf->unk_0c = (u8 *)sWm.wm9buf->status + WM_STATUS_AREA_SIZE;
    sWm.wm9buf->indBuf = (WMCommandBuf *)((u8 *)sWm.wm9buf->unk_0c + WM_INDICATION_BUF_SIZE);
    WmResetRssiPool();
    sWm.wm9buf->dmaNo = dmaNo;
    sWm.wm9buf->connectedAidBitmap = 0;
    sWm.wm9buf->myAid = 0;
    for (i = 0; i < WM_NUM_OF_PORT; i++) {
        sWm.wm9buf->portCallbackTable[i] = NULL;
        sWm.wm9buf->portCallbackArgTable[i] = NULL;
    }

    func_0207acc4(&sCommandQueue, sCommandMessages, WM_NUM_OF_COMMAND_BUFS);
    for (i = 0; i < WM_NUM_OF_COMMAND_BUFS; i++) {
        sCommandBufs[i].apiid = WM_COMMAND_BUF_FREE;
        cp15_cleanDC(&sCommandBufs[i], 2);
        OS_SendMessage(&sCommandQueue, &sCommandBufs[i], OS_MESSAGE_BLOCK);
    }

    func_0207c9fc(PXI_FIFO_TAG_WM, WmReceiveFifo);
    sWm.initialized = TRUE;
    CPU_SetIRQMask(irq);
    return WM_ERRCODE_SUCCESS;
}

// WM_Finish
int func_02080908(void) {
    u32 irq = CPU_IRQDisable();
    int result;

    if (func_02080a70() != WM_ERRCODE_SUCCESS) {
        CPU_SetIRQMask(irq);
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    result = func_02080ab0(1, WM_STATE_READY);
    if (result != WM_ERRCODE_SUCCESS) {
        // BUG: the interrupts stay disabled
#ifdef BUGFIX
        CPU_SetIRQMask(irq);
#endif
        return result;
    }

    WmResetRssiPool();
    func_0207c9fc(PXI_FIFO_TAG_WM, NULL);
    sWm.wm9buf = NULL;
    sWm.initialized = FALSE;
    CPU_SetIRQMask(irq);
    return WM_ERRCODE_SUCCESS;
}

// WMi_SetCallbackTable
void func_02080950(u32 apiid, WMCallbackFunc callback) {
    sWm.wm9buf->callbackTable[apiid] = callback;
}

// Takes a free buffer from the queue, or NULL if there is none
static void *WmGetCommandBuffer4Arm7(void) {
    WMCommandBuf *cmd;

    if (!OS_ReceiveMessage(&sCommandQueue, (OSMessage *)&cmd, OS_MESSAGE_NOBLOCK)) {
        return NULL;
    }
    cp15_invalidateDC(cmd, 2);
    if (!(cmd->apiid & WM_COMMAND_BUF_FREE)) {
        OS_JamMessage(&sCommandQueue, cmd, OS_MESSAGE_BLOCK);
        return NULL;
    }
    return cmd;
}

// WMi_SendCommand
int WMi_SendCommand(u16 apiid, u16 paramNum, ...) {
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

    cp15_cleanDC(cmd, WM_COMMAND_BUF_SIZE);
    fifoResult = PXI_SendWordByFifo(PXI_FIFO_TAG_WM, (u32)cmd, FALSE);
    OS_SendMessage(&sCommandQueue, cmd, OS_MESSAGE_BLOCK);
    return fifoResult < 0 ? WM_ERRCODE_FIFO_ERROR : WM_ERRCODE_OPERATING;
}

// WMi_SendCommandDirect
int WMi_SendCommandDirect(const void *cmd, u32 size) {
    WMCommandBuf *buf = WmGetCommandBuffer4Arm7();
    int fifoResult;

    if (buf == NULL) {
        return WM_ERRCODE_FIFO_ERROR;
    }
    sys_memcpy(cmd, buf, size);
    cp15_cleanDC(buf, size);
    fifoResult = PXI_SendWordByFifo(PXI_FIFO_TAG_WM, (u32)buf, FALSE);
    OS_SendMessage(&sCommandQueue, buf, OS_MESSAGE_BLOCK);
    return fifoResult < 0 ? WM_ERRCODE_FIFO_ERROR : WM_ERRCODE_OPERATING;
}

// WMi_GetSystemWork
WMArm9Buf *func_02080a64(void) {
    return sWm.wm9buf;
}

// WMi_CheckInitialized
int func_02080a70(void) {
    if (sWm.initialized) {
        return WM_ERRCODE_SUCCESS;
    }
    return WM_ERRCODE_ILLEGAL_STATE;
}

// WMi_CheckIdle
int func_02080a84(void) {
    int result = func_02080a70();

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    cp15_invalidateDC(sWm.wm9buf->status, 2);
    if (sWm.wm9buf->status->state <= WM_STATE_STOP) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    return WM_ERRCODE_SUCCESS;
}

// WMi_CheckStateEx
int func_02080ab0(int paramNum, ...) {
    int result = func_02080a70();
    va_list vl;
    u16 state;

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    cp15_invalidateDC(sWm.wm9buf->status, 2);
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
static void WmReceiveFifo(u32 tag, u32 data, BOOL err) {
    WMArm9Buf *w9b = sWm.wm9buf;
    WMCallback *cb = (WMCallback *)data;
    WMCallbackFunc callback;

    if (err) {
        return;
    }

    cp15_invalidateDC(w9b->indBuf, WM_INDICATION_BUF_SIZE);
    if (w9b->unk_16 == 0) {
        cp15_invalidateDC(w9b->status, WM_STATUS_AREA_SIZE);
    }
    if ((void *)cb != w9b->indBuf) {
        cp15_invalidateDC(cb, WM_COMMAND_BUF_SIZE);
    }

    if (cb->apiid >= WM_APIID_ASYNC_KIND_MAX) {
        if (cb->apiid == WM_APIID_INDICATION) {
            if (cb->errcode == WM_ERRCODE_FATAL) {
                sys_exit();
            }
            if (w9b->indCallback != NULL) {
                w9b->indCallback(cb);
            }
        } else if (cb->apiid == WM_APIID_PORT_RECV) {
            WMPortRecvCallback *portCb = (WMPortRecvCallback *)cb;

            if (w9b->portCallbackTable[portCb->port] != NULL) {
                portCb->arg = w9b->portCallbackArgTable[portCb->port];
                portCb->connectedAidBitmap = w9b->connectedAidBitmap;
                cp15_invalidateDC(portCb->recvBuf, w9b->status->mpRecvBufSize);
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
                cp15_invalidateDC(mpCb->recvBuf, w9b->status->mpRecvBufSize);
            }
        }

        if (((cb->apiid == WM_APIID_DISABLE || cb->apiid == WM_APIID_END) && cb->errcode == WM_ERRCODE_SUCCESS) ||
            ((cb->apiid == WM_APIID_ENABLE || cb->apiid == WM_APIID_INITIALIZE) && cb->errcode != WM_ERRCODE_SUCCESS)) {
            func_02080e48();
        }

        if (cb->apiid == WM_APIID_END && cb->errcode == WM_ERRCODE_SUCCESS) {
            callback = w9b->callbackTable[cb->apiid];
            func_02080908();
            if (callback != NULL) {
                callback(cb);
            }
            return;
        }

        callback = w9b->callbackTable[cb->apiid];
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
                WmStartConnectCallbackView *connectCb = (WmStartConnectCallbackView *)cb;

                state = connectCb->base.state;
                aid = 0;
                macAddress = connectCb->macAddress;
                ssid = NULL;
                myAid = connectCb->base.aid;
                reason = connectCb->unk_0c;
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
                sys_memset(&sPortRecvCallback, 0, sizeof(sPortRecvCallback));
                sPortRecvCallback.apiid = WM_APIID_PORT_RECV;
                sPortRecvCallback.errcode = WM_ERRCODE_SUCCESS;
                sPortRecvCallback.state = state;
                sPortRecvCallback.recvBuf = NULL;
                sPortRecvCallback.data = NULL;
                sPortRecvCallback.length = 0;
                sPortRecvCallback.aid = aid;
                sPortRecvCallback.myAid = myAid;
                sPortRecvCallback.connectedAidBitmap = w9b->connectedAidBitmap;
                sPortRecvCallback.seqNo = 0xffff;
                sPortRecvCallback.reason = reason;
                sys_memcpy(macAddress, sPortRecvCallback.macAddress, sizeof(sPortRecvCallback.macAddress));
                if (ssid != NULL) {
                    sys_memcpy16(ssid, sPortRecvCallback.ssid, sizeof(sPortRecvCallback.ssid));
                } else {
                    sys_memset16(0, sPortRecvCallback.ssid, sizeof(sPortRecvCallback.ssid));
                }
                sPortRecvCallback.maxSendDataSize = myAid == 0 ? parentSize : childSize;
                if (myAid != 0) {
                    childSize = parentSize;
                }
                sPortRecvCallback.maxRecvDataSize = childSize;

                for (port = 0; port < WM_NUM_OF_PORT; port++) {
                    sPortRecvCallback.port = port;
                    if (w9b->portCallbackTable[port] != NULL) {
                        sPortRecvCallback.arg = w9b->portCallbackArgTable[port];
                        w9b->portCallbackTable[port](&sPortRecvCallback);
                    }
                }
            }
        }
    }

    cp15_invalidateDC(w9b->indBuf, WM_COMMAND_BUF_SIZE);
    WmResetRssiPool();
    if ((void *)cb != w9b->indBuf) {
        cb->apiid |= WM_COMMAND_BUF_FREE;
        cp15_cleanDC(cb, WM_COMMAND_BUF_SIZE);
    }
}

// Clears bit 0 of the pool of signal strengths that the ARM7 writes
static void WmResetRssiPool(void) {
    u16 pool = *(vu16 *)HW_WM_RSSI_POOL;

    if (pool & 1) {
        *(vu16 *)HW_WM_RSSI_POOL = pool & 0xfffe;
    }
}

// WMi_GetStatusAddress
void *func_02080dbc(void) {
    if (func_02080a70() != WM_ERRCODE_SUCCESS) {
        return NULL;
    }
    return sWm.wm9buf->status;
}

// WMi_GetMyAid
u16 func_02080dd8(void) {
    u32 irq = CPU_IRQDisable();
    u16 aid;

    if (sWm.wm9buf != NULL) {
        aid = sWm.wm9buf->myAid;
    } else {
        aid = 0;
    }
    CPU_SetIRQMask(irq);
    return aid;
}

// WMi_GetConnectedAidBitmap
u16 func_02080dfc(void) {
    u32 irq = CPU_IRQDisable();
    u32 bitmap;

    if (sWm.wm9buf != NULL) {
        bitmap = sWm.wm9buf->connectedAidBitmap;
    } else {
        bitmap = 0;
    }
    CPU_SetIRQMask(irq);
    return bitmap;
}

// WMi_RegisterSleepCallback
void func_02080e24(void) {
    PM_SetSleepCallbackInfo(&sWm.sleepCallbackInfo, WmSleepCallback, NULL);
    func_0207f614(&sWm.sleepCallbackInfo, 1000);
}

// WMi_DeleteSleepCallback
void func_02080e48(void) {
    func_0207f62c(&sWm.sleepCallbackInfo);
}

// The game ends if the console goes to sleep while the manager is on
static void WmSleepCallback(void *arg) {
    sys_exit();
}

// Whether wireless is usable
BOOL func_02080e60(void) {
    if (func_0207c438() == TRUE && !func_0207c4d0()) {
        return TRUE;
    }
    return FALSE;
}
