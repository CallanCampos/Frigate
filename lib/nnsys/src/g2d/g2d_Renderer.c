#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_Renderer.c: the renderer, which draws cells, cell animations and multi-cell animations to its
// surfaces through the renderer core, with a matrix stack of its own. The 2D surfaces share their affine matrices
// through a cache, which tracks, per level of the stack, whether the level's scale and rotation have been cached.
// The file name is a guess from the NNS_G2d*Renderer* names: the ROM has no string for it. So are the names of the
// statics and the static functions, and the matrix stack's and cache's layout, which SDK headers may have held as
// inlines

#define MTX_STACK_SIZE 32
#define MTX_CACHE_NUM 32
#define MTX_STACK_POS_NONE 0xfffe

// How a level of the matrix stack stands with the cache: scaled or rotated at this level, or by the level below,
// since the last cache. The levels set up together share a group ID
enum {
    MTXSTATE_NO_SR,
    MTXSTATE_SR_NEW,
    MTXSTATE_SR_INHERITED,
    MTXSTATE_CACHED,
};

typedef struct {
    u16 mtxCacheIdx;
    u16 groupID;
    u16 state;
    u16 pad;
} MtxCacheState;

static void SetIdentityMtx_(void);
static void SetMtxCache_(void);
static void RndCoreCBFuncBeforeCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell);
static void RndCoreCBFuncAfterCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell);
static void RndCoreCBFuncBeforeOam_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx);
static void RndCoreCBFuncAfterOam_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx);
static void DrawCellImpl_(const NNSG2dCellData *cell);
static void DrawCellAnimationImpl_(const NNSG2dCellAnimation *cellAnim);
static void DrawNode_(const NNSG2dNode *node);
static void DrawMCInstance_(const NNSG2dMultiCellInstance *instance);

// The stack level from which the matrix is scaled or rotated, the current cell transfer handle, and the matrix that
// NNS_G2D_RDR_OPZHINT_NOT_SR builds from a translation
static u16 mtxStackPosSR_ = MTX_STACK_POS_NONE;
static u32 currentCellVramTransferHandle_ = NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE;
static MtxFx32 mtxTransOnly_ = { FX32_ONE, 0, 0, FX32_ONE, 0, 0 };

// Declared in this order, MWCC lays them out as the original does
static u16 mtxCacheGroupID_;
static u16 mtxCacheCount_;
static NNSG2dRendererInstance *pCurrentInstance_;
static int mtxStackPos_;
static BOOL bNotSR_;
static fx32 zStack_[MTX_STACK_SIZE];
static MtxCacheState mtxCacheState_[MTX_STACK_SIZE];
// The matrices, and the same with the inverse scale, which the 2D engines' affine parameters take
static MtxFx32 mtxStackFor2DHW_[MTX_STACK_SIZE];
static MtxFx32 mtxStack_[MTX_STACK_SIZE];
static NNSG2dRndCore2DMtxCache mtxCache_[MTX_CACHE_NUM];

static inline const MtxFx32 *GetCurrentMtx_(void) {
    if (bNotSR_) {
        mtxTransOnly_._20 = mtxStack_[mtxStackPos_]._20;
        mtxTransOnly_._21 = mtxStack_[mtxStackPos_]._21;
        return &mtxTransOnly_;
    } else {
        return &mtxStack_[mtxStackPos_];
    }
}

static inline const MtxFx32 *GetCurrentMtxFor2DHW_(void) {
    if (bNotSR_) {
        mtxTransOnly_._20 = mtxStack_[mtxStackPos_]._20;
        mtxTransOnly_._21 = mtxStack_[mtxStackPos_]._21;
        return &mtxTransOnly_;
    } else {
        return &mtxStackFor2DHW_[mtxStackPos_];
    }
}

static inline MtxCacheState *GetMtxCacheState_(u16 pos) {
    return &mtxCacheState_[pos];
}

static inline NNSG2dRndCore2DMtxCache *GetCurrentMtxCache_(void) {
    return &mtxCache_[GetMtxCacheState_(mtxStackPos_)->mtxCacheIdx];
}

static inline void InitMtxCache_(NNSG2dRndCore2DMtxCache *cache) {
    MI_CpuFillFast(cache->affineIndex, 0xffffffff, sizeof(cache->affineIndex));
}

