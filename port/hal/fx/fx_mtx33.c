// Host versions of the functions of NitroSDK's fx_mtx33.c that the SDK wrote in assembly. The originals are in
// lib/nitro/src/fx/fx_mtx33.c, behind #ifndef PLATFORM_PC; the rest of that file compiles on the host as it is.
// Each stores the words the original stores. Only MTX_Copy33To44_ reads memory, so only its order shows.
#include "nitro/fx.h"

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
void MTX_Copy33To44_(const MtxFx33 *src, MtxFx44 *dst) {
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

void MTX_Scale33_(MtxFx33 *mtx, fx32 x, fx32 y, fx32 z) {
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
