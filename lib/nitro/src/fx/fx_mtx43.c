// NitroSDK's libfx fx_mtx43.c: the 4x3 fixed point matrix functions (a 3x3 matrix and a translation row), after
// fx_mtx33.c in ARM9 main. The file split is by function group (one source file per matrix size) and fairly sure; the
// names are the SDK's.
// The functions that store through stm/ldm with writeback (MWCC does not generate those), and MTX_RotY43_, whose stores
// MWCC schedules differently, were hand-written assembly and are asm functions here.
#include "nitro/fx.h"
#include "nitro/mi.h"

#pragma thumb off
asm void MTX_Identity43_(MtxFx43 *mtx) {
    mov r2, #0x1000
    mov r3, #0
    stmia r0!, {r2, r3}
    mov r1, #0
    stmia r0!, {r1, r3}
    stmia r0!, {r2, r3}
    stmia r0!, {r1, r3}
    stmia r0!, {r2, r3}
    stmia r0!, {r1, r3}
    bx lr
}

asm void MTX_Copy43To44_(const MtxFx43 *src, MtxFx44 *dst) {
    stmdb sp!, {r4}
    mov ip, #0
    ldmia r0!, {r2, r3, r4}
    stmia r1!, {r2, r3, r4, ip}
    ldmia r0!, {r2, r3, r4}
    stmia r1!, {r2, r3, r4, ip}
    ldmia r0!, {r2, r3, r4}
    stmia r1!, {r2, r3, r4, ip}
    mov ip, #0x1000
    ldmia r0!, {r2, r3, r4}
    stmia r1!, {r2, r3, r4, ip}
    ldmia sp!, {r4}
    bx lr
}
#pragma thumb on

