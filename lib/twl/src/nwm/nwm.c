// TwlSDK's NWM, the DSi's wireless manager, ARM9 side, from NWM_End on: the commands that the ARM9 sends the ARM7
// over the PXI tag 0x16, each checked against the state that the ARM7 reports, the callback that unloads the device
// before the application exits, and the sending of the commands' buffers. ARM code in the DSi-only LTD autoload. The
// ROM names none of it; the file's name and those of the types and data are descriptive guesses, and the functions have
// the names of the SDK functions they implement where those are known, from what they do, and keep their default
// names otherwise. NWM_Init and its flag are nwm_setup.c's, and the system area and the events nwm_events.c's, whose
// statics are each laid out on their own. The SDK has more files here (cybervisi0n/libntr's reconstruction splits
// this part further), whose boundaries the ROM does not show

#include "types.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "nwm_private.h"
#include "twl/nwm.h"
#include <stdarg.h>

static NWMRetCode func_02702ca0(NWMCallback callback);
static void func_02702d68(void *event);
static void func_02702d7c(void *arg);
static void func_02703764(const u8 *destAddr, const u8 *srcAddr, u8 *frame, u32 size);
static void func_027038bc(void *event);
static void func_02703a34(u32 cmd, NWMCallback callback);
static void func_02703a54(u32 index, NWMCallback callback);
static NWMRetCode func_02703a74(u32 cmd, u16 count, ...);
static NWMRetCode func_02703b24(const void *cmd, u32 size);
static NWMCmdBuf *func_02703b98(void);
static BOOL func_02703c28(void);

// A pointer in the DSi's shared memory to a block of its settings. The library checks the 16 bytes at 0x1e0 of it:
// a word whose upper half is the CRC-16 of the 12 bytes that follow it
#define NWM_SETTINGS_PTR 0x02fffdfc

// The data, in memory: sUnloaded (set when the exit callback's unload is done) and sBlockBuf (the block that the
// command 8 copies for the ARM7). -ipa function lays out the tentative statics in the reverse of their declarations
static u32 sBlockBuf[0x210 / 4];
static u8 sUnloaded;

// Whether the ARM7 is running the command: it sets the command's byte in its status block while it does
static inline BOOL NWMi_IsPending(u32 cmd) {
    NWMSys *sys = func_027026dc();
    BOOL pending;

    if (sys != NULL) {
        DC_InvalidateRange(&sys->status->pending[cmd], 1);
        pending = sys->status->pending[cmd] == 1;
    } else {
        pending = FALSE;
    }
    return pending;
}

NWMRetCode NWM_End(void) {
    OSIntrMode irq;
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_INITIALIZED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    irq = OS_DisableInterrupts();
    NWMi_SystemArea->status->state = NWM_STATE_NONE;
    func_027026ec();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_NWM, NULL);
    NWMi_SystemArea = NULL;
    NWMi_SetUpDone = FALSE;
    OS_RestoreInterrupts(irq);
    return NWM_RETCODE_SUCCESS;
}

