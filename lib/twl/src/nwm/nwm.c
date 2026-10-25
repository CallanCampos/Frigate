// TwlSDK's NWM, the DSi's wireless manager, ARM9 side, from NWM_End on: the commands that the ARM9 sends the ARM7
// over the PXI tag 0x16, each checked against the state that the ARM7 reports, the callback that unloads the device
// before the application exits, and the sending of the commands' buffers. ARM code in the DSi-only LTD autoload. The
// ROM names none of it; the file's name and those of the private types and data are descriptive guesses, and the
// functions have the names of libntr's reconstruction of TwlSDK's headers (nwm.h, nwm_arm9_private.h), by the API ID
// each command sends, where those are known, and keep their default names otherwise. NWM_Init and its flag are
// nwm_setup.c's, and the system area and the events nwm_events.c's, whose statics are each laid out on their own. The
// SDK has more files here (cybervisi0n/libntr's reconstruction splits this part further), whose boundaries the ROM does
// not show

#include "types.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "nwm_private.h"
#include "twl/nwm.h"
#include <stdarg.h>

static NWMRetCode func_02702ca0(NWMCallbackFunc callback);
static void func_02702d68(void *event);
static void func_02702d7c(void *arg);
static void func_02703764(const u8 *destAddr, const u8 *srcAddr, u8 *frame, u32 size);
static void func_027038bc(void *event);
static void NWMi_SetCallbackTable(NWMApiid id, NWMCallbackFunc callback);
static void NWMi_SetReceiveCallbackTable(NWMFramePort port, NWMCallbackFunc callback);
static NWMRetCode NWMi_SendCommand(NWMApiid id, u16 paramNum, ...);
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
static inline BOOL NWMi_IsAPIActive(u32 apiid) {
    NWMSys *sys = NWMi_GetSystemWork();
    BOOL pending;

    if (sys != NULL) {
        DC_InvalidateRange(&sys->status->pending[apiid], 1);
        pending = sys->status->pending[apiid] == 1;
    } else {
        pending = FALSE;
    }
    return pending;
}

NWMRetCode NWM_End(void) {
    OSIntrMode irq;
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_INITIALIZED);
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

