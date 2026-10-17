// Host versions of the functions of NitroSDK's fx_mtx22.c that the SDK wrote in assembly. The originals are in
// lib/nitro/src/fx/fx_mtx22.c, behind #ifndef PLATFORM_PC; the rest of that file compiles on the host as it is.
// Each stores the words the original stores. None reads memory, so the order of the stores doesn't show.
#include "nitro/fx.h"

void MTX_Identity22_(MtxFx22 *mtx) {
    mtx->_00 = FX32_ONE;
    mtx->_01 = 0;
    mtx->_10 = 0;
    mtx->_11 = FX32_ONE;
}

void MAT2_Scaling(MtxFx22 *mtx, fx32 x, fx32 y) {
    mtx->_00 = x;
    mtx->_01 = 0;
    mtx->_10 = 0;
    mtx->_11 = y;
}

void MTX_Rot22_(MtxFx22 *mtx, fx32 sin, fx32 cos) {
    mtx->_00 = cos;
    mtx->_01 = sin;
    // Negated as a u32, as NEG does: FX32_MIN stays FX32_MIN, where a signed negation would overflow
    mtx->_10 = (fx32)(0 - (u32)sin);
    mtx->_11 = cos;
}
