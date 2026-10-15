#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_RendererCore.c: the renderer core, which draws a cell's OAMs to one surface with the current
// matrix. A 2D surface gets OAMs registered through the core's functions, transformed by an affine parameter or
// flipped; the 3D surface gets each OAM drawn as a quad (g2d_OamSoftwareSpriteDraw.c). The function names are the
// SDK's, swan's for NNS_G2dInitRndCore and NNS_G2dRndCoreBeginRendering; the file name is a guess from them, as the ROM
// has no string for it, and the statics are named here

static NNSG2dRndCoreInstance *pCurrentInstance_;

static const MtxFx32 mtxIdentity_ = { FX32_ONE, 0, 0, FX32_ONE, 0, 0 };

// An OAM's position, with x and y sign extended from their 9 and 8 bits
static inline s16 GetOamPosX_(const GXOamAttr *oam) {
    s16 x = oam->x;

    if (x > 0xff) {
        x |= 0xff00;
    }
    return x;
}

static inline s16 GetOamPosY_(const GXOamAttr *oam) {
    s16 y = oam->y;

    if (y > 0x7f) {
        y |= 0xff00;
    }
    return y;
}

// The same, read from the attributes' bits
static inline s16 GetOamAttrPosX_(const GXOamAttr *oam) {
    s16 x = (s16)((oam->attr01 & GX_OAM_ATTR01_X_MASK) >> GX_OAM_ATTR01_X_SHIFT);

    if (x > 0xff) {
        x |= 0xff00;
    }
    return x;
}

static inline s16 GetOamAttrPosY_(const GXOamAttr *oam) {
    s16 y = (s16)((oam->attr01 & GX_OAM_ATTR01_Y_MASK) >> GX_OAM_ATTR01_Y_SHIFT);

    if (y > 0x7f) {
        y |= 0xff00;
    }
    return y;
}

// An OAM's position, mirrored with its size when flip is set
static inline u16 GetFlippedOamPosX_(const GXOamAttr *oam, const GXOamShape *shape, BOOL flip) {
    if (flip) {
        return -(GetOamPosX_(oam) + NNS_G2dGetOamSizeX(shape));
    }
    return GetOamPosX_(oam);
}

static inline u16 GetFlippedOamPosY_(const GXOamAttr *oam, const GXOamShape *shape, BOOL flip) {
    if (flip) {
        return -(GetOamPosY_(oam) + NNS_G2dGetOamSizeY(shape));
    }
    return GetOamPosY_(oam);
}

// v * m for a 2x2 matrix; out may be v
static inline void MulVecMtx22_(const NNSG2dFVec2 *v, const MtxFx22 *m, NNSG2dFVec2 *out) {
    NNSG2dFVec2 tmp;
    NNSG2dFVec2 *p;

    if (out == v) {
        p = &tmp;
    } else {
        p = out;
    }
    p->x = (fx32)(((fx64)m->_10 * v->y + FX32_ONE + (fx64)m->_00 * v->x) >> FX32_SHIFT);
    p->y = (fx32)(((fx64)m->_11 * v->y + FX32_ONE + (fx64)m->_01 * v->x) >> FX32_SHIFT);
    if (p == &tmp) {
        *out = tmp;
    }
}

