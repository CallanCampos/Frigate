#include "nnsys/g3d.h"

// NitroSystem's G3D texture pattern animation (NSBTP, the "BTP0" files): the function that fills in an animation object
// for one, the function the object holds, which changes a material's texture and palette at a frame, and the two that
// set them in a material's result. The file is named after NitroSystem's nsbtp.c. The two global functions are the ones
// that anm.c's tables hold; the others are static because only this file calls them. The names are NitroSystem's, as
// ntrtwl's reconstruction of NitroSystem has them, misspelling included, except TEXIMAGE_PARAM_KEEP_MASK, which is ours

static void SetTexParamaters_(const NNSG3dResTex *tex, const NNSG3dResName *texName, NNSG3dMatAnmResult *result);
static void SetPlttParamaters_(const NNSG3dResTex *tex, const NNSG3dResName *plttName, NNSG3dMatAnmResult *result);

// The mask of the texture parameters that the animation leaves to the material: its repeat and flip flags and its
// texture coordinate generation mode
#define TEXIMAGE_PARAM_KEEP_MASK 0xc00f0000

// Fills in an animation object for a texture pattern animation. It applies the animation with
// NNS_G3dFuncAnmMatNsBtpDefault, and maps each material of the model to the data of the animation's material of the
// same name, which is flagged as existing and holds the data's index
void NNSi_G3dAnmObjInitNsBtp(NNSG3dAnmObj *anmObj, void *anm, const NNSG3dResMdl *mdl) {
    const NNSG3dResTexPatAnm *patAnm = anm;
    const NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    u32 i;

    anmObj->funcAnm = (void *)NNS_G3dFuncAnmMatNsBtpDefault;
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
// the result keeps the material's repeat, flip and texture coordinate generation. The texture's size is stored as the
// result's original size before the magnification compares the two, so the magnification always comes out as one.
// kernel.c's binding instead compares the texture with the material's own original size. The texture matrix then works
// in the new texture's size here, which agrees with kernel.c's for the translation, and for the rest of the matrix
// when the pattern's textures have the material's size. A texture of another size is the only case where the two
// differ, and nothing shows whether NitroSystem supports it, so this is not marked as a bug
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

    result->origWidth = texData->extraParam & NNS_G3D_TEXIMAGE_PARAMEX_ORIGW_MASK;
    result->origHeight =
        (texData->extraParam & NNS_G3D_TEXIMAGE_PARAMEX_ORIGH_MASK) >> NNS_G3D_TEXIMAGE_PARAMEX_ORIGH_SHIFT;

    width = texData->extraParam & NNS_G3D_TEXIMAGE_PARAMEX_ORIGW_MASK;
    height = (texData->extraParam & NNS_G3D_TEXIMAGE_PARAMEX_ORIGH_MASK) >> NNS_G3D_TEXIMAGE_PARAMEX_ORIGH_SHIFT;
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
void NNSi_G3dAnmCalcNsBtp(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx) {
    const NNSG3dResTexPatAnm *patAnm = anmObj->resAnm;
    const NNSG3dResTexPatAnmFV *fv = NNSi_G3dGetTexPatAnmFV(patAnm, (u16)dataIdx, (u16)(anmObj->frame >> FX32_SHIFT));

    SetTexParamaters_(anmObj->resTex, NNSi_G3dGetTexPatAnmTexNameByIdx(patAnm, fv->idTex), result);
    if (fv->idPltt != 0xff) {
        SetPlttParamaters_(anmObj->resTex, NNSi_G3dGetTexPatAnmPlttNameByIdx(patAnm, fv->idPltt), result);
    }
}
