#include "nnsys/g2d.h"

// NitroSystem's 2D affine matrix product, called by the renderer's matrix stack, and the renderer's multi-cell state,
// which g2d_Renderer.c reaches through its symbol as an object of another file. NNSi_G2dMtxConcat32 (swan's MAT32_Mul)
// and the file name are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them; the ROM has no string for
// the file

MCRenderState mcRenderState_;

// Sets ab to a * b, where the matrices are 3x2 with the translation in the last row. ab may be b
void NNSi_G2dMtxConcat32(const MtxFx32 *a, const MtxFx32 *b, MtxFx32 *ab) {
    MtxFx32 tmp;
    MtxFx32 *p;

    if (ab == b) {
        p = &tmp;
    } else {
        p = ab;
    }

    p->_00 = (fx32)(((fx64)a->_00 * b->_00 + (fx64)a->_01 * b->_10) >> FX32_SHIFT);
    p->_01 = (fx32)(((fx64)a->_00 * b->_01 + (fx64)a->_01 * b->_11) >> FX32_SHIFT);
    p->_10 = (fx32)(((fx64)a->_10 * b->_00 + (fx64)a->_11 * b->_10) >> FX32_SHIFT);
    p->_11 = (fx32)(((fx64)a->_10 * b->_01 + (fx64)a->_11 * b->_11) >> FX32_SHIFT);
    p->_20 = (fx32)(((fx64)a->_20 * b->_00 + (fx64)a->_21 * b->_10) >> FX32_SHIFT) + b->_20;
    p->_21 = (fx32)(((fx64)a->_20 * b->_01 + (fx64)a->_21 * b->_11) >> FX32_SHIFT) + b->_21;

    if (p == &tmp) {
        *ab = tmp;
    }
}
