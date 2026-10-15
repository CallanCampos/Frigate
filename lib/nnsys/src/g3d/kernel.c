#include "nnsys/g3d.h"

// NitroSystem's G3D kernel: the animation objects and the render objects they are bound to, then the textures and
// palettes of a texture block, and binding them to a model's materials. The file's name is a guess, since these are the
// core of NNS_G3d. The functions keep swan's names, and g3d.h gives NitroSystem's where the code shows them.
// NNS_G3DAnimationLink and NNS_G3DAnimationRegistTargets are static because only NNS_G3DAnimationBind calls them,
// like removeLink_ and the two bind helpers

static void NNS_G3DAnimationRegistTargets(u32 *hint, const NNSG3dAnmObj *anmObj);
static void NNS_G3DAnimationLink(NNSG3dAnmObj **head, NNSG3dAnmObj *anmObj);
static BOOL removeLink_(NNSG3dAnmObj **head, NNSG3dAnmObj *anmObj);
static void bindMdlTex_Internal_(NNSG3dResMat *mat, NNSG3dResDictTexToMatListData *list, const NNSG3dResTex *tex,
                                 const NNSG3dResDictTexData *texData);
static void bindMdlPltt_Internal_(NNSG3dResMat *mat, NNSG3dResDictPlttToMatListData *list, const NNSG3dResTex *tex,
                                  const NNSG3dResDictPlttData *plttData);

u32 NNS_G3DAnimationCalcAllocSize(const void *anm, const NNSG3dResMdl *mdl) {
    if (anm == NULL || mdl == NULL) {
        return 0;
    }

    switch (((const NNSG3dResAnmHeader *)anm)->category0) {
    case NNS_G3D_DATABLK_MAT_ANM:
        return NNS_G3D_ANMOBJ_SIZE(mdl->info.numMat);
    case NNS_G3D_DATABLK_JNT_ANM:
    case NNS_G3D_DATABLK_VIS_ANM:
        return NNS_G3D_ANMOBJ_SIZE(mdl->info.numNode);
    default:
        return 0;
    }
}

void NNS_G3DAnimationCreate(NNSG3dAnmObj *anmObj, void *anm, const NNSG3dResMdl *mdl, const NNSG3dResTex *tex) {
    const NNSG3dResAnmHeader *header = anm;
    u32 i;

    anmObj->frame = 0;
    anmObj->resAnm = anm;
    anmObj->next = NULL;
    anmObj->priority = 127;
    anmObj->ratio = FX32_ONE;
    anmObj->resTex = tex;
    anmObj->numMapData = 0;
    anmObj->funcAnm = NULL;

    for (i = 0; i < NNS_G3D_ANIMATION_INIT_HEADER_COUNT; i++) {
        if (header->category0 == NNS_G3D_ANIMATION_INIT_HEADERS[i].header.category0 &&
            header->category1 == NNS_G3D_ANIMATION_INIT_HEADERS[i].header.category1) {
            if (NNS_G3D_ANIMATION_INIT_HEADERS[i].init != NULL) {
                NNS_G3D_ANIMATION_INIT_HEADERS[i].init(anmObj, anm, mdl);
            }
            break;
        }
    }
}

void NNS_G3DModelAttachResource(NNSG3dRenderObj *obj, NNSG3dResMdl *mdl) {
    MI_CpuClear32(obj, sizeof(NNSG3dRenderObj));
    obj->funcBlendMat = NNS_G3dFuncBlendMat;
    obj->funcBlendJnt = NNS_G3dFuncBlendJnt;
    obj->funcBlendVis = NNS_G3dFuncBlendVis;
    obj->resMdl = mdl;
}

// Links the animation, and any that follow it, into the list ahead of the first object after the head that has no lower
// priority number
static void NNS_G3DAnimationLink(NNSG3dAnmObj **head, NNSG3dAnmObj *anmObj) {
    NNSG3dAnmObj *prev = *head;
    NNSG3dAnmObj *next;
    NNSG3dAnmObj *last;

    if (prev == NULL) {
        *head = anmObj;
        return;
    }

    next = prev->next;
    // BUG: the head is compared with the new object only when it is the only one, so with two or more, a new object
    // of a lower priority number than the head's goes behind it
#ifdef BUGFIX
    if (prev->priority > anmObj->priority) {
        last = anmObj;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = prev;
        *head = anmObj;
        return;
    }
#else
    if (next == NULL) {
        if (prev->priority > anmObj->priority) {
            last = anmObj;
            while (last->next != NULL) {
                last = last->next;
            }
            last->next = prev;
            *head = anmObj;
        } else {
            prev->next = anmObj;
        }
        return;
    }
#endif

    while (next != NULL) {
        if (next->priority >= anmObj->priority) {
            last = anmObj;
            while (last->next != NULL) {
                last = last->next;
            }
            prev->next = anmObj;
            last->next = next;
            return;
        }
        prev = next;
        next = next->next;
    }
    prev->next = anmObj;
}

