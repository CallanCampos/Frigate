// TwlSDK's NWM, the DSi's wireless manager, ARM9 side: it keeps a system area for the commands that it sends the ARM7
// over the PXI tag 0x16 and for the events that come back, checks that the commands suit the state that the ARM7
// reports, and runs the callbacks. ARM code in the DSi-only LTD autoload. The ROM names none of it; the file's name and
// those of the types and data are descriptive guesses, and the functions have the names of the SDK functions they
// implement where those are known, from what they do, and keep their default names otherwise.

#include "types.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "nitro/wm.h"
#include "twl/nwm.h"
#include <stdarg.h>

// The commands, by their numbers: the first halfword of the buffer that goes to the ARM7 and of the event that comes
// back. They also index the callbacks and the ARM7's flags of the commands it is running. The names are guessed from
// the states that take a command and from its parameters; a number is left in the name of those with nothing to
// show what they do
enum {
    NWM_CMD_1 = 1,
    NWM_CMD_LOAD_DEVICE = 3,
    NWM_CMD_UNLOAD_DEVICE = 4,
    NWM_CMD_OPEN = 5,
    NWM_CMD_CLOSE = 6,
    NWM_CMD_SCAN = 7,
    NWM_CMD_8 = 8,
    NWM_CMD_DISCONNECT = 9,
    NWM_CMD_SET_RECEIVE_BUFFER = 10,
    NWM_CMD_SEND_FRAME = 11,
    NWM_CMD_12 = 12,
    NWM_CMD_CONNECT = 13,
    NWM_CMD_14 = 14,
    NWM_CMD_SET_PMK = 18,
    NWM_CMD_19 = 19,
    NWM_CMD_COUNT = 20
};

// The results of an event that the library tells apart: a sent frame that the ARM7 gave up, and a receive buffer
// that is full (the callback of the command that sets it gives it back)
#define NWM_RESULT_12 12
#define NWM_RESULT_BUFFER_FULL 11

// What the ARM7 reports as the library's state, which the commands check. The numbers 0 to 4 are in the order that
// the commands take them in; 5 is only known from the command that unloads the device in any state
enum { NWM_STATE_NONE, NWM_STATE_INITIALIZED, NWM_STATE_LOADED, NWM_STATE_READY, NWM_STATE_CONNECTED, NWM_STATE_5 };

// The first halfword of a command buffer while the ARM7 is done with the buffer, and the number of buffers
#define NWM_CMD_BUF_FREE 0x8000
#define NWM_CMD_BUF_COUNT 32
#define NWM_INDICATE_COUNT 3

// The priority of the library's callbacks in the lists of those that run when the console sleeps and exits
#define NWM_PM_PRIORITY 0xffff

// What the exit callback waits for the ARM7: a first spin of 400000 cycles, spins of 10000 until the commands it
// runs are done, and at most about a second (the ARM9's clock, in cycles) for the device to unload
#define NWM_EXIT_SPIN_FIRST 400000
#define NWM_EXIT_SPIN_STEP 10000
#define NWM_EXIT_TIMEOUT 67028000

// A command buffer: the command and its words
typedef struct NWMCmdBuf {
    u16 cmd;
    u16 unk_02;
    u32 args[63];
} NWMCmdBuf;

// The flags that the ARM7 sets in the shared memory, of which the library clears the first
typedef struct NWMInfo {
    u8 flags;
    u8 unk_01[0x1f];
} NWMInfo;

#define NWM_INFO_PENDING 0x01

// The ARM7's status block, which it writes in the shared memory
typedef struct NWMStatus {
    u8 pending[0x24]; // By command: 1 while the ARM7 runs it
    u16 state;
    u8 unk_26[0x1a];
    u8 macAddr[6];
    u8 unk_46[0x58];
    u16 unk_9e;
    u8 unk_a0[0x20];
} NWMStatus;

// What the ARM7 sends: the number of the command it answers, its result, and the command's data
typedef struct NWMEvent {
    u16 cmd;
    u16 result;
    union {
        NWMCallback sendCallback; // The event of a frame sent: the callback that the frame was given
        u16 index;                // The event of frames received: the index of the callback
    } u04;
    u32 unk_08;
    union {
        u32 size;     // The event of a scan: the size of the data
        void *buffer; // The event of frames received: the buffer that is full
    } u0c;
    void *data;
    u8 unk_14[0xec];
} NWMEvent;

// The system area that the game gives the library, its first part: the buffers of the commands, the queue of those the
// ARM7 is done with, the callbacks and pointers to the three blocks that follow it (the flags, the status and an event)
typedef struct NWMSys {
    NWMCmdBuf cmdBuf[NWM_CMD_BUF_COUNT];      // 0x0000
    OSMessageQueue freeQueue;                 // 0x2000
    OSMessage freeMsgs[NWM_CMD_BUF_COUNT];    // 0x2020
    u8 unk_20a0[0x80];                        // 0x20a0
    u8 unk_2120[0x154];                       // 0x2120: cleared when the library starts
    NWMInfo *info;                            // 0x2274
    NWMStatus *status;                        // 0x2278
    NWMEvent *event;                          // 0x227c
    u16 dmaNo;                                // 0x2280
    NWMCallback callbacks[NWM_CMD_COUNT];     // 0x2284: of the commands
    NWMCallback indicate[NWM_INDICATE_COUNT]; // 0x22d4: of the frames received, by index
} NWMSys;

