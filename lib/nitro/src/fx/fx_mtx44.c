// NitroSDK's libfx fx_mtx44.c: the 4x4 fixed point matrix functions, after fx_mtx43.c in ARM9 main: the identity and
// conversion to 4x3, the product, and the frustum, perspective and orthographic projections, which divide on the
// hardware divider. The file split is by function group (one source file per matrix size) and fairly sure; the names
// are the SDK's.
// MTX_Identity44_ and MTX_Copy44To43_ store and load through stm/ldm with writeback, which MWCC does not generate, so
// they were hand-written assembly and are asm functions here.
#include "nitro/cp.h"
#include "nitro/fx.h"

#pragma thumb off
asm void MTX_Identity44_(MtxFx44 *mtx) {
    mov r2, #0x1000
    mov r3, #0
    stmia r0!, {r2, r3}
    mov r1, #0
    stmia r0!, {r1, r3}
    stmia r0!, {r1, r2, r3}
    stmia r0!, {r1, r3}
    stmia r0!, {r1, r2, r3}
    stmia r0!, {r1, r3}
    stmia r0!, {r1, r2}
    bx lr
}

asm void MTX_Copy44To43_(const MtxFx44 *src, MtxFx43 *dst) {
    ldmia r0!, {r2, r3, ip}
    add r0, r0, #4
    stmia r1!, {r2, r3, ip}
    ldmia r0!, {r2, r3, ip}
    add r0, r0, #4
    stmia r1!, {r2, r3, ip}
    ldmia r0!, {r2, r3, ip}
    add r0, r0, #4
    stmia r1!, {r2, r3, ip}
    ldmia r0!, {r2, r3, ip}
    add r0, r0, #4
    stmia r1!, {r2, r3, ip}
    bx lr
}

