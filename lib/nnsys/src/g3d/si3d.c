#include "nnsys/g3d.h"

// NitroSystem's G3D scaling rule and texture matrix mode of models made with Softimage 3D: the function that sends a
// joint's pose to the geometry engine, the one that sets the scale of a joint's pose from its node, and the one that
// sends a material's texture matrix. They are the ones that cgtool.c's tables hold. The file is named after
// NitroSystem's si3d.c. In this rule a joint cancels the scale of its parent, and the joints keep their scales in
// g_NNSG3DJointScales for the joints below them. NNSi_G3dGetJointScaleSi3d is NitroSystem's name, as pokeheartgold
// has it; the other two are NitroSystem's as recalled, parallel to maya.c's

#pragma thumb off

// Sends a joint's pose to the geometry engine. Unless the parent's scale is one, the joint scales by the parent's
// inverse scale (scaleEx1), translates by its translation scaled by the parent's scale (scaleEx0), rotates, and scales
// by the parent's scale again, so that the parent's scale moves the joint but does not reach its rotation. The
// rotation and translation go as one matrix when the parent's scale is one (the translation follows the rotation in
// the result), and either alone otherwise. The joint's own scale goes last
void NNSi_G3dSendJointSRTSi3d(const NNSG3dJntAnmResult *result) {
    BOOL hasTrans = FALSE;
    u32 parentScaleOne =
        result->flag & (NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE | NNS_G3D_JNTANM_RESULTFLAG_SCALEEX1_ONE);

    if (parentScaleOne == 0) {
        NNS_G3dGeScaleVec(&result->scaleEx1);
    }
    if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO)) {
        if (parentScaleOne != 0) {
            hasTrans = TRUE;
        } else {
            VecFx32 trans;

            trans.x = (fx32)(((fx64)result->trans.x * result->scaleEx0.x) >> FX32_SHIFT);
            trans.y = (fx32)(((fx64)result->trans.y * result->scaleEx0.y) >> FX32_SHIFT);
            trans.z = (fx32)(((fx64)result->trans.z * result->scaleEx0.z) >> FX32_SHIFT);
            NNS_G3dGeTranslateVec(&trans);
        }
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
    if (parentScaleOne == 0) {
        NNS_G3dGeScaleVec(&result->scaleEx0);
    }
    if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE)) {
        NNS_G3dGeScaleVec(&result->scale);
    }
}

// Sets the scale of a joint's pose from the scale of its node: the node's scale, or a scale of one if the node's flag
// says so. Every joint keeps its scale and inverse scale, relative to the root, in g_NNSG3DJointScales, and whether
// that scale is one in the render state. The joint's scaleEx0 and scaleEx1 are the scale and inverse scale of its
// parent, or flags say that they are one, as they are when the parent's scale is
void NNSi_G3dGetJointScaleSi3d(NNSG3dJntAnmResult *result, const NNSG3dJointScale *scale, const u8 *cmd, u32 srtFlag) {
    u32 node = cmd[1];
    u32 parent = cmd[2];

    if (srtFlag & NNS_G3D_SRTFLAG_SCALE_ONE) {
        result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE;
        if (g_NNSG3DModelRenderer->isScaleOne[parent >> 5] & (1 << (parent & 0x1f))) {
            g_NNSG3DModelRenderer->isScaleOne[node >> 5] |= 1 << (node & 0x1f);
            result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE | NNS_G3D_JNTANM_RESULTFLAG_SCALEEX1_ONE;
        } else {
            MIi_CpuCopy32(&g_NNSG3DJointScales[parent], &g_NNSG3DJointScales[node], sizeof(NNSG3dJointScale));
            MIi_CpuCopy32(&g_NNSG3DJointScales[parent], &result->scaleEx0, sizeof(NNSG3dJointScale));
        }
    } else {
        result->scale.x = scale->scale.x;
        result->scale.y = scale->scale.y;
        result->scale.z = scale->scale.z;
        if (g_NNSG3DModelRenderer->isScaleOne[parent >> 5] & (1 << (parent & 0x1f))) {
            MIi_CpuCopy32(scale, &g_NNSG3DJointScales[node], sizeof(NNSG3dJointScale));
            g_NNSG3DModelRenderer->isScaleOne[node >> 5] &= ~(1 << (node & 0x1f));
            result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE | NNS_G3D_JNTANM_RESULTFLAG_SCALEEX1_ONE;
        } else {
            g_NNSG3DModelRenderer->isScaleOne[node >> 5] &= ~(1 << (node & 0x1f));
            g_NNSG3DJointScales[node].scale.x =
                (fx32)(((fx64)scale->scale.x * g_NNSG3DJointScales[parent].scale.x) >> FX32_SHIFT);
            g_NNSG3DJointScales[node].scale.y =
                (fx32)(((fx64)scale->scale.y * g_NNSG3DJointScales[parent].scale.y) >> FX32_SHIFT);
            g_NNSG3DJointScales[node].scale.z =
                (fx32)(((fx64)scale->scale.z * g_NNSG3DJointScales[parent].scale.z) >> FX32_SHIFT);
            g_NNSG3DJointScales[node].invScale.x =
                (fx32)(((fx64)scale->invScale.x * g_NNSG3DJointScales[parent].invScale.x) >> FX32_SHIFT);
            g_NNSG3DJointScales[node].invScale.y =
                (fx32)(((fx64)scale->invScale.y * g_NNSG3DJointScales[parent].invScale.y) >> FX32_SHIFT);
            g_NNSG3DJointScales[node].invScale.z =
                (fx32)(((fx64)scale->invScale.z * g_NNSG3DJointScales[parent].invScale.z) >> FX32_SHIFT);
            MIi_CpuCopy32(&g_NNSG3DJointScales[parent], &result->scaleEx0, sizeof(NNSG3dJointScale));
        }
    }
}

