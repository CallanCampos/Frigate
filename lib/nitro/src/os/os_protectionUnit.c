// NitroSDK OS: the memory protection unit's switch (os_protectionUnit.c, the SDK's name for the module). ARM code.
// Both functions are assembly in the SDK, since only an instruction changes the CP15 control register.
#include "nitro/os.h"

// clang-format off

// NitroSDK's OS_EnableProtectionUnit, under swan's name: sets bit 0 (the MPU enable) of the CP15 control register
asm void cp15_mmu_enable(void) {
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #1
    mcr p15, 0, r0, c1, c0, 0
    bx lr
}

// NitroSDK's OS_DisableProtectionUnit, under swan's name: clears that bit
asm void cp15_mmu_disable(void) {
    mrc p15, 0, r0, c1, c0, 0
    bic r0, r0, #1
    mcr p15, 0, r0, c1, c0, 0
    bx lr
}
// clang-format on
