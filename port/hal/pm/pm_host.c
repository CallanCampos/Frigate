// NitroSDK's power management on the host: the decomp's lib/nitro/src/spi/pm.c, compiled unchanged through this file
// (port/hal/pm/replaced.txt keeps it out of the game's own sources), with its requests answered by the host's ARM7
// (pm7.c). One function is written again: PM_Init clears a word of the shared memory at the DS address that pm.c
// defines itself (HW_PM_EXIT_FLAG, 0x02fff684). A definition in a .c file isn't translated to host memory as the SDK
// headers' addresses are (ADR 0004), so the DS's PM_Init would write to an address the host doesn't map; this one
// writes to the host's shared memory. Once the address is defined in a header (nitro/pm.h), this file can go.
//
// pm.c's other writes of that word are in the answers to the DSi's exit notifications, which the host's ARM7 never
// sends.
//
// The host's PM_Init reaches pm.c's statics, whose names a rename upstream would change: PMi_IsInit, PMi_Work,
// PMi_ScreensOffCount, PMi_DisplayOffCount and the callback PMi_CommonCallback. It is pm.c's own function, so a change
// to pm.c's PM_Init must be made here too until the file goes.
#define PM_Init PMi_DsInit
#include "lib/nitro/src/spi/pm.c"
#undef PM_Init

// pm.h's prototype was renamed with the function
void PM_Init(void);

// NitroSDK's PM_Init, as pm.c has it, with the exit flag in host memory
void PM_Init(void) {
    if (PMi_IsInit) {
        return;
    }
    PMi_IsInit = TRUE;
    PMi_Work.lock = FALSE;
    PMi_Work.callback = NULL;
    *(vu32 *)PORT_HW_ADDR(HW_PM_EXIT_FLAG) = 0;
    PXI_Init();
    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_PM, PXI_PROC_ARM7)) {
        SVC_WaitByLoop(100);
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_PM, PMi_CommonCallback);
    PMi_ScreensOffCount = PMi_DisplayOffCount = OS_GetVBlankCount();
}
