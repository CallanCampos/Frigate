#ifndef POKEBW2_TWL_DSP_PRIVATE_H
#define POKEBW2_TWL_DSP_PRIVATE_H

#include "types.h"

// What the DSP library's files share and the SDK's headers don't export. The name is ours

// Swaps the halves of a word: the DSP's 32-bit values are two 16-bit halves, the high one first
static inline u32 DSPi_SwapHalves(u32 value) {
    return (value >> 16) | (value << 16);
}

// The size in bytes of the smallest unit that the DSP's memory is read and written in, as the ARM9 sees it: 3 less
// the size of a half word is 1 here, and 2 on the DSP, where the size of a half word is 1 (the shape of
// cybervisi0n/libntr's reconstruction; the name is ours). The pipes round their chunks down to it, which on the ARM9
// does nothing but narrow the chunk to u16 a second time
#define DSP_MEMORY_UNIT (3 - sizeof(u16))

#endif // POKEBW2_TWL_DSP_PRIVATE_H
