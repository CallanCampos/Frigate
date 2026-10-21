#include "nnsys/g3d.h"

// NitroSystem's G3D standard (basic, no tool) scaling rule: the function that sends a joint's pose to the geometry
// engine, and the one that sets the scale of a joint's pose from its node. The file is named after NitroSystem's
// basic.c. Both functions are the ones that cgtool.c's tables hold. Their names are NitroSystem's as recalled, not
// shown by the code, and ours in config/names.txt

// Sends a joint's pose to the geometry engine: the rotation and translation as one matrix (the translation follows the
// rotation in the result), or either of them alone, then the scale, each unless the result's flag says it is the
// identity
void NNSi_G3dSendJointSRTBasic(const NNSG3dJntAnmResult *result) {
    if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO)) {
        if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO)) {
            NNS_G3dGeMultMtx43((const MtxFx43 *)&result->rot);
        } else {
            NNS_G3dGeTranslateVec(&result->trans);
        }
    } else if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO)) {
        NNS_G3dGeMultMtx33(&result->rot);
    }
    if (!(result->flag & NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE)) {
        NNS_G3dGeScaleVec(&result->scale);
    }
}

// Sets the scale of a joint's pose from the scale of its node: the node's scale, or a scale of one if the node's flag
// says so. The standard rule has no other scales, which the result flags as one
void NNSi_G3dGetJointScaleBasic(NNSG3dJntAnmResult *result, const fx32 *p, const u8 *cmd, u32 srtFlag) {
    if (srtFlag & NNS_G3D_SRTFLAG_SCALE_ONE) {
        result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE;
    } else {
        result->scale.x = p[0];
        result->scale.y = p[1];
        result->scale.z = p[2];
    }
    result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE | NNS_G3D_JNTANM_RESULTFLAG_SCALEEX1_ONE;
}
