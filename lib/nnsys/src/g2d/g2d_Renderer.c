#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_Renderer.c: the renderer, which draws cells, cell animations and multi-cell animations to its
// surfaces through the renderer core, with a matrix stack of its own. The 2D surfaces share their affine matrices
// through a cache, which tracks, per level of the stack, whether the level's scale and rotation have been cached. The
// file name and the names are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them, the matrix stack's and
// cache's from its internal headers g2di_RendererMtxStack.hpp, g2di_RendererMtxCache.h and g2di_RendererMtxState.h; the
// ROM has no string for the file. ApplySRT_, DrawCellToCore_, DrawMCNodeShared_, GetMtxCacheState_, IsRndOverwriteOn_,
// SetMtxCacheToGroup_ and TranslateFlipped_ are ours, and SetOBJMode_ is a copy of NitroSDK's G2_SetOBJMode

#define NNS_G2D_MAXLENGTH_OF_2DMTXSTACK 32
#define G2Di_NUM_MTX_CACHE 32
#define G2Di_RENDERER_MTX_SR_NONE 0xfffe

// How a level of the matrix stack stands with the cache: scaled or rotated at this level, or by the level below,
// since the last cache. The levels set up together share a group ID
enum {
    MCM_MTX_NOT_SR,
    MCM_MTX_SR_NOT_CACHELOADED,
    MCM_MTX_SR_NOT_CACHELOADED_STACKCHANGED,
    MCM_MTX_SR_CACHELOADED,
};

typedef struct {
    u16 mtxCacheIdx;
    u16 groupID;
    u16 stateType;
    u16 pad16;
} MCMMtxState;

static void NNSi_G2dIdentity(void);
static void NNSi_G2dMCMStoreCurrentMtxToMtxCache(void);
static void RndCoreCBFuncBeforeCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell);
static void RndCoreCBFuncAfterCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell);
static void RndCoreCBFuncBeforeOBJ_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx);
static void RndCoreCBFuncAfterOBJ_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx);
static void DrawCellImpl_(const NNSG2dCellData *cell);
static void DrawCellAnimationImpl_(const NNSG2dCellAnimation *cellAnim);
static void DrawNode_(const NNSG2dNode *node);
static void NNS_G2dDrawMultiCell(const NNSG2dMultiCellInstance *instance);

// The stack level from which the matrix is scaled or rotated, the current cell transfer handle, and the matrix that
// NNS_G2D_RDR_OPZHINT_NOT_SR builds from a translation
static u16 firstSRTransformedMtxIdx_ = G2Di_RENDERER_MTX_SR_NONE;
static u32 currenVramTransferHandle_ = NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE;
static MtxFx32 mtxI_ = { FX32_ONE, 0, 0, FX32_ONE, 0, 0 };

// Declared in this order, MWCC lays them out as the original does
static u16 groupID_;
static u16 currentMtxCachePos_;
static NNSG2dRendererInstance *pCurrentInstance_;
// The SDK's s32, a long, as the public decompilation of NitroSystem declares the stack position: as an int,
// NNS_G2dPushMtx keeps the position it loaded first where the original loads it again after the bound check
static long stackPos_;
static BOOL bDonotUseSRTransform;
static fx32 fxZStack_[NNS_G2D_MAXLENGTH_OF_2DMTXSTACK];
static MCMMtxState mtxStateStack_[NNS_G2D_MAXLENGTH_OF_2DMTXSTACK];
// The matrices, and the same with the inverse scale, which the 2D engines' affine parameters take
static MtxFx32 mtxStackFor2DHW_[NNS_G2D_MAXLENGTH_OF_2DMTXSTACK];
static MtxFx32 mtxStack_[NNS_G2D_MAXLENGTH_OF_2DMTXSTACK];
static NNSG2dRndCore2DMtxCache mtxCacheBuffer_[G2Di_NUM_MTX_CACHE];

static inline const MtxFx32 *NNSi_G2dGetCurrentMtx(void) {
    if (bDonotUseSRTransform) {
        mtxI_._20 = mtxStack_[stackPos_]._20;
        mtxI_._21 = mtxStack_[stackPos_]._21;
        return &mtxI_;
    } else {
        return &mtxStack_[stackPos_];
    }
}