asm void MTX_Scale43_(MtxFx43 *mtx, fx32 x, fx32 y, fx32 z) {
    stmia r0!, {r1}
    mov r1, #0
    str r3, [r0, #0x1c]
    mov r3, #0
    stmia r0!, {r1, r3}
    stmia r0!, {r1, r2, r3}
    mov r2, #0
    stmia r0!, {r1, r3}
    add r0, #4
    stmia r0!, {r1, r2, r3}
    bx lr
}

#pragma thumb off
void MTX_ScaleApply43(const MtxFx43 *src, MtxFx43 *dest, fx32 x, fx32 y, fx32 z) {
    MTX_ScaleApply33(&src->rt.rot, &dest->rt.rot, x, y, z);
    dest->m[3][0] = src->m[3][0];
    dest->m[3][1] = src->m[3][1];
    dest->m[3][2] = src->m[3][2];
}
#pragma thumb on

asm void MTX_RotY43_(MtxFx43 *mtx, fx32 sin, fx32 cos) {
    str r1, [r0, #0x18]
    mov r3, #0
    stmia r0!, {r2, r3}
    neg r1, r1
    stmia r0!, {r1, r3}
    mov r1, #1
    lsl r1, r1, #0xc
    stmia r0!, {r1, r3}
    add r0, #4
    mov r1, #0
    stmia r0!, {r1, r2, r3}
    stmia r0!, {r1, r3}
    bx lr
}

#pragma thumb off
void MTX_RotAxis43(MtxFx43 *mtx, const VecFx32 *axis, fx16 sin, fx16 cos) {
    MAT3_RotationAxisAngle(&mtx->rt.rot, axis, sin, cos);
    mtx->rt.trans.x = 0;
    mtx->rt.trans.y = 0;
    mtx->rt.trans.z = 0;
}

int MTX_Inverse43(const MtxFx43 *mtx, MtxFx43 *inv) {
    MtxFx43 tmp;
    MtxFx43 *dst;
    fx32 t0, t1, t2, u0, u1, u2, u3, u4, u5;
    fx32 det;
    fx32 invDet;

    dst = (mtx == inv) ? &tmp : inv;

    t0 = (fx32)(((fx64)mtx->m[1][0] * mtx->m[2][2] - (fx64)mtx->m[1][2] * mtx->m[2][0] + 0x800) >> FX32_SHIFT);
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
    dst->m[3][0] = -(fx32)(((fx64)dst->m[0][0] * mtx->m[3][0] + (fx64)dst->m[1][0] * mtx->m[3][1] +
                            (fx64)dst->m[2][0] * mtx->m[3][2]) >>
                           FX32_SHIFT);
    dst->m[3][1] = -(fx32)(((fx64)dst->m[0][1] * mtx->m[3][0] + (fx64)dst->m[1][1] * mtx->m[3][1] +
                            (fx64)dst->m[2][1] * mtx->m[3][2]) >>
                           FX32_SHIFT);
    dst->m[3][2] = -(fx32)(((fx64)dst->m[0][2] * mtx->m[3][0] + (fx64)dst->m[1][2] * mtx->m[3][1] +
                            (fx64)dst->m[2][2] * mtx->m[3][2]) >>
                           FX32_SHIFT);
    if (dst == &tmp) {
        MI_Copy48B(&tmp, inv);
    }
    return 0;
}

void MTX_Concat43(const MtxFx43 *a, const MtxFx43 *b, MtxFx43 *ab) {
    MtxFx43 tmp;
    MtxFx43 *dst = ab;
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
    b0 = b->m[0][2];
    b1 = b->m[1][2];
    b2 = b->m[2][2];
    ab->m[2][2] = (fx32)(((fx64)a->m[2][0] * b0 + (fx64)a->m[2][1] * b1 + (fx64)a->m[2][2] * b2) >> FX32_SHIFT);
    ab->m[3][2] =
        b->m[3][2] + (fx32)(((fx64)a->m[3][0] * b0 + (fx64)a->m[3][1] * b1 + (fx64)a->m[3][2] * b2) >> FX32_SHIFT);
    ab->m[3][1] =
        b->m[3][1] +
        (fx32)(((fx64)a->m[3][0] * b->m[0][1] + (fx64)a->m[3][1] * b->m[1][1] + (fx64)a->m[3][2] * b->m[2][1]) >>
               FX32_SHIFT);
    ab->m[3][0] =
        b->m[3][0] +
        (fx32)(((fx64)a->m[3][0] * b->m[0][0] + (fx64)a->m[3][1] * b->m[1][0] + (fx64)a->m[3][2] * b->m[2][0]) >>
               FX32_SHIFT);
    if (ab == &tmp) {
        *dst = tmp;
    }
}

void MTX_MultVec43(const VecFx32 *vec, const MtxFx43 *mtx, VecFx32 *dest) {
    dest->x = (fx32)(((fx64)vec->x * mtx->m[0][0] + (fx64)vec->y * mtx->m[1][0] + (fx64)vec->z * mtx->m[2][0]) >>
                     FX32_SHIFT) +
              mtx->m[3][0];
    dest->y = (fx32)(((fx64)vec->x * mtx->m[0][1] + (fx64)vec->y * mtx->m[1][1] + (fx64)vec->z * mtx->m[2][1]) >>
                     FX32_SHIFT) +
              mtx->m[3][1];
    dest->z = (fx32)(((fx64)vec->x * mtx->m[0][2] + (fx64)vec->y * mtx->m[1][2] + (fx64)vec->z * mtx->m[2][2]) >>
                     FX32_SHIFT) +
              mtx->m[3][2];
}

void MTX_LookAt(const VecFx32 *camPos, const VecFx32 *camUp, const VecFx32 *target, MtxFx43 *mtx) {
    VecFx32 forward;
    VecFx32 side;
    VecFx32 up;

    forward.x = camPos->x - target->x;
    forward.y = camPos->y - target->y;
    forward.z = camPos->z - target->z;
    VEC_Normalize(&forward, &forward);
    VEC_CrossProduct(camUp, &forward, &side);
    VEC_Normalize(&side, &side);
    VEC_CrossProduct(&forward, &side, &up);
    mtx->m[0][0] = side.x;
    mtx->m[0][1] = up.x;
    mtx->m[0][2] = forward.x;
    mtx->m[1][0] = side.y;
    mtx->m[1][1] = up.y;
    mtx->m[1][2] = forward.y;
    mtx->m[2][0] = side.z;
    mtx->m[2][1] = up.z;
    mtx->m[2][2] = forward.z;
    mtx->m[3][0] = -VEC_DotProduct(camPos, &side);
    mtx->m[3][1] = -VEC_DotProduct(camPos, &up);
    mtx->m[3][2] = -VEC_DotProduct(camPos, &forward);
}
#pragma thumb on
