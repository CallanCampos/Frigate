// Host versions of the functions of NitroSDK's fx_mtx44.c that the SDK wrote in assembly, and of MTX_Concat44, whose C
// reads an argument after storing over it where the original doesn't (lib/nitro/src/fx/fx_mtx44.c). The originals are
// there, behind #ifndef PLATFORM_PC; the rest of that file compiles on the host as it is. Each stores the words the
// original stores, and MTX_Copy44To43_ and MTX_Concat44 read memory in the original's order around their stores, so
// that arguments that overlap give what the original gives.
#include "nitro/fx.h"

// A product's word as the SDK's SMULL and SMLAL chains and LSR and ORR pair make it: the products' 64-bit sum, which
// wraps, and its bits 12 to 43
static fx32 Shifted(u64 sum) {
    return (fx32)(u32)(sum >> FX32_SHIFT);
}

static u64 Product(fx32 a, fx32 b) {
    return (u64)((s64)a * b);
}

void MTX_Identity44_(MtxFx44 *mtx) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[0][3] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[1][2] = 0;
    mtx->m[1][3] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = FX32_ONE;
    mtx->m[2][3] = 0;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
    mtx->m[3][2] = 0;
    mtx->m[3][3] = FX32_ONE;
}

// Row by row, each row read before it is written, as the SDK's LDM and STM pairs do: when the two matrices overlap,
// a row stored over source words that are still to be read changes what is read, the same way. The rows are walked
// through fx32 pointers, which may alias each other whatever the compiler's aliasing rules
void MTX_Copy44To43_(const MtxFx44 *src, MtxFx43 *dst) {
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
    }
}

// As MTX_Concat33 (port/hal/fx/fx_mtx33.c), four by four
void MTX_Concat44(const MtxFx44 *a, const MtxFx44 *b, MtxFx44 *ab) {
    MtxFx44 tmp;
    MtxFx44 *p = ab == b ? &tmp : ab;
    fx32 a00, a01, a02, a03, a10, a11, a12, a13, a20, a21, a22, a23, a30, a31, a32, a33;
    fx32 b0, b1, b2, b3;

    a01 = a->m[0][1];
    b1 = b->m[1][0];
    a00 = a->m[0][0];
    b0 = b->m[0][0];
    a02 = a->m[0][2];
    b2 = b->m[2][0];
    a03 = a->m[0][3];
    b3 = b->m[3][0];
    a11 = a->m[1][1];
    p->m[0][0] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2) + Product(a03, b3));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    b3 = b->m[3][1];
    a10 = a->m[1][0];
    p->m[0][1] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2) + Product(a03, b3));
    b3 = b->m[3][3];
    b2 = b->m[2][3];
    b1 = b->m[1][3];
    b0 = b->m[0][3];
    a12 = a->m[1][2];
    p->m[0][3] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2) + Product(a03, b3));
    b1 = b->m[1][2];
    b0 = b->m[0][2];
    b2 = b->m[2][2];
    b3 = b->m[3][2];
    a13 = a->m[1][3];
    p->m[0][2] = Shifted(Product(a00, b0) + Product(a01, b1) + Product(a02, b2) + Product(a03, b3));
    p->m[1][2] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2) + Product(a13, b3));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    b3 = b->m[3][1];
    p->m[1][1] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2) + Product(a13, b3));
    b0 = b->m[0][3];
    b2 = b->m[2][3];
    b1 = b->m[1][3];
    b3 = b->m[3][3];
    a21 = a->m[2][1];
    p->m[1][3] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2) + Product(a13, b3));
    b1 = b->m[1][0];
    b0 = b->m[0][0];
    b2 = b->m[2][0];
    a20 = a->m[2][0];
    b3 = b->m[3][0];
    a22 = a->m[2][2];
    a23 = a->m[2][3];
    p->m[1][0] = Shifted(Product(a10, b0) + Product(a11, b1) + Product(a12, b2) + Product(a13, b3));
    p->m[2][0] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2) + Product(a23, b3));
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    b1 = b->m[1][1];
    b3 = b->m[3][1];
    a31 = a->m[3][1];
    p->m[2][1] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2) + Product(a23, b3));
    b2 = b->m[2][3];
    b1 = b->m[1][3];
    b0 = b->m[0][3];
    a30 = a->m[3][0];
    a32 = a->m[3][2];
    a33 = a->m[3][3];
    b3 = b->m[3][3];
    p->m[2][3] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2) + Product(a23, b3));
    b1 = b->m[1][2];
    b0 = b->m[0][2];
    b2 = b->m[2][2];
    b3 = b->m[3][2];
    p->m[2][2] = Shifted(Product(a20, b0) + Product(a21, b1) + Product(a22, b2) + Product(a23, b3));
    p->m[3][2] = Shifted(Product(a30, b0) + Product(a31, b1) + Product(a32, b2) + Product(a33, b3));
    b1 = b->m[1][1];
    b0 = b->m[0][1];
    b2 = b->m[2][1];
    b3 = b->m[3][1];
    p->m[3][1] = Shifted(Product(a30, b0) + Product(a31, b1) + Product(a32, b2) + Product(a33, b3));
    b1 = b->m[1][0];
    b0 = b->m[0][0];
    b2 = b->m[2][0];
    b3 = b->m[3][0];
    p->m[3][0] = Shifted(Product(a30, b0) + Product(a31, b1) + Product(a32, b2) + Product(a33, b3));
    b1 = b->m[1][3];
    b3 = b->m[3][3];
    b2 = b->m[2][3];
    b0 = b->m[0][3];
    p->m[3][3] = Shifted(Product(a30, b0) + Product(a31, b1) + Product(a32, b2) + Product(a33, b3));
    if (p == &tmp) {
        *ab = tmp;
    }
}