// Moves an affine OAM to where the matrix puts it: an affine OAM turns about its center, so its corner goes where the
// matrix takes the center, less half its size. The affine mode may be overwritten, and the base translation is added
static void SetAffinedOamPosition_(const MtxFx22 *mtx, GXOamAttr *oam, const NNSG2dFVec2 *baseTrans) {
    GXOamShape shape;
    NNSG2dFVec2 pos;
    fx32 halfW;
    fx32 halfH;
    int w;
    int h;
    u32 effect;

    pos.x = GetOamAttrPosX_(oam) << FX32_SHIFT;
    pos.y = GetOamAttrPosY_(oam) << FX32_SHIFT;
    if (G2_GetOBJEffect(oam) == GX_OAM_EFFECT_AFFINE_DOUBLE) {
        shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
        pos.x += NNS_G2dGetOamSizeX(&shape) * FX32_HALF;
        pos.y += NNS_G2dGetOamSizeY(&shape) * FX32_HALF;
    }
    MulVecMtx22_(&pos, mtx, &pos);

    if (pCurrentInstance_->affineOverwriteMode != NNS_G2D_RND_AFFINE_OVERWRITE_NONE) {
        G2_SetOBJEffect(oam,
                        pCurrentInstance_->affineOverwriteMode == NNS_G2D_RND_AFFINE_OVERWRITE_DOUBLE
                            ? GX_OAM_EFFECT_AFFINE_DOUBLE
                            : GX_OAM_EFFECT_AFFINE,
                        0);
    }

    effect = G2_GetOBJEffect(oam);
    shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
    w = NNS_G2dGetOamSizeX(&shape) >> 1;
    h = NNS_G2dGetOamSizeY(&shape) >> 1;
    halfW = w << FX32_SHIFT;
    halfH = h << FX32_SHIFT;
    pos.x += -halfW + mtx->_00 * w + mtx->_10 * h;
    pos.y += -halfH + mtx->_01 * w + mtx->_11 * h;
    if (effect == GX_OAM_EFFECT_AFFINE_DOUBLE) {
        pos.x -= halfW;
        pos.y -= halfH;
    }

    pos.x += baseTrans->x;
    pos.y += baseTrans->y;
    G2_SetOBJPosition(oam, pos.x >> FX32_SHIFT, pos.y >> FX32_SHIFT);
}

// The current matrix, or the identity without one
static inline const MtxFx32 *GetCurrentMtx_(void) {
    if (pCurrentInstance_->pCurrentMtx == NULL) {
        return &mtxIdentity_;
    }
    return pCurrentInstance_->pCurrentMtx;
}

// The affine parameter of the matrix cache for the OAM's flip on the surface, registered the first time
static inline u16 GetAffineIndex_(const NNSG2dRenderSurface *surface, const GXOamAttr *oam) {
    u32 flip = oam->flipV << 1 | oam->flipH;
    NNSG2dRndCore2DMtxCache *cache = (NNSG2dRndCore2DMtxCache *)pCurrentInstance_->pCurrentMxtCacheFor2D;
    u16 affineIdx = cache->affineIndex[flip][surface->type - NNS_G2D_SURFACETYPE_MAIN2D];

    if (affineIdx == NNS_G2D_OAM_AFFINE_IDX_NOT_CACHED) {
        if (flip == 0) {
            affineIdx = pCurrentInstance_->pFuncOamAffineRegister(&cache->m22);
        } else {
            MtxFx22 flipped = cache->m22;

            if (flip & NNS_G2D_RENDERERFLIP_H) {
                flipped._00 = -flipped._00;
                flipped._01 = -flipped._01;
            }
            if (flip & NNS_G2D_RENDERERFLIP_V) {
                flipped._10 = -flipped._10;
                flipped._11 = -flipped._11;
            }
            affineIdx = pCurrentInstance_->pFuncOamAffineRegister(&flipped);
        }
        cache->affineIndex[flip][surface->type - NNS_G2D_SURFACETYPE_MAIN2D] = affineIdx;
    }
    return affineIdx;
}

