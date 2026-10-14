// NitroSDK's libfx fx_mtx33.c: the 3x3 fixed point matrix functions, after fx_mtx22.c in ARM9 main. The file split is
// by function group (one source file per matrix size) and fairly sure; the names are the SDK's.
// The functions that store through stm/ldm with writeback (MWCC does not generate those), and the three rotations,
// whose stores MWCC schedules differently, were hand-written assembly and are asm functions here.
#include "nitro/fx.h"
#include "nitro/mi.h"

#pragma thumb off
asm void MTX_Identity33_(MtxFx33 *mtx) {
    mov r2, #0x1000
    str r2, [r0, #0x20]
    mov r3, #0
    stmia r0!, {r2, r3}
    mov r1, #0
    stmia r0!, {r1, r3}
    stmia r0!, {r2, r3}
    stmia r0!, {r1, r3}
    bx lr
}

asm void MAT3_To4x4(const MtxFx33 *src, MtxFx44 *dst) {
    stmdb sp!, {r4}
    mov ip, #0
    ldmia r0!, {r2, r3, r4}
    stmia r1!, {r2, r3, r4, ip}
    ldmia r0!, {r2, r3, r4}
    stmia r1!, {r2, r3, r4, ip}
    ldmia r0!, {r2, r3, r4}
    stmia r1!, {r2, r3, r4, ip}
    mov r2, #0x1000
    str ip, [r1, #0]
    str ip, [r1, #4]
    str ip, [r1, #8]
    str r2, [r1, #0xc]
    ldmia sp!, {r4}
    bx lr
}
#pragma thumb on

asm void MAT3_Scaling(MtxFx33 *mtx, fx32 x, fx32 y, fx32 z) {
    stmia r0!, {r1}
    mov r1, #0
    str r3, [r0, #0x1c]
    mov r3, #0
    stmia r0!, {r1, r3}
    stmia r0!, {r1, r2, r3}
    mov r2, #0
    stmia r0!, {r1, r3}
    bx lr
}

#pragma thumb off
void MTX_ScaleApply33(const MtxFx33 *src, MtxFx33 *dest, fx32 x, fx32 y, fx32 z) {
    dest->m[0][0] = (fx32)(((fx64)x * src->m[0][0]) >> FX32_SHIFT);
    dest->m[0][1] = (fx32)(((fx64)x * src->m[0][1]) >> FX32_SHIFT);
    dest->m[0][2] = (fx32)(((fx64)x * src->m[0][2]) >> FX32_SHIFT);
    dest->m[1][0] = (fx32)(((fx64)y * src->m[1][0]) >> FX32_SHIFT);
    dest->m[1][1] = (fx32)(((fx64)y * src->m[1][1]) >> FX32_SHIFT);
    dest->m[1][2] = (fx32)(((fx64)y * src->m[1][2]) >> FX32_SHIFT);
    dest->m[2][0] = (fx32)(((fx64)z * src->m[2][0]) >> FX32_SHIFT);
    dest->m[2][1] = (fx32)(((fx64)z * src->m[2][1]) >> FX32_SHIFT);
    dest->m[2][2] = (fx32)(((fx64)z * src->m[2][2]) >> FX32_SHIFT);
}
#pragma thumb on

asm void MTX_RotX33_(MtxFx33 *mtx, fx32 sin, fx32 cos) {
    mov r3, #1
    lsl r3, r3, #0xc
    str r3, [r0, #0]
    mov r3, #0
    str r3, [r0, #4]
    str r3, [r0, #8]
    str r3, [r0, #0xc]
    str r2, [r0, #0x10]
    str r1, [r0, #0x14]
    str r3, [r0, #0x18]
    neg r1, r1
    str r1, [r0, #0x1c]
    str r2, [r0, #0x20]
    bx lr
}

asm void MTX_RotY33_(MtxFx33 *mtx, fx32 sin, fx32 cos) {
    str r2, [r0, #0]
    str r2, [r0, #0x20]
    mov r3, #0
    str r3, [r0, #4]
    str r3, [r0, #0xc]
    str r3, [r0, #0x14]
    str r3, [r0, #0x1c]
    neg r2, r1
    mov r3, #1
    lsl r3, r3, #0xc
    str r1, [r0, #0x18]
    str r2, [r0, #8]
    str r3, [r0, #0x10]
    bx lr
}

asm void MTX_RotZ33_(MtxFx33 *mtx, fx32 sin, fx32 cos) {
    stmia r0!, {r2}
    mov r3, #0
    stmia r0!, {r1, r3}
    neg r1, r1
    stmia r0!, {r1, r2}
    mov r1, #1
    lsl r1, r1, #0xc
    str r3, [r0, #0]
    str r3, [r0, #4]
    str r3, [r0, #8]
    str r1, [r0, #0xc]
    bx lr
}

#pragma thumb off
void MAT3_RotationAxisAngle(MtxFx33 *mtx, const VecFx32 *axis, fx32 sin, fx32 cos) {
    fx32 omc = FX32_ONE - cos;
    fx32 x = axis->x;
    fx32 y = axis->y;
    fx32 z = axis->z;
    mtx->m[0][0] = cos + (fx32)(((fx64)omc * ((fx64)x * x)) >> 24);
    mtx->m[1][1] = cos + (fx32)(((fx64)omc * ((fx64)y * y)) >> 24);
    mtx->m[2][2] = cos + (fx32)(((fx64)omc * ((fx64)z * z)) >> 24);
    mtx->m[1][0] = (fx32)(((fx64)omc * x * y) >> 24) - (fx32)(((fx64)sin * z) >> FX32_SHIFT);
    mtx->m[0][2] = (fx32)(((fx64)omc * x * z) >> 24) - (fx32)(((fx64)sin * y) >> FX32_SHIFT);
    mtx->m[2][0] = (fx32)(((fx64)sin * y) >> FX32_SHIFT) + (fx32)(((fx64)omc * x * z) >> 24);
    mtx->m[0][1] = (fx32)(((fx64)sin * z) >> FX32_SHIFT) + (fx32)(((fx64)omc * x * y) >> 24);
    mtx->m[1][2] = (fx32)(((fx64)sin * x) >> FX32_SHIFT) + (fx32)(((fx64)omc * y * z) >> 24);
    mtx->m[2][1] = (fx32)(((fx64)omc * y * z) >> 24) - (fx32)(((fx64)sin * x) >> FX32_SHIFT);
}

int MAT3_Invert(const MtxFx33 *mtx, MtxFx33 *inv) {
    MtxFx33 tmp;
    MtxFx33 *dst;
    fx32 t0, t1, t2, u0, u1, u2, u3, u4, u5;
    fx32 det;
    fx32 invDet;

    dst = (mtx == inv) ? &tmp : inv;

    t0 = (fx32)(((fx64)mtx->m[1][0] * mtx->m[2][2] - (fx64)mtx->m[2][0] * mtx->m[1][2] + 0x800) >> FX32_SHIFT);
    t1 = (fx32)(((fx64)mtx->m[1][1] * mtx->m[2][2] - (fx64)mtx->m[1][2] * mtx->m[2][1] + 0x800) >> FX32_SHIFT);
    t2 = (fx32)(((fx64)mtx->m[1][0] * mtx->m[2][1] - (fx64)mtx->m[1][1] * mtx->m[2][0] + 0x800) >> FX32_SHIFT);
    det = (fx32)(((fx64)mtx->m[0][0] * t1 - (fx64)mtx->m[0][1] * t0 + (fx64)mtx->m[0][2] * t2 + 0x800) >> FX32_SHIFT);
    if (det == 0) {
        return -1;
    }
    FX_InvAsync(det);
    u0 = (fx32)(((fx64)mtx->m[0][1] * mtx->m[2][2] - (fx64)mtx->m[2][1] * mtx->m[0][2]) >> FX32_SHIFT);
    u1 = (fx32)(((fx64)mtx->m[0][1] * mtx->m[1][2] - (fx64)mtx->m[1][1] * mtx->m[0][2]) >> FX32_SHIFT);
    u2 = (fx32)(((fx64)mtx->m[0][0] * mtx->m[2][2] - (fx64)mtx->m[2][0] * mtx->m[0][2]) >> FX32_SHIFT);
    u3 = (fx32)(((fx64)mtx->m[0][0] * mtx->m[1][2] - (fx64)mtx->m[1][0] * mtx->m[0][2]) >> FX32_SHIFT);
    invDet = FX_GetDivResult();
    dst->m[0][0] = (fx32)(((fx64)invDet * t1) >> FX32_SHIFT);
    dst->m[0][1] = -(fx32)(((fx64)invDet * u0) >> FX32_SHIFT);
    dst->m[0][2] = (fx32)(((fx64)invDet * u1) >> FX32_SHIFT);
    dst->m[1][0] = -(fx32)(((fx64)invDet * t0) >> FX32_SHIFT);
    dst->m[1][1] = (fx32)(((fx64)invDet * u2) >> FX32_SHIFT);
    dst->m[1][2] = -(fx32)(((fx64)invDet * u3) >> FX32_SHIFT);
    dst->m[2][0] = (fx32)(((fx64)invDet * t2) >> FX32_SHIFT);
    u4 = (fx32)(((fx64)mtx->m[0][0] * mtx->m[2][1] - (fx64)mtx->m[2][0] * mtx->m[0][1]) >> FX32_SHIFT);
    dst->m[2][1] = -(fx32)(((fx64)invDet * u4) >> FX32_SHIFT);
    u5 = (fx32)(((fx64)mtx->m[0][0] * mtx->m[1][1] - (fx64)mtx->m[1][0] * mtx->m[0][1]) >> FX32_SHIFT);
    dst->m[2][2] = (fx32)(((fx64)invDet * u5) >> FX32_SHIFT);
    if (dst == &tmp) {
        MI_Copy36B(&tmp, inv);
    }
    return 0;
}

void MTX_Concat33(const MtxFx33 *a, const MtxFx33 *b, MtxFx33 *ab) {
    MtxFx33 tmp;
    MtxFx33 *dst = ab;
    fx32 b0, b1, b2;

    if (ab == b) {
        ab = &tmp;
    }
    ab->m[0][0] =
        (fx32)(((fx64)a->m[0][0] * b->m[0][0] + (fx64)a->m[0][1] * b->m[1][0] + (fx64)a->m[0][2] * b->m[2][0]) >>
               FX32_SHIFT);
    ab->m[0][1] =
        (fx32)(((fx64)a->m[0][0] * b->m[0][1] + (fx64)a->m[0][1] * b->m[1][1] + (fx64)a->m[0][2] * b->m[2][1]) >>
               FX32_SHIFT);
    b0 = b->m[0][2];
    b1 = b->m[1][2];
    b2 = b->m[2][2];
    ab->m[0][2] = (fx32)(((fx64)a->m[0][0] * b0 + (fx64)a->m[0][1] * b1 + (fx64)a->m[0][2] * b2) >> FX32_SHIFT);
    ab->m[1][2] = (fx32)(((fx64)a->m[1][0] * b0 + (fx64)a->m[1][1] * b1 + (fx64)a->m[1][2] * b2) >> FX32_SHIFT);
    ab->m[1][1] =
        (fx32)(((fx64)a->m[1][0] * b->m[0][1] + (fx64)a->m[1][1] * b->m[1][1] + (fx64)a->m[1][2] * b->m[2][1]) >>
               FX32_SHIFT);
    b0 = b->m[0][0];
    b1 = b->m[1][0];
    b2 = b->m[2][0];
    ab->m[1][0] = (fx32)(((fx64)a->m[1][0] * b0 + (fx64)a->m[1][1] * b1 + (fx64)a->m[1][2] * b2) >> FX32_SHIFT);
    ab->m[2][0] = (fx32)(((fx64)a->m[2][0] * b0 + (fx64)a->m[2][1] * b1 + (fx64)a->m[2][2] * b2) >> FX32_SHIFT);
    ab->m[2][1] =
        (fx32)(((fx64)a->m[2][0] * b->m[0][1] + (fx64)a->m[2][1] * b->m[1][1] + (fx64)a->m[2][2] * b->m[2][1]) >>
               FX32_SHIFT);
    ab->m[2][2] =
        (fx32)(((fx64)a->m[2][0] * b->m[0][2] + (fx64)a->m[2][1] * b->m[1][2] + (fx64)a->m[2][2] * b->m[2][2]) >>
               FX32_SHIFT);
    if (ab == &tmp) {
        *dst = tmp;
    }
}

void MTX_MultVec33(const VecFx32 *vec, const MtxFx33 *mtx, VecFx32 *dest) {
    fx32 x = vec->x;
    fx32 y = vec->y;
    fx32 z = vec->z;

    dest->x = (fx32)(((fx64)x * mtx->m[0][0] + (fx64)y * mtx->m[1][0] + (fx64)z * mtx->m[2][0]) >> FX32_SHIFT);
    dest->y = (fx32)(((fx64)x * mtx->m[0][1] + (fx64)y * mtx->m[1][1] + (fx64)z * mtx->m[2][1]) >> FX32_SHIFT);
    dest->z = (fx32)(((fx64)x * mtx->m[0][2] + (fx64)y * mtx->m[1][2] + (fx64)z * mtx->m[2][2]) >> FX32_SHIFT);
}
#pragma thumb on