static inline const MtxFx32 *NNSi_G2dGetCurrentMtxFor2DHW(void) {
    if (bDonotUseSRTransform) {
        mtxI_._20 = mtxStack_[stackPos_]._20;
        mtxI_._21 = mtxStack_[stackPos_]._21;
        return &mtxI_;
    } else {
        return &mtxStackFor2DHW_[stackPos_];
    }
}

static inline MCMMtxState *GetMtxCacheState_(u16 pos) {
    return &mtxStateStack_[pos];
}

static inline NNSG2dRndCore2DMtxCache *NNSi_G2dMCMGetCurrentMtxCache(void) {
    return &mtxCacheBuffer_[GetMtxCacheState_(stackPos_)->mtxCacheIdx];
}

static inline void NNS_G2dInitRndCore2DMtxCache(NNSG2dRndCore2DMtxCache *cache) {
    MI_CpuFillFast(cache->affineIndex, 0xffffffff, sizeof(cache->affineIndex));
}

static inline u16 NNSi_RMCUseNewMtxCache(void) {
    u16 idx = currentMtxCachePos_;

    if (currentMtxCachePos_ < G2Di_NUM_MTX_CACHE - 1) {
        currentMtxCachePos_++;
    }
    return idx;
}

static inline BOOL NNSi_G2dMCMShouldCurrentMtxBeLoadedToMtxCache(const MCMMtxState *state) {
    return state->stateType == MCM_MTX_SR_NOT_CACHELOADED ||
           state->stateType == MCM_MTX_SR_NOT_CACHELOADED_STACKCHANGED;
}

static inline u32 NNSi_G2dIsRndCurrentMtxSRTransformed(void) {
    if (!bDonotUseSRTransform && firstSRTransformedMtxIdx_ != G2Di_RENDERER_MTX_SR_NONE) {
        return TRUE;
    }
    return FALSE;
}

static inline u32 IsRendererDrawingVramTransferedCell_(u32 handle) {
    if (handle != NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE) {
        return TRUE;
    }
    return FALSE;
}

static inline u32 NNSi_G2dIsStackEmpty(void) {
    if (stackPos_ <= 0) {
        return TRUE;
    }
    return FALSE;
}

// A new group of levels was scaled or rotated
static inline void NNSi_G2dMCMSetCurrentMtxSRChanged(void) {
    MCMMtxState *state;

    if (!NNSi_G2dIsRndCurrentMtxSRTransformed()) {
        firstSRTransformedMtxIdx_ = stackPos_;
    }
    state = GetMtxCacheState_(stackPos_);
    switch (state->stateType) {
    case MCM_MTX_NOT_SR:
    case MCM_MTX_SR_NOT_CACHELOADED_STACKCHANGED:
    case MCM_MTX_SR_CACHELOADED:
        state->groupID = groupID_++;
        state->stateType = MCM_MTX_SR_NOT_CACHELOADED;
        break;
    case MCM_MTX_SR_NOT_CACHELOADED:
        break;
    }
}

static inline void TranslateFlipped_(const NNSG2dSVec2 *pos) {
    int x, y;

    y = pos->y;
    x = pos->x;

    if (NNS_G2dIsRndCoreFlipH(&pCurrentInstance_->rendererCore)) {
        x = -x;
    }
    if (NNS_G2dIsRndCoreFlipV(&pCurrentInstance_->rendererCore)) {
        y = -y;
    }
    NNS_G2dTranslate(x << FX32_SHIFT, y << FX32_SHIFT, 0);
}

static inline void ApplySRT_(const NNSG2dSRTData *srt) {
    if (srt->SRT_EnableFlag & NNS_G2D_AFFINEENABLE_TRANS) {
        TranslateFlipped_(&srt->trans);
    }
    if (srt->SRT_EnableFlag & NNS_G2D_AFFINEENABLE_ROTATE) {
        NNS_G2dRotZ(FX_SinIdx(srt->rotZ), FX_CosIdx(srt->rotZ));
    }
    if (srt->SRT_EnableFlag & NNS_G2D_AFFINEENABLE_SCALE) {
        NNS_G2dScale(srt->scale.x, srt->scale.y, FX32_ONE);
    }
}

static inline void DrawCellToCore_(const NNSG2dCellData *cell) {
    if (IsRendererDrawingVramTransferedCell_(currenVramTransferHandle_)) {
        NNS_G2dRndCoreDrawCellVramTransfer(cell, currenVramTransferHandle_);
    } else {
        NNS_G2dRndCoreDrawCell(cell);
    }
}