static inline u16 GetNewMtxCacheIdx_(void) {
    u16 idx = mtxCacheCount_;

    if (mtxCacheCount_ < MTX_CACHE_NUM - 1) {
        mtxCacheCount_++;
    }
    return idx;
}

static inline BOOL IsMtxCacheStateNotCached_(const MtxCacheState *state) {
    return state->state == MTXSTATE_SR_NEW || state->state == MTXSTATE_SR_INHERITED;
}

static inline u32 IsMtxSR_(void) {
    if (!bNotSR_ && mtxStackPosSR_ != MTX_STACK_POS_NONE) {
        return TRUE;
    }
    return FALSE;
}

static inline u32 IsVramTransferHandleValid_(u32 handle) {
    if (handle != NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE) {
        return TRUE;
    }
    return FALSE;
}

static inline u32 IsMtxStackEmpty_(void) {
    if (mtxStackPos_ <= 0) {
        return TRUE;
    }
    return FALSE;
}

// A new group of levels was scaled or rotated
static inline void SetMtxStateSR_(void) {
    MtxCacheState *state;

    if (!IsMtxSR_()) {
        mtxStackPosSR_ = mtxStackPos_;
    }
    state = GetMtxCacheState_(mtxStackPos_);
    switch (state->state) {
    case MTXSTATE_NO_SR:
    case MTXSTATE_SR_INHERITED:
    case MTXSTATE_CACHED:
        state->groupID = mtxCacheGroupID_++;
        state->state = MTXSTATE_SR_NEW;
        break;
    case MTXSTATE_SR_NEW:
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

static inline void ApplySRT_(const NNSG2dSRT *srt) {
    if (srt->SRT_EnableFlag & NNS_G2D_SRTFLAG_TRANS) {
        TranslateFlipped_(&srt->trans);
    }
    if (srt->SRT_EnableFlag & NNS_G2D_SRTFLAG_ROTZ) {
        NNS_G2dRotZ(FX_SinIdx(srt->rotZ), FX_CosIdx(srt->rotZ));
    }
    if (srt->SRT_EnableFlag & NNS_G2D_SRTFLAG_SCALE) {
        NNS_G2dScale(srt->scale.x, srt->scale.y, FX32_ONE);
    }
}

static inline void DrawCellToCore_(const NNSG2dCellData *cell) {
    if (IsVramTransferHandleValid_(currentCellVramTransferHandle_)) {
        NNS_G2dRndCoreDrawCellVramTransfer(cell, currentCellVramTransferHandle_);
    } else {
        NNS_G2dRndCoreDrawCell(cell);
    }
}

static inline void BeginRndCoreRendering2D_(NNSG2dRendererInstance *rend, NNSG2dRenderSurface *surface) {
    rend->pCurrentSurface = surface;
    NNS_G2dSetRndCoreSurface(&rend->rendererCore, surface);
    NNS_G2dSetRndCoreOamRegisterFunc(&rend->rendererCore, surface->pFuncOamRegister, surface->pFuncOamAffineRegister);
    NNS_G2dRndCoreBeginRendering(&rend->rendererCore);
}

static inline void BeginRndCoreRendering3D_(NNSG2dRendererInstance *rend, NNSG2dRenderSurface *surface) {
    rend->pCurrentSurface = surface;
    NNS_G2dSetRndCoreSurface(&rend->rendererCore, surface);
    NNS_G2dRndCoreBeginRendering(&rend->rendererCore);
}

static inline void EndRndCoreRendering_(void) {
    pCurrentInstance_->pCurrentSurface = NULL;
    NNS_G2dRndCoreEndRendering();
}

static void SetIdentityMtx_(void) {
    if (bNotSR_) {
        mtxStack_[mtxStackPos_]._20 = 0;
        mtxStack_[mtxStackPos_]._21 = 0;
    } else {
        MtxFx32 *mtx = &mtxStack_[mtxStackPos_];

        mtx->_00 = FX32_ONE;
        mtx->_01 = 0;
        mtx->_10 = 0;
        mtx->_11 = FX32_ONE;
        mtx->_20 = 0;
        mtx->_21 = 0;
        mtxStackFor2DHW_[mtxStackPos_] = *mtx;
    }
    zStack_[mtxStackPos_] = 0;
}

static inline void SetMtxCacheToGroup_(u16 pos, u16 groupID, u16 cacheIdx) {
    int i;

    for (i = pos; i >= 0; i--) {
        if (mtxCacheState_[i].groupID != groupID) {
            break;
        }
        mtxCacheState_[i].state = MTXSTATE_CACHED;
        mtxCacheState_[i].mtxCacheIdx = cacheIdx;
    }
}

// Gives the current level's scale and rotation a cache, shared with the levels below it of the same group
static void SetMtxCache_(void) {
    MtxCacheState *state = GetMtxCacheState_(mtxStackPos_);

    if (IsMtxCacheStateNotCached_(state)) {
        const u16 cacheIdx = GetNewMtxCacheIdx_();
        const u16 groupID = state->groupID;
        const MtxFx32 *mtx;
        NNSG2dRndCore2DMtxCache *cache;

        state->mtxCacheIdx = cacheIdx;
        InitMtxCache_(&mtxCache_[cacheIdx]);
        mtx = GetCurrentMtxFor2DHW_();
        cache = GetCurrentMtxCache_();
        cache->m22._00 = mtx->_00;
        cache->m22._01 = mtx->_01;
        cache->m22._10 = mtx->_10;
        cache->m22._11 = mtx->_11;
        SetMtxCacheToGroup_(mtxStackPos_, groupID, cacheIdx);
    }
}

static void RndCoreCBFuncBeforeCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell) {
    NNSG2dRenderSurface *surface = pCurrentInstance_->pCurrentSurface;

    if (surface->pFuncVisibilityCulling != NULL) {
        if (!surface->pFuncVisibilityCulling(cell, GetCurrentMtx_(), &surface->viewRect)) {
            core->bDrawEnable = FALSE;
            return;
        }
        core->bDrawEnable = TRUE;
    }
    if (surface->pBeforeDrawCellBackFunc != NULL) {
        surface->pBeforeDrawCellBackFunc(pCurrentInstance_, surface, cell, GetCurrentMtx_());
    }
}

static void RndCoreCBFuncAfterCell_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell) {
    NNSG2dRenderSurface *surface = pCurrentInstance_->pCurrentSurface;

    if (surface->pAfterDrawCellBackFunc != NULL) {
        surface->pAfterDrawCellBackFunc(pCurrentInstance_, surface, cell, GetCurrentMtx_());
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

static void RndCoreCBFuncBeforeOam_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx) {
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
            surface->pBeforeDrawOamBackFunc(rend, surface, cell, oamIdx, GetCurrentMtx_());
        }
    }
}

