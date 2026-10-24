// NitroSDK's OS_Init on the host (os_init.c): the same sequence as the decomp's lib/nitro/src/os/os_init.c, which
// starts the OS and the libraries that the SDK starts with it, first of all in the game's boot (SystemInit,
// src/system/machine_use.c). The parts that are the OS's run on the host's versions in port/hal/os; the others are the
// decomp's C or the HAL's modules for them: PXI_Init boots the host ARM7 (port/hal/pxi), CARD_Init the card and its
// save chip (port/hal/card), PM_Init power management (port/hal/pm). The DSi's parts are skipped as on a DS, since
// OS_IsRunOnTwl gives FALSE (the HAL's SCFG registers read 0). The wait for the screen's first line (OSi_WaitVCount0,
// assembly) waits on the virtual clock, so that OS_Init returns at the start of a frame as on the DS, and the rest of
// the boot keeps the DS's frame phase.
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"
#include "nitro/card.h"
#include "nitro/ctrdg.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "twl/scfg.h"

static void OSi_InitBody(void);

// NitroSDK's OS_Init, which only starts the function that does the work
void OS_Init(void) {
    OSi_InitBody();
}

// TwlSDK's OSi_WaitVCount0 (assembly, static in the SDK; os_internal.h declares it for the tests): turns IME off,
// waits for the screen's line counter to read 0 and sets IME back. The clock runs to the start of the next frame, or
// not at all when it is on line 0 already. An interrupt that came meanwhile and that IE enables is taken when IME
// comes back, as the DS takes it on the next instruction if the CPSR lets it
void OSi_WaitVCount0(void) {
    u16 ime = reg_OS_IME;

    reg_OS_IME = 0;
    if (port_os_vcount() != 0) {
        port_osi_spin(PORT_OS_CYCLES_PER_FRAME - port_os_cycles() % PORT_OS_CYCLES_PER_FRAME);
    }
    reg_OS_IME = ime;
    port_osi_poll_irq();
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

// NitroSDK's OSi_ReferSymbol, which the libraries call with their signature strings to keep them in the link: it does
// nothing
void OSi_ReferSymbol(void *signature) {
    (void)signature;
}
