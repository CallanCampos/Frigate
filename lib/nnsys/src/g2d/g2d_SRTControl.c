#include "nitro/mi.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_SRTControl.c: the scale, rotation and translation that a cell or multi-cell animation's frame
// gives, with a flag for each one that is set. The file name is a guess from the functions' NNSi_G2dSrtc prefix

void NNSi_G2dSrtcSetTrans(NNSG2dSRTControl *srtCtrl, s16 x, s16 y) {
    if (srtCtrl->type == NNS_G2D_SRTCONTROLTYPE_SRT) {
        srtCtrl->data.srtData.SRT_EnableFlag |= NNS_G2D_SRTFLAG_TRANS;
        srtCtrl->data.srtData.trans.x = x;
        srtCtrl->data.srtData.trans.y = y;
    }
}

void NNSi_G2dSrtcSetSRTRotZ(NNSG2dSRTControl *srtCtrl, u16 rotZ) {
    if (srtCtrl->type == NNS_G2D_SRTCONTROLTYPE_SRT) {
        srtCtrl->data.srtData.SRT_EnableFlag |= NNS_G2D_SRTFLAG_ROTZ;
        srtCtrl->data.srtData.rotZ = rotZ;
    }
}

void NNSi_G2dSrtcSetSRTScale(NNSG2dSRTControl *srtCtrl, fx32 sx, fx32 sy) {
    if (srtCtrl->type == NNS_G2D_SRTCONTROLTYPE_SRT) {
        srtCtrl->data.srtData.SRT_EnableFlag |= NNS_G2D_SRTFLAG_SCALE;
        srtCtrl->data.srtData.scale.x = sx;
        srtCtrl->data.srtData.scale.y = sy;
    }
}

void NNSi_G2dSrtcInitControl(NNSG2dSRTControl *srtCtrl, u32 type) {
    srtCtrl->type = type;
    NNSi_G2dSrtcSetInitialValue(srtCtrl);
}

// No scale, rotation or translation set, at a scale of 1
void NNSi_G2dSrtcSetInitialValue(NNSG2dSRTControl *srtCtrl) {
    MI_CpuFill16(&srtCtrl->data, 0, sizeof(srtCtrl->data));
    srtCtrl->data.srtData.scale.x = FX32_ONE;
    srtCtrl->data.srtData.scale.y = FX32_ONE;
}