// Registers each of a cell's OAMs to a 2D surface, moved by the current matrix's translation and with its characters
// moved by the image's offset. With a matrix cache, each OAM gets the cached matrix's affine parameters, registered
// once per flip and surface; otherwise the renderer's flip is applied to it
static void DrawCellToSurface2D_(const NNSG2dRenderSurface *surface, const NNSG2dCellData *cell) {
    u32 baseCharOffset;
    u16 i;
    GXOamAttr *oam;
    NNSG2dFVec2 baseTrans;
    const MtxFx32 *mtx = GetCurrentMtx_();
    BOOL affined;

    baseTrans.x = mtx->_20;
    baseTrans.y = mtx->_21;
    baseTrans.x -= surface->viewRect.posTopLeft.x;
    baseTrans.y -= surface->viewRect.posTopLeft.y;

    baseCharOffset = pCurrentInstance_->base2DCharOffset;
    affined = (pCurrentInstance_->flipFlag == NNS_G2D_RENDERERFLIP_NONE && pCurrentInstance_->pCurrentMxtCacheFor2D != NULL)
                  ? TRUE
                  : FALSE;

    oam = &pCurrentInstance_->currentOam;
    for (i = 0; i < cell->numOAMAttrs; i++) {
        u16 affineIdx;

        pCurrentInstance_->bDrawEnable = TRUE;
        NNS_G2dCopyCellAsOamAttr(cell, i, oam);
        if (surface->pBeforeDrawOamBackFuncCore != NULL) {
            surface->pBeforeDrawOamBackFuncCore(pCurrentInstance_, cell, i);
        }
        if (!pCurrentInstance_->bDrawEnable) {
            continue;
        }

        oam->charNo += baseCharOffset;
        if (affined) {
            MtxFx22 m22;

            affineIdx = GetAffineIndex_(surface, oam);
            m22._00 = mtx->_00;
            m22._01 = mtx->_01;
            m22._10 = mtx->_10;
            m22._11 = mtx->_11;
            SetAffinedOamPosition_(&m22, oam, &baseTrans);
        } else {
            affineIdx = NNS_G2D_OAM_AFFINE_IDX_NONE;
            if (pCurrentInstance_->flipFlag != NNS_G2D_RENDERERFLIP_NONE) {
                GXOamShape shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
                u32 oamFlip;
                u32 flipH;
                u32 flipV;

                oamFlip = oam->flipH;
                flipH = NNS_G2dIsRndCoreFlipH(pCurrentInstance_);
                flipH ^= oamFlip;
                oamFlip = oam->flipV;
                flipV = NNS_G2dIsRndCoreFlipV(pCurrentInstance_);
                flipV ^= oamFlip;

                oam->attr01 = (oam->attr01 & ~GX_OAM_ATTR01_HF_MASK) | (flipH << GX_OAM_ATTR01_HF_SHIFT);
                oam->attr01 = (oam->attr01 & ~GX_OAM_ATTR01_VF_MASK) | (flipV << GX_OAM_ATTR01_VF_SHIFT);
                if (NNS_G2dIsRndCoreFlipH(pCurrentInstance_)) {
                    oam->x = -(oam->x + NNS_G2dGetOamSizeX(&shape));
                }
                if (NNS_G2dIsRndCoreFlipV(pCurrentInstance_)) {
                    oam->y = -(oam->y + NNS_G2dGetOamSizeY(&shape));
                }
            }
            oam->x += baseTrans.x >> FX32_SHIFT;
            oam->y += baseTrans.y >> FX32_SHIFT;
        }

        if (pCurrentInstance_->pFuncOamRegister(oam, affineIdx, FALSE) != TRUE) {
            return;
        }
        if (surface->pAfterDrawOamBackFuncCore != NULL) {
            surface->pAfterDrawOamBackFuncCore(pCurrentInstance_, cell, i);
        }
    }
}