// Sends a packet of num words to the geometry engine: the packed opcodes, then the arguments of their commands
static inline void sendPacket_(const u32 *packet, u32 num) {
    NNS_G3dGeBufferOP_N(packet[0], &packet[1], num - 1);
}

// Sends the texture matrix of a material's result: the scale and translation of the texture, which Softimage has no
// rotation for, with the scale of the texture's size to its original size on the S and T of the matrix, and then the
// geometry engine's matrix mode back to position and vector. With TEXMTX_SET it is loaded, and with TEXMTX_MULT it is
// multiplied into the matrix that is there
void NNSi_G3dSendTexSRTSi3d(NNSG3dMatAnmResult *result) {
    struct {
        u32 op; // MTX_MODE, MTX_LOAD_4x3 or MTX_MULT_4x3, MTX_MODE
        u32 modeTexture;
        MtxFx43 mtx;
        u32 modePosition;
    } cmd;

    if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SET) {
        cmd.op = GX_PACK_OP(G3OP_MTX_MODE, G3OP_MTX_LOAD_4x3, G3OP_MTX_MODE, G3OP_NOP);
    } else {
        cmd.op = GX_PACK_OP(G3OP_MTX_MODE, G3OP_MTX_MULT_4x3, G3OP_MTX_MODE, G3OP_NOP);
    }
    cmd.modeTexture = GX_MTXMODE_TEXTURE;
    cmd.modePosition = GX_MTXMODE_POSITION_VECTOR;
    cmd.mtx.m[0][1] = cmd.mtx.m[0][2] = cmd.mtx.m[1][0] = cmd.mtx.m[1][2] = cmd.mtx.m[2][0] = cmd.mtx.m[2][1] =
        cmd.mtx.m[2][2] = cmd.mtx.m[3][2] = 0;
    if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_TRANSZERO) {
        cmd.mtx.m[3][0] = 0;
        cmd.mtx.m[3][1] = 0;
        if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE) {
            cmd.mtx.m[0][0] = FX32_ONE;
            cmd.mtx.m[1][1] = FX32_ONE;
        } else {
            cmd.mtx.m[0][0] = result->scaleS;
            cmd.mtx.m[1][1] = result->scaleT;
        }
    } else if (result->flag & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE) {
        cmd.mtx.m[3][0] = -(result->transS << 4) * result->origWidth;
        cmd.mtx.m[3][1] = -(result->transT << 4) * result->origHeight;
        cmd.mtx.m[0][0] = FX32_ONE;
        cmd.mtx.m[1][1] = FX32_ONE;
    } else {
        cmd.mtx.m[3][0] = -(fx32)(((fx64)result->scaleS * result->transS) >> 8) * result->origWidth;
        cmd.mtx.m[3][1] = -(fx32)(((fx64)result->scaleT * result->transT) >> 8) * result->origHeight;
        cmd.mtx.m[0][0] = result->scaleS;
        cmd.mtx.m[1][1] = result->scaleT;
    }
    if (result->magW != FX32_ONE) {
        cmd.mtx.m[0][0] = (fx32)(((fx64)result->magW * cmd.mtx.m[0][0]) >> FX32_SHIFT);
        cmd.mtx.m[3][0] = (fx32)(((fx64)result->magW * cmd.mtx.m[3][0]) >> FX32_SHIFT);
    }
    if (result->magH != FX32_ONE) {
        cmd.mtx.m[1][1] = (fx32)(((fx64)result->magH * cmd.mtx.m[1][1]) >> FX32_SHIFT);
        cmd.mtx.m[3][1] = (fx32)(((fx64)result->magH * cmd.mtx.m[3][1]) >> FX32_SHIFT);
    }
    sendPacket_((const u32 *)&cmd, sizeof(cmd) / sizeof(u32));
}
#pragma thumb on