static void RndCoreCBFuncAfterOam_(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx) {
    NNSG2dRendererInstance *rend = pCurrentInstance_;
    NNSG2dRenderSurface *surface = rend->pCurrentSurface;

    if (surface->pAfterDrawOamBackFunc != NULL) {
        surface->pAfterDrawOamBackFunc(rend, surface, cell, oamIdx, GetCurrentMtx_());
    }
}

static inline void DrawCellToSurface2D_(const NNSG2dCellData *cell) {
    NNSG2dRndCore2DMtxCache *cache = NULL;

    if (IsMtxSR_()) {
        if (NNSi_G2dMCRenderState.bDrawMC) {
            cache = NNSi_G2dMCRenderState.cellAnimMtxCache[NNSi_G2dMCRenderState.currentCellAnimIdx];
            if (cache == NULL) {
                SetMtxCache_();
                cache = GetCurrentMtxCache_();
                NNSi_G2dMCRenderState.cellAnimMtxCache[NNSi_G2dMCRenderState.currentCellAnimIdx] = cache;
            }
        } else {
            SetMtxCache_();
            cache = GetCurrentMtxCache_();
        }
    }
    NNS_G2dSetRndCoreCurrentMtx2D(GetCurrentMtx_(), cache);
    DrawCellToCore_(cell);
}

