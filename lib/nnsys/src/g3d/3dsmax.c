#include "nnsys/g3d.h"

// NitroSystem's G3D texture matrix mode of models made with 3ds Max: the function that sends a material's texture
// matrix, which cgtool.c's table holds, and the eight that make its elements. The file is named after NitroSystem's
// 3dsmax.c, and holds no joint functions: cgtool.c's tables of them have no entry for 3ds Max. A texture matrix is
// made by one of eight functions, by which of the scale, rotation and translation the material's result has, as in
// maya.c. A 3ds Max texture turns and scales about its center, half its original size: the matrix's translation is
// the center plus the offsets from it, turned and scaled, where the offsets (offsetS and offsetT) are the center and
// the translation negated, in the texture coordinates' 12.4 fixed point, and the T translation has the opposite sign. A
// product here is truncated, not rounded. The function that sends the matrix, NNSi_G3dSendTexSRT3dsMax, has
// NitroSystem's name as recalled, parallel to maya.c's; the statics' names are ours

// A function that makes the elements of a material's texture matrix that its scale, rotation and translation set: the
// matrix (of which the caller sets the others) and the result
typedef void (*TexMtx3dsMaxFunc)(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);

static void calcTexMtx3dsMaxSRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtx3dsMaxRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtx3dsMaxST_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtx3dsMaxT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtx3dsMaxSR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtx3dsMaxR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtx3dsMaxS_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtx3dsMaxIdentity_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);

#pragma thumb off