void MTX_Concat44(const MtxFx44 *a, const MtxFx44 *b, MtxFx44 *ab) {
    MtxFx44 tmp;
    MtxFx44 *dst = (ab == b) ? &tmp : ab;
    fx32 b0, b1, b2, b3;
    dst->m[0][0] = (fx32)(((fx64)a->m[0][1] * b->m[1][0] + (fx64)a->m[0][0] * b->m[0][0] +
                           (fx64)a->m[0][2] * b->m[2][0] + (fx64)a->m[0][3] * b->m[3][0]) >>
                          FX32_SHIFT);
    dst->m[0][1] = (fx32)(((fx64)a->m[0][1] * b->m[1][1] + (fx64)a->m[0][0] * b->m[0][1] +
                           (fx64)a->m[0][2] * b->m[2][1] + (fx64)a->m[0][3] * b->m[3][1]) >>
                          FX32_SHIFT);
    dst->m[0][3] = (fx32)(((fx64)a->m[0][1] * b->m[1][3] + (fx64)a->m[0][0] * b->m[0][3] +
                           (fx64)a->m[0][2] * b->m[2][3] + (fx64)a->m[0][3] * b->m[3][3]) >>
                          FX32_SHIFT);
    b0 = b->m[0][2];
    b1 = b->m[1][2];
    b2 = b->m[2][2];
    b3 = b->m[3][2];
    dst->m[0][2] =
        (fx32)(((fx64)a->m[0][1] * b1 + (fx64)a->m[0][0] * b0 + (fx64)a->m[0][2] * b2 + (fx64)a->m[0][3] * b3) >>
               FX32_SHIFT);
    dst->m[1][2] =
        (fx32)(((fx64)a->m[1][1] * b1 + (fx64)a->m[1][0] * b0 + (fx64)a->m[1][2] * b2 + (fx64)a->m[1][3] * b3) >>
               FX32_SHIFT);
    dst->m[1][1] = (fx32)(((fx64)a->m[1][1] * b->m[1][1] + (fx64)a->m[1][0] * b->m[0][1] +
                           (fx64)a->m[1][2] * b->m[2][1] + (fx64)a->m[1][3] * b->m[3][1]) >>
                          FX32_SHIFT);
    dst->m[1][3] = (fx32)(((fx64)a->m[1][1] * b->m[1][3] + (fx64)a->m[1][0] * b->m[0][3] +
                           (fx64)a->m[1][2] * b->m[2][3] + (fx64)a->m[1][3] * b->m[3][3]) >>
                          FX32_SHIFT);
    b0 = b->m[0][0];
    b1 = b->m[1][0];
    b2 = b->m[2][0];
    b3 = b->m[3][0];
    dst->m[1][0] =
        (fx32)(((fx64)a->m[1][1] * b1 + (fx64)a->m[1][0] * b0 + (fx64)a->m[1][2] * b2 + (fx64)a->m[1][3] * b3) >>
               FX32_SHIFT);
    dst->m[2][0] =
        (fx32)(((fx64)a->m[2][1] * b1 + (fx64)a->m[2][0] * b0 + (fx64)a->m[2][2] * b2 + (fx64)a->m[2][3] * b3) >>
               FX32_SHIFT);
    dst->m[2][1] = (fx32)(((fx64)a->m[2][1] * b->m[1][1] + (fx64)a->m[2][0] * b->m[0][1] +
                           (fx64)a->m[2][2] * b->m[2][1] + (fx64)a->m[2][3] * b->m[3][1]) >>
                          FX32_SHIFT);
    dst->m[2][3] = (fx32)(((fx64)a->m[2][1] * b->m[1][3] + (fx64)a->m[2][0] * b->m[0][3] +
                           (fx64)a->m[2][2] * b->m[2][3] + (fx64)a->m[2][3] * b->m[3][3]) >>
                          FX32_SHIFT);
    b0 = b->m[0][2];
    b1 = b->m[1][2];
    b2 = b->m[2][2];
    b3 = b->m[3][2];
    dst->m[2][2] =
        (fx32)(((fx64)a->m[2][1] * b1 + (fx64)a->m[2][0] * b0 + (fx64)a->m[2][2] * b2 + (fx64)a->m[2][3] * b3) >>
               FX32_SHIFT);
    dst->m[3][2] =
        (fx32)(((fx64)a->m[3][1] * b1 + (fx64)a->m[3][0] * b0 + (fx64)a->m[3][2] * b2 + (fx64)a->m[3][3] * b3) >>
               FX32_SHIFT);
    dst->m[3][1] = (fx32)(((fx64)a->m[3][1] * b->m[1][1] + (fx64)a->m[3][0] * b->m[0][1] +
                           (fx64)a->m[3][2] * b->m[2][1] + (fx64)a->m[3][3] * b->m[3][1]) >>
                          FX32_SHIFT);
    dst->m[3][3] = (fx32)(((fx64)a->m[3][1] * b->m[1][3] + (fx64)a->m[3][0] * b->m[0][3] +
                           (fx64)a->m[3][2] * b->m[2][3] + (fx64)a->m[3][3] * b->m[3][3]) >>
                          FX32_SHIFT);
    dst->m[3][0] = (fx32)(((fx64)a->m[3][1] * b->m[1][0] + (fx64)a->m[3][0] * b->m[0][0] +
                           (fx64)a->m[3][2] * b->m[2][0] + (fx64)a->m[3][3] * b->m[3][0]) >>
                          FX32_SHIFT);
    if (dst == &tmp) {
        *ab = tmp;
    }
}

