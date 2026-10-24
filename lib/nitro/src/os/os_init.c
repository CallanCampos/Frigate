// TwlSDK OS: OS_Init (os_init.c, the SDK's name for the module). Thumb code, with the one function that waits for the
// first line of the screen in ARM.
#include "nitro/card.h"
#include "nitro/ctrdg.h"
#include "nitro/mi.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "twl/scfg.h"

// clang-format off
#pragma thumb off

// TwlSDK's OSi_WaitVCount0, which is assembly there: waits for the screen's line counter to be 0, with the interrupts
// off (IME is stored from the register that holds its address, a value whose bit 0 is clear)
static asm void OSi_WaitVCount0(void) {
    mov ip, #0x4000000
    ldr r1, [ip, #0x208]
    str ip, [ip, #0x208]
@wait:
    ldrh r0, [ip, #6]
    cmp r0, #0
    bne @wait
    str r1, [ip, #0x208]
    bx lr
}
#pragma thumb on
// clang-format on

static void OSi_InitBody(void);

// NitroSDK's OS_Init, which only starts the function that does the work
void OS_Init(void) {
    OSi_InitBody();
}

// The work of OS_Init (the name is ours): starts the OS and the libraries that the SDK starts with it, in this order
static void OSi_InitBody(void) {
    PXI_Init();
    OS_InitArena();
    OS_InitLock();
    OS_InitArenaEx();
    OS_InitIrqTable();
    OS_SetIrqStackChecker();
    OS_InitException();
    MI_Init();
    OS_InitVAlarm();
    OSi_InitVramExclusive();
    OS_InitThread();
    OS_InitReset();
    CTRDG_Init(); // the game's, which replaces the SDK's weak CTRDG_Init
    CARD_Init();
    MI_InitWramManager(); // TwlSDK's shared WRAM manager, on a DSi
    if (OS_IsRunOnTwl() == TRUE) {
        SCFG_Init(); // SCFG: sets the PXI callback for its tag
    }
    PM_Init();
    OSi_WaitVCount0();
    if (OS_IsRunOnTwl() == TRUE) {
        OSi_InitPrevTitleId();
    }
}
