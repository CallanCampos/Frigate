#include "nnsys/g3d.h"

// NitroSystem's G3D texture matrix mode of models made with Softimage XSI: the function that sends a material's
// texture matrix, which cgtool.c's table holds, and the eight that make its elements. The file is named after
// NitroSystem's xsi.c, and holds no joint functions: cgtool.c's tables of them have no entry for XSI. A texture matrix
// is made by one of eight functions, by which of the scale, rotation and translation the material's result has, as in
// maya.c and 3dsmax.c. An XSI texture turns and scales about a corner, not its center, and its translation is turned
// and scaled with it, so the S and T translations enter through their turned values. A product here is truncated, not
// rounded. The function that sends the matrix first sets the identity values into the parts of the result that its
// flag says are the identity, so it takes the result without const, and so does the table's type. Its name,
// NNSi_G3dSendTexSRTXsi, is NitroSystem's as recalled, parallel to maya.c's; the statics' names are ours

// A function that makes the elements of a material's texture matrix that its scale, rotation and translation set: the
// matrix (of which the caller sets the others) and the result
typedef void (*TexMtxXsiFunc)(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);

static void calcTexMtxXsiSRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxXsiRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxXsiST_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxXsiT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxXsiSR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxXsiR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxXsiS_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxXsiIdentity_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);

#pragma thumb off