static inline void BeginRndCoreRendering2D_(NNSG2dRendererInstance *rend, NNSG2dRenderSurface *surface) {
    rend->pCurrentSurface = surface;
    NNS_G2dSetRndCoreSurface(&rend->rendererCore, &surface->coreSurface);
    NNS_G2dSetRndCoreOamRegisterFunc(&rend->rendererCore, surface->pFuncOamRegister, surface->pFuncOamAffineRegister);
    NNS_G2dRndCoreBeginRendering(&rend->rendererCore);
}

static inline void BeginRndCoreRendering3D_(NNSG2dRendererInstance *rend, NNSG2dRenderSurface *surface) {
    rend->pCurrentSurface = surface;
    NNS_G2dSetRndCoreSurface(&rend->rendererCore, &surface->coreSurface);
    NNS_G2dRndCoreBeginRendering(&rend->rendererCore);
}

static inline void EndRndCoreRendering_(void) {
    pCurrentInstance_->pCurrentSurface = NULL;
    NNS_G2dRndCoreEndRendering();
}

static void NNSi_G2dIdentity(void) {
    if (bDonotUseSRTransform) {
        mtxStack_[stackPos_]._20 = 0;
        mtxStack_[stackPos_]._21 = 0;
    } else {
        MtxFx32 *mtx = &mtxStack_[stackPos_];

        mtx->_00 = FX32_ONE;
        mtx->_01 = 0;
        mtx->_10 = 0;
        mtx->_11 = FX32_ONE;
        mtx->_20 = 0;
        mtx->_21 = 0;
        mtxStackFor2DHW_[stackPos_] = *mtx;
    }
    fxZStack_[stackPos_] = 0;
}

static inline void SetMtxCacheToGroup_(u16 pos, u16 groupID, u16 cacheIdx) {
    int i;

    for (i = pos; i >= 0; i--) {
        if (mtxStateStack_[i].groupID != groupID) {
            break;
        }
        mtxStateStack_[i].stateType = MCM_MTX_SR_CACHELOADED;
        mtxStateStack_[i].mtxCacheIdx = cacheIdx;
    }
}

// Gives the current level's scale and rotation a cache, shared with the levels below it of the same group
static void NNSi_G2dMCMStoreCurrentMtxToMtxCache(void) {
    MCMMtxState *state = GetMtxCacheState_(stackPos_);

    if (NNSi_G2dMCMShouldCurrentMtxBeLoadedToMtxCache(state)) {
        const u16 cacheIdx = NNSi_RMCUseNewMtxCache();
        const u16 groupID = state->groupID;
        const MtxFx32 *mtx;
        NNSG2dRndCore2DMtxCache *cache;

        state->mtxCacheIdx = cacheIdx;
        NNS_G2dInitRndCore2DMtxCache(&mtxCacheBuffer_[cacheIdx]);
        mtx = NNSi_G2dGetCurrentMtxFor2DHW();
        cache = NNSi_G2dMCMGetCurrentMtxCache();
        cache->m22._00 = mtx->_00;
        cache->m22._01 = mtx->_01;
        cache->m22._10 = mtx->_10;
        cache->m22._11 = mtx->_11;
        SetMtxCacheToGroup_(stackPos_, groupID, cacheIdx);
    }
}

static void RndCoreCBFuncBeforeCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell) {
    NNSG2dRenderSurface *surface = pCurrentInstance_->pCurrentSurface;

    if (surface->pFuncVisibilityCulling != NULL) {
        if (!surface->pFuncVisibilityCulling(cell, NNSi_G2dGetCurrentMtx(), &surface->viewRect)) {
            core->bDrawEnable = FALSE;
            return;
        }
        core->bDrawEnable = TRUE;
    }
    if (surface->pBeforeDrawCellBackFunc != NULL) {
        surface->pBeforeDrawCellBackFunc(pCurrentInstance_, surface, cell, NNSi_G2dGetCurrentMtx());
    }
}

static void RndCoreCBFuncAfterCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell) {
    NNSG2dRenderSurface *surface = pCurrentInstance_->pCurrentSurface;

    if (surface->pAfterDrawCellBackFunc != NULL) {
        surface->pAfterDrawCellBackFunc(pCurrentInstance_, surface, cell, NNSi_G2dGetCurrentMtx());
    }
}

