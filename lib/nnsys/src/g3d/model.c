#include "nnsys/g3d.h"

// NitroSystem's G3D model setters (model.c): they change the polygon attribute that a material of a model resource
// draws with, which the GPU takes from bits of it (GX_PACK_POLYGONATTR_PARAM): the lights in the low four, the
// polygon mode, culling, fog, the alpha and the polygon ID. None of them checks that the material exists. The
// functions have NitroSystem's names, and g3d.h notes swan's

#define POLYGON_ATTR_LIGHT_MASK 0x0000000f
#define POLYGON_ATTR_MODE_MASK 0x00000030
#define POLYGON_ATTR_CULL_MASK 0x000000c0
#define POLYGON_ATTR_ALPHA_MASK 0x001f0000
#define POLYGON_ATTR_ID_MASK 0x3f000000

void NNS_G3dMdlSetMdlLightEnableFlag(NNSG3dResMdl *mdl, u32 matID, u32 lights) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matID);

    matData->polyAttr &= ~POLYGON_ATTR_LIGHT_MASK;
    matData->polyAttr |= lights;
}

void NNS_G3dMdlSetMdlPolygonMode(NNSG3dResMdl *mdl, u32 matID, u32 mode) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matID);

    matData->polyAttr &= ~POLYGON_ATTR_MODE_MASK;
    matData->polyAttr |= mode << REG_G3_POLYGON_ATTR_PM_SHIFT;
}

void NNS_G3dMdlSetMdlCullMode(NNSG3dResMdl *mdl, u32 matID, u32 cull) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matID);

    matData->polyAttr &= ~POLYGON_ATTR_CULL_MASK;
    matData->polyAttr |= cull << REG_G3_POLYGON_ATTR_BK_SHIFT;
}

void NNS_G3dMdlSetMdlPolygonID(NNSG3dResMdl *mdl, u32 matID, u32 polyID) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matID);

    matData->polyAttr &= ~POLYGON_ATTR_ID_MASK;
    matData->polyAttr |= polyID << REG_G3_POLYGON_ATTR_ID_SHIFT;
}

void NNS_G3dMdlSetMdlAlpha(NNSG3dResMdl *mdl, u32 matID, u32 alpha) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matID);

    matData->polyAttr &= ~POLYGON_ATTR_ALPHA_MASK;
    matData->polyAttr |= alpha << REG_G3_POLYGON_ATTR_ALPHA_SHIFT;
}

void NNS_G3dMdlSetMdlFogEnableFlag(NNSG3dResMdl *mdl, int matID, BOOL enable) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matID);

    if (enable) {
        matData->polyAttr |= GX_POLYGON_ATTR_MISC_FOG;
    } else {
        matData->polyAttr &= ~GX_POLYGON_ATTR_MISC_FOG;
    }
}

u32 NNS_G3dMdlGetMdlAlpha(const NNSG3dResMdl *mdl, u32 matID) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matID);

    return (matData->polyAttr & POLYGON_ATTR_ALPHA_MASK) >> REG_G3_POLYGON_ATTR_ALPHA_SHIFT;
}

// Sets the polygon ID of every material of a model
void NNS_G3dMdlSetMdlPolygonIDAll(NNSG3dResMdl *mdl, u32 polyID) {
    u32 i;

    for (i = 0; i < mdl->info.numMat; i++) {
        NNS_G3dMdlSetMdlPolygonID(mdl, i, polyID);
    }
}

// Sets the alpha of every material of a model
void NNS_G3dMdlSetMdlAlphaAll(NNSG3dResMdl *mdl, int alpha) {
    u32 i;

    for (i = 0; i < mdl->info.numMat; i++) {
        NNS_G3dMdlSetMdlAlpha(mdl, i, alpha);
    }
}

void NNS_G3dMdlSetMdlFogEnableFlagAll(NNSG3dResMdl *mdl, BOOL enable) {
    u32 i;

    for (i = 0; i < mdl->info.numMat; i++) {
        NNS_G3dMdlSetMdlFogEnableFlag(mdl, i, enable);
    }
}
