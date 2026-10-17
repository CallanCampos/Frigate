// Host version of NitroSDK's MATH_CountLeadingZerosFunc, a CLZ instruction in the SDK's assembly. The original is in
// lib/nitro/src/math/math_pop.c, behind #ifndef PLATFORM_PC. The host's MATH_CountLeadingZeros in nitro/math.h
// counts the same way, with 32 for zero as CLZ gives.
#include "nitro/math.h"

u32 MATH_CountLeadingZerosFunc(u32 x) {
    return MATH_CountLeadingZeros(x);
}