// Sets the bit of every material or node that an animation of the list has data for
static void NNS_G3DAnimationRegistTargets(u32 *hint, const NNSG3dAnmObj *anmObj) {
    while (anmObj != NULL) {
        int i;

        for (i = 0; i < anmObj->numMapData; i++) {
            if (anmObj->mapData[i] & NNS_G3D_ANMOBJ_MAPDATA_EXIST) {
                hint[i >> 5] |= 1 << (i & 0x1f);
            }
        }
        anmObj = anmObj->next;
    }
}

void NNS_G3DAnimationBind(NNSG3dRenderObj *obj, NNSG3dAnmObj *anmObj) {
    if (anmObj != NULL && anmObj->resAnm != NULL) {
        switch (((const NNSG3dResAnmHeader *)anmObj->resAnm)->category0) {
        case NNS_G3D_DATABLK_MAT_ANM:
            NNS_G3DAnimationRegistTargets(obj->hintMatAnmExist, anmObj);
            NNS_G3DAnimationLink(&obj->anmMat, anmObj);
            break;
        case NNS_G3D_DATABLK_JNT_ANM:
            NNS_G3DAnimationRegistTargets(obj->hintJntAnmExist, anmObj);
            NNS_G3DAnimationLink(&obj->anmJnt, anmObj);
            break;
        case NNS_G3D_DATABLK_VIS_ANM:
            NNS_G3DAnimationRegistTargets(obj->hintVisAnmExist, anmObj);
            NNS_G3DAnimationLink(&obj->anmVis, anmObj);
            break;
        }
    }
}

static BOOL removeLink_(NNSG3dAnmObj **head, NNSG3dAnmObj *anmObj) {
    NNSG3dAnmObj *next;
    NNSG3dAnmObj *prev = *head;

    if (prev == NULL) {
        return FALSE;
    }

    if (prev == anmObj) {
        *head = prev->next;
        anmObj->next = NULL;
        return TRUE;
    }

    next = prev->next;
    while (next != NULL) {
        if (next == anmObj) {
            prev->next = next->next;
            next->next = NULL;
            return TRUE;
        }
        prev = next;
        next = next->next;
    }
    return FALSE;
}

void NNS_G3dRenderObjRemoveAnmObj(NNSG3dRenderObj *obj, NNSG3dAnmObj *anmObj) {
    if (removeLink_(&obj->anmMat, anmObj) || removeLink_(&obj->anmJnt, anmObj) || removeLink_(&obj->anmVis, anmObj)) {
        obj->flag |= NNS_G3D_RENDEROBJ_FLAG_HINT_OBSOLETE;
    }
}

u32 NNS_G3DResTexGetTexDataSize(const NNSG3dResTex *tex) {
    if (tex != NULL) {
        return tex->texInfo.sizeTex << 3;
    }
    return 0;
}

u32 NNS_G3dTexGetRequiredSize4x4(const NNSG3dResTex *tex) {
    if (tex != NULL) {
        return tex->tex4x4Info.sizeTex << 3;
    }
    return 0;
}

void NNS_G3DResTexSetGfdKeys(NNSG3dResTex *tex, NNSGfdTexKey key, NNSGfdTexKey key4x4) {
    if (key != 0) {
        tex->texInfo.vramKey = key;
    }
    if (key4x4 != 0) {
        tex->tex4x4Info.vramKey = key4x4;
    }
}

void NNS_G3DResTexUploadTex(NNSG3dResTex *tex, BOOL exec_begin_end) {
    u32 size;
    u32 size4x4;

    if (exec_begin_end) {
        gfxBeginTextureUpload();
    }

    size = tex->texInfo.sizeTex << 3;
    if (size != 0) {
        gfxUploadTexture((u8 *)tex + tex->texInfo.ofsTex, NNS_GfdGetTexKeyAddr(tex->texInfo.vramKey), size);
        tex->texInfo.flag |= NNS_G3D_RESTEX_LOADED;
    }

    size4x4 = tex->tex4x4Info.sizeTex << 3;
    if (size4x4 != 0) {
        u32 addr = NNS_GfdGetTexKeyAddr(tex->tex4x4Info.vramKey);
        u32 ofsTexPlttIdx = tex->tex4x4Info.ofsTexPlttIdx;

        gfxUploadTexture((u8 *)tex + tex->tex4x4Info.ofsTex, addr, size4x4);
        gfxUploadTexture((u8 *)tex + ofsTexPlttIdx, 0x20000 + ((addr & 0x1ffff) >> 1) + ((addr & 0x40000) >> 2),
                         size4x4 >> 1);
        tex->tex4x4Info.flag |= NNS_G3D_RESTEX_LOADED;
    }

    if (exec_begin_end) {
        gfxEndTextureUpload();
    }
}