// The texture matrix elements of a material with a scale, a rotation and a translation
static void calcTexMtxXsiSRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 scaleSCos, scaleSSin, scaleTCos, scaleTSin;
    fx64 turnedS, turnedT;
    fx32 scaledS, scaledT;

    FX_DivAsync(height, width);
    scaleSCos = (fx32)(((fx64)result->scaleS * result->cosR) >> FX32_SHIFT);
    scaleSSin = (fx32)(((fx64)result->scaleS * result->sinR) >> FX32_SHIFT);
    scaleTCos = (fx32)(((fx64)result->scaleT * result->cosR) >> FX32_SHIFT);
    scaleTSin = (fx32)(((fx64)result->scaleT * result->sinR) >> FX32_SHIFT);
    mtx->m[0][0] = scaleSCos;
    mtx->m[1][1] = scaleTCos;
    mtx->m[0][1] = (scaleTSin * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    turnedS = ((fx64)result->transS * result->cosR + (fx64)result->transT * result->sinR) >> FX32_SHIFT;
    turnedT = ((fx64)result->transS * result->sinR - (fx64)result->transT * result->cosR) >> FX32_SHIFT;
    scaledT = (fx32)((turnedT * result->scaleT) >> FX32_SHIFT);
    scaledS = (fx32)((turnedS * result->scaleS) >> FX32_SHIFT);
    mtx->m[3][0] = (result->origWidth * (scaleSSin - scaledS)) << 4;
    mtx->m[3][1] = (-result->origHeight * (scaleTCos + scaledT - FX32_ONE)) << 4;
    mtx->m[1][0] = (-scaleSSin * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a scale of one
static void calcTexMtxXsiRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 turnedS, turnedT;

    FX_DivAsync(height, width);
    mtx->m[0][0] = result->cosR;
    mtx->m[1][1] = result->cosR;
    mtx->m[0][1] = (result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    turnedS = (fx32)(((fx64)result->transS * result->cosR + (fx64)result->transT * result->sinR) >> FX32_SHIFT);
    turnedT = (fx32)(((fx64)result->transS * result->sinR - (fx64)result->transT * result->cosR) >> FX32_SHIFT);
    mtx->m[3][0] = (result->origWidth * (result->sinR - turnedS)) << 4;
    mtx->m[3][1] = (-result->origHeight * (result->cosR + turnedT - FX32_ONE)) << 4;
    mtx->m[1][0] = (-result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero rotation
static void calcTexMtxXsiST_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 scaledS, scaledT;

    mtx->m[0][0] = result->scaleS;
    mtx->m[1][1] = result->scaleT;
    mtx->m[0][1] = 0;
    scaledS = -(fx32)(((fx64)result->transS * result->scaleS) >> FX32_SHIFT);
    scaledT = (fx32)(((fx64)(-result->transT) * result->scaleT) >> FX32_SHIFT);
    mtx->m[3][0] = (result->origWidth * scaledS) << 4;
    mtx->m[3][1] = (-result->origHeight * (result->scaleT + scaledT - FX32_ONE)) << 4;
    mtx->m[1][0] = 0;
}

// The same for a zero rotation and a scale of one
static void calcTexMtxXsiT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    // The turned translations of calcTexMtxXsiRT_, with a zero rotation: S as it is and T negated
    fx32 turnedS, turnedT;

    mtx->m[0][0] = FX32_ONE;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[0][1] = 0;
    turnedS = result->transS;
    turnedT = -result->transT;
    mtx->m[3][0] = (result->origWidth * -turnedS) << 4;
    mtx->m[3][1] = (-result->origHeight * turnedT) << 4;
    mtx->m[1][0] = 0;
}

// The same for a zero translation
static void calcTexMtxXsiSR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 scaleSCos, scaleSSin, scaleTCos, scaleTSin;

    FX_DivAsync(height, width);
    scaleSCos = (fx32)(((fx64)result->scaleS * result->cosR) >> FX32_SHIFT);
    scaleSSin = (fx32)(((fx64)result->scaleS * result->sinR) >> FX32_SHIFT);
    scaleTCos = (fx32)(((fx64)result->scaleT * result->cosR) >> FX32_SHIFT);
    scaleTSin = (fx32)(((fx64)result->scaleT * result->sinR) >> FX32_SHIFT);
    mtx->m[0][0] = scaleSCos;
    mtx->m[1][1] = scaleTCos;
    mtx->m[0][1] = (scaleTSin * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    mtx->m[3][0] = (result->origWidth * scaleSSin) << 4;
    mtx->m[3][1] = (-result->origHeight * (scaleTCos - FX32_ONE)) << 4;
    mtx->m[1][0] = (-scaleSSin * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero translation and a scale of one
static void calcTexMtxXsiR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;

    FX_DivAsync(height, width);
    mtx->m[0][0] = result->cosR;
    mtx->m[1][1] = result->cosR;
    mtx->m[0][1] = (result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    mtx->m[3][0] = (result->origWidth * result->sinR) << 4;
    mtx->m[3][1] = (-result->origHeight * (result->cosR - FX32_ONE)) << 4;
    mtx->m[1][0] = (-result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero translation and a zero rotation
static void calcTexMtxXsiS_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = result->scaleS;
    mtx->m[1][1] = result->scaleT;
    mtx->m[0][1] = 0;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = (-result->origHeight * (result->scaleT - FX32_ONE)) << 4;
    mtx->m[1][0] = 0;
}

// The same for none of them
static void calcTexMtxXsiIdentity_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
}

// Indexed by the low three bits of a material result's flag: the scale of one, the zero rotation and the zero
// translation
static TexMtxXsiFunc sTexMtxXsiFuncs[8] = {
    calcTexMtxXsiSRT_, calcTexMtxXsiRT_, calcTexMtxXsiST_, calcTexMtxXsiT_,
    calcTexMtxXsiSR_,  calcTexMtxXsiR_,  calcTexMtxXsiS_,  calcTexMtxXsiIdentity_,
};

// Sends the texture matrix of a material's result: first the identity values in the parts that its flag says are the
// identity, then the matrix of the function that suits the flag, with the first row and the S translation scaled by
// magW, the ratio of the texture's size to its original size, and the second row and the T translation by magH,
// between a change of the geometry engine's matrix mode to texture and back to position and vector. With TEXMTX_SET
// it is loaded, and with TEXMTX_MULT it is multiplied into the matrix that is there
void NNSi_G3dSendTexSRTXsi(NNSG3dMatAnmResult *result) {
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

    if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE) {
        result->scaleS = result->scaleT = FX32_ONE;
    }
    if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_ROTZERO) {
        result->cosR = FX32_ONE;
        result->sinR = 0;
    }
    if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_TRANSZERO) {
        result->transS = result->transT = 0;
    }
    sTexMtxXsiFuncs[result->flag &
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
