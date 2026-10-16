#ifndef POKEBW2_TWL_DSP_PRIVATE_H
#define POKEBW2_TWL_DSP_PRIVATE_H

#include "types.h"

// What the DSP library's files share and the SDK's headers don't export. The name is ours

// Swaps the halves of a word: the DSP's 32-bit values are two 16-bit halves, the high one first
static inline u32 DSPi_SwapHalves(u32 value) {
    return (value >> 16) | (value << 16);
}

#endif // POKEBW2_TWL_DSP_PRIVATE_H