NWMRetCode NWM_LoadDevice(NWMCallback callback) {
    NWMSys *sys = func_027026dc();
    NWMRetCode result;
    PMExitCallbackInfo *info;

    result = func_027028e4(1, NWM_STATE_INITIALIZED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_LOAD_DEVICE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703c28();
    func_02703a34(NWM_CMD_LOAD_DEVICE, callback);
    func_02702a40();
    info = &NWMi_UnloadOnExit;
    PM_SetExitCallbackInfo(info, func_02702d7c, NULL);
    PM_InsertPreExitCallback(info, NWM_PM_PRIORITY);
    result = func_02703a74(NWM_CMD_LOAD_DEVICE, 4, sys->info, sys->status, sys->event, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_UnloadDevice(NWMCallback callback) {
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_LOADED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_UNLOAD_DEVICE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703a34(NWM_CMD_UNLOAD_DEVICE, callback);
    func_027026dc();
    result = func_02703a74(NWM_CMD_UNLOAD_DEVICE, 1, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// Unloads the device in any state after the initialized one
static NWMRetCode func_02702ca0(NWMCallback callback) {
    NWMRetCode result;

    result =
        func_027028e4(5, NWM_STATE_INITIALIZED, NWM_STATE_LOADED, NWM_STATE_READY, NWM_STATE_CONNECTED, NWM_STATE_5);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_UNLOAD_DEVICE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703a34(NWM_CMD_UNLOAD_DEVICE, callback);
    func_027026dc();
    result = func_02703a74(NWM_CMD_UNLOAD_DEVICE, 1, 1);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

static void func_02702d68(void *event) {
    sUnloaded = TRUE;
}

// Runs before the application exits: waits for the ARM7 to finish what it is doing, then unloads the device
static void func_02702d7c(void *arg) {
    s32 timeout;
    NWMSys *sys;

    if (!PMi_GetExitType()) {
        return;
    }
    sys = func_027026dc();
    timeout = NWM_EXIT_TIMEOUT;
    if (sys == NULL) {
        return;
    }
    OS_SpinWaitSysCycles(NWM_EXIT_SPIN_FIRST);
    while ((NWMi_IsPending(NWM_CMD_LOAD_DEVICE) == TRUE && func_027028b0() == 1) || NWMi_IsPending(NWM_CMD_1) == TRUE ||
           NWMi_IsPending(NWM_CMD_19) == TRUE) {
        OS_SpinWaitSysCycles(NWM_EXIT_SPIN_STEP);
        PXIi_HandlerRecvFifoNotEmpty();
    }
    if (func_027028e4(4, NWM_STATE_LOADED, NWM_STATE_READY, NWM_STATE_CONNECTED, NWM_STATE_5) != 0) {
        return;
    }
    sUnloaded = FALSE;
    func_02702ca0(func_02702d68);
    do {
        OS_SpinWaitSysCycles(NWM_EXIT_SPIN_STEP);
        PXIi_HandlerRecvFifoNotEmpty();
        timeout -= NWM_EXIT_SPIN_STEP;
        if (timeout < 0) {
            return;
        }
    } while (!sUnloaded);
}

NWMRetCode NWM_Open(NWMCallback callback) {
    NWMRetCode result;

    if (!func_027026b4()) {
        return NWM_RETCODE_DISABLED;
    }
    result = func_027028e4(1, NWM_STATE_LOADED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_OPEN) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703a34(NWM_CMD_OPEN, callback);
    func_027026dc();
    result = func_02703a74(NWM_CMD_OPEN, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_Close(NWMCallback callback) {
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_READY);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_CLOSE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703a34(NWM_CMD_CLOSE, callback);
    func_027026dc();
    result = func_02703a74(NWM_CMD_CLOSE, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// The command of a scan, with the fields that it takes from the parameters
typedef struct NWMScanCmd {
    u16 cmd;
    u16 channelList;
    u16 bufferSize;
    u16 dwellTime;
    u8 *buffer;
    u16 scanType;
    u16 ssidLength;
    u16 ssidMatchLength;
    u8 ssid[32];
    u8 bssid[6];
} NWMScanCmd;

NWMRetCode NWM_StartScan(NWMCallback callback, const NWMScanParam *param) {
    NWMScanCmd cmd;
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_READY);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_SCAN) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (param == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (param->buffer == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (param->ssidLength > 32) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (param->ssidMatchLength > 32) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    MI_CpuFill8(param->buffer, 0, (param->bufferSize + 3) & ~3);
    DC_StoreRange(param->buffer, (param->bufferSize + 3) & ~3);
    func_02703a34(NWM_CMD_SCAN, callback);
    cmd.cmd = NWM_CMD_SCAN;
    cmd.channelList = param->channelList;
    cmd.bufferSize = param->bufferSize;
    cmd.dwellTime = param->dwellTime;
    cmd.buffer = param->buffer;
    cmd.scanType = param->scanType;
    cmd.ssidLength = param->ssidLength;
    cmd.ssidMatchLength = param->ssidMatchLength;
    MI_CpuCopy8(param->ssid, cmd.ssid, 32);
    MI_CpuCopy8(param->bssid, cmd.bssid, 6);
    result = func_02703b24(&cmd, sizeof(cmd));
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

u8 *func_02703208(NWMBssDesc *bssDesc, u32 id) {
    u8 *element = bssDesc->elements;
    u8 *end = element + bssDesc->length - NWM_BSS_DESC_ELEMENTS_OFFSET;

    while (element[0] != id && element + element[1] + 2 <= end) {
        element += element[1] + 2;
    }
    return element[0] == id ? element : NULL;
}

u8 *func_02703250(NWMBssDesc *bssDesc, u32 id, const u8 *oui) {
    u8 *element = bssDesc->elements;
    u8 *end = element + bssDesc->length - NWM_BSS_DESC_ELEMENTS_OFFSET;

    if (id != 0xdd) {
        return NULL;
    }
    while (element + element[1] + 2 <= end) {
        if (element[0] == id && MI_CpuComp8(element + 2, oui, 4) == 0) {
            return element;
        }
        element += element[1] + 2;
    }
    return NULL;
}

// The command that sends the block, which the ARM7 reads from sBlockBuf
typedef struct NWMBlockCmd {
    u16 cmd;
    u8 index;
    void *data;
} NWMBlockCmd;

NWMRetCode func_027032e0(NWMCallback callback, u32 index, const NWMBlockParam *data) {
    NWMBlockCmd cmd;
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_READY);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_8) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (data == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (data->length > 0x200) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (index >= 3) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (data->unk_0a == 0) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    // The block is cleaned for twice its length and copied for its length: one of them is not in bytes
    DC_StoreRange(data, data->length * 2);
    func_02703a34(NWM_CMD_8, callback);
    cmd.cmd = NWM_CMD_8;
    cmd.index = index;
    cmd.data = sBlockBuf;
    MI_CpuCopy8(data, sBlockBuf, data->length);
    DC_StoreRange(cmd.data, 0x200);
    DC_StoreRange(&cmd, 8);
    result = func_02703b24(&cmd, 8);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_Disconnect(NWMCallback callback) {
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_DISCONNECT) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703a34(NWM_CMD_DISCONNECT, callback);
    result = func_02703a74(NWM_CMD_DISCONNECT, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// The command that connects: the library's mode becomes the one that the ARM7 takes, and 0x50 bytes go along
typedef struct NWMConnectCmd {
    u16 cmd;
    u16 arg2;
    u16 arg;
    u16 mode;
    u8 data[0x50];
} NWMConnectCmd;

NWMRetCode NWM_Connect(NWMCallback callback, u32 mode, u32 arg, const void *data, u16 arg2) {
    NWMConnectCmd cmd;
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_READY);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_CONNECT) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    switch (mode) {
    case 0:
        mode = 0;
        break;
    case 1:
        mode = 5;
        break;
    case 2:
        mode = 13;
        break;
    case 3:
        mode = 16;
        break;
    default:
        return NWM_RETCODE_INVALID_PARAM;
    }
    DC_StoreRange(data, 0x50);
    func_02703a34(NWM_CMD_CONNECT, callback);
    cmd.cmd = NWM_CMD_CONNECT;
    cmd.arg = arg;
    cmd.mode = mode;
    cmd.arg2 = arg2;
    MI_CpuCopy8(data, cmd.data, 0x50);
    result = func_02703b24(&cmd, sizeof(cmd));
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode func_027035f4(NWMCallback callback, u32 arg) {
    NWMRetCode result;

    result = func_027028e4(2, NWM_STATE_READY, NWM_STATE_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsPending(NWM_CMD_14) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703a34(NWM_CMD_14, callback);
    func_027026dc();
    result = func_02703a74(NWM_CMD_14, 1, arg);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_SendFrame(NWMCallback callback, const u8 *destAddr, u8 *frame, u32 size) {
    NWMSys *sys = func_027026dc();
    NWMRetCode result;
    OSIntrMode irq;

    result = func_027028e4(1, NWM_STATE_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    irq = OS_DisableInterrupts();
    if (size > 0x5f2) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    DC_StoreRange(frame, size);
    func_02703764(destAddr, sys->status->macAddr, frame, size);
    DC_StoreRange(frame, 14);
    result = func_02703a74(NWM_CMD_SEND_FRAME, 3, callback, frame, size);
    // Only the result 0 leaves without restoring the interrupts, and the command never gives it
    if (result == NWM_RETCODE_SUCCESS) {
        return NWM_RETCODE_OPERATING;
    }
    OS_RestoreInterrupts(irq);
    return result;
}

// Writes the two MAC addresses at the start of the frame
static void func_02703764(const u8 *destAddr, const u8 *srcAddr, u8 *frame, u32 size) {
    if (size < 14) {
        return;
    }
    MI_CpuCopy8(destAddr, frame, 6);
    MI_CpuCopy8(srcAddr, frame + 6, 6);
}

NWMRetCode NWM_SetReceivingFrameBuffer(NWMCallback receiveCallback, void *buffer, u32 size, u32 index) {
    NWMSys *sys = func_027026dc();
    NWMRetCode result;

    result = func_027028e4(2, NWM_STATE_READY, NWM_STATE_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    MIi_DmaFill32(sys->dmaNo, buffer, 0, size, TRUE);
    DC_StoreRange(buffer, size);
    func_02703a34(NWM_CMD_SET_RECEIVE_BUFFER, func_027038bc);
    func_02703a54(index, receiveCallback);
    result = func_02703a74(NWM_CMD_SET_RECEIVE_BUFFER, 3, buffer, size, index);
    if (result == NWM_RETCODE_SUCCESS) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode func_02703854(NWMCallback callback, u32 index) {
    NWMRetCode result;

    func_027026dc();
    result = func_027028e4(2, NWM_STATE_READY, NWM_STATE_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    func_02703a34(NWM_CMD_12, callback);
    func_02703a54(index, NULL);
    result = func_02703a74(NWM_CMD_12, 1, index);
    if (result == NWM_RETCODE_SUCCESS) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// The callback of the command that gives the ARM7 a buffer to receive in: runs the callback of the event's index
// with the event, and when the buffer is full gives it to the ARM7 again
static void func_027038bc(void *event) {
    NWMEvent *ev = event;
    NWMSys *sys = func_027026dc();
    BOOL ok;

    if (ev->result != 0) {
        if (ev->result != NWM_RESULT_BUFFER_FULL) {
            return;
        }
        ok = FALSE;
    } else {
        ok = TRUE;
    }
    if (sys->indicate[ev->u04.index] != NULL) {
        DC_StoreRange(ev, 16);
        sys->indicate[ev->u04.index](ev);
    }
    if (ok) {
        return;
    }
    func_02703a74(NWM_CMD_SET_RECEIVE_BUFFER, 3, ev->u0c.buffer, 0, ev->u04.index);
}

// The command that sets the key
typedef struct NWMPmkCmd {
    u16 cmd;
    u16 type;
    u8 pmk[32];
} NWMPmkCmd;

NWMRetCode func_02703960(NWMCallback callback, NWMPmkParam *param) {
    NWMPmkCmd cmd;
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_READY);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (param == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    DC_StoreRange(param, 0x22);
    switch (param->type) {
    case 4:
    case 5:
    case 6:
    case 7:
        break;
    default:
        return NWM_RETCODE_INVALID_PARAM;
    }
    func_02703a34(NWM_CMD_SET_PMK, callback);
    cmd.cmd = NWM_CMD_SET_PMK;
    cmd.type = param->type;
    MI_CpuCopy8(param->pmk, cmd.pmk, 32);
    result = func_02703b24(&cmd, sizeof(cmd));
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

static void func_02703a34(u32 cmd, NWMCallback callback) {
    func_027026dc()->callbacks[cmd] = callback;
}

static void func_02703a54(u32 index, NWMCallback callback) {
    func_027026dc()->indicate[index] = callback;
}

// Sends a command with its words
static NWMRetCode func_02703a74(u32 cmd, u16 count, ...) {
    int sent;
    va_list args;
    int i;
    NWMCmdBuf *buf;
    NWMSys *sys = func_027026dc();
    NWMRetCode result;

    buf = func_02703b98();
    if (buf == NULL) {
        return NWM_RETCODE_SEND_ERROR;
    }
    buf->cmd = cmd;
    va_start(args, count);
    for (i = 0; i < count; i++) {
        buf->args[i] = va_arg(args, u32);
    }
    DC_StoreRange(buf, sizeof(NWMCmdBuf));
    sent = PXI_SendWordByFifo(PXI_FIFO_TAG_NWM, (u32)buf, FALSE);
    OS_SendMessage(&sys->freeQueue, buf, OS_MESSAGE_BLOCK);
    result = NWM_RETCODE_SEND_ERROR;
    if (sent >= 0) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// Sends a command that is a struct
static NWMRetCode func_02703b24(const void *cmd, u32 size) {
    NWMSys *sys = func_027026dc();
    NWMCmdBuf *buf;
    int sent;
    NWMRetCode result;

    buf = func_02703b98();
    if (buf == NULL) {
        return NWM_RETCODE_SEND_ERROR;
    }
    MI_CpuCopy8(cmd, buf, size);
    DC_StoreRange(buf, size);
    sent = PXI_SendWordByFifo(PXI_FIFO_TAG_NWM, (u32)buf, FALSE);
    OS_SendMessage(&sys->freeQueue, buf, OS_MESSAGE_BLOCK);
    result = NWM_RETCODE_SEND_ERROR;
    if (sent >= 0) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// Takes the next command buffer that the ARM7 is done with, or NULL when the queue is empty
static NWMCmdBuf *func_02703b98(void) {
    NWMCmdBuf *buf = NULL;
    NWMSys *sys = func_027026dc();

    do {
        if (!OS_ReceiveMessage(&sys->freeQueue, (OSMessage *)&buf, OS_MESSAGE_NOBLOCK)) {
            return NULL;
        }
        DC_InvalidateRange(sys, 0x2000);
        DC_InvalidateRange(buf, 2);
        if (!(buf->cmd & NWM_CMD_BUF_FREE)) {
            OS_SendMessage(&sys->freeQueue, buf, OS_MESSAGE_BLOCK);
            buf = NULL;
        }
    } while (buf == NULL);
    return buf;
}

// Whether the CRC of the wireless settings is right (the result is not used)
static BOOL func_02703c28(void) {
    u8 *settings = *(u8 **)NWM_SETTINGS_PTR;

    return SVC_GetCRC16(0xffff, settings + 0x1e4, 12) == *(u32 *)(settings + 0x1e0) >> 16;
}