// A BOOL expression: as a u32 if-form, MWCC keeps the flag for the first test that follows
static inline BOOL IsRndOverwriteOn_(const NNSG2dRendererInstance *rend) {
    return rend->overwriteEnableFlag != NNS_G2D_RND_OVERWRITE_NONE;
}

// NitroSDK's G2_SetOBJMode
static inline void SetOBJMode_(GXOamAttr *oam, int mode, int cParam) {
    oam->attr01 = (mode << GX_OAM_ATTR01_MODE_SHIFT) | (oam->attr01 & ~GX_OAM_ATTR01_MODE_MASK);
    oam->attr2 = (u16)((oam->attr2 & ~GX_OAM_ATTR2_CPARAM_MASK) | (cParam << GX_OAM_ATTR2_CPARAM_SHIFT));
}

static void RndCoreCBFuncBeforeOBJ_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx) {
    GXOamAttr *oam = &core->currentOam;
    const NNSG2dPaletteSwapTable *tbl = NNS_G2dGetRendererPaletteTbl(pCurrentInstance_);

    if (tbl != NULL) {
        oam->cParam = NNS_G2dGetPaletteTableValue(
            tbl, (u16)((oam->attr2 & GX_OAM_ATTR2_CPARAM_MASK) >> GX_OAM_ATTR2_CPARAM_SHIFT));
    }
    if (IsRndOverwriteOn_(pCurrentInstance_)) {
        if (pCurrentInstance_->overwriteEnableFlag & NNS_G2D_RND_OVERWRITE_PRIORITY) {
            oam->priority = pCurrentInstance_->overwritePriority;
        }
        if (pCurrentInstance_->overwriteEnableFlag & NNS_G2D_RND_OVERWRITE_PLTTNO) {
            oam->cParam = pCurrentInstance_->overwritePlttNo;
        }
        if (pCurrentInstance_->overwriteEnableFlag & NNS_G2D_RND_OVERWRITE_PLTTNO_OFFS) {
            oam->cParam = oam->cParam + pCurrentInstance_->overwritePlttNoOffset;
        }
        if (pCurrentInstance_->overwriteEnableFlag & NNS_G2D_RND_OVERWRITE_MOSAIC) {
            if (pCurrentInstance_->overwriteMosaicFlag) {
                oam->attr01 |= GX_OAM_ATTR01_MOSAIC_MASK;
            } else {
                oam->attr01 &= ~GX_OAM_ATTR01_MOSAIC_MASK;
            }
        }
        if (pCurrentInstance_->overwriteEnableFlag & NNS_G2D_RND_OVERWRITE_OBJMODE) {
            SetOBJMode_(oam, pCurrentInstance_->overwriteObjMode,
                        (oam->attr2 & GX_OAM_ATTR2_CPARAM_MASK) >> GX_OAM_ATTR2_CPARAM_SHIFT);
        }
    }
    {
        NNSG2dRendererInstance *rend = pCurrentInstance_;
        NNSG2dRenderSurface *surface = rend->pCurrentSurface;

        if (surface->pBeforeDrawOamBackFunc != NULL) {
            surface->pBeforeDrawOamBackFunc(rend, surface, cell, oamIdx, NNSi_G2dGetCurrentMtx());
        }
    }
}

static void RndCoreCBFuncAfterOBJ_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx) {
    NNSG2dRendererInstance *rend = pCurrentInstance_;
    NNSG2dRenderSurface *surface = rend->pCurrentSurface;

    if (surface->pAfterDrawOamBackFunc != NULL) {
        surface->pAfterDrawOamBackFunc(rend, surface, cell, oamIdx, NNSi_G2dGetCurrentMtx());
    }
}

static inline void DrawCellToSurface2D_(const NNSG2dCellData *cell) {
    NNSG2dRndCore2DMtxCache *cache = NULL;

    if (NNSi_G2dIsRndCurrentMtxSRTransformed()) {
        if (mcRenderState_.bDrawMC) {
            cache = mcRenderState_.cellAnimMtxCacheTbl[mcRenderState_.currentCellAnimIdx];
            if (cache == NULL) {
                NNSi_G2dMCMStoreCurrentMtxToMtxCache();
                cache = NNSi_G2dMCMGetCurrentMtxCache();
                mcRenderState_.cellAnimMtxCacheTbl[mcRenderState_.currentCellAnimIdx] = cache;
            }
        } else {
            NNSi_G2dMCMStoreCurrentMtxToMtxCache();
            cache = NNSi_G2dMCMGetCurrentMtxCache();
        }
    }
    NNS_G2dSetRndCoreCurrentMtx2D(NNSi_G2dGetCurrentMtx(), cache);
    DrawCellToCore_(cell);
}

