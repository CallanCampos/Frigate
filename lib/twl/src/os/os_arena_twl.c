// TwlSDK's DSi part of os_arena.c (ARM9 main), in the LTD autoload: what main's OS_InitArenaEx does on a DSi. The file
// name is inferred from main's half; the function's name is ours
#include "nitro/os.h"
#include "twl/os.h"

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

void OSi_InitArenaExTwl(void) {
    // The SDK reads the console type here and drops it, as main's half does
    OS_GetConsoleType();
    OS_SetProtectionRegion(3, HW_CTRDG_ROM, HW_C6_PR_64MB);
}

#pragma section LTDMAIN end
