// NitroSDK OS: tightly coupled memory (os_tcm.c, the SDK's name for the module). ARM code.
#include "nitro/os.h"

// clang-format off

// NitroSDK's OS_GetDTCMAddress: where the data TCM is mapped, from the CP15 register of its region (the low 12 bits are
// the size and enable bit). This is assembly in the SDK, since only an instruction reads the coprocessor
asm u32 OS_GetDTCMAddress(void) {
    mrc p15, 0, r0, c9, c1, 0
    ldr r1, =0xfffff000
    and r0, r0, r1
    bx lr
}
// clang-format on
