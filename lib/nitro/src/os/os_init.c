// TwlSDK OS: OS_Init (os_init.c, the SDK's name for the module). Thumb code, with the one function that waits for the
// first line of the screen in ARM.
#include "nitro/card.h"
#include "nitro/ctrdg.h"
#include "nitro/mi.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/scfg.h"

// clang-format off

// TwlSDK's OSi_WaitVCount0, which is assembly there: waits for the screen's line counter to be 0, with the interrupts
// off (IME is stored from the register that holds its address, a value whose bit 0 is clear)
#pragma thumb off
static asm void func_0207b104(void) {
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

static void func_0207b12c(void);

// NitroSDK's OS_Init, which only starts the function that does the work
void os_init(void) {
    func_0207b12c();
}

// The work of OS_Init: starts the OS and the libraries that the SDK starts with it, in this order
static void func_0207b12c(void) {
    PXI_Init();
    OS_InitArena();
    func_02079fe4(); // OS_InitLock
    func_0207b23c(); // OS_InitArenaEx
    func_02079d10(); // OS_InitIrqTable
    OS_SetIrqStackChecker();
    exceptions_init(); // OS_InitException
    func_020791d0();   // MI_Init
    OS_InitVAlarm();
    func_0207c4fc();         // OSi_InitVramExclusive
    scheduler_init();        // OS_InitThread
    func_0207c248();         // OS_InitReset
    func_0200548c();         // CTRDG_Init
    cart_read_thread_init(); // CARD_Init
    func_02078000();         // TwlSDK's shared WRAM manager, on a DSi
    if (hw_isDSi() == TRUE) {
        func_0207f6cc(); // SCFG: sets the PXI callback for its tag
    }
    func_0207ebb8(); // PM_Init
    func_0207b104(); // OSi_WaitVCount0
    if (hw_isDSi() == TRUE) {
        func_0207c888();
    }
}