void NNS_G3dTexReleaseTexKey(NNSG3dResTex *tex, NNSGfdTexKey *key, NNSGfdTexKey *key4x4) {
    if (key != NULL) {
        tex->texInfo.flag &= ~NNS_G3D_RESTEX_LOADED;
        *key = tex->texInfo.vramKey;
        tex->texInfo.vramKey = 0;
    }
    if (key4x4 != NULL) {
        tex->tex4x4Info.flag &= ~NNS_G3D_RESTEX_LOADED;
        *key4x4 = tex->tex4x4Info.vramKey;
        tex->tex4x4Info.vramKey = 0;
    }
}

u32 NNS_G3DResTexGetPaletteDataSize(const NNSG3dResTex *tex) {
    if (tex != NULL) {
        return tex->plttInfo.sizePltt << 3;
    }
    return 0;
}

void NNS_G3DResTexSetPltGfdKey(NNSG3dResTex *tex, NNSGfdPlttKey plttKey) {
    tex->plttInfo.vramKey = plttKey;
}

void NNS_G3DResTexUploadPlt(NNSG3dResTex *tex, BOOL exec_begin_end) {
    if (exec_begin_end) {
        gfxBeginPaletteUpload();
    }

    gfxUploadPalette((u8 *)tex + tex->plttInfo.ofsPlttData, NNS_GfdGetPlttKeyAddr(tex->plttInfo.vramKey),
                     tex->plttInfo.sizePltt << 3);
    tex->plttInfo.flag |= NNS_G3D_RESPLTT_LOADED;

    if (exec_begin_end) {
        gfxEndPaletteUpload();
    }
}

NNSGfdPlttKey NNS_G3dPlttReleasePlttKey(NNSG3dResTex *tex) {
    NNSGfdPlttKey key;

    tex->plttInfo.flag &= ~NNS_G3D_RESPLTT_LOADED;
    key = tex->plttInfo.vramKey;
    tex->plttInfo.vramKey = 0;
    return key;
}

// Adds the texture's place in VRAM to the texture parameters of the materials of the list, and scales their texture
// coordinates by how much the texture differs from the size the material was made for
static void bindMdlTex_Internal_(NNSG3dResMat *mat, NNSG3dResDictTexToMatListData *list, const NNSG3dResTex *tex,
                                 const NNSG3dResDictTexData *texData) {
    const u8 *matIds = (u8 *)mat + list->offset;
    u32 vramOffset;
    u32 i;

    if ((texData->texImageParam & (7 << REG_G3_TEXIMAGE_PARAM_TEXFMT_SHIFT)) !=
        (GX_TEXFMT_COMP4x4 << REG_G3_TEXIMAGE_PARAM_TEXFMT_SHIFT)) {
        vramOffset = NNS_GfdGetTexKeyAddr(tex->texInfo.vramKey) >> NNS_GFD_KEY_ADDR_SHIFT;
    } else {
        vramOffset = NNS_GfdGetTexKeyAddr(tex->tex4x4Info.vramKey) >> NNS_GFD_KEY_ADDR_SHIFT;
    }

    for (i = 0; i < list->numIdx; i++) {
        NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matIds[i]);
        u32 width;
        u32 height;

        matData->texImageParam |= texData->texImageParam + vramOffset;

        width = texData->extraParam & NNS_G3D_RESDICTTEX_EXTRA_WIDTH_MASK;
        height = (texData->extraParam & NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_MASK) >> NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_SHIFT;

        if (width != matData->origWidth) {
            matData->magW = FX_Div(width << FX32_SHIFT, matData->origWidth << FX32_SHIFT);
        } else {
            matData->magW = FX32_ONE;
        }
        if (height != matData->origHeight) {
            matData->magH = FX_Div(height << FX32_SHIFT, matData->origHeight << FX32_SHIFT);
        } else {
            matData->magH = FX32_ONE;
        }
    }
    list->flag |= NNS_G3D_MATLIST_BOUND;
}