static inline void DrawCellToSurface3D_(NNSG2dRendererInstance *rend, const NNSG2dCellData *cell) {
    NNS_G2dSetRndCore3DSoftSpriteZvalue(&rend->rendererCore, fxZStack_[stackPos_]);
    NNS_G2dSetRndCoreCurrentMtx3D(NNSi_G2dGetCurrentMtx());
    DrawCellToCore_(cell);
}

static void DrawCellImpl_(const NNSG2dCellData *cell) {
    NNSG2dRendererInstance *rend = pCurrentInstance_;
    NNSG2dRenderSurface *surface = rend->pTargetSurfaceList;

    if (rend->opzHint & NNS_G2D_RDR_OPZHINT_LOCK_PARAMS) {
        if (surface->type != NNS_G2D_SURFACETYPE_MAIN3D) {
            DrawCellToSurface2D_(cell);
        } else {
            DrawCellToSurface3D_(rend, cell);
        }
    } else {
        while (surface != NULL) {
            if (surface->bActive) {
                if (surface->type != NNS_G2D_SURFACETYPE_MAIN3D) {
                    BeginRndCoreRendering2D_(pCurrentInstance_, surface);
                    DrawCellToSurface2D_(cell);
                    EndRndCoreRendering_();
                } else {
                    BeginRndCoreRendering3D_(pCurrentInstance_, surface);
                    DrawCellToSurface3D_(rend, cell);
                    EndRndCoreRendering_();
                }
            }
            surface = surface->pNextSurface;
        }
    }
}

static void DrawCellAnimationImpl_(const NNSG2dCellAnimation *cellAnim) {
    const NNSG2dCellData *cell = cellAnim->pCurrentCell;

    if (cellAnim->srtCtrl.srtData.SRT_EnableFlag == NNS_G2D_AFFINEENABLE_NONE) {
        if (IsRendererDrawingVramTransferedCell_(cellAnim->cellTransferStateHandle)) {
            currenVramTransferHandle_ = cellAnim->cellTransferStateHandle;
            DrawCellImpl_(cell);
            currenVramTransferHandle_ = NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE;
        } else {
            DrawCellImpl_(cell);
        }
    } else {
        NNS_G2dPushMtx();
        ApplySRT_(&cellAnim->srtCtrl.srtData);
        if (IsRendererDrawingVramTransferedCell_(cellAnim->cellTransferStateHandle)) {
            currenVramTransferHandle_ = cellAnim->cellTransferStateHandle;
            DrawCellImpl_(cell);
            currenVramTransferHandle_ = NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE;
        } else {
            DrawCellImpl_(cell);
        }
        NNS_G2dPopMtx(1);
    }
}

static void DrawNode_(const NNSG2dNode *node) {
    if (node->bVisible) {
        const NNSG2dCellAnimation *cellAnim = node->pContent;

        NNS_G2dPushMtx();
        ApplySRT_(&node->srtCtrl.srtData);
        DrawCellAnimationImpl_(cellAnim);
        NNS_G2dPopMtx(1);
    }
}

void NNS_G2dInitRenderer(NNSG2dRendererInstance *rend) {
    int i;

    NNS_G2dInitRndCore(&rend->rendererCore);
    rend->pTargetSurfaceList = NULL;
    rend->pCurrentSurface = NULL;
    rend->pPaletteSwapTbl = NULL;
    rend->opzHint = NNS_G2D_RDR_OPZHINT_NONE;
    rend->spriteZoffsetStep = 0;
    rend->overwriteEnableFlag = NNS_G2D_RND_OVERWRITE_NONE;
    rend->overwritePriority = 0;
    rend->overwritePlttNo = 0;
    rend->overwriteObjMode = GX_OAM_MODE_NORMAL;
    rend->overwriteMosaicFlag = FALSE;
    rend->overwritePlttNoOffset = 0;

    stackPos_ = 0;
    firstSRTransformedMtxIdx_ = G2Di_RENDERER_MTX_SR_NONE;
    for (i = 0; i < G2Di_NUM_MTX_CACHE; i++) {
        NNS_G2dInitRndCore2DMtxCache(&mtxCacheBuffer_[i]);
    }
    currentMtxCachePos_ = 0;
    groupID_ = 0;
    MI_CpuClearFast(mtxStateStack_, sizeof(mtxStateStack_));
    bDonotUseSRTransform = FALSE;
}

