#ifndef POKEBW2_TWL_NWM_PRIVATE_H
#define POKEBW2_TWL_NWM_PRIVATE_H

#include "types.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "twl/nwm.h"

// What NWM's files share and twl/nwm.h doesn't export: the system area and the blocks that the ARM7 shares with the
// ARM9, and the library's internal functions (libntr's nwm_arm9_private.h names those). The name is ours

// The first halfword of a command buffer while the ARM7 is done with the buffer, and the number of buffers
#define NWM_CMD_BUF_FREE 0x8000
#define NWM_CMD_BUF_COUNT 32

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
        NWMCallbackFunc sendCallback; // The event of a frame sent: the callback that the frame was given
        u16 port;                     // The event of frames received: the port of the callback
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
    NWMCmdBuf cmdBuf[NWM_CMD_BUF_COUNT];                 // 0x0000
    OSMessageQueue freeQueue;                            // 0x2000
    OSMessage freeMsgs[NWM_CMD_BUF_COUNT];               // 0x2020
    u8 unk_20a0[0x80];                                   // 0x20a0
    u8 unk_2120[0x154];                                  // 0x2120: cleared when the library starts
    NWMInfo *info;                                       // 0x2274
    NWMStatus *status;                                   // 0x2278
    NWMEvent *event;                                     // 0x227c
    u16 dmaNo;                                           // 0x2280
    NWMCallbackFunc callbacks[NWM_APIID_ASYNC_KIND_MAX]; // 0x2284: of the commands
    NWMCallbackFunc indicate[NWM_PORT_NUM];              // 0x22d4: of the frames received, by port
} NWMSys;

// nwm_setup.c: whether NWM_Init has set the library up
extern u8 NWMi_SetUpDone;
BOOL NWMi_CheckEnableFlag(void);

// nwm_events.c: the system area that NWM_Init was given, and the callback that unloads the device before the
// application exits (NWM_LoadDevice puts it)
extern NWMSys *NWMi_SystemArea;
extern PMExitCallbackInfo NWMi_UnloadOnExit;
NWMSys *NWMi_GetSystemWork(void);
void func_027026ec(void);
void func_02702740(PXIFifoTag tag, u32 data, BOOL err);
NWMRetCode NWMi_CheckState(s32 paramNum, ...);
void func_02702a40(void);

#endif // POKEBW2_TWL_NWM_PRIVATE_H
