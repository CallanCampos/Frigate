// NitroSDK libfx's fx_cp.c: fixed point division and square root on the hardware divider and square root unit.
// The split from fx_mtx44.c and fx_vec.c is certain (a run of Thumb functions that all use the unit's registers).
#include "nitro/cp.h"
#include "nitro/fx.h"

static void FX_SqrtAsync(fx32 x);
static fx32 FX_GetSqrtResult(void);

fx32 FX_Div(fx32 numer, fx32 denom) {
    FX_DivAsync(numer, denom);
    return FX_GetDivResult();
}

fx32 FX_Inv(fx32 denom) {
    FX_InvAsync(denom);
    return FX_GetDivResult();
}

fx64 FX_InvFx64c(fx32 denom) {
    FX_InvAsync(denom);
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    return reg_CP_DIV_RESULT;
}

fx32 FX_Sqrt(fx32 x) {
    if (x > 0) {
        reg_CP_SQRTCNT = 1;
        reg_CP_SQRT_PARAM = (u64)(u32)x << 32;
        return FX_GetSqrtResult();
    }
    return 0;
}

fx32 FX_InvSqrt(fx32 x) {
    fx64 inv;

    if (x > 0) {
        FX_InvAsync(x);
        FX_SqrtAsync(x);
        inv = FX_GetDivResultFx64c();
        while (reg_CP_SQRTCNT & REG_CP_BUSY_MASK) {
        }
        return (fx32)((inv * reg_CP_SQRT_RESULT + 0x20000000000LL) >> 42);
    }
    return 0;
}

fx64 FX_GetDivResultFx64c(void) {
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    return reg_CP_DIV_RESULT;
}

fx32 FX_GetDivResult(void) {
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    return (fx32)((reg_CP_DIV_RESULT + 0x80000) >> 20);
}

void FX_InvAsync(fx32 denom) {
    reg_CP_DIVCNT = 1;
    reg_CP_DIV_NUMER = 0x100000000000LL;
    reg_CP_DIV_DENOM = (u32)denom;
}

static void FX_SqrtAsync(fx32 x) {
    if (x > 0) {
        reg_CP_SQRTCNT = 1;
        reg_CP_SQRT_PARAM = (u64)(u32)x << 32;
    } else {
        reg_CP_SQRTCNT = 1;
        reg_CP_SQRT_PARAM = 0;
    }
}

static fx32 FX_GetSqrtResult(void) {
    while (reg_CP_SQRTCNT & REG_CP_BUSY_MASK) {
    }
    return (reg_CP_SQRT_RESULT + 0x200) >> 10;
}

void FX_DivAsync(fx32 numer, fx32 denom) {
    reg_CP_DIVCNT = 1;
    reg_CP_DIV_NUMER = (u64)numer << 32;
    reg_CP_DIV_DENOM = (u32)denom;
}

s32 FX_DivS32(s32 numer, s32 denom) {
    reg_CP_DIVCNT = 0;
    reg_CP_DIV_NUMER_LO = numer;
    reg_CP_DIV_DENOM = (u32)denom;
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    return reg_CP_DIV_RESULT_LO;
}

s32 FX_ModS32(s32 numer, s32 denom) {
    reg_CP_DIVCNT = 0;
    reg_CP_DIV_NUMER_LO = numer;
    reg_CP_DIV_DENOM = (u32)denom;
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    return (s32)reg_CP_DIVREM_RESULT;
}