// The texture matrix elements of a material with a scale, a rotation and a translation
static void calcTexMtx3dsMaxSRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 scaleSCos, scaleSSin, scaleTSin, scaleTCos;
    fx32 offsetS, offsetT;

    FX_DivAsync(height, width);
    scaleSCos = (fx32)(((fx64)result->scaleS * result->cosR) >> FX32_SHIFT);
    scaleSSin = (fx32)(((fx64)result->scaleS * result->sinR) >> FX32_SHIFT);
    scaleTCos = (fx32)(((fx64)result->scaleT * result->cosR) >> FX32_SHIFT);
    scaleTSin = (fx32)(((fx64)result->scaleT * result->sinR) >> FX32_SHIFT);

    mtx->m[0][0] = scaleSCos;
    mtx->m[1][1] = scaleTCos;
    mtx->m[0][1] = (scaleTSin * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    offsetS = (-result->origWidth << 11) - result->transS * result->origWidth;
    offsetT = result->transT * result->origHeight + (-result->origHeight << 11);
    mtx->m[3][0] = (fx32)(((fx64)scaleSCos * offsetS - (fx64)scaleSSin * offsetT) >> 8) + (result->origWidth << 15);
    mtx->m[3][1] = (fx32)(((fx64)scaleTSin * offsetS + (fx64)scaleTCos * offsetT) >> 8) + (result->origHeight << 15);
    mtx->m[1][0] = (-scaleSSin * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a scale of one
static void calcTexMtx3dsMaxRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 offsetS, offsetT;

    FX_DivAsync(height, width);
    mtx->m[0][0] = result->cosR;
    mtx->m[1][1] = result->cosR;
    mtx->m[0][1] = (result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    offsetS = (-result->origWidth << 11) - result->transS * result->origWidth;
    offsetT = result->transT * result->origHeight + (-result->origHeight << 11);
    mtx->m[3][0] =
        (fx32)(((fx64)result->cosR * offsetS - (fx64)result->sinR * offsetT) >> 8) + (result->origWidth << 15);
    mtx->m[3][1] =
        (fx32)(((fx64)result->sinR * offsetS + (fx64)result->cosR * offsetT) >> 8) + (result->origHeight << 15);
    mtx->m[1][0] = (-result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero rotation
static void calcTexMtx3dsMaxST_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 offsetS, offsetT;

    mtx->m[0][0] = result->scaleS;
    mtx->m[1][1] = result->scaleT;
    mtx->m[0][1] = 0;
    offsetS = (-result->origWidth << 11) - result->transS * result->origWidth;
    offsetT = result->transT * result->origHeight + (-result->origHeight << 11);
    mtx->m[3][0] = (fx32)(((fx64)result->scaleS * offsetS) >> 8) + (result->origWidth << 15);
    mtx->m[3][1] = (fx32)(((fx64)result->scaleT * offsetT) >> 8) + (result->origHeight << 15);
    mtx->m[1][0] = 0;
}

// The same for a zero rotation and a scale of one
static void calcTexMtx3dsMaxT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[3][0] = (-result->transS * result->origWidth) << 4;
    mtx->m[3][1] = (result->transT * result->origHeight) << 4;
    mtx->m[1][0] = 0;
}

// The same for a zero translation
static void calcTexMtx3dsMaxSR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 scaleSCos, scaleSSin, scaleTSin, scaleTCos;
    fx32 offsetS, offsetT;

    FX_DivAsync(height, width);
    scaleSCos = (fx32)(((fx64)result->scaleS * result->cosR) >> FX32_SHIFT);
    scaleSSin = (fx32)(((fx64)result->scaleS * result->sinR) >> FX32_SHIFT);
    scaleTCos = (fx32)(((fx64)result->scaleT * result->cosR) >> FX32_SHIFT);
    scaleTSin = (fx32)(((fx64)result->scaleT * result->sinR) >> FX32_SHIFT);

    mtx->m[0][0] = scaleSCos;
    mtx->m[1][1] = scaleTCos;
    mtx->m[0][1] = (scaleTSin * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    offsetS = -result->origWidth << 11;
    offsetT = -result->origHeight << 11;
    mtx->m[3][0] = (fx32)(((fx64)scaleSCos * offsetS - (fx64)scaleSSin * offsetT) >> 8) + (result->origWidth << 15);
    mtx->m[3][1] = (fx32)(((fx64)scaleTSin * offsetS + (fx64)scaleTCos * offsetT) >> 8) + (result->origHeight << 15);
    mtx->m[1][0] = (-scaleSSin * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero translation and a scale of one
static void calcTexMtx3dsMaxR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 offsetS, offsetT;

    FX_DivAsync(height, width);
    mtx->m[0][0] = result->cosR;
    mtx->m[1][1] = result->cosR;
    mtx->m[0][1] = (result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    offsetS = -result->origWidth << 11;
    offsetT = -result->origHeight << 11;
    mtx->m[3][0] =
        (fx32)(((fx64)result->cosR * offsetS - (fx64)result->sinR * offsetT) >> 8) + (result->origWidth << 15);
    mtx->m[3][1] =
        (fx32)(((fx64)result->sinR * offsetS + (fx64)result->cosR * offsetT) >> 8) + (result->origHeight << 15);
    mtx->m[1][0] = (-result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero translation and a zero rotation
static void calcTexMtx3dsMaxS_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = result->scaleS;
    mtx->m[1][1] = result->scaleT;
    mtx->m[0][1] = 0;
    mtx->m[3][0] = ((FX32_ONE - result->scaleS) * result->origWidth) << 3;
    mtx->m[3][1] = ((FX32_ONE - result->scaleT) * result->origHeight) << 3;
    mtx->m[1][0] = 0;
}

// The same for none of them
static void calcTexMtx3dsMaxIdentity_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
}

// Indexed by the low three bits of a material result's flag: the scale of one, the zero rotation and the zero
// translation
static TexMtx3dsMaxFunc sTexMtx3dsMaxFuncs[8] = {
    calcTexMtx3dsMaxSRT_, calcTexMtx3dsMaxRT_, calcTexMtx3dsMaxST_, calcTexMtx3dsMaxT_,
    calcTexMtx3dsMaxSR_,  calcTexMtx3dsMaxR_,  calcTexMtx3dsMaxS_,  calcTexMtx3dsMaxIdentity_,
};

// Sends the texture matrix of a material's result: the matrix of the function that suits its flag, with the first row
// and the S translation scaled by magW, the ratio of the texture's size to its original size, and the second row and
// the T translation by magH, between a change of the geometry engine's matrix mode to texture and back to position and
// vector. With TEXMTX_SET it is loaded, and with TEXMTX_MULT it is multiplied into the matrix that is there
void NNSi_G3dSendTexSRT3dsMax(NNSG3dMatAnmResult *result) {
    struct {
        u32 op; // MTX_MODE, MTX_LOAD_4x4 or MTX_MULT_4x4, MTX_MODE
        u32 modeTexture;
        MtxFx44 mtx;
        u32 modePosition;
    } cmd;

    if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SET) {
        cmd.op = GX_PACK_OP(G3OP_MTX_MODE, G3OP_MTX_LOAD_4x4, G3OP_MTX_MODE, G3OP_NOP);
    } else {
        cmd.op = GX_PACK_OP(G3OP_MTX_MODE, G3OP_MTX_MULT_4x4, G3OP_MTX_MODE, G3OP_NOP);
    }
    cmd.modeTexture = GX_MTXMODE_TEXTURE;
    cmd.modePosition = GX_MTXMODE_POSITION_VECTOR;
    cmd.mtx.m[0][2] = cmd.mtx.m[0][3] = cmd.mtx.m[1][2] = cmd.mtx.m[1][3] = cmd.mtx.m[2][0] = cmd.mtx.m[2][1] =
        cmd.mtx.m[2][2] = cmd.mtx.m[2][3] = cmd.mtx.m[3][2] = 0;
    cmd.mtx.m[3][3] = FX32_ONE;

    sTexMtx3dsMaxFuncs[result->flag &
                       (NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE | NNS_G3D_MATANM_RESULTFLAG_TEXMTX_ROTZERO |
                        NNS_G3D_MATANM_RESULTFLAG_TEXMTX_TRANSZERO)](&cmd.mtx, result);
    if (result->magW != FX32_ONE) {
        cmd.mtx.m[0][0] = (fx32)(((fx64)result->magW * cmd.mtx.m[0][0]) >> FX32_SHIFT);
        cmd.mtx.m[0][1] = (fx32)(((fx64)result->magW * cmd.mtx.m[0][1]) >> FX32_SHIFT);
        cmd.mtx.m[3][0] = (fx32)(((fx64)result->magW * cmd.mtx.m[3][0]) >> FX32_SHIFT);
    }
    if (result->magH != FX32_ONE) {
        cmd.mtx.m[1][0] = (fx32)(((fx64)result->magH * cmd.mtx.m[1][0]) >> FX32_SHIFT);
        cmd.mtx.m[1][1] = (fx32)(((fx64)result->magH * cmd.mtx.m[1][1]) >> FX32_SHIFT);
        cmd.mtx.m[3][1] = (fx32)(((fx64)result->magH * cmd.mtx.m[3][1]) >> FX32_SHIFT);
    }
    // The arguments, the matrix modes and the matrix, are the words that follow the opcodes
    NNS_G3dGeBufferOP_N(cmd.op, &cmd.modeTexture, sizeof(cmd) / sizeof(u32) - 1);
}
#pragma thumb on