// A pointer in the DSi's shared memory to a block of its settings. The library checks the 16 bytes at 0x1e0 of it:
// a word whose upper half is the CRC-16 of the 12 bytes that follow it
#define NWM_SETTINGS_PTR 0x02fffdfc

static NWMRetCode func_02702468(NWMSys *sys, u32 size, u32 dmaNo);
static BOOL func_027026b4(void);
static NWMSys *func_027026dc(void);
static void func_027026ec(void);
static void func_02702740(u32 tag, u32 data, BOOL err);
static NWMRetCode func_027028e4(u32 count, ...);
static void func_02702a40(void);
static void func_02702a78(void);
static void func_02702a8c(void *arg);
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

// The data, in memory: sNwmInitialized, sNwm (the system area and the two PM callbacks), sUnloaded (set when the exit
// callback's unload is done) and sBlockBuf (the block that the command 8 copies for the ARM7). -ipa function lays out
// the statics defined with an initializer first and the tentative ones after them, in the reverse of their
// declarations. The flag's initializer also makes it known to the functions below it, which reach it from the base of
// the file's .bss and pass its address through a literal of its own, as func_02702468 does
static u8 sNwmInitialized = FALSE;
static u32 sBlockBuf[0x210 / 4];
static u8 sUnloaded;
static struct {
    NWMSys *sys;
    PMExitCallbackInfo exitCallback;
    PMSleepCallbackInfo sleepCallback;
} sNwm;

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

NWMRetCode NWM_Init(void *systemBuffer, u32 size, u32 dmaNo) {
    if (!func_027026b4()) {
        return NWM_RETCODE_UNAVAILABLE;
    }
    return func_02702468(systemBuffer, size, dmaNo);
}

static NWMRetCode func_02702468(NWMSys *sys, u32 size, u32 dmaNo) {
    u32 irq;
    int i;

    irq = OS_DisableInterrupts();
    if (WM_CheckInitialized() == 0) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (sNwmInitialized) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (sys == NULL) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (dmaNo > 3) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (((u32)sys & 0x1f) != 0) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    PXI_Init();
    if (!PXI_IsCallbackReady(PXI_FIFO_TAG_NWM, PXI_PROC_ARM7)) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_UNAVAILABLE;
    }
    DC_InvalidateRange(sys, size);
    MIi_DmaFill32(dmaNo, sys, 0, size, TRUE);
    sNwm.sys = sys;
    DC_StoreRange(&sNwm.sys, 4);
    sNwm.sys->info = (NWMInfo *)(sNwm.sys + 1);
    sNwm.sys->status = (NWMStatus *)(sNwm.sys->info + 1);
    sNwm.sys->event = (NWMEvent *)(sNwm.sys->status + 1);
    func_027026ec();
    sNwm.sys->dmaNo = dmaNo;
    OS_InitMessageQueue(&sNwm.sys->freeQueue, sNwm.sys->freeMsgs, NWM_CMD_BUF_COUNT);
    for (i = 0; i < NWM_CMD_BUF_COUNT; i++) {
        sNwm.sys->cmdBuf[i].cmd = NWM_CMD_BUF_FREE;
        DC_StoreRange(&sNwm.sys->cmdBuf[i], 2);
        OS_SendMessage(&sNwm.sys->freeQueue, &sNwm.sys->cmdBuf[i], OS_MESSAGE_BLOCK);
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_NWM, func_02702740);
    MI_CpuFill8(sNwm.sys->unk_2120, 0, sizeof(sNwm.sys->unk_2120));
    sNwmInitialized = TRUE;
    sNwm.sys->status->state = NWM_STATE_INITIALIZED;
    DC_StoreRange(sNwm.sys, sizeof(NWMSys));
    DC_StoreRange(&sNwmInitialized, 1);
    OS_RestoreInterrupts(irq);
    return NWM_RETCODE_SUCCESS;
}

NWMRetCode func_02702698(void) {
    return sNwmInitialized ? NWM_RETCODE_SUCCESS : NWM_RETCODE_ILLEGAL_STATE;
}

// Whether the wireless is available and not turned off
static BOOL func_027026b4(void) {
    if (OS_IsAvailableWireless() == TRUE && !OS_IsForceDisableWireless()) {
        return TRUE;
    }
    return FALSE;
}

// The library's system area. The commands' bare calls of it, whose result they drop, are left from the SDK's fetches of
// the work for assertions that its release build compiles out
static NWMSys *func_027026dc(void) {
    return sNwm.sys;
}

// Clears the flag by which the ARM7 says it has something for the ARM9
static void func_027026ec(void) {
    NWMInfo *info;

    if (sNwm.sys == NULL) {
        return;
    }
    info = sNwm.sys->info;
    DC_InvalidateRange(info, 1);
    if (info->flags & NWM_INFO_PENDING) {
        info->flags &= ~NWM_INFO_PENDING;
        DC_StoreRange(info, 1);
    }
}

