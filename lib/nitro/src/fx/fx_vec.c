// NitroSDK libfx's fx_vec.c: vector arithmetic on VecFx32 and VecFx16, with the lengths and the normalizing taken
// from the hardware divider and square root unit (the unit's registers are in nitro/cp.h). The split from fx_cp.c
// (the unit's own functions) and fx_atanidx.c is certain: the Thumb adds start the run and the ARM functions follow.
#include "nitro/cp.h"
#include "nitro/fx.h"

void VEC_Add(const VecFx32 *a, const VecFx32 *b, VecFx32 *ab) {
    ab->x = a->x + b->x;
    ab->y = a->y + b->y;
    ab->z = a->z + b->z;
}

void VEC_Subtract(const VecFx32 *a, const VecFx32 *b, VecFx32 *ab) {
    ab->x = a->x - b->x;
    ab->y = a->y - b->y;
    ab->z = a->z - b->z;
}

void VEC_Fx16Add(const VecFx16 *a, const VecFx16 *b, VecFx16 *ab) {
    ab->x = a->x + b->x;
    ab->y = a->y + b->y;
    ab->z = a->z + b->z;
}

#pragma thumb off

fx32 VEC_DotProduct(const VecFx32 *a, const VecFx32 *b) {
    fx64 sum = (fx64)a->x * b->x + (fx64)a->y * b->y + (fx64)a->z * b->z;
    return (fx32)((sum + 0x800) >> FX32_SHIFT);
}

fx32 VEC_Fx16DotProduct(const VecFx16 *a, const VecFx16 *b) {
    fx64 sum = a->x * b->x + a->y * b->y;
    sum += a->z * b->z + 0x800;
    return (fx32)(sum >> FX32_SHIFT);
}

void VEC_CrossProduct(const VecFx32 *a, const VecFx32 *b, VecFx32 *axb) {
    fx32 x = (fx32)(((fx64)a->y * b->z - (fx64)a->z * b->y + 0x800) >> FX32_SHIFT);
    fx32 y = (fx32)(((fx64)a->z * b->x - (fx64)a->x * b->z + 0x800) >> FX32_SHIFT);
    fx32 z = (fx32)(((fx64)a->x * b->y - (fx64)a->y * b->x + 0x800) >> FX32_SHIFT);
    axb->x = x;
    axb->y = y;
    axb->z = z;
}

void VEC_Fx16CrossProduct(const VecFx16 *a, const VecFx16 *b, VecFx16 *axb) {
    fx16 x = (a->y * b->z - a->z * b->y + 0x800) >> FX32_SHIFT;
    fx16 y = (a->z * b->x - a->x * b->z + 0x800) >> FX32_SHIFT;
    fx16 z = (a->x * b->y - a->y * b->x + 0x800) >> FX32_SHIFT;
    axb->x = x;
    axb->y = y;
    axb->z = z;
}

fx32 VEC_Mag(const VecFx32 *v) {
    fx64 sum = (fx64)v->x * v->x;
    sum += (fx64)v->y * v->y;
    sum += (fx64)v->z * v->z;
    reg_CP_SQRTCNT = 1;
    reg_CP_SQRT_PARAM = sum << 2;
    while (reg_CP_SQRTCNT & REG_CP_BUSY_MASK) {
    }
    return ((fx32)reg_CP_SQRT_RESULT + 1) >> 1;
}

fx32 VEC_Fx16Mag(const VecFx16 *v) {
    fx64 sum = v->x * v->x;
    sum += v->y * v->y;
    sum += v->z * v->z;
    reg_CP_SQRTCNT = 1;
    reg_CP_SQRT_PARAM = sum << 2;
    while (reg_CP_SQRTCNT & REG_CP_BUSY_MASK) {
    }
    return ((fx32)reg_CP_SQRT_RESULT + 1) >> 1;
}

void VEC_Normalize(const VecFx32 *src, VecFx32 *dest) {
    fx64 sum = (fx64)src->x * src->x;
    sum += (fx64)src->y * src->y;
    sum += (fx64)src->z * src->z;
    fx64 invLen;
    fx32 len;

    reg_CP_DIVCNT = 2;
    reg_CP_DIV_NUMER = 0x100000000000000LL;
    reg_CP_DIV_DENOM = sum;
    reg_CP_SQRTCNT = 1;
    reg_CP_SQRT_PARAM = sum << 2;
    while (reg_CP_SQRTCNT & REG_CP_BUSY_MASK) {
    }
    len = reg_CP_SQRT_RESULT;
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    invLen = reg_CP_DIV_RESULT * len;
    dest->x = (fx32)((invLen * src->x + 0x100000000000LL) >> 45);
    dest->y = (fx32)((invLen * src->y + 0x100000000000LL) >> 45);
    dest->z = (fx32)((invLen * src->z + 0x100000000000LL) >> 45);
}

void VEC_Fx16Normalize(const VecFx16 *src, VecFx16 *dest) {
    fx64 sum = src->x * src->x;
    sum += src->y * src->y;
    sum += src->z * src->z;
    fx64 invLen;
    fx32 len;

    reg_CP_DIVCNT = 2;
    reg_CP_DIV_NUMER = 0x100000000000000LL;
    reg_CP_DIV_DENOM = sum;
    reg_CP_SQRTCNT = 1;
    reg_CP_SQRT_PARAM = sum << 2;
    while (reg_CP_SQRTCNT & REG_CP_BUSY_MASK) {
    }
    len = reg_CP_SQRT_RESULT;
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    invLen = reg_CP_DIV_RESULT * len;
    dest->x = (fx16)((invLen * src->x + 0x100000000000LL) >> 45);
    dest->y = (fx16)((invLen * src->y + 0x100000000000LL) >> 45);
    dest->z = (fx16)((invLen * src->z + 0x100000000000LL) >> 45);
}

void VEC_MultAdd(fx32 scale, const VecFx32 *a, const VecFx32 *b, VecFx32 *dest) {
    dest->x = b->x + (fx32)(((fx64)scale * a->x) >> FX32_SHIFT);
    dest->y = b->y + (fx32)(((fx64)scale * a->y) >> FX32_SHIFT);
    dest->z = b->z + (fx32)(((fx64)scale * a->z) >> FX32_SHIFT);
}

fx32 VEC_Distance(const VecFx32 *a, const VecFx32 *b) {
    fx32 dx = a->x - b->x;
    fx32 dy = a->y - b->y;
    fx32 dz = a->z - b->z;
    fx64 sum = (fx64)dx * dx;
    sum += (fx64)dy * dy;
    sum += (fx64)dz * dz;
    reg_CP_SQRTCNT = 1;
    reg_CP_SQRT_PARAM = sum << 2;
    while (reg_CP_SQRTCNT & REG_CP_BUSY_MASK) {
    }
    return ((fx32)reg_CP_SQRT_RESULT + 1) >> 1;
}

#pragma thumb on
