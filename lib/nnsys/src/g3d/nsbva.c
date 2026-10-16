#include "nnsys/g3d.h"

// NitroSystem's G3D visibility animation (NSBVA, the "BVA0" files): the function that fills in an animation object for
// one, and the function the object holds, which tells whether a node shows at a frame. The file is named after
// NitroSystem's nsbva.c. Both functions are the ones that anm.c's tables hold

// Fills in an animation object for a visibility animation (NitroSystem's NNS_G3dAnmObjInitNsBva). It applies the
// animation with NNS_G3dFuncAnmVisNsBva, and maps each node of the model to the data at its index, which is flagged as
// existing
void NNS_G3DAnimationInit_VA(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl) {
    u32 i;

    anmObj->funcAnm = NNS_G3dFuncAnmVisNsBva;
    anmObj->numMapData = mdl->info.numNode;
    anmObj->resAnm = anm;
    for (i = 0; i < anmObj->numMapData; i++) {
        anmObj->mapData[i] = NNS_G3D_ANMOBJ_MAPDATA_EXIST | i;
    }
}

// The function of an animation object of a visibility animation: whether a node shows at the object's frame, from the
// node's bit of the frame
void NNS_G3DAnimationApply_VA(NNSG3dVisAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx) {
    const NNSG3dResVisAnm *visAnm = anmObj->resAnm;
    u32 bit = (anmObj->frame >> FX32_SHIFT) * visAnm->numNode + dataIdx;

    result->isVisible = visAnm->visData[bit >> 5] & (1 << (bit & 31));
}