void NNS_G2dAddRendererTargetSurface(NNSG2dRendererInstance *rend, NNSG2dRenderSurface *surface) {
    surface->pNextSurface = rend->pTargetSurfaceList;
    rend->pTargetSurfaceList = surface;
}

void NNS_G2dInitRenderSurface(NNSG2dRenderSurface *surface) {
    MI_CpuClear16(surface, sizeof(NNSG2dRenderSurface));
    surface->bActive = TRUE;
    surface->type = NNS_G2D_SURFACETYPE_MAX;
    surface->pBeforeDrawCellBackFuncCore = RndCoreCBFuncBeforeCell_;
    surface->pAfterDrawCellBackFuncCore = RndCoreCBFuncAfterCell_;
    surface->pBeforeDrawOamBackFuncCore = RndCoreCBFuncBeforeOBJ_;
    surface->pAfterDrawOamBackFuncCore = RndCoreCBFuncAfterOBJ_;
}

void NNS_G2dBeginRendering(NNSG2dRendererInstance *rend) {
    int i;

    pCurrentInstance_ = rend;
    stackPos_ = 0;
    firstSRTransformedMtxIdx_ = G2Di_RENDERER_MTX_SR_NONE;
    for (i = 0; i < currentMtxCachePos_; i++) {
        NNS_G2dInitRndCore2DMtxCache(&mtxCacheBuffer_[i]);
    }
    currentMtxCachePos_ = 0;
    groupID_ = 0;
    MI_CpuClearFast(mtxStateStack_, sizeof(mtxStateStack_));
    G3_PushMtx();
    G3_Identity();
    NNSi_G2dIdentity();
}

void NNS_G2dBeginRenderingEx(NNSG2dRendererInstance *rend, u32 opzHint) {
    rend->opzHint = opzHint;
    if (opzHint & NNS_G2D_RDR_OPZHINT_NOT_SR) {
        bDonotUseSRTransform = TRUE;
    }
    if (opzHint & NNS_G2D_RDR_OPZHINT_LOCK_PARAMS) {
        NNSG2dRenderSurface *surface = rend->pTargetSurfaceList;

        if (surface->bActive) {
            if (surface->type != NNS_G2D_SURFACETYPE_MAIN3D) {
                BeginRndCoreRendering2D_(rend, surface);
            } else {
                BeginRndCoreRendering3D_(rend, surface);
            }
        }
    }
    NNS_G2dBeginRendering(rend);
}

void NNS_G2dEndRendering(void) {
    G3_PopMtx(1);
    if (pCurrentInstance_->opzHint != NNS_G2D_RDR_OPZHINT_NONE) {
        const u32 opzHint = pCurrentInstance_->opzHint;

        if (opzHint & NNS_G2D_RDR_OPZHINT_NOT_SR) {
            bDonotUseSRTransform = FALSE;
        }
        if (opzHint & NNS_G2D_RDR_OPZHINT_LOCK_PARAMS) {
            EndRndCoreRendering_();
        }
        pCurrentInstance_->opzHint = NNS_G2D_RDR_OPZHINT_NONE;
    }
    pCurrentInstance_ = NULL;
}

void NNS_G2dDrawCell(const NNSG2dCellData *cell) {
    if (pCurrentInstance_->spriteZoffsetStep != 0) {
        fx32 step = NNSi_G2dGetOamSoftEmuAutoZOffsetStep();

        NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(TRUE);
        NNSi_G2dSetOamSoftEmuAutoZOffsetStep(pCurrentInstance_->spriteZoffsetStep);
        DrawCellImpl_(cell);
        NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(FALSE);
        NNSi_G2dSetOamSoftEmuAutoZOffsetStep(step);
        NNSi_G2dResetOamSoftEmuAutoZOffset();
    } else {
        DrawCellImpl_(cell);
    }
}

