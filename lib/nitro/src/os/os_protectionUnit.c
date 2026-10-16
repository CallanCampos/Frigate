// NitroSDK OS: the memory protection unit's switch (os_protectionUnit.c, the SDK's name for the module). ARM code.
// Both functions are assembly in the SDK, since only an instruction changes the CP15 control register.
#include "nitro/os.h"

// clang-format off

// NitroSDK's OS_EnableProtectionUnit: sets bit 0 (the MPU enable) of the CP15 control register
asm void OS_EnableProtectionUnit(void) {
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #1
    mcr p15, 0, r0, c1, c0, 0
    bx lr
}

// NitroSDK's OS_DisableProtectionUnit: clears that bit
asm void OS_DisableProtectionUnit(void) {
    mrc p15, 0, r0, c1, c0, 0
    bic r0, r0, #1
    mcr p15, 0, r0, c1, c0, 0
    bx lr
}
// clang-format on
