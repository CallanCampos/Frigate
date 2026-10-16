#include "nnsys/g3d.h"

// NitroSystem's G3D texture pattern animation (NSBTP, the "BTP0" files): the function that fills in an animation
// object for one, the function the object holds, which changes a material's texture and palette at a frame, and the
// two that set them in a material's result. The file is named after NitroSystem's nsbtp.c. The two global functions
// are the ones that anm.c's tables hold; the others are static because only this file calls them

static void SetTexParamaters_(const NNSG3dResTex *tex, const NNSG3dResName *texName, NNSG3dMatAnmResult *result);
static void SetPlttParamaters_(const NNSG3dResTex *tex, const NNSG3dResName *plttName, NNSG3dMatAnmResult *result);

// The mask of the texture parameters that the animation leaves to the material: its repeat and flip flags and its
// texture coordinate generation mode
#define TEXIMAGE_PARAM_KEEP_MASK 0xc00f0000

// Fills in an animation object for a texture pattern animation (NitroSystem's NNS_G3dAnmObjInitNsBtp). It applies the
// animation with the variable that swan names NNS_G3dFuncAnmMatNsBta, which seems swapped with NNS_G3dFuncAnmMatNsBtp
// (see nsbta.c). It maps each material of the model to the data of the animation's material of the same name, which is
// flagged as existing and holds the data's index
void NNS_G3DAnimationInit_TP(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl) {
    const NNSG3dResTexPatAnm *patAnm = anm;
    const NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    u32 i;

    anmObj->funcAnm = NNS_G3dFuncAnmMatNsBta;
    anmObj->numMapData = mdl->info.numMat;
    anmObj->resAnm = anm;
    MI_CpuClear16(anmObj->mapData, anmObj->numMapData * sizeof(u16));
    for (i = 0; i < patAnm->dict.numEntry; i++) {
        const NNSG3dResName *name = NNS_G3dGetResNameByIdx(&patAnm->dict, i);
        int idx = mat != NULL ? NNS_G3dGetResDictIdxByName(&mat->dict, name) : -1;

        if (idx >= 0) {
            anmObj->mapData[idx] = NNS_G3D_ANMOBJ_MAPDATA_EXIST | i;
        }
    }
}

// Sets the texture of a material's result to the one of the name: its parameters, with its place in VRAM added, while
// the result keeps the material's repeat, flip and texture coordinate generation. The texture's size becomes the
// result's original size, so the magnification that follows, which compares the texture's size with the original
// size as the binding of a texture to a material does in kernel.c, always comes out as one
static void SetTexParamaters_(const NNSG3dResTex *tex, const NNSG3dResName *texName, NNSG3dMatAnmResult *result) {
    const NNSG3dResDictTexData *texData = tex != NULL ? NNS_G3dGetResDataByName(&tex->dict, texName) : NULL;
    u32 vramOffset;
    u32 width;
    u32 height;

    if ((texData->texImageParam & (7 << REG_G3_TEXIMAGE_PARAM_TEXFMT_SHIFT)) !=
        (GX_TEXFMT_COMP4x4 << REG_G3_TEXIMAGE_PARAM_TEXFMT_SHIFT)) {
        vramOffset = NNS_GfdGetTexKeyAddr(tex->texInfo.vramKey) >> NNS_GFD_KEY_ADDR_SHIFT;
    } else {
        vramOffset = NNS_GfdGetTexKeyAddr(tex->tex4x4Info.vramKey) >> NNS_GFD_KEY_ADDR_SHIFT;
    }

    result->prmTexImage &= TEXIMAGE_PARAM_KEEP_MASK;
    result->prmTexImage |= texData->texImageParam + vramOffset;

    result->origWidth = texData->extraParam & NNS_G3D_RESDICTTEX_EXTRA_WIDTH_MASK;
    result->origHeight =
        (texData->extraParam & NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_MASK) >> NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_SHIFT;

    width = texData->extraParam & NNS_G3D_RESDICTTEX_EXTRA_WIDTH_MASK;
    height = (texData->extraParam & NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_MASK) >> NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_SHIFT;
    result->magW = width != result->origWidth ? FX_Div(width << FX32_SHIFT, result->origWidth << FX32_SHIFT) : FX32_ONE;
    result->magH =
        height != result->origHeight ? FX_Div(height << FX32_SHIFT, result->origHeight << FX32_SHIFT) : FX32_ONE;
}

// Sets the palette of a material's result to the one of the name: where it is in VRAM
static void SetPlttParamaters_(const NNSG3dResTex *tex, const NNSG3dResName *plttName, NNSG3dMatAnmResult *result) {
    const NNSG3dResDictPlttData *plttData =
        tex != NULL && tex->plttInfo.ofsDict != 0
            ? NNS_G3dGetResDataByName((const NNSG3dResDict *)((const u8 *)tex + tex->plttInfo.ofsDict), plttName)
            : NULL;
    u16 offset = plttData->offset;
    u16 vramOffset = NNS_GfdGetPlttKeyAddr(tex->plttInfo.vramKey) >> NNS_GFD_KEY_ADDR_SHIFT;

    if (!(plttData->flag & NNS_G3D_RESDICTPLTT_USEPLTT4)) {
        offset >>= 1;
        vramOffset >>= 1;
    }
    result->prmTexPltt = offset + vramOffset;
}

// The function of an animation object of a texture pattern animation: the material's texture, and its palette if the
// animation has one, at the object's frame
void NNS_G3DAnimationApply_TP(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx) {
    const NNSG3dResTexPatAnm *patAnm = anmObj->resAnm;
    const NNSG3dResTexPatAnmFV *fv = NNSi_G3dGetTexPatAnmFV(patAnm, (u16)dataIdx, (u16)(anmObj->frame >> FX32_SHIFT));

    SetTexParamaters_(anmObj->resTex, NNS_G3dGetTexPatAnmTexNameByIdx(patAnm, fv->idTex), result);
    if (fv->idPltt != 0xff) {
        SetPlttParamaters_(anmObj->resTex, NNS_G3dGetTexPatAnmPlttNameByIdx(patAnm, fv->idPltt), result);
    }
}
