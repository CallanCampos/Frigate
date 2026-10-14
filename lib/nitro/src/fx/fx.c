// NitroSDK libfx's fx.c: the fixed point setup stub and the function forms of the multiplications and the split into
// whole and fraction. The split from fx_atanidx.c and fx_trig.c is fairly sure: the Thumb functions around the two ARM
// leaf functions are small and unrelated to the tables.
#include "nitro/fx.h"

void FX_Init(void) {
}

#pragma thumb off
fx32 FX_MulFunc(fx32 v1, fx32 v2) {
    return (fx32)(((fx64)v1 * v2 + 0x800LL) >> FX32_SHIFT);
}

fx32 FX_Mul32x64cFunc(fx32 v32, fx64 v64c) {
    return (fx32)((v64c * v32 + 0x80000000LL) >> 32);
}
#pragma thumb on

fx32 FX_Modf(fx32 x, fx32 *whole) {
    if (x >= 0) {
        *whole = x & 0x7ffff000;
        return x & FX32_DEC_MASK;
    }
    *whole = -(-x & 0x7ffff000);
    return -(-x & FX32_DEC_MASK);
}