static inline void DrawCellToSurface3D_(NNSG2dRendererInstance *rend, const NNSG2dCellData *cell) {
    NNS_G2dSetRndCore3DSoftSpriteZvalue(&rend->rendererCore, zStack_[mtxStackPos_]);
    NNS_G2dSetRndCoreCurrentMtx3D(GetCurrentMtx_());
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

    if (cellAnim->srtCtrl.data.srtData.SRT_EnableFlag == NNS_G2D_SRTFLAG_IDENTITY) {
        if (IsVramTransferHandleValid_(cellAnim->cellTransferStateHandle)) {
            currentCellVramTransferHandle_ = cellAnim->cellTransferStateHandle;
            DrawCellImpl_(cell);
            currentCellVramTransferHandle_ = NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE;
        } else {
            DrawCellImpl_(cell);
        }
    } else {
        NNS_G2dPushMtx();
        ApplySRT_(&cellAnim->srtCtrl.data.srtData);
        if (IsVramTransferHandleValid_(cellAnim->cellTransferStateHandle)) {
            currentCellVramTransferHandle_ = cellAnim->cellTransferStateHandle;
            DrawCellImpl_(cell);
            currentCellVramTransferHandle_ = NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE;
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
        ApplySRT_(&node->srtCtrl.data.srtData);
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

    mtxStackPos_ = 0;
    mtxStackPosSR_ = MTX_STACK_POS_NONE;
    for (i = 0; i < MTX_CACHE_NUM; i++) {
        InitMtxCache_(&mtxCache_[i]);
    }
    mtxCacheCount_ = 0;
    mtxCacheGroupID_ = 0;
    MI_CpuClearFast(mtxCacheState_, sizeof(mtxCacheState_));
    bNotSR_ = FALSE;
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
    surface->pBeforeDrawOamBackFuncCore = RndCoreCBFuncBeforeOam_;
    surface->pAfterDrawOamBackFuncCore = RndCoreCBFuncAfterOam_;
}

void NNS_G2dBeginRendering(NNSG2dRendererInstance *rend) {
    int i;

    pCurrentInstance_ = rend;
    mtxStackPos_ = 0;
    mtxStackPosSR_ = MTX_STACK_POS_NONE;
    for (i = 0; i < mtxCacheCount_; i++) {
        InitMtxCache_(&mtxCache_[i]);
    }
    mtxCacheCount_ = 0;
    mtxCacheGroupID_ = 0;
    MI_CpuClearFast(mtxCacheState_, sizeof(mtxCacheState_));
    G3_PushMtx();
    G3_Identity();
    SetIdentityMtx_();
}

void NNS_G2dBeginRenderingEx(NNSG2dRendererInstance *rend, u32 opzHint) {
    rend->opzHint = opzHint;
    if (opzHint & NNS_G2D_RDR_OPZHINT_NOT_SR) {
        bNotSR_ = TRUE;
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
            bNotSR_ = FALSE;
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

static inline u16 GetMCNodeCellAnimIdx_(const NNSG2dMultiCellHierarchyData *node) {
    return (node->nodeAttr & 0xff00) >> 8;
}

static inline void DrawMCNodeShared_(const NNSG2dMultiCellInstance *instance, u16 nodeIdx) {
    const NNSG2dMultiCellHierarchyData *node = &instance->pCurrentMultiCell->pHierDataArray[nodeIdx];
    const u16 cellAnimIdx = GetMCNodeCellAnimIdx_(node);
    NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimArray;

    NNSi_G2dMCRenderState.currentCellAnimIdx = cellAnimIdx;
    NNS_G2dPushMtx();
    TranslateFlipped_(&node->pos);
    DrawCellAnimationImpl_(&cellAnims[cellAnimIdx].cellAnim);
    NNS_G2dPopMtx(1);
}

static void DrawMCInstance_(const NNSG2dMultiCellInstance *instance) {
    u16 i;

    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        for (i = 0; i < instance->pCurrentMultiCell->numCellAnim; i++) {
            NNSi_G2dMCRenderState.cellAnimMtxCache[i] = NULL;
        }
        NNSi_G2dMCRenderState.bDrawMC = TRUE;
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
        NNSi_G2dMCRenderState.bDrawMC = FALSE;
    } else {
        const NNSG2dNode *nodes = instance->pCellAnimArray;

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
    if (mcAnim->srtCtrl.data.srtData.SRT_EnableFlag == NNS_G2D_SRTFLAG_IDENTITY) {
        DrawMCInstance_(&mcAnim->multiCellInstance);
    } else {
        NNS_G2dPushMtx();
        ApplySRT_(&mcAnim->srtCtrl.data.srtData);
        DrawMCInstance_(&mcAnim->multiCellInstance);
        NNS_G2dPopMtx(1);
    }
}

static inline u32 CanPushMtx_(int pos) {
    if (pos < MTX_STACK_SIZE) {
        return TRUE;
    }
    return FALSE;
}

static inline void PushMtxStack_(void) {
    const int next = mtxStackPos_ + 1;

    if (CanPushMtx_(next)) {
        if (bNotSR_) {
            mtxStack_[next]._20 = mtxStack_[mtxStackPos_]._20;
            mtxStack_[next]._21 = mtxStack_[mtxStackPos_]._21;
            zStack_[next] = zStack_[mtxStackPos_];
        } else {
            mtxStack_[next] = mtxStack_[mtxStackPos_];
            mtxStackFor2DHW_[next] = mtxStackFor2DHW_[mtxStackPos_];
            zStack_[next] = zStack_[mtxStackPos_];
        }
        mtxStackPos_ = next;
    }
}

void NNS_G2dPushMtx(void) {
    if (!(pCurrentInstance_->opzHint & NNS_G2D_RDR_OPZHINT_NOT_SR)) {
        const u16 lastPos = mtxStackPos_;

        PushMtxStack_();
        mtxCacheState_[(u16)mtxStackPos_] = mtxCacheState_[lastPos];
        if (mtxCacheState_[lastPos].state == MTXSTATE_SR_NEW) {
            mtxCacheState_[(u16)mtxStackPos_].state = MTXSTATE_SR_INHERITED;
        } else {
            mtxCacheState_[(u16)mtxStackPos_].state = mtxCacheState_[lastPos].state;
        }
    } else {
        PushMtxStack_();
    }
}

void NNS_G2dPopMtx(u16 num) {
    if (!IsMtxStackEmpty_()) {
        mtxStackPos_--;
        if (mtxStackPosSR_ > mtxStackPos_) {
            mtxStackPosSR_ = MTX_STACK_POS_NONE;
        }
    }
}

void NNS_G2dTranslate(fx32 x, fx32 y, fx32 z) {
    if (bNotSR_) {
        mtxStack_[mtxStackPos_]._20 += x;
        mtxStack_[mtxStackPos_]._21 += y;
        zStack_[mtxStackPos_] += z;
    } else {
        MtxFx32 mtx;

        mtx._00 = FX32_ONE;
        mtx._01 = 0;
        mtx._10 = 0;
        mtx._11 = FX32_ONE;
        mtx._20 = x;
        mtx._21 = y;
        MAT32_Mul(&mtx, &mtxStack_[mtxStackPos_], &mtxStack_[mtxStackPos_]);
        MAT32_Mul(&mtx, &mtxStackFor2DHW_[mtxStackPos_], &mtxStackFor2DHW_[mtxStackPos_]);
        zStack_[mtxStackPos_] += z;
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
    MAT32_Mul(&mtx, &mtxStack_[mtxStackPos_], &mtxStack_[mtxStackPos_]);
    mtx._00 = FX_Inv(x);
    mtx._01 = 0;
    mtx._10 = 0;
    mtx._11 = FX_Inv(y);
    mtx._20 = 0;
    mtx._21 = 0;
    MAT32_Mul(&mtx, &mtxStackFor2DHW_[mtxStackPos_], &mtxStackFor2DHW_[mtxStackPos_]);
    SetMtxStateSR_();
}

void NNS_G2dRotZ(fx32 sin, fx32 cos) {
    MtxFx32 mtx;

    mtx._00 = cos;
    mtx._01 = sin;
    mtx._10 = -sin;
    mtx._11 = cos;
    mtx._20 = 0;
    mtx._21 = 0;
    MAT32_Mul(&mtx, &mtxStack_[mtxStackPos_], &mtxStack_[mtxStackPos_]);
    MAT32_Mul(&mtx, &mtxStackFor2DHW_[mtxStackPos_], &mtxStackFor2DHW_[mtxStackPos_]);
    SetMtxStateSR_();
}

const NNSG2dPaletteSwapTable *NNS_G2dGetRendererPaletteTbl(const NNSG2dRendererInstance *rend) {
    return rend->pPaletteSwapTbl;
}

void NNS_G2dSetRendererImageProxy(NNSG2dRendererInstance *rend, const NNSG2dImageProxy *imgProxy,
                                  const NNSG2dImagePaletteProxy *pltProxy) {
    NNS_G2dSetRndCoreImageProxy(&rend->rendererCore, imgProxy, pltProxy);
}
