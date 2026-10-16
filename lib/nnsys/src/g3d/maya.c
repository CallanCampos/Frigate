#include "nnsys/g3d.h"

// NitroSystem's G3D scaling rule and texture matrix mode of models made with Maya: the function that sends a joint's
// pose to the geometry engine, the one that sets the scale of a joint's pose from its node, and the one that sends a
// material's texture matrix. They are the ones that cgtool.c's tables hold. The file is named after NitroSystem's
// maya.c. A Maya joint can cancel the scale of its parent (segment scale compensate): the parent keeps its inverse
// scale for the joint, and the joint scales by it after its translation. A texture matrix is made by one of eight
// functions, by which of the scale, rotation and translation the material's result has. The three global names are
// NitroSystem's as recalled, parallel to basic.c's; the statics' names are ours

// A function that makes the elements of a material's texture matrix that its scale, rotation and translation set: the
// matrix (of which the caller sets the others) and the result
typedef void (*TexMtxMayaFunc)(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);

static void calcTexMtxMayaSRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxMayaRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxMayaST_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxMayaT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxMayaSR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxMayaR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxMayaS_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);
static void calcTexMtxMayaIdentity_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result);

// Sends a joint's pose to the geometry engine. A joint that cancels the scale of its parent sends the translation
// first and then the inverse of its parent's scale (scaleEx0), so that the parent's scale moves the joint but does not
// reach its rotation or scale; then the rotation and translation (the translation follows the rotation in the result)
// or either alone, and the joint's own scale
void NNSi_G3dSendJointSRTMaya(const NNSG3dJntAnmResult *result) {
    BOOL hasTrans = FALSE;

    if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO)) {
        hasTrans = TRUE;
    }
    if ((result->flag & NNS_G3D_JNTANM_RESULTFLAG_MAYA_SSC_APPLY) &&
        !(result->flag & NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE)) {
        if (hasTrans) {
            NNS_G3dGeTranslateVec(&result->trans);
            hasTrans = FALSE;
        }
        NNS_G3dGeScaleVec(&result->scaleEx0);
    }
    if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO)) {
        if (hasTrans) {
            NNS_G3dGeMultMtx43((const MtxFx43 *)&result->rot);
        } else {
            NNS_G3dGeMultMtx33(&result->rot);
        }
    } else if (hasTrans) {
        NNS_G3dGeTranslateVec(&result->trans);
    }
    if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE)) {
        NNS_G3dGeScaleVec(&result->scale);
    }
}

// Sets the scale of a joint's pose from the scale of its node: the node's scale, or a scale of one if the node's flag
// says so. A joint whose children cancel its scale (the NODEDESC command's flag) keeps its inverse scale in
// g_NNSG3DJointScales and whether its scale is one in the render state. A joint that cancels its parent's scale takes
// the parent's inverse scale as its scaleEx0, or flags it as one if the parent's scale is
void NNSi_G3dGetJointScaleMaya(NNSG3dJntAnmResult *result, const NNSG3dJointScale *scale, const u8 *cmd, u32 srtFlag) {
    u32 flags = cmd[3];

    if (srtFlag & NNS_G3D_SRTFLAG_SCALE_ONE) {
        result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE;
        if (flags & NNS_G3D_SBC_NODEDESC_FLAG_MAYA_SSC_CHILD) {
            u32 node = cmd[1];

            g_NNSG3DModelRenderer->isScaleOne[node >> 5] |= 1 << (node & 0x1f);
        }
    } else {
        result->scale.x = scale->scale.x;
        result->scale.y = scale->scale.y;
        result->scale.z = scale->scale.z;
        if (flags & NNS_G3D_SBC_NODEDESC_FLAG_MAYA_SSC_CHILD) {
            u32 node = cmd[1];

            g_NNSG3DModelRenderer->isScaleOne[node >> 5] &= ~(1 << (node & 0x1f));
            g_NNSG3DJointScales[node].invScale.x = scale->invScale.x;
            g_NNSG3DJointScales[node].invScale.y = scale->invScale.y;
            g_NNSG3DJointScales[node].invScale.z = scale->invScale.z;
        }
    }
    if (flags & NNS_G3D_SBC_NODEDESC_FLAG_MAYA_SSC_APPLY) {
        u32 parent = cmd[2];

        result->flag |= NNS_G3D_JNTANM_RESULTFLAG_MAYA_SSC_APPLY;
        if (g_NNSG3DModelRenderer->isScaleOne[parent >> 5] & (1 << (parent & 0x1f))) {
            result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE;
        } else {
            result->scaleEx0 = g_NNSG3DJointScales[parent].invScale;
        }
    }
    result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALEEX1_ONE;
}