void NNS_G2dDrawCellAnimation(const NNSG2dCellAnimation *cellAnim) {
    if (pCurrentInstance_->spriteZoffsetStep != 0) {
        fx32 step = NNSi_G2dGetOamSoftEmuAutoZOffsetStep();

        NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(TRUE);
        NNSi_G2dSetOamSoftEmuAutoZOffsetStep(pCurrentInstance_->spriteZoffsetStep);
        DrawCellAnimationImpl_(cellAnim);
        NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(FALSE);
        NNSi_G2dSetOamSoftEmuAutoZOffsetStep(step);
        NNSi_G2dResetOamSoftEmuAutoZOffset();
    } else {
        DrawCellAnimationImpl_(cellAnim);
    }
}

static inline u16 NNSi_G2dGetMC2NodeCellAinmIdx(const NNSG2dMultiCellHierarchyData *node) {
    return (node->nodeAttr & 0xff00) >> 8;
}

static inline void DrawMCNodeShared_(const NNSG2dMultiCellInstance *instance, u16 nodeIdx) {
    const NNSG2dMultiCellHierarchyData *node = &instance->pCurrentMultiCell->pHierDataArray[nodeIdx];
    const u16 cellAnimIdx = NNSi_G2dGetMC2NodeCellAinmIdx(node);
    NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimInstasnces;

    mcRenderState_.currentCellAnimIdx = cellAnimIdx;
    NNS_G2dPushMtx();
    TranslateFlipped_(&node->pos);
    DrawCellAnimationImpl_(&cellAnims[cellAnimIdx].cellAnim);
    NNS_G2dPopMtx(1);
}

static void NNS_G2dDrawMultiCell(const NNSG2dMultiCellInstance *instance) {
    u16 i;

    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        for (i = 0; i < instance->pCurrentMultiCell->numCellAnim; i++) {
            mcRenderState_.cellAnimMtxCacheTbl[i] = NULL;
        }
        mcRenderState_.bDrawMC = TRUE;
        if (pCurrentInstance_->spriteZoffsetStep != 0) {
            fx32 step = NNSi_G2dGetOamSoftEmuAutoZOffsetStep();

            NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(TRUE);
            NNSi_G2dSetOamSoftEmuAutoZOffsetStep(pCurrentInstance_->spriteZoffsetStep);
            for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
                DrawMCNodeShared_(instance, i);
            }
            NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(FALSE);
            NNSi_G2dSetOamSoftEmuAutoZOffsetStep(step);
            NNSi_G2dResetOamSoftEmuAutoZOffset();
        } else {
            for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
                DrawMCNodeShared_(instance, i);
            }
        }
        mcRenderState_.bDrawMC = FALSE;
    } else {
        const NNSG2dNode *nodes = instance->pCellAnimInstasnces;

        if (pCurrentInstance_->spriteZoffsetStep != 0) {
            fx32 step = NNSi_G2dGetOamSoftEmuAutoZOffsetStep();

            NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(TRUE);
            NNSi_G2dSetOamSoftEmuAutoZOffsetStep(pCurrentInstance_->spriteZoffsetStep);
            for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
                DrawNode_(&nodes[i]);
            }
            NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(FALSE);
            NNSi_G2dSetOamSoftEmuAutoZOffsetStep(step);
            NNSi_G2dResetOamSoftEmuAutoZOffset();
        } else {
            for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
                DrawNode_(&nodes[i]);
            }
        }
    }
}

void NNS_G2dDrawMultiCellAnimation(const NNSG2dMultiCellAnimation *mcAnim) {
    if (mcAnim->srtCtrl.srtData.SRT_EnableFlag == NNS_G2D_AFFINEENABLE_NONE) {
        NNS_G2dDrawMultiCell(&mcAnim->multiCellInstance);
    } else {
        NNS_G2dPushMtx();
        ApplySRT_(&mcAnim->srtCtrl.srtData);
        NNS_G2dDrawMultiCell(&mcAnim->multiCellInstance);
        NNS_G2dPopMtx(1);
    }
}

static inline u32 NNSi_G2dStackHasEnoughCapacity(int pos) {
    if (pos < NNS_G2D_MAXLENGTH_OF_2DMTXSTACK) {
        return TRUE;
    }
    return FALSE;
}

static inline void NNSi_G2dMtxPush(void) {
    const int next = stackPos_ + 1;

    if (NNSi_G2dStackHasEnoughCapacity(next)) {
        if (bDonotUseSRTransform) {
            mtxStack_[next]._20 = mtxStack_[stackPos_]._20;
            mtxStack_[next]._21 = mtxStack_[stackPos_]._21;
            fxZStack_[next] = fxZStack_[stackPos_];
        } else {
            mtxStack_[next] = mtxStack_[stackPos_];
            mtxStackFor2DHW_[next] = mtxStackFor2DHW_[stackPos_];
            fxZStack_[next] = fxZStack_[stackPos_];
        }
        stackPos_ = next;
    }
}

