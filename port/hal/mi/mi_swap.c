// Host version of NitroSDK's mi_swap.c (lib/nitro/src/mi/mi_swap.c): MI_SwapWord, which the SDK wrote in assembly for
// the ARM's SWP instruction.
#include "nitro/mi.h"

// Stores data at destp and returns the word that was there. The original does it with SWP, in one step that no
// interrupt or other processor can come between. The port runs the game on one host thread, where nothing can come
// between a read and a write, so they do the same here
u32 MI_SwapWord(u32 data, vu32 *destp) {
    u32 old = *destp;

    *destp = data;
    return old;
}