#pragma thumb off

// The texture matrix elements of a material with a scale, a rotation and a translation. A Maya texture is turned about
// its center and flipped vertically, so the translation has the center and the flip in it, and the shear of the
// rotation follows the texture's proportions. A product here is truncated, not rounded
static void calcTexMtxMayaSRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;
    fx32 scaleSCos, scaleSSin, scaleTSin, scaleTCos;

    FX_DivAsync(height, width);
    scaleSSin = (fx32)(((fx64)result->scaleS * result->sinR) >> FX32_SHIFT);
    scaleSCos = (fx32)(((fx64)result->scaleS * result->cosR) >> FX32_SHIFT);
    scaleTSin = (fx32)(((fx64)result->scaleT * result->sinR) >> FX32_SHIFT);
    scaleTCos = (fx32)(((fx64)result->scaleT * result->cosR) >> FX32_SHIFT);

    mtx->m[0][0] = scaleSCos;
    mtx->m[1][1] = scaleTCos;
    mtx->m[0][1] = (-scaleTSin * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    mtx->m[3][0] = ((result->origWidth * (result->scaleS - (scaleSSin + scaleSCos))) << 3) -
                   result->origWidth * (fx32)(((fx64)result->scaleS * result->transS) >> 8);
    mtx->m[3][1] = ((result->origHeight * (scaleTSin - scaleTCos - result->scaleT + 2 * FX32_ONE)) << 3) +
                   result->origHeight * (fx32)(((fx64)result->scaleT * result->transT) >> 8);
    mtx->m[1][0] = (scaleSSin * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a scale of one
static void calcTexMtxMayaRT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;

    FX_DivAsync(height, width);
    mtx->m[0][0] = result->cosR;
    mtx->m[1][1] = result->cosR;
    mtx->m[0][1] = (-result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    mtx->m[3][0] = ((result->origWidth * (-(result->sinR + result->cosR) + FX32_ONE)) << 3) -
                   ((result->transS * result->origWidth) << 4);
    mtx->m[3][1] = ((result->origHeight * (result->sinR - result->cosR + FX32_ONE)) << 3) +
                   ((result->transT * result->origHeight) << 4);
    mtx->m[1][0] = (result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero rotation
static void calcTexMtxMayaST_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = result->scaleS;
    mtx->m[1][1] = result->scaleT;
    mtx->m[0][1] = 0;
    mtx->m[3][0] = -(fx32)(((fx64)result->scaleS * result->transS) >> 8) * result->origWidth;
    mtx->m[3][1] = ((result->origHeight * (-2 * result->scaleT + 2 * FX32_ONE)) << 3) +
                   result->origHeight * (fx32)(((fx64)result->scaleT * result->transT) >> 8);
    mtx->m[1][0] = 0;
}

// The same for a zero rotation and a scale of one
static void calcTexMtxMayaT_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[3][0] = (-(result->transS * result->origWidth)) << 4;
    mtx->m[3][1] = (result->transT * result->origHeight) << 4;
    mtx->m[1][0] = 0;
}

// The same for a zero translation
static void calcTexMtxMayaSR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 scaleSCos;
    fx32 scaleSSin;
    fx32 scaleTSin;
    fx32 scaleTCos;
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;

    FX_DivAsync(height, width);
    scaleSSin = (fx32)(((fx64)result->scaleS * result->sinR) >> FX32_SHIFT);
    scaleSCos = (fx32)(((fx64)result->scaleS * result->cosR) >> FX32_SHIFT);
    scaleTSin = (fx32)(((fx64)result->scaleT * result->sinR) >> FX32_SHIFT);
    scaleTCos = (fx32)(((fx64)result->scaleT * result->cosR) >> FX32_SHIFT);
    mtx->m[0][0] = scaleSCos;
    mtx->m[1][1] = scaleTCos;
    mtx->m[0][1] = (-scaleTSin * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    mtx->m[3][0] = (result->origWidth * (result->scaleS - (scaleSSin + scaleSCos))) << 3;
    mtx->m[3][1] = (result->origHeight * (scaleTSin - scaleTCos - result->scaleT + 2 * FX32_ONE)) << 3;
    mtx->m[1][0] = (scaleSSin * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero translation and a scale of one
static void calcTexMtxMayaR_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    fx32 width = result->origWidth << FX32_SHIFT;
    fx32 height = result->origHeight << FX32_SHIFT;

    FX_DivAsync(height, width);
    mtx->m[0][0] = result->cosR;
    mtx->m[1][1] = result->cosR;
    mtx->m[0][1] = (-result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
    FX_DivAsync(width, height);
    mtx->m[3][0] = (result->origWidth * (-(result->sinR + result->cosR) + FX32_ONE)) << 3;
    mtx->m[3][1] = (result->origHeight * (result->sinR - result->cosR + FX32_ONE)) << 3;
    mtx->m[1][0] = (result->sinR * FX_GetDivResult()) >> FX32_SHIFT;
}

// The same for a zero translation and a zero rotation
static void calcTexMtxMayaS_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = result->scaleS;
    mtx->m[1][1] = result->scaleT;
    mtx->m[0][1] = 0;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = (result->origHeight * (-2 * result->scaleT + 2 * FX32_ONE)) << 3;
    mtx->m[1][0] = 0;
}

// The same for none of them
static void calcTexMtxMayaIdentity_(MtxFx44 *mtx, const NNSG3dMatAnmResult *result) {
    mtx->m[0][0] = FX32_ONE;
    mtx->m[0][1] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[3][0] = 0;
    mtx->m[3][1] = 0;
}

// Indexed by the low three bits of a material result's flag: the scale of one, the zero rotation and the zero
// translation
static TexMtxMayaFunc sTexMtxMayaFuncs[8] = {
    calcTexMtxMayaSRT_, calcTexMtxMayaRT_, calcTexMtxMayaST_, calcTexMtxMayaT_,
    calcTexMtxMayaSR_,  calcTexMtxMayaR_,  calcTexMtxMayaS_,  calcTexMtxMayaIdentity_,
};

// Sends a packet of num words to the geometry engine: the packed opcodes, then the arguments of their commands
static inline void sendPacket_(const u32 *packet, u32 num) {
    NNS_G3dGeBufferOP_N(packet[0], &packet[1], num - 1);
}

// Sends the texture matrix of a material's result: the matrix of the function that suits its flag, with the first row
// and the S translation scaled by magW, the ratio of the texture's size to its original size, and the second row and
// the T translation by magH, between a change of the geometry engine's matrix mode to texture and back to position and
// vector. With TEXMTX_SET it is loaded, and with TEXMTX_MULT it is multiplied into the matrix that is there
void NNSi_G3dSendTexSRTMaya(NNSG3dMatAnmResult *result) {
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

    sTexMtxMayaFuncs[result->flag &
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
    sendPacket_((const u32 *)&cmd, sizeof(cmd) / sizeof(u32));
}
#pragma thumb on