// The PXI callback: takes the event that the ARM7 points to, runs the callbacks and gives the buffer back
static void func_02702740(u32 tag, u32 data, BOOL err) {
    NWMSys *sys = sNwm.sys;
    NWMEvent *event = (NWMEvent *)data;
    NWMCallback callback;

    if (sys == NULL) {
        return;
    }
    if (err) {
        return;
    }
    if (event == NULL) {
        return;
    }
    // BUG: the event is read before its cache lines are invalidated, so the check can see the event that was there
#ifdef BUGFIX
    DC_InvalidateRange(event, sizeof(NWMEvent));
#endif
    if (event->cmd >= NWM_CMD_COUNT) {
        return;
    }
    DC_InvalidateRange(sys->event, sizeof(NWMEvent));
    DC_InvalidateRange(sys->status, sizeof(NWMStatus));
    if (event != sys->event) {
        DC_InvalidateRange(event, sizeof(NWMEvent));
    }
    if ((event->cmd == NWM_CMD_LOAD_DEVICE && event->result != 0) ||
        (event->cmd == NWM_CMD_UNLOAD_DEVICE && event->result == 0) || event->cmd == NWM_CMD_19) {
        func_02702a78();
    }
    if (event->cmd == NWM_CMD_SEND_FRAME && event->result != NWM_RESULT_12 && event->u04.sendCallback != NULL) {
        event->u04.sendCallback(event);
    }
    if (event->cmd != NWM_CMD_SEND_FRAME) {
        if (event->cmd == NWM_CMD_UNLOAD_DEVICE || event->cmd == NWM_CMD_19 ||
            (event->cmd == NWM_CMD_LOAD_DEVICE && event->result != 0)) {
            PM_DeletePreExitCallback(&sNwm.exitCallback);
        }
        callback = sys->callbacks[event->cmd];
        if (event->cmd == NWM_CMD_SCAN) {
            DC_InvalidateRange(event->data, event->u0c.size);
        }
        if (callback != NULL) {
            callback(event);
        }
    }
    MI_CpuFill8(event, 0, sizeof(NWMEvent));
    DC_StoreRange(event, sizeof(NWMEvent));
    if (sys != NULL) {
        func_027026ec();
    }
}

u16 func_027028b0(void) {
    NWMSys *sys = func_027026dc();
    NWMStatus *status;
    u16 state = 0;

    if (sys != NULL) {
        status = sys->status;
        // BUG: the state is 0x24 bytes into the block, in another cache line than the one that is invalidated
#ifdef BUGFIX
        DC_InvalidateRange(&status->state, 2);
#else
        DC_InvalidateRange(status, 2);
#endif
        state = status->state;
    }
    return state;
}

// Whether the state is one of the count states that follow
static NWMRetCode func_027028e4(u32 count, ...) {
    NWMRetCode result;
    u16 state;
    va_list args;
    NWMSys *sys = func_027026dc();

    result = func_02702698();
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    DC_InvalidateRange(&sys->status->state, 2);
    state = sys->status->state;
    va_start(args, count);
    result = NWM_RETCODE_ILLEGAL_STATE;
    for (; count != 0; count--) {
        if (va_arg(args, u32) == state) {
            result = NWM_RETCODE_SUCCESS;
        }
    }
    return result;
}

u16 func_02702980(void) {
    return func_027026dc()->status->unk_9e;
}

u16 func_02702998(int type) {
    u8 mac[6];
    u16 sum;
    int i;
    int value;

    OS_GetMacAddress(mac);
    for (i = 0, sum = 0; i < 6; i++) {
        sum += mac[i];
    }
    sum += OS_GetVBlankCount();
    value = sum * 13;
    if (type == 1) {
        return value % 10 + 30;
    }
    return value % 10 + 105;
}

// Puts the callback that exits the application before the console sleeps (the wireless would be on)
static void func_02702a40(void) {
    PM_SetSleepCallbackInfo(&sNwm.sleepCallback, func_02702a8c, NULL);
    PM_InsertPreSleepCallback(&sNwm.sleepCallback, NWM_PM_PRIORITY);
}

static void func_02702a78(void) {
    PM_DeletePreSleepCallback(&sNwm.sleepCallback);
}

static void func_02702a8c(void *arg) {
    OS_Terminate();
}

NWMRetCode NWM_End(void) {
    u32 irq;
    NWMRetCode result;

    result = func_027028e4(1, NWM_STATE_INITIALIZED);
    if (result != NWM_RETCODE_SUCCESS && result != NWM_RETCODE_OPERATING) {
        return result;
    }
    irq = OS_DisableInterrupts();
    sNwm.sys->status->state = NWM_STATE_NONE;
    func_027026ec();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_NWM, NULL);
    sNwm.sys = NULL;
    sNwmInitialized = FALSE;
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
    info = &sNwm.exitCallback;
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

    if (!func_02769520()) {
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
    u32 irq;

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
