// Host versions of the functions of NitroSDK's fx_mtx43.c that the SDK wrote in assembly, and of MTX_Concat43 and
// MTX_MultVec43, whose C reads an argument after storing over it where the original doesn't
// (lib/nitro/src/fx/fx_mtx43.c). The originals are there, behind #ifndef PLATFORM_PC; the rest of that file compiles
// on the host as it is. Each stores the words the original stores, and MTX_Copy43To44_, MTX_Concat43 and
// MTX_MultVec43 read memory in the original's order around their stores, so that arguments that overlap give what the
// original gives.
#include "nitro/fx.h"

// A product's word as the SDK's SMULL and SMLAL chains and LSR and ORR pair make it: the products' 64-bit sum, which
// wraps, and its bits 12 to 43
static fx32 Shifted(u64 sum) {
    return (fx32)(u32)(sum >> FX32_SHIFT);
}

static u64 Product(fx32 a, fx32 b) {
    return (u64)((s64)a * b);
}

void MTX_Identity43_(MtxFx43 *mtx) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = FX32_ONE;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
    mtx->m[3][2] = 0;
}

// Row by row, each row read before it is written, as the SDK's LDM and STM pairs do: when the two matrices overlap,
// a row stored over source words that are still to be read changes what is read, the same way. The rows are walked
// through fx32 pointers, which may alias each other whatever the compiler's aliasing rules
void MTX_Copy43To44_(const MtxFx43 *src, MtxFx44 *dst) {
    int row;

    for (row = 0; row < 4; row++) {
        const fx32 *from = src->m[row];
        fx32 *to = dst->m[row];
        fx32 x = from[0];
        fx32 y = from[1];
        fx32 z = from[2];

        to[0] = x;
        to[1] = y;
        to[2] = z;
        to[3] = row == 3 ? FX32_ONE : 0;
    }
}

void MTX_Scale43_(MtxFx43 *mtx, fx32 x, fx32 y, fx32 z) {
    mtx->m[0][0] = x;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = y;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = z;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
    mtx->m[3][2] = 0;
}

void MTX_RotY43_(MtxFx43 *mtx, fx32 sin, fx32 cos) {
    mtx->m[0][0] = cos;
    mtx->m[0][1] = 0;
    // Negated as a u32, as NEG does: FX32_MIN stays FX32_MIN, where a signed negation would overflow
    mtx->m[0][2] = (fx32)(0 - (u32)sin);
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = sin;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = cos;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
    mtx->m[3][2] = 0;
}

// As MTX_Concat33 (port/hal/fx/fx_mtx33.c), with the translation row, whose sums add b's translation as 32-bit words
void MTX_Concat43(const MtxFx43 *a, const MtxFx43 *b, MtxFx43 *ab) {
    MtxFx43 tmp;
    MtxFx43 *p = ab == b ? &tmp : ab;
    fx32 a00, a01, a02, a10, a11, a12, a20, a21, a22, a30, a31, a32;
    fx32 b0, b1, b2, b3;

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
    a11 = a->m[1][1];
    p->m[0][1] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2));
    b1 = b->m[1][2];
    b0 = b->m[0][2];
    a10 = a->m[1][0];
    b2 = b->m[2][2];
    a12 = a->m[1][2];
    p->m[0][2] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2));
    p->m[1][2] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    a20 = a->m[2][0];
    p->m[1][1] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2));
    b1 = b->m[1][0];
    b0 = b->m[0][0];
    b2 = b->m[2][0];
    a21 = a->m[2][1];
    p->m[1][0] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2));
    a22 = a->m[2][2];
    a31 = a->m[3][1];
    p->m[2][0] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2));
    b1 = b->m[1][1];
    a30 = a->m[3][0];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    p->m[2][1] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2));
    b1 = b->m[1][2];
    b0 = b->m[0][2];
    b2 = b->m[2][2];
    a32 = a->m[3][2];
    p->m[2][2] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2));
    b3 = b->m[3][2];
    p->m[3][2] = (fx32)((u32)b3 + (u32)Shifted(Product(a30, b0) + Product(a31, b1) + Product(a32, b2)));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    b3 = b->m[3][1];
    p->m[3][1] = (fx32)((u32)b3 + (u32)Shifted(Product(a30, b0) + Product(a31, b1) + Product(a32, b2)));
    b3 = b->m[3][0];
    b2 = b->m[2][0];
    b0 = b->m[0][0];
    b1 = b->m[1][0];
    p->m[3][0] = (fx32)((u32)b3 + (u32)Shifted(Product(a30, b0) + Product(a31, b1) + Product(a32, b2)));
    if (p == &tmp) {
        *ab = tmp;
    }
}

// The vector is read before the matrix and the stores, so the result may replace it (dest == vec)
void MTX_MultVec43(const VecFx32 *vec, const MtxFx43 *mtx, VecFx32 *dest) {
    fx32 y = vec->y;
    fx32 m10 = mtx->m[1][0];
    fx32 m11 = mtx->m[1][1];
    fx32 m12 = mtx->m[1][2];
    fx32 x = vec->x;
    fx32 m00 = mtx->m[0][0];
    fx32 m01 = mtx->m[0][1];
    fx32 m02 = mtx->m[0][2];
    fx32 z = vec->z;
    fx32 m20 = mtx->m[2][0];
    fx32 m21 = mtx->m[2][1];
    fx32 m22 = mtx->m[2][2];
    fx32 m30 = mtx->m[3][0];
    fx32 m31 = mtx->m[3][1];
    fx32 m32 = mtx->m[3][2];

    dest->x = (fx32)((u32)Shifted(Product(x, m00) + Product(y, m10) + Product(z, m20)) + (u32)m30);
    dest->y = (fx32)((u32)Shifted(Product(x, m01) + Product(y, m11) + Product(z, m21)) + (u32)m31);
    dest->z = (fx32)((u32)Shifted(Product(x, m02) + Product(y, m12) + Product(z, m22)) + (u32)m32);
}
