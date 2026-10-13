#include "nnsys/g3d.h"
#include "nnsys/fnd.h"

NNSG3dRenderObj *NNS_G3dAllocRenderObj(NNSFndAllocator *allocator) {
    return NNS_FndAllocFromAllocator(allocator, sizeof(NNSG3dRenderObj));
}

void NNS_G3dFreeRenderObj(NNSFndAllocator *allocator, NNSG3dRenderObj *obj) {
    NNS_FndFreeToAllocator(allocator, obj);
}

NNSG3dAnmObj *NNS_G3DAnimationAlloc(NNSFndAllocator *allocator, const void *anm, const NNSG3dResMdl *mdl) {
    u32 size = NNS_G3DAnimationCalcAllocSize(anm, mdl);
    return NNS_FndAllocFromAllocator(allocator, size);
}

void NNS_G3dFreeAnmObj(NNSFndAllocator *allocator, NNSG3dAnmObj *anmObj) {
    NNS_FndFreeToAllocator(allocator, anmObj);
}
