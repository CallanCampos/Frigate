// Host versions of the functions of NitroSDK's fx_mtx33.c that the SDK wrote in assembly, and of MTX_Concat33, whose C
// reads an argument after storing over it where the original doesn't (lib/nitro/src/fx/fx_mtx33.c). The originals are
// there, behind #ifndef PLATFORM_PC; the rest of that file compiles on the host as it is. Each stores the words the
// original stores, and MAT3_To4x4 and MTX_Concat33 read memory in the original's order around their stores, so that
// arguments that overlap give what the original gives.
#include "nitro/fx.h"

// A product's word as the SDK's SMULL and SMLAL chains and LSR and ORR pair make it: the products' 64-bit sum, which
// wraps, and its bits 12 to 43
static fx32 Shifted(u64 sum) {
    return (fx32)(u32)(sum >> FX32_SHIFT);
}

static u64 Product(fx32 a, fx32 b) {
    return (u64)((s64)a * b);
}

void MTX_Identity33_(MtxFx33 *mtx) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = FX32_ONE;
}

// Row by row, each row read before it is written, as the SDK's LDM and STM pairs do: when the two matrices overlap,
// a row stored over source words that are still to be read changes what is read, the same way. The rows are walked
// through fx32 pointers, which may alias each other whatever the compiler's aliasing rules
void MAT3_To4x4(const MtxFx33 *src, MtxFx44 *dst) {
    int row;

    for (row = 0; row < 3; row++) {
        const fx32 *from = src->m[row];
        fx32 *to = dst->m[row];
        fx32 x = from[0];
        fx32 y = from[1];
        fx32 z = from[2];

        to[0] = x;
        to[1] = y;
        to[2] = z;
        to[3] = 0;
    }
    dst->m[3][0] = 0;
    dst->m[3][1] = 0;
    dst->m[3][2] = 0;
    dst->m[3][3] = FX32_ONE;
}

void MAT3_Scaling(MtxFx33 *mtx, fx32 x, fx32 y, fx32 z) {
    mtx->m[0][0] = x;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = y;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = z;
}

// The rotations negate the sine as a u32, as NEG does: FX32_MIN stays FX32_MIN, where a signed negation would overflow

void MTX_RotX33_(MtxFx33 *mtx, fx32 sin, fx32 cos) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = cos;
    mtx->m[1][2] = sin;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = (fx32)(0 - (u32)sin);
    mtx->m[2][2] = cos;
}

void MTX_RotY33_(MtxFx33 *mtx, fx32 sin, fx32 cos) {
    mtx->m[0][0] = cos;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = (fx32)(0 - (u32)sin);
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = sin;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = cos;
}

void MTX_RotZ33_(MtxFx33 *mtx, fx32 sin, fx32 cos) {
    mtx->m[0][0] = cos;
    mtx->m[0][1] = sin;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = (fx32)(0 - (u32)sin);
    mtx->m[1][1] = cos;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = FX32_ONE;
}

// Rows of a and columns of b, loaded where the original loads them: a's first row before any store, each other row of a
// before the first store into it, b where each sum needs it. So a product may replace a (ab == a), one that replaces b
// is made in a temporary and copied over it, and arguments that overlap otherwise give what the original gives
void MTX_Concat33(const MtxFx33 *a, const MtxFx33 *b, MtxFx33 *ab) {
    MtxFx33 tmp;
    MtxFx33 *p = ab == b ? &tmp : ab;
    fx32 a00, a01, a02, a10, a11, a12, a20, a21, a22;
    fx32 b0, b1, b2;

    a01 = a->m[0][1];
    b1 = b->m[1][0];
    a00 = a->m[0][0];
    b0 = b->m[0][0];
    a02 = a->m[0][2];
    b2 = b->m[2][0];
    p->m[0][0] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    p->m[0][1] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2));
    b1 = b->m[1][2];
    a11 = a->m[1][1];
    b0 = b->m[0][2];
    a10 = a->m[1][0];
    b2 = b->m[2][2];
    a12 = a->m[1][2];
    p->m[0][2] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2));
    p->m[1][2] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    a21 = a->m[2][1];
    p->m[1][1] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2));
    b1 = b->m[1][0];
    b0 = b->m[0][0];
    a20 = a->m[2][0];
    b2 = b->m[2][0];
    a22 = a->m[2][2];
    p->m[1][0] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2));
    p->m[2][0] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    p->m[2][1] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2));
    b2 = b->m[2][2];
    b0 = b->m[0][2];
    b1 = b->m[1][2];
    p->m[2][2] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2));
    if (p == &tmp) {
        *ab = tmp;
    }
}