// Draws an OAM as a quad in the current 3D matrix, flipped by the renderer's flip
static void DrawOamToSurface3D_(GXOamAttr *oam) {
    gfxLoadMatrix4x3(&pCurrentInstance_->mtx3D);

    if (pCurrentInstance_->flipFlag != NNS_G2D_RENDERERFLIP_NONE) {
        GXOamShape shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
        s16 posX = GetFlippedOamPosX_(oam, &shape, NNS_G2dIsRndCoreFlipH(pCurrentInstance_));
        s16 posY = GetFlippedOamPosY_(oam, &shape, NNS_G2dIsRndCoreFlipV(pCurrentInstance_));
        u32 oamFlip;
        u32 flipH;
        u32 flipV;

        // The renderer's flip toggles the OAM's own
        oamFlip = oam->flipH;
        flipH = NNS_G2dIsRndCoreFlipH(pCurrentInstance_);
        flipH ^= oamFlip;
        oamFlip = oam->flipV;
        flipV = NNS_G2dIsRndCoreFlipV(pCurrentInstance_);
        flipV ^= oamFlip;
        oam->attr01 = (oam->attr01 & ~GX_OAM_ATTR01_HF_MASK) | (flipH << GX_OAM_ATTR01_HF_SHIFT);
        oam->attr01 = (oam->attr01 & ~GX_OAM_ATTR01_VF_MASK) | (flipV << GX_OAM_ATTR01_VF_SHIFT);
        NNS_G2dDrawOneOam3DDirectWithPosFast(posX, posY, -1, oam, &pCurrentInstance_->pImageProxy->attr,
                                             pCurrentInstance_->baseTexAddr3D, pCurrentInstance_->basePltAddr3D);
    } else {
        NNS_G2dDrawOneOam3DDirectWithPosFast(GetOamPosX_(oam), GetOamPosY_(oam), -1, oam,
                                             &pCurrentInstance_->pImageProxy->attr, pCurrentInstance_->baseTexAddr3D,
                                             pCurrentInstance_->basePltAddr3D);
    }
}

static void DrawCellToSurface3D_(const NNSG2dRenderSurface *surface, const NNSG2dCellData *cell) {
    u16 i;
    GXOamAttr *oam = &pCurrentInstance_->currentOam;

    for (i = 0; i < cell->numOAMAttrs; i++) {
        pCurrentInstance_->bDrawEnable = TRUE;
        NNS_G2dCopyCellAsOamAttr(cell, i, oam);
        if (surface->pBeforeDrawOamBackFuncCore != NULL) {
            surface->pBeforeDrawOamBackFuncCore(pCurrentInstance_, cell, i);
        }
        if (pCurrentInstance_->bDrawEnable) {
            DrawOamToSurface3D_(oam);
        }
        if (surface->pAfterDrawOamBackFuncCore != NULL) {
            surface->pAfterDrawOamBackFuncCore(pCurrentInstance_, cell, i);
        }
    }
}

void NNS_G2dInitRndCore(NNSG2dRndCoreInstance *core) {
    sys_memset16(0, core, sizeof(NNSG2dRndCoreInstance));
    core->pCurrentTargetSurface = NULL;
    core->affineOverwriteMode = NNS_G2D_RND_AFFINE_OVERWRITE_DOUBLE;
    core->pImageProxy = NULL;
    core->pPaletteProxy = NULL;
    core->flipFlag = NNS_G2D_RENDERERFLIP_NONE;
    core->bDrawEnable = TRUE;
    core->pCurrentMxtCacheFor2D = NULL;
}

void NNS_G2dSetRndCoreImageProxy(NNSG2dRndCoreInstance *core, const NNSG2dImageProxy *imgProxy,
                                 const NNSG2dImagePaletteProxy *pltProxy) {
    core->pImageProxy = imgProxy;
    core->pPaletteProxy = pltProxy;
}

void NNS_G2dSetRndCoreOamRegisterFunc(NNSG2dRndCoreInstance *core, NNSG2dOamRegisterFunction oamRegister,
                                      NNSG2dAffineRegisterFunction affineRegister) {
    core->pFuncOamRegister = oamRegister;
    core->pFuncOamAffineRegister = affineRegister;
}

void NNS_G2dSetRndCoreAffineOverwriteMode(NNSG2dRndCoreInstance *core, u32 mode) {
    core->affineOverwriteMode = mode;
}

// Sets the matrix of the 3D surface: the 2D matrix, with Z the core's Z for software sprites
void NNS_G2dSetRndCoreCurrentMtx3D(const MtxFx32 *mtx) {
    NNSG2dRndCoreInstance *core = pCurrentInstance_;

    core->pCurrentMtx = mtx;
    core->mtx3D.m[0][0] = mtx->_00;
    core->mtx3D.m[0][1] = mtx->_01;
    core->mtx3D.m[0][2] = 0;
    core->mtx3D.m[1][0] = mtx->_10;
    core->mtx3D.m[1][1] = mtx->_11;
    core->mtx3D.m[1][2] = 0;
    core->mtx3D.m[2][0] = 0;
    core->mtx3D.m[2][1] = 0;
    core->mtx3D.m[2][2] = FX32_ONE;
    core->mtx3D.m[3][0] = mtx->_20;
    core->mtx3D.m[3][1] = mtx->_21;
    core->mtx3D.m[3][2] = core->zFor3DSoftwareSprite;
}