void NNS_G2dPushMtx(void) {
    if (!(pCurrentInstance_->opzHint & NNS_G2D_RDR_OPZHINT_NOT_SR)) {
        const u16 lastPos = stackPos_;

        NNSi_G2dMtxPush();
        mtxStateStack_[(u16)stackPos_] = mtxStateStack_[lastPos];
        if (mtxStateStack_[lastPos].stateType == MCM_MTX_SR_NOT_CACHELOADED) {
            mtxStateStack_[(u16)stackPos_].stateType = MCM_MTX_SR_NOT_CACHELOADED_STACKCHANGED;
        } else {
            mtxStateStack_[(u16)stackPos_].stateType = mtxStateStack_[lastPos].stateType;
        }
    } else {
        NNSi_G2dMtxPush();
    }
}

void NNS_G2dPopMtx(u16 num) {
    if (!NNSi_G2dIsStackEmpty()) {
        stackPos_--;
        if (firstSRTransformedMtxIdx_ > stackPos_) {
            firstSRTransformedMtxIdx_ = G2Di_RENDERER_MTX_SR_NONE;
        }
    }
}

void NNS_G2dTranslate(fx32 x, fx32 y, fx32 z) {
    if (bDonotUseSRTransform) {
        mtxStack_[stackPos_]._20 += x;
        mtxStack_[stackPos_]._21 += y;
        fxZStack_[stackPos_] += z;
    } else {
        MtxFx32 mtx;

        mtx._00 = FX32_ONE;
        mtx._01 = 0;
        mtx._10 = 0;
        mtx._11 = FX32_ONE;
        mtx._20 = x;
        mtx._21 = y;
        NNSi_G2dMtxConcat32(&mtx, &mtxStack_[stackPos_], &mtxStack_[stackPos_]);
        NNSi_G2dMtxConcat32(&mtx, &mtxStackFor2DHW_[stackPos_], &mtxStackFor2DHW_[stackPos_]);
        fxZStack_[stackPos_] += z;
    }
}

void NNS_G2dScale(fx32 x, fx32 y, fx32 z) {
    MtxFx32 mtx;

    mtx._00 = x;
    mtx._01 = 0;
    mtx._10 = 0;
    mtx._11 = y;
    mtx._20 = 0;
    mtx._21 = 0;
    NNSi_G2dMtxConcat32(&mtx, &mtxStack_[stackPos_], &mtxStack_[stackPos_]);
    mtx._00 = FX_Inv(x);
    mtx._01 = 0;
    mtx._10 = 0;
    mtx._11 = FX_Inv(y);
    mtx._20 = 0;
    mtx._21 = 0;
    NNSi_G2dMtxConcat32(&mtx, &mtxStackFor2DHW_[stackPos_], &mtxStackFor2DHW_[stackPos_]);
    NNSi_G2dMCMSetCurrentMtxSRChanged();
}

void NNS_G2dRotZ(fx32 sin, fx32 cos) {
    MtxFx32 mtx;

    mtx._00 = cos;
    mtx._01 = sin;
    mtx._10 = -sin;
    mtx._11 = cos;
    mtx._20 = 0;
    mtx._21 = 0;
    NNSi_G2dMtxConcat32(&mtx, &mtxStack_[stackPos_], &mtxStack_[stackPos_]);
    NNSi_G2dMtxConcat32(&mtx, &mtxStackFor2DHW_[stackPos_], &mtxStackFor2DHW_[stackPos_]);
    NNSi_G2dMCMSetCurrentMtxSRChanged();
}

const NNSG2dPaletteSwapTable *NNS_G2dGetRendererPaletteTbl(NNSG2dRendererInstance *rend) {
    return rend->pPaletteSwapTbl;
}

void NNS_G2dSetRendererImageProxy(NNSG2dRendererInstance *rend, const NNSG2dImageProxy *imgProxy,
                                  const NNSG2dImagePaletteProxy *pltProxy) {
    NNS_G2dSetRndCoreImageProxy(&rend->rendererCore, imgProxy, pltProxy);
}
