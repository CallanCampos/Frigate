#ifndef POKEBW2_TWL_NWM_PRIVATE_H
#define POKEBW2_TWL_NWM_PRIVATE_H

#include "types.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "twl/nwm.h"

// What NWM's files share and twl/nwm.h doesn't export: the commands, the system area and the blocks that the ARM7
// shares with the ARM9. The name is ours

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

// nwm_setup.c: whether NWM_Init has set the library up
extern u8 NWMi_SetUpDone;
BOOL func_027026b4(void);

// nwm_events.c: the system area that NWM_Init was given, and the callback that unloads the device before the
// application exits (NWM_LoadDevice puts it)
extern NWMSys *NWMi_SystemArea;
extern PMExitCallbackInfo NWMi_UnloadOnExit;
NWMSys *func_027026dc(void);
void func_027026ec(void);
void func_02702740(PXIFifoTag tag, u32 data, BOOL err);
NWMRetCode func_027028e4(u32 count, ...);
void func_02702a40(void);

#endif // POKEBW2_TWL_NWM_PRIVATE_H