void NNS_G2dSetRndCoreCurrentMtx2D(const MtxFx32 *mtx, const NNSG2dRndCore2DMtxCache *cache) {
    NNSG2dRndCoreInstance *core = pCurrentInstance_;

    core->pCurrentMtx = mtx;
    core->pCurrentMxtCacheFor2D = cache;
}

void NNS_G2dSetRndCore3DSoftSpriteZvalue(NNSG2dRndCoreInstance *core, fx32 z) {
    core->zFor3DSoftwareSprite = z;
}

void NNS_G2dSetRndCoreSurface(NNSG2dRndCoreInstance *core, NNSG2dRenderSurface *surface) {
    core->pCurrentTargetSurface = surface;
}

BOOL NNS_G2dIsRndCoreFlipH(const NNSG2dRndCoreInstance *core) {
    return (core->flipFlag & NNS_G2D_RENDERERFLIP_H) != 0;
}

BOOL NNS_G2dIsRndCoreFlipV(const NNSG2dRndCoreInstance *core) {
    if (core->flipFlag & NNS_G2D_RENDERERFLIP_V) {
        return TRUE;
    }
    return FALSE;
}

void NNS_G2dSetRndCoreFlipMode(NNSG2dRndCoreInstance *core, BOOL flipH, BOOL flipV) {
    if (flipH) {
        core->flipFlag |= NNS_G2D_RENDERERFLIP_H;
    } else {
        core->flipFlag &= ~NNS_G2D_RENDERERFLIP_H;
    }
    if (flipV) {
        core->flipFlag |= NNS_G2D_RENDERERFLIP_V;
    } else {
        core->flipFlag &= ~NNS_G2D_RENDERERFLIP_V;
    }
}

// Where an image or a palette is in the 3D engine's VRAM, or 0 when it isn't there
static inline u32 GetImageLocation3D_(const NNSG2dImageProxy *proxy) {
    if (NNS_G2dIsImageReadyToUse(proxy, NNS_G2D_VRAM_TYPE_3DMAIN)) {
        return NNS_G2dGetImageLocation(proxy, NNS_G2D_VRAM_TYPE_3DMAIN);
    }
    return 0;
}

static inline u32 GetPaletteLocation3D_(const NNSG2dImagePaletteProxy *proxy) {
    if (NNS_G2dIsImagePaletteReadyToUse(proxy, NNS_G2D_VRAM_TYPE_3DMAIN)) {
        return NNS_G2dGetImagePaletteLocation(proxy, NNS_G2D_VRAM_TYPE_3DMAIN);
    }
    return 0;
}

// Starts drawing to the core's surface. The 3D surface's view moves the projection matrix, and the image and palette
// give the base addresses of the textures; a 2D surface's image gives the offset of the characters
void NNS_G2dRndCoreBeginRendering(NNSG2dRndCoreInstance *core) {
    u32 type;

    pCurrentInstance_ = core;
    type = core->pCurrentTargetSurface->type;
    if (type == NNS_G2D_SURFACETYPE_MAIN3D) {
        G3_MtxMode(GX_MTXMODE_PROJECTION);
        G3_PushMtx();
        G3_Translate(-core->pCurrentTargetSurface->viewRect.posTopLeft.x,
                     -core->pCurrentTargetSurface->viewRect.posTopLeft.y, 0);
        G3_MtxMode(GX_MTXMODE_POSITION);

        core->baseTexAddr3D = GetImageLocation3D_(pCurrentInstance_->pImageProxy);
        core->basePltAddr3D = GetPaletteLocation3D_(pCurrentInstance_->pPaletteProxy);
    } else {
        const NNSG2dImageProxy *proxy = core->pImageProxy;
        u32 offset;

        if (NNS_G2dIsImageReadyToUse(proxy, type)) {
            offset = NNS_G2dGetImageLocation(proxy, type) >>
                     (((proxy->attr.mappingType & REG_GX_DISPCNT_EXOBJ_MASK) >> REG_GX_DISPCNT_EXOBJ_SHIFT) + 5);
        } else {
            offset = 0;
        }
        pCurrentInstance_->base2DCharOffset = offset;
    }
}