void MTX_FrustumW(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW, MtxFx44 *mtx) {
    fx64 invWidth;
    fx64 invHeight;
    fx64 invDepth;
    fx32 n2;

    FX_InvAsync(r - l);
    n2 = n * 2;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[0][3] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][2] = 0;
    mtx->m[1][3] = 0;
    mtx->m[2][3] = -scaleW;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
    mtx->m[3][3] = 0;
    invWidth = FX_GetDivResultFx64c();
    reg_CP_DIV_NUMER = 0x100000000000LL;
    reg_CP_DIV_DENOM = (u32)(t - b);
    if (scaleW != FX32_ONE) {
        invWidth = invWidth * scaleW / FX32_ONE;
    }
    mtx->m[0][0] = (fx32)((invWidth * n2 + 0x80000000LL) >> 32);
    invHeight = FX_GetDivResultFx64c();
    reg_CP_DIV_NUMER = 0x100000000000LL;
    reg_CP_DIV_DENOM = (u32)(n - f);
    if (scaleW != FX32_ONE) {
        invHeight = invHeight * scaleW / FX32_ONE;
    }
    mtx->m[1][1] = (fx32)((invHeight * n2 + 0x80000000LL) >> 32);
    mtx->m[2][0] = (fx32)((invWidth * (r + l) + 0x80000000LL) >> 32);
    mtx->m[2][1] = (fx32)((invHeight * (t + b) + 0x80000000LL) >> 32);
    invDepth = FX_GetDivResultFx64c();
    if (scaleW != FX32_ONE) {
        invDepth = invDepth * scaleW / FX32_ONE;
    }
    mtx->m[2][2] = (fx32)((invDepth * (f + n) + 0x80000000LL) >> 32);
    mtx->m[3][2] = (fx32)((invDepth * FX_Mul(n2, f) + 0x80000000LL) >> 32);
}

void MTX_PerspectiveW(fx32 fovySin, fx32 fovyCos, fx32 aspect, fx32 n, fx32 f, fx32 scaleW, MtxFx44 *mtx) {
    fx32 cot;
    fx64 invDepth;

    cot = FX_Div(fovyCos, fovySin);
    reg_CP_DIV_NUMER = 0x100000000000LL;
    reg_CP_DIV_DENOM = (u32)(n - f);
    if (scaleW != FX32_ONE) {
        cot = cot * scaleW / FX32_ONE;
    }
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[0][3] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = cot;
    mtx->m[1][2] = 0;
    mtx->m[1][3] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][3] = -scaleW;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
    mtx->m[3][3] = 0;
    invDepth = FX_GetDivResultFx64c();
    reg_CP_DIV_NUMER = (u64)(u32)cot << 32;
    reg_CP_DIV_DENOM = (u32)aspect;
    if (scaleW != FX32_ONE) {
        invDepth = invDepth * scaleW / FX32_ONE;
    }
    mtx->m[2][2] = (fx32)((invDepth * (f + n) + 0x80000000LL) >> 32);
    mtx->m[3][2] = (fx32)((invDepth * FX_Mul(n * 2, f) + 0x80000000LL) >> 32);
    mtx->m[0][0] = FX_GetDivResult();
}

void MTX_OrthoW(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW, MtxFx44 *mtx) {
    fx64 invWidth;
    fx64 invHeight;
    fx64 invDepth;

    FX_InvAsync(r - l);
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[0][3] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][2] = 0;
    mtx->m[1][3] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][3] = 0;
    mtx->m[3][3] = scaleW;
    invWidth = FX_GetDivResultFx64c();
    reg_CP_DIV_NUMER = 0x100000000000LL;
    reg_CP_DIV_DENOM = (u32)(t - b);
    if (scaleW != FX32_ONE) {
        invWidth = invWidth * scaleW / FX32_ONE;
    }
    mtx->m[0][0] = (fx32)((invWidth * (2 * FX32_ONE) + 0x80000000LL) >> 32);
    invHeight = FX_GetDivResultFx64c();
    reg_CP_DIV_NUMER = 0x100000000000LL;
    reg_CP_DIV_DENOM = (u32)(n - f);
    if (scaleW != FX32_ONE) {
        invHeight = invHeight * scaleW / FX32_ONE;
    }
    mtx->m[1][1] = (fx32)((invHeight * (2 * FX32_ONE) + 0x80000000LL) >> 32);
    invDepth = FX_GetDivResultFx64c();
    if (scaleW != FX32_ONE) {
        invDepth = invDepth * scaleW / FX32_ONE;
    }
    mtx->m[2][2] = (fx32)((invDepth * (2 * FX32_ONE) + 0x80000000LL) >> 32);
    mtx->m[3][0] = (fx32)((invWidth * -(r + l) + 0x80000000LL) >> 32);
    mtx->m[3][1] = (fx32)((invHeight * -(t + b) + 0x80000000LL) >> 32);
    mtx->m[3][2] = (fx32)((invDepth * (f + n) + 0x80000000LL) >> 32);
}
#pragma thumb on