BOOL NNS_G3dBindMdlTex(NNSG3dResMdl *mdl, const NNSG3dResTex *tex) {
    BOOL result = TRUE;
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    const NNSG3dResDict *texToMatDict = (const NNSG3dResDict *)((u8 *)mat + mat->ofsDictTexToMatList);
    u32 i;

    for (i = 0; i < texToMatDict->numEntry; i++) {
        const NNSG3dResName *name = NNS_G3dGetResNameByIdx(texToMatDict, i);
        const NNSG3dResDictTexData *texData = tex != NULL ? NNS_G3DFind(&tex->dict, name) : NULL;

        if (texData != NULL) {
            NNSG3dResDictTexToMatListData *list = NNS_G3dGetResDataByIdx(texToMatDict, i);

            if (!(list->flag & NNS_G3D_MATLIST_BOUND)) {
                bindMdlTex_Internal_(mat, list, tex, texData);
            }
        } else {
            result = FALSE;
        }
    }
    return result;
}

// Sets the palette base of the materials of the list to where the palette is in VRAM
static void bindMdlPltt_Internal_(NNSG3dResMat *mat, NNSG3dResDictPlttToMatListData *list, const NNSG3dResTex *tex,
                                  const NNSG3dResDictPlttData *plttData) {
    const u8 *matIds = (u8 *)mat + list->offset;
    u16 offset = plttData->offset;
    u16 vramOffset = NNS_GfdGetPlttKeyAddr(tex->plttInfo.vramKey) >> NNS_GFD_KEY_ADDR_SHIFT;
    u32 i;

    if (!(plttData->flag & NNS_G3D_RESDICTPLTT_USEPLTT4)) {
        offset >>= 1;
        vramOffset >>= 1;
    }

    for (i = 0; i < list->numIdx; i++) {
        NNSG3dResMatData *matData = NNS_G3dGetMatDataByIdx(mat, matIds[i]);

        matData->texPlttBase = offset + vramOffset;
    }
    list->flag |= NNS_G3D_MATLIST_BOUND;
}

BOOL NNS_G3dBindMdlPltt(NNSG3dResMdl *mdl, const NNSG3dResTex *tex) {
    BOOL result = TRUE;
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    const NNSG3dResDict *plttToMatDict = (const NNSG3dResDict *)((u8 *)mat + mat->ofsDictPlttToMatList);
    u32 i;

    for (i = 0; i < plttToMatDict->numEntry; i++) {
        const NNSG3dResName *name = NNS_G3dGetResNameByIdx(plttToMatDict, i);
        const NNSG3dResDictPlttData *plttData;

        if (tex != NULL && tex->plttInfo.ofsDict != 0) {
            plttData = NNS_G3DFind((const NNSG3dResDict *)((u8 *)tex + tex->plttInfo.ofsDict), name);
        } else {
            plttData = NULL;
        }

        if (plttData != NULL) {
            NNSG3dResDictPlttToMatListData *list = NNS_G3dGetResDataByIdx(plttToMatDict, i);

            if (!(list->flag & NNS_G3D_MATLIST_BOUND)) {
                bindMdlPltt_Internal_(mat, list, tex, plttData);
            }
        } else {
            result = FALSE;
        }
    }
    return result;
}

BOOL NNS_G3dForceBindMdlPltt(NNSG3dResMdl *mdl, const NNSG3dResTex *tex, u32 plttToMatListIdx, u32 plttNameIdx) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    const NNSG3dResDict *plttToMatDict = (const NNSG3dResDict *)((u8 *)mat + mat->ofsDictPlttToMatList);
    const NNSG3dResDictPlttData *plttData;
    NNSG3dResDictPlttToMatListData *list;

    if (tex != NULL && tex->plttInfo.ofsDict != 0) {
        plttData = NNS_G3dGetResDataByIdx((const NNSG3dResDict *)((u8 *)tex + tex->plttInfo.ofsDict), plttNameIdx);
    } else {
        plttData = NULL;
    }

    list = NNS_G3dGetResDataByIdx(plttToMatDict, plttToMatListIdx);
    if (list != NULL && !(list->flag & NNS_G3D_MATLIST_BOUND)) {
        bindMdlPltt_Internal_(mat, list, tex, plttData);
        return TRUE;
    }
    return FALSE;
}

void NNS_G3dReleaseMdlPltt(NNSG3dResMdl *mdl) {
    NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    const NNSG3dResDict *plttToMatDict = (const NNSG3dResDict *)((u8 *)mat + mat->ofsDictPlttToMatList);
    u32 i;

    for (i = 0; i < plttToMatDict->numEntry; i++) {
        NNSG3dResDictPlttToMatListData *list = NNS_G3dGetResDataByIdx(plttToMatDict, i);

        if (list->flag & NNS_G3D_MATLIST_BOUND) {
            list->flag &= ~NNS_G3D_MATLIST_BOUND;
        }
    }
}