void NNS_G2dRndCoreEndRendering(void) {
    if (pCurrentInstance_->pCurrentTargetSurface->type == NNS_G2D_SURFACETYPE_MAIN3D) {
        G3_MtxMode(GX_MTXMODE_PROJECTION);
        G3_PopMtx(1);
        G3_MtxMode(GX_MTXMODE_POSITION);
    } else {
        pCurrentInstance_->base2DCharOffset = 0;
        pCurrentInstance_->pCurrentMtx = NULL;
        pCurrentInstance_->pCurrentMxtCacheFor2D = NULL;
    }
    pCurrentInstance_ = NULL;
}

void NNS_G2dRndCoreDrawCell(const NNSG2dCellData *cell) {
    NNSG2dRenderSurface *surface = pCurrentInstance_->pCurrentTargetSurface;

    if (!surface->bActive) {
        return;
    }
    pCurrentInstance_->bDrawEnable = TRUE;
    if (surface->pBeforeDrawCellBackFuncCore != NULL) {
        surface->pBeforeDrawCellBackFuncCore(pCurrentInstance_, cell);
    }
    if (pCurrentInstance_->bDrawEnable) {
        switch (surface->type) {
        case NNS_G2D_SURFACETYPE_MAIN3D:
            DrawCellToSurface3D_(surface, cell);
            break;
        case NNS_G2D_SURFACETYPE_MAIN2D:
        case NNS_G2D_SURFACETYPE_SUB2D:
            DrawCellToSurface2D_(surface, cell);
            break;
        // The case keeps the jump table four entries long, as the SDK's switch makes it
        case NNS_G2D_SURFACETYPE_MAX:
        default:
            break;
        }
    }
    if (surface->pAfterDrawCellBackFuncCore != NULL) {
        surface->pAfterDrawCellBackFuncCore(pCurrentInstance_, cell);
    }
}

// Draws a cell whose characters are transferred to VRAM, and marks them drawn by the surface's engine, so that they
// are transferred there
void NNS_G2dRndCoreDrawCellVramTransfer(const NNSG2dCellData *cell, u32 cellVramTransferHandle) {
    NNSG2dRenderSurface *surface = pCurrentInstance_->pCurrentTargetSurface;

    if (!surface->bActive) {
        return;
    }
    pCurrentInstance_->bDrawEnable = TRUE;
    if (surface->pBeforeDrawCellBackFuncCore != NULL) {
        surface->pBeforeDrawCellBackFuncCore(pCurrentInstance_, cell);
    }
    if (pCurrentInstance_->bDrawEnable) {
        if (cellVramTransferHandle != NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE) {
            u32 type = surface->type;
            NNSG2dCellTransferState *state = NNSi_G2dGetCellTransferState(cellVramTransferHandle);

            state->bDrawn = (state->bDrawn & ~(1 << type)) | (1 << type);
        }
        switch (surface->type) {
        case NNS_G2D_SURFACETYPE_MAIN3D:
            DrawCellToSurface3D_(surface, cell);
            break;
        case NNS_G2D_SURFACETYPE_MAIN2D:
        case NNS_G2D_SURFACETYPE_SUB2D:
            DrawCellToSurface2D_(surface, cell);
            break;
        // The case keeps the jump table four entries long, as the SDK's switch makes it
        case NNS_G2D_SURFACETYPE_MAX:
        default:
            break;
        }
    }
    if (surface->pAfterDrawCellBackFuncCore != NULL) {
        surface->pAfterDrawCellBackFuncCore(pCurrentInstance_, cell);
    }
}