NWMRetCode NWM_LoadDevice(NWMCallbackFunc callback) {
    NWMSys *sys = NWMi_GetSystemWork();
    NWMRetCode result;
    PMExitCallbackInfo *info;

    result = NWMi_CheckState(1, NWM_STATE_INITIALIZED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_LOAD_DEVICE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    func_02703c28();
    NWMi_SetCallbackTable(NWM_APIID_LOAD_DEVICE, callback);
    func_02702a40();
    info = &NWMi_UnloadOnExit;
    PM_SetExitCallbackInfo(info, func_02702d7c, NULL);
    PMi_InsertPostExitCallbackEx(info, NWM_PM_PRIORITY);
    result = NWMi_SendCommand(NWM_APIID_LOAD_DEVICE, 4, sys->info, sys->status, sys->event, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_UnloadDevice(NWMCallbackFunc callback) {
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_LOADED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_UNLOAD_DEVICE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    NWMi_SetCallbackTable(NWM_APIID_UNLOAD_DEVICE, callback);
    NWMi_GetSystemWork();
    result = NWMi_SendCommand(NWM_APIID_UNLOAD_DEVICE, 1, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// Unloads the device in any state after the initialized one
static NWMRetCode func_02702ca0(NWMCallbackFunc callback) {
    NWMRetCode result;

    result = NWMi_CheckState(5, NWM_STATE_INITIALIZED, NWM_STATE_LOADED, NWM_STATE_DISCONNECTED,
                             NWM_STATE_INFRA_CONNECTED, NWM_STATE_ADHOC_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_UNLOAD_DEVICE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    NWMi_SetCallbackTable(NWM_APIID_UNLOAD_DEVICE, callback);
    NWMi_GetSystemWork();
    result = NWMi_SendCommand(NWM_APIID_UNLOAD_DEVICE, 1, 1);
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

    if (!PM_GetExitFactor()) {
        return;
    }
    sys = NWMi_GetSystemWork();
    timeout = NWM_EXIT_TIMEOUT;
    if (sys == NULL) {
        return;
    }
    OS_SpinWaitSysCycles(NWM_EXIT_SPIN_FIRST);
    while ((NWMi_IsAPIActive(NWM_APIID_LOAD_DEVICE) == TRUE && NWM_GetState() == 1) ||
           NWMi_IsAPIActive(NWM_APIID_RESET) == TRUE || NWMi_IsAPIActive(NWM_APIID_INSTALL_FIRMWARE) == TRUE) {
        OS_SpinWaitSysCycles(NWM_EXIT_SPIN_STEP);
        PXIi_HandlerRecvFifoNotEmpty();
    }
    if (NWMi_CheckState(4, NWM_STATE_LOADED, NWM_STATE_DISCONNECTED, NWM_STATE_INFRA_CONNECTED,
                        NWM_STATE_ADHOC_CONNECTED) != 0) {
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

NWMRetCode NWM_Open(NWMCallbackFunc callback) {
    NWMRetCode result;

    if (!NWMi_CheckEnableFlag()) {
        return NWM_RETCODE_FATAL_ERROR;
    }
    result = NWMi_CheckState(1, NWM_STATE_LOADED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_OPEN) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    NWMi_SetCallbackTable(NWM_APIID_OPEN, callback);
    NWMi_GetSystemWork();
    result = NWMi_SendCommand(NWM_APIID_OPEN, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_Close(NWMCallbackFunc callback) {
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_DISCONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_CLOSE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    NWMi_SetCallbackTable(NWM_APIID_CLOSE, callback);
    NWMi_GetSystemWork();
    result = NWMi_SendCommand(NWM_APIID_CLOSE, 0);
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
    NWMBssDesc *buffer;
    u16 scanType;
    u16 ssidLength;
    u16 ssidMatchLength;
    u8 ssid[32];
    u8 bssid[6];
} NWMScanCmd;

NWMRetCode NWM_StartScanEx(NWMCallbackFunc callback, const NWMScanExParam *param) {
    NWMScanCmd cmd;
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_DISCONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_START_SCAN) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (param == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (param->scanBuf == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (param->ssidLength > 32) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (param->ssidMatchLength > 32) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    MI_CpuFill8(param->scanBuf, 0, (param->scanBufSize + 3) & ~3);
    DC_StoreRange(param->scanBuf, (param->scanBufSize + 3) & ~3);
    NWMi_SetCallbackTable(NWM_APIID_START_SCAN, callback);
    cmd.cmd = NWM_APIID_START_SCAN;
    cmd.channelList = param->channelList;
    cmd.bufferSize = param->scanBufSize;
    cmd.dwellTime = param->channelDwellTime;
    cmd.buffer = param->scanBuf;
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

// The elements are walked byte by byte: each is its ID, its length and that many bytes
NWMInfoElements *NWM_GetInfoElements(NWMBssDesc *bssDesc, u8 elementID) {
    u8 *element = (u8 *)bssDesc->elements;
    u8 *end = element + bssDesc->length - NWM_BSS_DESC_ELEMENTS_OFFSET;

    while (element[0] != elementID && element + element[1] + 2 <= end) {
        element += element[1] + 2;
    }
    return element[0] == elementID ? (NWMInfoElements *)element : NULL;
}

NWMInfoElements *NWM_GetVenderInfoElements(NWMBssDesc *bssDesc, u8 elementID, const u8 ouiType[4]) {
    u8 *element = (u8 *)bssDesc->elements;
    u8 *end = element + bssDesc->length - NWM_BSS_DESC_ELEMENTS_OFFSET;

    if (elementID != 0xdd) {
        return NULL;
    }
    while (element + element[1] + 2 <= end) {
        if (element[0] == elementID && MI_CpuComp8(element + 2, ouiType, 4) == 0) {
            return (NWMInfoElements *)element;
        }
        element += element[1] + 2;
    }
    return NULL;
}

// The command that connects, which the ARM7 reads the scan result from sBlockBuf for
typedef struct NWMConnectCmd {
    u16 cmd;
    u8 nwType;
    void *bssDesc;
} NWMConnectCmd;

NWMRetCode NWM_Connect(NWMCallbackFunc callback, u8 nwType, const NWMBssDesc *pBdesc) {
    NWMConnectCmd cmd;
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_DISCONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_CONNECT) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (pBdesc == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (pBdesc->length > 0x200) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (nwType >= NWM_NWTYPE_NUM) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (pBdesc->ssidLength == 0) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    // The scan result is cleaned for twice its length and copied for its length: one of them is not in bytes
    DC_StoreRange(pBdesc, pBdesc->length * 2);
    NWMi_SetCallbackTable(NWM_APIID_CONNECT, callback);
    cmd.cmd = NWM_APIID_CONNECT;
    cmd.nwType = nwType;
    cmd.bssDesc = sBlockBuf;
    MI_CpuCopy8(pBdesc, sBlockBuf, pBdesc->length);
    DC_StoreRange(cmd.bssDesc, 0x200);
    DC_StoreRange(&cmd, 8);
    result = func_02703b24(&cmd, 8);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_Disconnect(NWMCallbackFunc callback) {
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_INFRA_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_DISCONNECT) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    NWMi_SetCallbackTable(NWM_APIID_DISCONNECT, callback);
    result = NWMi_SendCommand(NWM_APIID_DISCONNECT, 0);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// The command that sets the WEP keys: the WEP mode becomes the key's length in bytes, and the 0x50 bytes of the keys go
// along
typedef struct NWMWepKeyCmd {
    u16 cmd;
    u16 authMode;
    u16 wepkeyid;
    u16 wepmode;
    u8 wepkey[0x50];
} NWMWepKeyCmd;

NWMRetCode NWM_SetWEPKey(NWMCallbackFunc callback, u16 wepmode, u16 wepkeyid, const u8 *wepkey, u16 authMode) {
    NWMWepKeyCmd cmd;
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_DISCONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_SET_WEPKEY) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    switch (wepmode) {
    case 0:
        wepmode = 0;
        break;
    case 1:
        wepmode = 5;
        break;
    case 2:
        wepmode = 13;
        break;
    case 3:
        wepmode = 16;
        break;
    default:
        return NWM_RETCODE_INVALID_PARAM;
    }
    DC_StoreRange(wepkey, 0x50);
    NWMi_SetCallbackTable(NWM_APIID_SET_WEPKEY, callback);
    cmd.cmd = NWM_APIID_SET_WEPKEY;
    cmd.wepkeyid = wepkeyid;
    cmd.wepmode = wepmode;
    cmd.authMode = authMode;
    MI_CpuCopy8(wepkey, cmd.wepkey, 0x50);
    result = func_02703b24(&cmd, sizeof(cmd));
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_SetPowerSaveMode(NWMCallbackFunc callback, NWMPowerMode powerSave) {
    NWMRetCode result;

    result = NWMi_CheckState(2, NWM_STATE_DISCONNECTED, NWM_STATE_INFRA_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (NWMi_IsAPIActive(NWM_APIID_SET_PS_MODE) == TRUE) {
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    NWMi_SetCallbackTable(NWM_APIID_SET_PS_MODE, callback);
    NWMi_GetSystemWork();
    result = NWMi_SendCommand(NWM_APIID_SET_PS_MODE, 1, powerSave);
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_SendFrame(NWMCallbackFunc callback, const u8 *destAddr, const u16 *sendFrame, u16 sendFrameSize) {
    NWMSys *sys = NWMi_GetSystemWork();
    NWMRetCode result;
    OSIntrMode irq;

    result = NWMi_CheckState(1, NWM_STATE_INFRA_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    irq = OS_DisableInterrupts();
    if (sendFrameSize > 0x5f2) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    DC_StoreRange(sendFrame, sendFrameSize);
    // The library writes the MAC addresses into the frame that the SDK's API takes as const
    func_02703764(destAddr, sys->status->macAddr, (u8 *)sendFrame, sendFrameSize);
    DC_StoreRange(sendFrame, 14);
    result = NWMi_SendCommand(NWM_APIID_SEND_FRAME, 3, callback, sendFrame, sendFrameSize);
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

NWMRetCode NWM_SetReceivingFrameBuffer(NWMCallbackFunc callback, u8 *recvBuf, u16 recvBufSize, u16 protocol) {
    NWMSys *sys = NWMi_GetSystemWork();
    NWMRetCode result;

    result = NWMi_CheckState(2, NWM_STATE_DISCONNECTED, NWM_STATE_INFRA_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    MIi_DmaFill32(sys->dmaNo, recvBuf, 0, recvBufSize, TRUE);
    DC_StoreRange(recvBuf, recvBufSize);
    NWMi_SetCallbackTable(NWM_APIID_SET_RECEIVING_BUF, func_027038bc);
    NWMi_SetReceiveCallbackTable(protocol, callback);
    result = NWMi_SendCommand(NWM_APIID_SET_RECEIVING_BUF, 3, recvBuf, recvBufSize, protocol);
    if (result == NWM_RETCODE_SUCCESS) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

NWMRetCode NWM_UnsetReceivingFrameBuffer(NWMCallbackFunc callback, u16 protocol) {
    NWMRetCode result;

    NWMi_GetSystemWork();
    result = NWMi_CheckState(2, NWM_STATE_DISCONNECTED, NWM_STATE_INFRA_CONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    NWMi_SetCallbackTable(NWM_APIID_UNSET_RECEIVING_BUF, callback);
    NWMi_SetReceiveCallbackTable(protocol, NULL);
    result = NWMi_SendCommand(NWM_APIID_UNSET_RECEIVING_BUF, 1, protocol);
    if (result == NWM_RETCODE_SUCCESS) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// The callback of the command that gives the ARM7 a buffer to receive in: runs the callback of the event's index
// with the event, and when the buffer is full gives it to the ARM7 again
static void func_027038bc(void *event) {
    NWMEvent *ev = event;
    NWMSys *sys = NWMi_GetSystemWork();
    BOOL ok;

    if (ev->result != 0) {
        if (ev->result != NWM_RETCODE_RECV_IND) {
            return;
        }
        ok = FALSE;
    } else {
        ok = TRUE;
    }
    if (sys->indicate[ev->u04.port] != NULL) {
        DC_StoreRange(ev, 16);
        sys->indicate[ev->u04.port](ev);
    }
    if (ok) {
        return;
    }
    NWMi_SendCommand(NWM_APIID_SET_RECEIVING_BUF, 3, ev->u0c.buffer, 0, ev->u04.port);
}

// The command that sets the pre-shared key
typedef struct NWMWpaPskCmd {
    u16 cmd;
    u16 auth;
    u8 psk[32];
} NWMWpaPskCmd;

NWMRetCode NWM_SetWPAPSK(NWMCallbackFunc callback, NWMWpaParam *param) {
    NWMWpaPskCmd cmd;
    NWMRetCode result;

    result = NWMi_CheckState(1, NWM_STATE_DISCONNECTED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    if (param == NULL) {
        return NWM_RETCODE_INVALID_PARAM;
    }
    DC_StoreRange(param, 0x22);
    switch (param->auth) {
    case 4:
    case 5:
    case 6:
    case 7:
        break;
    default:
        return NWM_RETCODE_INVALID_PARAM;
    }
    NWMi_SetCallbackTable(NWM_APIID_SET_WPA_PSK, callback);
    cmd.cmd = NWM_APIID_SET_WPA_PSK;
    cmd.auth = param->auth;
    MI_CpuCopy8(param->psk, cmd.psk, 32);
    result = func_02703b24(&cmd, sizeof(cmd));
    if (result == NWM_RETCODE_SUCCESS || result == NWM_RETCODE_OPERATING) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

static void NWMi_SetCallbackTable(NWMApiid id, NWMCallbackFunc callback) {
    NWMi_GetSystemWork()->callbacks[id] = callback;
}

static void NWMi_SetReceiveCallbackTable(NWMFramePort port, NWMCallbackFunc callback) {
    NWMi_GetSystemWork()->indicate[port] = callback;
}

// Sends a command with its words
static NWMRetCode NWMi_SendCommand(NWMApiid id, u16 paramNum, ...) {
    int sent;
    va_list args;
    int i;
    NWMCmdBuf *buf;
    NWMSys *sys = NWMi_GetSystemWork();
    NWMRetCode result;

    buf = func_02703b98();
    if (buf == NULL) {
        return NWM_RETCODE_FIFO_ERROR;
    }
    buf->cmd = id;
    va_start(args, paramNum);
    for (i = 0; i < paramNum; i++) {
        buf->args[i] = va_arg(args, u32);
    }
    DC_StoreRange(buf, sizeof(NWMCmdBuf));
    sent = PXI_SendWordByFifo(PXI_FIFO_TAG_NWM, (u32)buf, FALSE);
    OS_SendMessage(&sys->freeQueue, buf, OS_MESSAGE_BLOCK);
    result = NWM_RETCODE_FIFO_ERROR;
    if (sent >= 0) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// Sends a command that is a struct
static NWMRetCode func_02703b24(const void *cmd, u32 size) {
    NWMSys *sys = NWMi_GetSystemWork();
    NWMCmdBuf *buf;
    int sent;
    NWMRetCode result;

    buf = func_02703b98();
    if (buf == NULL) {
        return NWM_RETCODE_FIFO_ERROR;
    }
    MI_CpuCopy8(cmd, buf, size);
    DC_StoreRange(buf, size);
    sent = PXI_SendWordByFifo(PXI_FIFO_TAG_NWM, (u32)buf, FALSE);
    OS_SendMessage(&sys->freeQueue, buf, OS_MESSAGE_BLOCK);
    result = NWM_RETCODE_FIFO_ERROR;
    if (sent >= 0) {
        result = NWM_RETCODE_OPERATING;
    }
    return result;
}

// Takes the next command buffer that the ARM7 is done with, or NULL when the queue is empty
static NWMCmdBuf *func_02703b98(void) {
    NWMCmdBuf *buf = NULL;
    NWMSys *sys = NWMi_GetSystemWork();

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
