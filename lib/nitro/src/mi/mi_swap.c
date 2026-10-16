// NitroSDK's mi_swap.c: MI_SwapWord. The SDK wrote it in assembly, for the SWP instruction that C has no way to
// ask for.
#include "nitro/mi.h"

// Stores data at destp and returns the word that was there, in one step. OSi_DoTryLockByWord uses it
// clang-format off
asm u32 memswap(u32 data, vu32 *destp) {
    swp r0, r0, [r1]
    bx lr
}
// clang-format on
