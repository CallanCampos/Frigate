#include "types.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/clact_itcm.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/vman.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"

// The OAM managers, renderer, units of actors and resources of the cell actor system

// A resource that is not loaded
#define CLACT_RES_NONE 0xffffffff
// Positions that are not relative to a surface of the renderer
#define CLACT_SURFACE_NONE 0xffff

struct ClActCharRes {
    NNSG2dImageProxy proxy;
    void *data;
    // The characters a VRAM transfer cell animation sends from
    NNSG2dCharacterData *transferChar;
    VRAMAlloc allocMain;
    VRAMAlloc allocSub;
    // Whether data is someone else's, which freeing the resource leaves
    u32 extData : 1;
    u32 size : 30;
    u32 free : 1;
};

struct ClActPlttRes {
    NNSG2dImagePaletteProxy proxy;
    u32 size : 31;
    u32 free : 1;
};

struct ClActCellAnimRes {
    void *cellFile;
    void *animFile;
    NNSG2dCellDataBank *cells;
    NNSG2dAnimBankData *anims;
    BOOL free;
};

typedef struct {
    u16 charCount;
    u16 plttCount;
    u16 cellAnimCount;
    u16 unk6;
    // Where in OBJ VRAM the resources go, in 32-byte units
    u16 offsetMain;
    u16 offsetSub;
} ClActResManSetup;

typedef struct {
    NNSG2dOamRegisterFunction oamRegister;
    NNSG2dAffineRegisterFunction affineRegister;
    NNSG2dRndCellCullingFunction culling;
} ClActSurfaceFuncs;

// A transfer of a VRAM transfer cell animation's characters, at the vertical blank
struct ClActTransferTask {
    void *src;
    u32 dst;
    u32 size;
    u32 screen;
};

// The resources an actor is made with
typedef struct {
    u16 charIdx;
    u16 plttIdx;
    ClActCharRes *chars;
    ClActPlttRes *pltt;
    NNSG2dCellDataBank *cells;
    NNSG2dAnimBankData *anims;
    NNSG2dMultiCellDataBank *mcBank;
    NNSG2dAnimBankData *mcAnims;
    NNSG2dCharacterData *transferChar;
} ClActResSetup;

// What the renderers pass the culling callback of a surface for each OAM, which sets skip when it is out of view
typedef struct {
    u8 unk0[0x30];
    BOOL skip;
    u32 unk34;
    GXOamAttr oam;
} ClActDrawState;

ClActSys *g_ClActSys;

void ClActRenderer_InitForSystem(ClActRenderer *renderer, const ClActSysSetup *setup, HeapID heapId);
static void ClActSys_SetUpdating(BOOL updating);
static BOOL ClActSys_IsUpdating(void);
static void ClActSys_SetOamReady(BOOL ready);
static BOOL ClActSys_IsOamReady(void);
static void ClActSys_ResetOamBuffersIfReady(void);
static void ClActSys_DrawUnits(void);
static void ClActUnit_Link(ClActUnit *unit);
static void ClActUnit_Unlink(ClActUnit *unit);
static void ClActOamMan_InitAll(ClActOamMan *mans, u8 fromMain, u8 numMain, u8 fromSub, u8 numSub);
static void ClActOamMan_ShutdownAll(ClActOamMan *mans);
static void ClActOamMan_ApplyAll(ClActOamMan *mans);
static void ClActOamMan_ResetAll(ClActOamMan *mans);
static void ClActOamMan_Init(ClActOamMan *man, u8 from, u8 num, u32 type);
static void ClActOamMan_Apply(ClActOamMan *man);
static void ClActOamMan_Reset(ClActOamMan *man);
static void ClActRenderer_Init(ClActRenderer *renderer, const ClActSurfaceSetup *setups, u16 count, HeapID heapId);
static void ClActRenderer_Free(ClActRenderer *renderer);
static void ClActRenderer_SetCullCallback(ClActRenderer *renderer, BOOL cull);
static void ClActSurface_Init(NNSG2dRenderSurface *surface, const ClActSurfaceSetup *setup);
static void ClActSurface_SetType(NNSG2dRenderSurface *surface, u32 screen);
static BOOL ClActOamMan_EntryOam(ClActOamMan *man, const GXOamAttr *oam, u16 affineIdx, u16 palette, u32 mode);
static BOOL ClActOam_EntryMain(const GXOamAttr *oam, u16 affineIdx, BOOL doubleAffine);
static BOOL ClActOam_EntrySub(const GXOamAttr *oam, u16 affineIdx, BOOL doubleAffine);
static u16 ClActAffine_EntryMain(const MtxFx22 *mtx);
static u16 ClActAffine_EntrySub(const MtxFx22 *mtx);
static BOOL ClAct_IsCellInViewTransformed(const NNSG2dCellData *cell, const MtxFx32 *mtx, const NNSG2dViewRect *view);
static BOOL ClAct_IsCellInView(const NNSG2dCellData *cell, const MtxFx32 *mtx, const NNSG2dViewRect *view);
static void ClAct_CullOam(ClActDrawState *state, const NNSG2dViewRect *view, u32 a2, u32 a3, const MtxFx32 *mtx);
static BOOL ClAct_IsRectInView(fx32 posX, fx32 posY, fx32 left, fx32 right, fx32 top, fx32 bottom, const MtxFx32 *mtx,
                               const NNSG2dViewRect *view);
static void ClActTransfer_Init(ClActTransfer *transfer, u8 count, HeapID heapId);
static void ClActTransfer_Free(ClActTransfer *transfer);
static void ClActTransfer_AllocTasks(ClActTransfer *transfer, u32 count, HeapID heapId);
static void ClActTransfer_FreeTasks(ClActTransfer *transfer);
static void ClActTransfer_RunTasks(ClActTransfer *transfer);
static ClActTransferTask *ClActTransfer_FindFreeTask(ClActTransfer *transfer);
static BOOL ClActTransfer_Request(NNS_GFD_DST_TYPE type, u32 dstAddr, void *src, u32 size);
static void ClActTransferTask_Set(ClActTransferTask *task, u32 screen, u32 dst, void *src, u32 size);
static void ClActTransferTask_Run(ClActTransferTask *task);
static ClActor *ClActUnit_FindFreeActor(ClActUnit *unit);
static void ClActUnit_InsertActor(ClActUnit *unit, ClActor *actor);
static void ClActUnit_RemoveActor(ClActUnit *unit, ClActor *actor);
static ClActor *ClActUnit_FindInsertPosFromHead(ClActor *head, u8 priority);
static ClActor *ClActUnit_FindInsertPosFromTail(ClActor *tail, u8 priority);
static void ClActor_Reset(ClActor *actor);
static void ClActor_ApplySetup(ClActor *actor, const ClActorSetup *setup, u16 surface);
static void ClActor_ApplyRes(ClActor *actor, const ClActResSetup *res);
static void ClActor_GetSurfaceOffset(ClActor *actor, u32 surface, ClActorPos *offset);
static void ClActResSetup_Init(ClActResSetup *res, u16 charIdx, u16 plttIdx, ClActCharRes *chars, ClActPlttRes *pltt,
                               NNSG2dCellDataBank *cells, NNSG2dAnimBankData *anims);
static ClActor *ClActor_CreateFromRes(ClActUnit *unit, const ClActorSetup *setup, const ClActResSetup *res, u16 surface,
                                      HeapID heapId);
static ClActor *ClActor_CreateExFromRes(ClActUnit *unit, const ClActorSetupEx *setup, const ClActResSetup *res,
                                        u16 surface, HeapID heapId);
static void ClActAnim_Init(ClActAnim *anim, const ClActResSetup *res, HeapID heapId);
static void ClActAnim_Free(ClActAnim *anim);
static u32 ClActAnim_GetType(const ClActResSetup *res);
static void ClActAnim_InitCell(ClActAnim *anim, const ClActResSetup *res, HeapID heapId);
static void ClActAnim_InitVramTransfer(ClActAnim *anim, const ClActResSetup *res, HeapID heapId);
static void ClActAnim_InitMultiCell(ClActAnim *anim, const ClActResSetup *res, HeapID heapId);
static void ClActAnim_FreeCell(ClActAnim *anim);
static void ClActAnim_FreeVramTransfer(ClActAnim *anim);
static void ClActAnim_FreeMultiCell(ClActAnim *anim);
static void ClActAnim_SetSequence(ClActAnim *anim, u16 sequence);
static void ClActAnim_SetSequenceCell(ClActAnim *anim, u16 sequence);
static void ClActAnim_SetSequenceVramTransfer(ClActAnim *anim, u16 sequence);
static void setSeqToMultiCellAnim(ClActAnim *anim, u16 sequence);
static void ClActAnim_SetTime(ClActAnim *anim, fx32 time);
static void ClActAnim_TickCell(ClActAnim *anim, fx32 frames);
static void ClActAnim_TickVramTransfer(ClActAnim *anim, fx32 frames);
static void ClActAnim_TickMultiCell(ClActAnim *anim, fx32 frames);
static NNSG2dAnimController *ClActAnim_GetCtrl(ClActAnim *anim);
static NNSG2dAnimController *ClActAnim_GetCtrlCell(ClActAnim *anim);
static NNSG2dAnimController *ClActAnim_GetCtrlVramTransfer(ClActAnim *anim);
static NNSG2dAnimController *ClActAnim_GetCtrlMultiCell(ClActAnim *anim);
static NNSG2dAnimController *ClActAnim_GetCtrl2(ClActAnim *anim);
static NNSG2dAnimController *ClActAnim_GetCtrl2Cell(ClActAnim *anim);
static NNSG2dAnimController *ClActAnim_GetCtrl2VramTransfer(ClActAnim *anim);
static NNSG2dAnimController *ClActAnim_GetCtrl2MultiCell(ClActAnim *anim);
static void ClActAnim_SetFrame(ClActAnim *anim, u16 frame);
static void ClActAnim_SetFrameCell(ClActAnim *anim, u16 frame);
static void ClActAnim_SetFrameVramTransfer(ClActAnim *anim, u16 frame);
static void ClActAnim_SetFrameMultiCell(ClActAnim *anim, u16 frame);
static void ClActAnim_SetSpeedCell(ClActAnim *anim, fx32 speed);
static void ClActAnim_SetSpeedVramTransfer(ClActAnim *anim, fx32 speed);
static void ClActAnim_SetSpeedMultiCell(ClActAnim *anim, fx32 speed);
static void ClActAnim_Restart(ClActAnim *anim);
static void ClActAnim_RestartCell(ClActAnim *anim);
static void ClActAnim_RestartVramTransfer(ClActAnim *anim);
static void ClActAnim_RestartMultiCell(ClActAnim *anim);
static const NNSG2dUserExAnimSequenceAttr *ClActAnim_GetUserExSeqAttrCell(ClActAnim *anim, u32 sequence);
static const NNSG2dUserExAnimSequenceAttr *ClActAnim_GetUserExSeqAttrVramTransfer(ClActAnim *anim, u32 sequence);
static const NNSG2dUserExAnimSequenceAttr *ClActAnim_GetUserExSeqAttrMultiCell(ClActAnim *anim, u32 sequence);
static const NNSG2dUserExCellAttrBank *ClActAnim_GetUserExCellAttrBankCell(ClActAnim *anim);
static const NNSG2dUserExCellAttrBank *ClActAnim_GetUserExCellAttrBankVramTransfer(ClActAnim *anim);
static const NNSG2dUserExCellAttrBank *ClActAnim_GetUserExCellAttrBankMultiCell(ClActAnim *anim);
static u32 ClActAnim_GetSequenceCount(ClActAnim *anim);
static u32 ClActAnim_GetSequenceCountCell(ClActAnim *anim);
static u32 ClActAnim_GetSequenceCountVramTransfer(ClActAnim *anim);
static u32 ClActAnim_GetSequenceCountMultiCell(ClActAnim *anim);
static void ClActAnim_SetCallback(ClActAnim *anim, const ClActorCallback *callback);
static void ClActAnim_ClearCallback(ClActAnim *anim);
static void ClActVRAMManager_Init(ClActResMan *man, HeapID heapId, const BGSysVRAMConfig *vramConfig,
                                  const ClActResManSetup *setup);
static void ClActResMan_Free(ClActResMan *man);
static BOOL ClAct_VramTypeIncludes(u32 vramType, u32 which);
static void *ClAct_ReadCharFile(ArcTool *arc, u32 fileId, BOOL compressed, HeapID heapId);
static ClActCharRes *ClActCharRes_AllocArray(HeapID heapId, u32 count);
static void ClActResMan_SetupChars(ClActResMan *man, u32 idx, NNSG2dCharacterData *charData,
                                   NNSG2dCellDataBank *transferCells, u32 vramType);
static void ClActCharRes_FreeArray(ClActCharRes *chars);
static void ClAct_SetCharMapping(NNSG2dCharacterData *charData, NNSG2dVRamType type);
static ClActPlttRes *ClActPlttRes_AllocArray(HeapID heapId, u32 count);
static void ClActResMan_SetupPlttRange(ClActResMan *man, u32 idx, void *file, u32 vramType, u16 offset, u16 start,
                                       u16 count);
static void ClActResMan_SetupPlttEx(ClActResMan *man, u32 idx, void *file, u32 vramType, u16 offset);
static void ClActPlttRes_FreeArray(ClActPlttRes *pltts);
static ClActCellAnimRes *ClActCellAnimRes_AllocArray(HeapID heapId, u32 count);
static BOOL ClActResMan_SetupCellAnims(ClActResMan *man, u32 idx, void *cellFile, void *animFile);
static void ClActCellAnimRes_FreeArray(ClActCellAnimRes *cellAnims);
static void ClActResMan_SetupCharsFromFile(ClActResMan *man, u32 idx, void *file, u32 vramType,
                                           NNSG2dCellDataBank *transferCells);

// MWCC sorts a file's data by size, in an order that depends on where each object is declared; these are declared in
// the order that lays them out as in the ROM
static const u16 sSurfaceTypes[2] = { NNS_G2D_SURFACETYPE_MAIN2D, NNS_G2D_SURFACETYPE_SUB2D };
// Unused
const u16 data_02093e44[2] = { NNS_G2D_VRAM_TYPE_2DMAIN, NNS_G2D_VRAM_TYPE_2DSUB };
static void (*const sTransferFuncs[2])(const void *src, u32 offset, u32 size) = {
    GX_LoadOBJ,
    GXS_LoadOBJ,
};
// Global, as only functions that are never compiled read them
const NNSG2dUserExAnimSequenceAttr *(*const gClActAnimGetSeqAttrFuncs[3])(ClActAnim *anim, u32 sequence) = {
    ClActAnim_GetUserExSeqAttrCell,
    ClActAnim_GetUserExSeqAttrVramTransfer,
    ClActAnim_GetUserExSeqAttrMultiCell,
};
static NNSG2dAnimController *(*const sAnimGetCtrlFuncs[3])(ClActAnim *anim) = {
    ClActAnim_GetCtrl2Cell,
    ClActAnim_GetCtrl2VramTransfer,
    ClActAnim_GetCtrl2MultiCell,
};
static void (*const sAnimFreeFuncs[3])(ClActAnim *anim) = {
    ClActAnim_FreeCell,
    ClActAnim_FreeVramTransfer,
    ClActAnim_FreeMultiCell,
};
static const NNSG2dRndCellCullingFunction sCullingFuncs[3] = {
    ClAct_IsCellInViewTransformed,
    ClAct_IsCellInView,
    NULL,
};
static NNSG2dAnimController *(*const sAnimGetCtrlFuncs2[3])(ClActAnim *anim) = {
    ClActAnim_GetCtrlCell,
    ClActAnim_GetCtrlVramTransfer,
    ClActAnim_GetCtrlMultiCell,
};
const NNSG2dUserExCellAttrBank *(*const gClActAnimGetCellAttrFuncs[3])(ClActAnim *anim) = {
    ClActAnim_GetUserExCellAttrBankCell,
    ClActAnim_GetUserExCellAttrBankVramTransfer,
    ClActAnim_GetUserExCellAttrBankMultiCell,
};
static void (*const sAnimSetFrameFuncs[3])(ClActAnim *anim, u16 frame) = {
    ClActAnim_SetFrameCell,
    ClActAnim_SetFrameVramTransfer,
    ClActAnim_SetFrameMultiCell,
};
static void (*const sAnimInitFuncs[3])(ClActAnim *anim, const ClActResSetup *res, HeapID heapId) = {
    ClActAnim_InitCell,
    ClActAnim_InitVramTransfer,
    ClActAnim_InitMultiCell,
};
static u32 (*const sAnimGetSequenceCountFuncs[3])(ClActAnim *anim) = {
    ClActAnim_GetSequenceCountCell,
    ClActAnim_GetSequenceCountVramTransfer,
    ClActAnim_GetSequenceCountMultiCell,
};
static void (*const sAnimSetSequenceFuncs[3])(ClActAnim *anim, u16 sequence) = {
    ClActAnim_SetSequenceCell,
    ClActAnim_SetSequenceVramTransfer,
    setSeqToMultiCellAnim,
};
static const ClActSurfaceFuncs sSurfaceFuncs[2] = {
    { ClActOam_EntryMain, ClActAffine_EntryMain, NULL },
    { ClActOam_EntrySub, ClActAffine_EntrySub, NULL },
};
static const NNSG2dCharacterData sCharDataTemplate = { 0xffff, 0xffff, GX_TEXFMT_PLTT16, 0, 0, 0, NULL };
static void (*const sAnimTickFuncs[3])(ClActAnim *anim, fx32 frames) = {
    ClActAnim_TickCell,
    ClActAnim_TickVramTransfer,
    ClActAnim_TickMultiCell,
};
const ClActSysSetup data_02093f08 = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
static void (*sAnimSetSpeedFuncs[3])(ClActAnim *anim, fx32 speed) = {
    ClActAnim_SetSpeedCell,
    ClActAnim_SetSpeedVramTransfer,
    ClActAnim_SetSpeedMultiCell,
};
static void (*sAnimRestartFuncs[3])(ClActAnim *anim) = {
    ClActAnim_RestartCell,
    ClActAnim_RestartVramTransfer,
    ClActAnim_RestartMultiCell,
};
const ClActSysSetup data_02093f24 = { 0, 0, 0, 192, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
static void (*renderers[4])(ClActUnit *unit) = {
    mainRenderer,
    visibleAnimRenderer,
    sharedParamsRenderer,
    sharedParamsVisibleAnimRenderer,
};
// The surfaces of the system's renderer, at the positions ClActSys_Create is given
static ClActSurfaceSetup sSurfaceSetups[2] = {
    { 0, 0, 256, 192, 0, 0 },
    { 0, 0, 256, 192, 1, 0 },
};

static inline void ClAct_SetVec(NNSG2dFVec2 *vec, s16 x, s16 y) {
    vec->x = x << FX32_SHIFT;
    vec->y = y << FX32_SHIFT;
}

static inline void ClAct_RunTransfers(ClActTransfer *transfer) {
    if (transfer->active) {
        ClActTransfer_RunTasks(transfer);
    }
}

// The most characters a VRAM transfer cell animation of the bank sends at once
static inline u32 ClAct_GetTransferSize(const NNSG2dCellDataBank *bank) {
    BOOL has = TRUE;
    const NNSG2dVramTransferData *data = bank->pVramTransferData;

    if (data == NULL) {
        has = FALSE;
    }
    return has ? data->szByteMax : 0;
}

static inline u32 ClAct_FindFreeChar(const ClActResMan *man) {
    u32 i;

    for (i = 0; i < man->charCount; i++) {
        if (man->chars[i].free) {
            return i;
        }
    }
    return CLACT_RES_NONE;
}

static inline u32 ClAct_FindFreePltt(const ClActResMan *man) {
    u32 i;

    for (i = 0; i < man->plttCount; i++) {
        if (man->pltts[i].free) {
            return i;
        }
    }
    return CLACT_RES_NONE;
}

static inline u32 ClAct_FindFreeCellAnim(const ClActResMan *man) {
    u32 i;

    for (i = 0; i < man->cellAnimCount; i++) {
        if (man->cellAnims[i].free) {
            return i;
        }
    }
    return CLACT_RES_NONE;
}

void ClActSys_Create(const ClActSysSetup *setup, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    ClActResManSetup resSetup;
    int i = 0;

    g_ClActSys = GFL_HeapAllocate(heapId, sizeof(ClActSys), FALSE, "clact.c", 671);
    MI_CpuFill8(g_ClActSys, 0, sizeof(ClActSys));
    resSetup.charCount = setup->charCount;
    resSetup.plttCount = setup->plttCount;
    resSetup.cellAnimCount = setup->cellAnimCount;
    resSetup.unk6 = setup->unk16;
    resSetup.offsetMain = setup->charOffsetMain;
    resSetup.offsetSub = setup->charOffsetSub;
    ClActVRAMManager_Init(&g_ClActSys->res, heapId, vramConfig, &resSetup);
    ClActOamMan_InitAll(g_ClActSys->oamMans, setup->oamStartMain, setup->oamCountMain, setup->oamStartSub,
                        setup->oamCountSub);
    ClActRenderer_InitForSystem(&g_ClActSys->renderer, setup, heapId);
    ClActTransfer_Init(&g_ClActSys->transfer, setup->transferCount, heapId);
    for (; i < 2; i++) {
        g_ClActSys->palettes[i] = 0;
    }
    ClActSys_SetUpdating(TRUE);
    ClActSys_SetOamReady(TRUE);
}

void ClActSys_Delete(void) {
    ClActSys_SetUpdating(FALSE);
    ClActOamMan_ShutdownAll(g_ClActSys->oamMans);
    ClActRenderer_Free(&g_ClActSys->renderer);
    ClActTransfer_Free(&g_ClActSys->transfer);
    ClActResMan_Free(&g_ClActSys->res);
    GFL_HeapFree(g_ClActSys);
    g_ClActSys = NULL;
}

void ClActSys_Draw(void) {
    if (g_ClActSys != NULL) {
        ClActSys_SetUpdating(FALSE);
        ClActSys_ResetOamBuffersIfReady();
        ClActSys_DrawUnits();
        if (g_ClActSys->transfer.active) {
            NNS_G2dUpdateCellTransferStateManager();
        }
        ClActSys_SetUpdating(TRUE);
    }
}

void ClActSys_VBlank(void) {
    if (g_ClActSys != NULL && ClActSys_IsUpdating()) {
        ClActOamMan_ApplyAll(g_ClActSys->oamMans);
        ClAct_RunTransfers(&g_ClActSys->transfer);
        ClActSys_SetOamReady(TRUE);
    }
}

void ClActSys_ResetOamBuffers(void) {
    if (g_ClActSys != NULL) {
        ClActSys_SetUpdating(FALSE);
        ClActSys_ResetOamBuffersIfReady();
        ClActSys_SetUpdating(TRUE);
    }
}

u32 ClActSys_LoadChars(ArcTool *arc, u32 fileId, BOOL compressed, u32 vramType, HeapID heapId) {
    u32 idx = ClAct_FindFreeChar(&g_ClActSys->res);

    if (idx != CLACT_RES_NONE) {
        ClActCharRes *res = &g_ClActSys->res.chars[idx];
        void *data = ClAct_ReadCharFile(arc, fileId, compressed, HEAPID_TAIL(heapId));

        ClActResMan_SetupCharsFromFile(&g_ClActSys->res, idx, data, vramType, NULL);
        GFL_HeapFree(data);
        res->data = NULL;
        res->extData = FALSE;
        return idx;
    }
    return CLACT_RES_NONE;
}

u32 ClActSys_AllocChars(u32 size, u32 vramType, HeapID heapId) {
    u32 idx = ClAct_FindFreeChar(&g_ClActSys->res);
    ClActCharRes *res;
    NNSG2dCharacterData charData;

    if (idx == CLACT_RES_NONE) {
        return CLACT_RES_NONE;
    }
    res = &g_ClActSys->res.chars[idx];
    charData = sCharDataTemplate;
    charData.mappingType = GX_GetOBJVRamModeChar();
    charData.size = size;
    charData.rawData = GFL_HeapAllocate(HEAPID_TAIL(heapId), size, TRUE, "clact.c", 992);
    ClActResMan_SetupChars(&g_ClActSys->res, idx, &charData, NULL, vramType);
    res->free = FALSE;
    res->data = NULL;
    res->extData = FALSE;
    GFL_HeapFree(charData.rawData);
    return idx;
}

void ClActSys_FreeChars(u32 chars) {
    if (!GFL_VRAMManagerAllocIsInvalid(&g_ClActSys->res.chars[chars].allocMain)) {
        GFL_VRAMManagerDeAlloc(g_ClActSys->res.vmanMain, &g_ClActSys->res.chars[chars].allocMain);
    }
    if (!GFL_VRAMManagerAllocIsInvalid(&g_ClActSys->res.chars[chars].allocSub)) {
        GFL_VRAMManagerDeAlloc(g_ClActSys->res.vmanSub, &g_ClActSys->res.chars[chars].allocSub);
    }
    if (g_ClActSys->res.chars[chars].data != NULL) {
        if (g_ClActSys->res.chars[chars].extData == FALSE) {
            GFL_HeapFree(g_ClActSys->res.chars[chars].data);
        } else {
            g_ClActSys->res.chars[chars].extData = FALSE;
        }
        g_ClActSys->res.chars[chars].data = NULL;
    }
    g_ClActSys->res.chars[chars].free = TRUE;
    g_ClActSys->res.chars[chars].size = 0;
}

void ClActSys_ReplaceCharData(u32 chars, NNSG2dCharacterData *character) {
    if (NNS_G2dIsImageReadyToUse(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DMAIN)) {
        u32 addr = NNS_G2dGetImageLocation(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DMAIN);

        DC_FlushRange(character->rawData, character->size);
        GX_LoadOBJ(character->rawData, addr, character->size);
    }
    if (NNS_G2dIsImageReadyToUse(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DSUB)) {
        u32 addr = NNS_G2dGetImageLocation(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DSUB);

        DC_FlushRange(character->rawData, character->size);
        GXS_LoadOBJ(character->rawData, addr, character->size);
    }
}

void ClActSys_WriteCharData(u32 chars, void *src, u32 size, u32 offset, u32 vramType) {
    if (vramType == CLACT_VRAM_MAIN || vramType == CLACT_VRAM_BOTH) {
        if (NNS_G2dIsImageReadyToUse(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DMAIN)) {
            u32 addr = NNS_G2dGetImageLocation(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DMAIN);

            DC_FlushRange(src, size);
            GX_LoadOBJ(src, addr + offset, size);
        }
    }
    if (vramType == CLACT_VRAM_SUB || vramType == CLACT_VRAM_BOTH) {
        if (NNS_G2dIsImageReadyToUse(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DSUB)) {
            u32 addr = NNS_G2dGetImageLocation(&g_ClActSys->res.chars[chars].proxy, NNS_G2D_VRAM_TYPE_2DSUB);

            DC_FlushRange(src, size);
            GXS_LoadOBJ(src, addr + offset, size);
        }
    }
}

void ClActSys_GetCharProxy(u32 chars, NNSG2dImageProxy *proxy) {
    *proxy = g_ClActSys->res.chars[chars].proxy;
}

u32 ClActSys_GetCharVramAddr(u32 chars, BOOL sub) {
    NNSG2dImageProxy proxy;
    NNSG2dVRamType type = NNS_G2D_VRAM_TYPE_2DMAIN;

    if (sub) {
        type = NNS_G2D_VRAM_TYPE_2DSUB;
    }
    ClActSys_GetCharProxy(chars, &proxy);
    return NNS_G2dGetImageLocation(&proxy, type);
}

u32 ClActSys_LoadPltt(ArcTool *arc, u32 fileId, u32 vramType, u16 offset, HeapID heapId) {
    return ClActSys_LoadPlttRange(arc, fileId, vramType, offset, 0, 0, heapId);
}

u32 ClActSys_LoadPlttRange(ArcTool *arc, u32 fileId, u32 vramType, u16 offset, u16 start, u16 count, HeapID heapId) {
    u32 idx = ClAct_FindFreePltt(&g_ClActSys->res);

    if (idx != CLACT_RES_NONE) {
        void *data = GFL_ArcToolReadHeapNew(arc, fileId, HEAPID_TAIL(heapId));

        ClActResMan_SetupPlttRange(&g_ClActSys->res, idx, data, vramType, offset, start, count);
        GFL_HeapFree(data);
        return idx;
    }
    return idx;
}

u32 ClActSys_LoadPlttEx(ArcTool *arc, u32 fileId, u32 vramType, u16 offset, HeapID heapId) {
    u32 idx = ClAct_FindFreePltt(&g_ClActSys->res);

    if (idx != CLACT_RES_NONE) {
        void *data = GFL_ArcToolReadHeapNew(arc, fileId, HEAPID_TAIL(heapId));

        ClActResMan_SetupPlttEx(&g_ClActSys->res, idx, data, vramType, offset);
        GFL_HeapFree(data);
        return idx;
    }
    return idx;
}

void ClActSys_FreePltt(u32 palette) {
    g_ClActSys->res.pltts[palette].free = TRUE;
    g_ClActSys->res.pltts[palette].size = 0;
}

void ClActSys_ReplacePlttData(u32 palette, NNSG2dPaletteData *data, u32 count) {
    u32 size = count * 32;

    if (count == 0) {
        size = data->size;
    }
    if (NNS_G2dIsImagePaletteReadyToUse(&g_ClActSys->res.pltts[palette].proxy, NNS_G2D_VRAM_TYPE_2DMAIN)) {
        u32 addr = NNS_G2dGetImagePaletteLocation(&g_ClActSys->res.pltts[palette].proxy, NNS_G2D_VRAM_TYPE_2DMAIN);

        DC_FlushRange(data->rawData, size);
        GX_LoadOBJPltt(data->rawData, addr, size);
    }
    if (NNS_G2dIsImagePaletteReadyToUse(&g_ClActSys->res.pltts[palette].proxy, NNS_G2D_VRAM_TYPE_2DSUB)) {
        u32 addr = NNS_G2dGetImagePaletteLocation(&g_ClActSys->res.pltts[palette].proxy, NNS_G2D_VRAM_TYPE_2DSUB);

        DC_FlushRange(data->rawData, size);
        GXS_LoadOBJPltt(data->rawData, addr, size);
    }
}

void ClActSys_GetPlttProxy(u32 palette, NNSG2dImagePaletteProxy *proxy) {
    *proxy = g_ClActSys->res.pltts[palette].proxy;
}

u32 ClActSys_GetPlttVramAddr(u32 palette, BOOL sub) {
    NNSG2dImagePaletteProxy proxy;
    NNSG2dVRamType type = NNS_G2D_VRAM_TYPE_2DMAIN;

    if (sub) {
        type = NNS_G2D_VRAM_TYPE_2DSUB;
    }
    ClActSys_GetPlttProxy(palette, &proxy);
    return NNS_G2dGetImagePaletteLocation(&proxy, type);
}

u32 ClActSys_LoadCellAnims(ArcTool *arc, u32 cellFileId, u32 animFileId, HeapID heapId) {
    u32 idx = ClAct_FindFreeCellAnim(&g_ClActSys->res);
    ClActCellAnimRes *cellAnims = g_ClActSys->res.cellAnims;
    ClActCellAnimRes *res = &cellAnims[idx];

    // BUG: this does not check that a cell animation was free, and writes before the array when none is
#ifdef BUGFIX
    if (idx == CLACT_RES_NONE) {
        return CLACT_RES_NONE;
    }
#endif
    cellAnims[idx].cellFile = GFL_ArcToolReadHeapNew(arc, cellFileId, heapId);
    res->animFile = GFL_ArcToolReadHeapNew(arc, animFileId, heapId);
    ClActResMan_SetupCellAnims(&g_ClActSys->res, idx, cellAnims[idx].cellFile, res->animFile);
    return idx;
}

void ClActSys_FreeCellAnims(u32 cellAnims) {
    ClActCellAnimRes *res = &g_ClActSys->res.cellAnims[cellAnims];

    if (res->cellFile != NULL) {
        GFL_HeapFree(res->cellFile);
        res->cellFile = NULL;
    }
    if (res->animFile != NULL) {
        GFL_HeapFree(res->animFile);
        res->animFile = NULL;
    }
    res->free = TRUE;
}

ClActRenderer *ClActRenderer_Create(const ClActSurfaceSetup *setups, u16 count, HeapID heapId) {
    ClActRenderer *renderer = GFL_HeapAllocate(heapId, sizeof(ClActRenderer), TRUE, "clact.c", 1611);

    ClActRenderer_Init(renderer, setups, count, heapId);
    return renderer;
}

void ClActRenderer_Delete(ClActRenderer *renderer) {
    ClActRenderer_Free(renderer);
    GFL_HeapFree(renderer);
}

void ClActRenderer_SetSurfacePos(ClActRenderer *renderer, u32 surface, const ClActorPos *pos) {
    ClAct_SetVec(&renderer->surfaces[surface].viewRect.posTopLeft, pos->x, pos->y);
}

void ClActRenderer_GetSurfacePos(ClActRenderer *renderer, u32 surface, ClActorPos *pos) {
    const NNSG2dFVec2 *vec = &renderer->surfaces[surface].viewRect.posTopLeft;

    pos->x = vec->x >> FX32_SHIFT;
    pos->y = vec->y >> FX32_SHIFT;
}

void ClActRenderer_SetCulling(ClActRenderer *renderer, BOOL cull) {
    ClActRenderer_SetCullCallback(renderer, cull);
}

ClActUnit *ClActUnit_Create(u16 count, u8 priority, HeapID heapId) {
    ClActUnit *unit;
    int i = 0;

    unit = GFL_HeapAllocate(heapId, sizeof(ClActUnit), FALSE, "clact.c", 1795);
    MI_CpuFill8(unit, 0, sizeof(ClActUnit));
    unit->actors = GFL_HeapAllocate(heapId, count * sizeof(ClActor), FALSE, "clact.c", 1799);
    unit->count = count;
    unit->priority = priority;
    for (; i < count; i++) {
        ClActor_Reset(&unit->actors[i]);
    }
    ClActUnit_UseSystemRenderer(unit);
    ClActUnit_SetActive(unit, TRUE);
    ClActUnit_Link(unit);
    return unit;
}

void ClActUnit_Delete(ClActUnit *unit) {
    int i;

    ClActUnit_Unlink(unit);
    for (i = 0; i < unit->count; i++) {
        if (unit->actors[i].next != NULL) {
            ClActor_Delete(&unit->actors[i]);
        }
    }
    GFL_HeapFree(unit->actors);
    GFL_HeapFree(unit);
}

void ClActUnit_SetActive(ClActUnit *unit, BOOL active) {
    unit->active = active;
}

BOOL ClActUnit_IsActive(ClActUnit *unit) {
    return unit->active;
}

void ClActUnit_SetVisibleAnimOnly(ClActUnit *unit, BOOL a1) {
    unit->visibleAnimOnly = a1;
}

void ClActUnit_SetSharedParams(ClActUnit *unit, BOOL a1) {
    unit->sharedParams = a1;
}

void ClActUnit_SetRenderer(ClActUnit *unit, ClActRenderer *renderer) {
    unit->renderer = renderer;
    unit->rendererType = 1;
}

void ClActUnit_UseSystemRenderer(ClActUnit *unit) {
    unit->renderer = &g_ClActSys->renderer;
    unit->rendererType = 0;
}

ClActor *ClActor_Create(ClActUnit *unit, u32 chars, u32 palette, u32 cellAnims, const ClActorSetup *setup, u16 surface,
                        HeapID heapId) {
    ClActResSetup res;

    ClActResSetup_Init(&res, chars, palette, &g_ClActSys->res.chars[chars], &g_ClActSys->res.pltts[palette],
                       g_ClActSys->res.cellAnims[cellAnims].cells, g_ClActSys->res.cellAnims[cellAnims].anims);
    return ClActor_CreateFromRes(unit, setup, &res, surface, heapId);
}

ClActor *ClActor_CreateEx(ClActUnit *unit, u32 chars, u32 palette, u32 cellAnims, const ClActorSetupEx *setup,
                          u16 surface, HeapID heapId) {
    ClActResSetup res;

    ClActResSetup_Init(&res, chars, palette, &g_ClActSys->res.chars[chars], &g_ClActSys->res.pltts[palette],
                       g_ClActSys->res.cellAnims[cellAnims].cells, g_ClActSys->res.cellAnims[cellAnims].anims);
    return ClActor_CreateExFromRes(unit, setup, &res, surface, heapId);
}

void ClActor_Delete(ClActor *actor) {
    ClActUnit_RemoveActor(actor->unit, actor);
    ClActAnim_Free(&actor->anim);
    ClActor_Reset(actor);
}

void ClActor_SetVisible(ClActor *actor, BOOL visible) {
    actor->visible = visible;
}

BOOL ClActor_IsVisible(ClActor *actor) {
    return actor->visible;
}

void ClActor_SetSurfacePos(ClActor *actor, const ClActorPos *pos, u16 surface) {
    ClActorPos offset;
    ClActorPos actorPos;

    ClActor_GetSurfaceOffset(actor, surface, &offset);
    actorPos.x = pos->x + offset.x;
    actorPos.y = pos->y + offset.y;
    ClActor_SetPos(actor, &actorPos);
}

void ClActor_GetSurfacePos(ClActor *actor, ClActorPos *pos, u16 surface) {
    ClActorPos offset;

    ClActor_GetSurfaceOffset(actor, surface, &offset);
    ClActor_GetPos(actor, pos);
    pos->x -= offset.x;
    pos->y -= offset.y;
}

void ClActor_SetSurfacePosAxis(ClActor *actor, s16 value, u16 surface, u32 axis) {
    ClActorPos offset;

    ClActor_GetSurfaceOffset(actor, surface, &offset);
    if (axis == 0) {
        value += offset.x;
    } else {
        value += offset.y;
    }
    ClActor_SetPosAxis(actor, value, axis);
}

s16 ClActor_GetSurfacePosAxis(ClActor *actor, u16 surface, u32 axis) {
    ClActorPos offset;
    s16 value;

    ClActor_GetSurfaceOffset(actor, surface, &offset);
    value = ClActor_GetPosAxis(actor, axis);
    if (axis == 0) {
        return value - offset.x;
    }
    return value - offset.y;
}

void ClActor_SetPos(ClActor *actor, const ClActorPos *pos) {
    actor->pos = *pos;
}

void ClActor_GetPos(const ClActor *actor, ClActorPos *pos) {
    *pos = actor->pos;
}

void ClActor_SetPosAxis(ClActor *actor, s16 value, u32 axis) {
    if (axis == 0) {
        actor->pos.x = value;
    } else {
        actor->pos.y = value;
    }
}

s16 ClActor_GetPosAxis(ClActor *actor, u32 axis) {
    if (axis == 0) {
        return actor->pos.x;
    }
    return actor->pos.y;
}

void ClActor_SetAffineMode(ClActor *actor, u32 affineMode) {
    actor->affineMode = affineMode;
}

void ClActor_SetAffineCenter(ClActor *actor, const ClActorPos *center) {
    actor->affineCenter = *center;
}

void ClActor_SetAffineCenterAxis(ClActor *actor, s16 value, u32 axis) {
    if (axis == 0) {
        actor->affineCenter.x = value;
    } else {
        actor->affineCenter.y = value;
    }
}

void ClActor_SetScale(ClActor *actor, const ClActorScale *scale) {
    actor->scale = *scale;
}

void ClActor_GetScale(ClActor *actor, ClActorScale *scale) {
    *scale = actor->scale;
}

void ClActor_SetScaleAxis(ClActor *actor, fx32 value, u32 axis) {
    if (axis == 0) {
        actor->scale.x = value;
    } else {
        actor->scale.y = value;
    }
}

fx32 ClActor_GetScaleAxis(ClActor *actor, u32 axis) {
    if (axis == 0) {
        return actor->scale.x;
    }
    return actor->scale.y;
}

void ClActor_SetRotation(ClActor *actor, u16 rotation) {
    actor->rotation = rotation;
}

u16 ClActor_GetRotation(ClActor *actor) {
    return actor->rotation;
}

void ClActor_SetFlip(ClActor *actor, u32 axis, BOOL flip) {
    u32 bit = 1;

    if (axis) {
        bit = 2;
    }
    if (flip == FALSE) {
        actor->flip &= ~bit;
    } else {
        actor->flip |= bit;
    }
}

BOOL ClActor_GetFlip(ClActor *actor, u32 axis) {
    BOOL flip = FALSE;

    if (axis == 0) {
        if (actor->flip & 1) {
            flip = TRUE;
        }
    } else if (actor->flip & 2) {
        flip = TRUE;
    }
    return flip;
}

void ClActor_SetObjMode(ClActor *actor, u32 mode) {
    actor->objMode = mode;
    if (mode == GX_OAM_MODE_NORMAL) {
        if (actor->overwrite & NNS_G2D_RND_OVERWRITE_OBJMODE) {
            actor->overwrite ^= NNS_G2D_RND_OVERWRITE_OBJMODE;
        }
    } else {
        actor->overwrite |= NNS_G2D_RND_OVERWRITE_OBJMODE;
    }
}

u32 ClActor_GetObjMode(ClActor *actor) {
    return actor->objMode;
}

void ClActor_SetPalette(ClActor *actor, u32 palette, u32 a2) {
    actor->palette = palette;
    actor->paletteMode = a2;
}

u8 ClActor_GetPalette(ClActor *actor) {
    return actor->palette;
}

void ClActor_SetPlttProxy(ClActor *actor, const NNSG2dImagePaletteProxy *proxy) {
    NNS_G2dIsImagePaletteReadyToUse(proxy, NNS_G2D_VRAM_TYPE_2DMAIN);
    NNS_G2dIsImagePaletteReadyToUse(proxy, NNS_G2D_VRAM_TYPE_2DSUB);
    actor->pltProxy = *proxy;
}

void ClActor_GetPlttProxy(ClActor *actor, NNSG2dImagePaletteProxy *proxy) {
    *proxy = actor->pltProxy;
}

void ClActor_SetImageProxy(ClActor *actor, const NNSG2dImageProxy *proxy) {
    NNS_G2dIsImageReadyToUse(proxy, NNS_G2D_VRAM_TYPE_2DMAIN);
    NNS_G2dIsImageReadyToUse(proxy, NNS_G2D_VRAM_TYPE_2DSUB);
    actor->imgProxy = *proxy;
}

void ClActor_GetImageProxy(ClActor *actor, NNSG2dImageProxy *proxy) {
    *proxy = actor->imgProxy;
}

u16 ClActor_GetCharIdx(ClActor *actor) {
    return actor->charIdx;
}

u16 ClActor_GetPlttIdx(ClActor *actor) {
    return actor->plttIdx;
}

void ClActor_SetPriority(ClActor *actor, u8 priority) {
    actor->priority = priority;
    ClActUnit_RemoveActor(actor->unit, actor);
    ClActUnit_InsertActor(actor->unit, actor);
}

u8 ClActor_GetPriority(ClActor *actor) {
    return actor->priority;
}

void ClActor_SetBgPriority(ClActor *actor, u32 bgPriority) {
    actor->bgPriority = bgPriority;
}

u8 ClActor_GetBgPriority(ClActor *actor) {
    return actor->bgPriority;
}

void ClActor_SetSequence(ClActor *actor, u16 sequence) {
    actor->sequence = sequence;
    ClActAnim_SetSequence(&actor->anim, sequence);
    ClActor_StartAnim(actor);
}

u16 ClActor_GetSequence(ClActor *actor) {
    return actor->sequence;
}

u16 ClActor_GetSequenceCount(ClActor *actor) {
    return ClActAnim_GetSequenceCount(&actor->anim);
}

void ClActor_ChangeSequence(ClActor *actor, u16 sequence) {
    if (sequence != actor->sequence) {
        ClActor_SetSequence(actor, sequence);
        ClActor_StartAnim(actor);
    }
}

void ClActor_SetAnimTime(ClActor *actor, fx32 time) {
    ClActAnim_SetTime(&actor->anim, time);
}

void ClActor_SetAnimSpeed(ClActor *actor, fx32 speed) {
    ClActAnim_SetSpeed(&actor->anim, speed);
    ClActAnim_Tick(&actor->anim, FX32_ONE);
}

fx32 ClActor_GetAnimTime(ClActor *actor) {
    return ClActAnim_GetCtrl2(&actor->anim)->currentTime;
}

void ClActor_SetAnimFrame(ClActor *actor, u16 frame) {
    ClActAnim_SetFrame(&actor->anim, frame);
}

u16 ClActor_GetAnimFrame(ClActor *actor) {
    return NNS_G2dGetAnimCtrlCurrentFrame(ClActAnim_GetCtrl2(&actor->anim));
}

void ClActor_SetAutoAnim(ClActor *actor, BOOL a1) {
    actor->autoAnim = a1;
}

BOOL ClActor_IsAutoAnim(ClActor *actor) {
    return actor->autoAnim;
}

void ClActor_SetAutoAnimSpeed(ClActor *actor, fx32 a1) {
    actor->autoAnimSpeed = a1;
}

void ClActor_StartAnim(ClActor *actor) {
    ClActAnim_GetCtrl(&actor->anim)->bActive = TRUE;
}

void ClActor_StopAnim(ClActor *actor) {
    ClActAnim_GetCtrl(&actor->anim)->bActive = FALSE;
}

BOOL ClActor_IsAnimActive(ClActor *actor) {
    return ClActAnim_GetCtrl2(&actor->anim)->bActive;
}

void ClActor_RestartAnim(ClActor *actor) {
    ClActAnim_GetCtrl(&actor->anim)->bActive = TRUE;
    ClActAnim_Restart(&actor->anim);
}

void ClActor_SetPlayMode(ClActor *actor, u32 playMode) {
    NNSG2dAnimController *animCtrl;

    actor->playMode = playMode;
    animCtrl = ClActAnim_GetCtrl(&actor->anim);
    if (playMode == 0) {
        animCtrl->bReverse = FALSE;
        animCtrl->overriddenPlayMode = 0;
    } else {
        animCtrl->overriddenPlayMode = playMode;
    }
}

void ClActor_SetAnimCallback(ClActor *actor, const ClActorCallback *callback) {
    ClActAnim_SetCallback(&actor->anim, callback);
}

void ClActor_ClearAnimCallback(ClActor *actor) {
    ClActAnim_ClearCallback(&actor->anim);
}

void ClActor_SetDrawAnim(ClActor *actor, BOOL a1) {
    if (a1) {
        actor->cellOnly = 0;
    } else {
        actor->cellOnly = 1;
    }
}

// The user extended attributes of the actor's animations and cells, which nothing calls
static const NNSG2dUserExAnimSequenceAttr *ClActor_GetUserExAnimSequenceAttr(ClActor *actor, u32 sequence) {
    return gClActAnimGetSeqAttrFuncs[actor->anim.type](&actor->anim, sequence);
}

static const NNSG2dUserExCellAttrBank *ClActor_GetUserExCellAttrBank(ClActor *actor) {
    return gClActAnimGetCellAttrFuncs[actor->anim.type](&actor->anim);
}

void ClActRenderer_InitForSystem(ClActRenderer *renderer, const ClActSysSetup *setup, HeapID heapId) {
    sSurfaceSetups[0].x = setup->mainX;
    sSurfaceSetups[0].y = setup->mainY;
    sSurfaceSetups[1].x = setup->subX;
    sSurfaceSetups[1].y = setup->subY;
    ClActRenderer_Init(renderer, sSurfaceSetups, 2, heapId);
}

static void ClActSys_SetUpdating(BOOL updating) {
    if (g_ClActSys != NULL) {
        g_ClActSys->updating = updating;
    }
}

static BOOL ClActSys_IsUpdating(void) {
    if (g_ClActSys != NULL) {
        return g_ClActSys->updating;
    }
    return FALSE;
}

static void ClActSys_SetOamReady(BOOL ready) {
    if (g_ClActSys != NULL) {
        g_ClActSys->oamReady = ready;
    }
}

static BOOL ClActSys_IsOamReady(void) {
    if (g_ClActSys != NULL) {
        return g_ClActSys->oamReady;
    }
    return FALSE;
}

static void ClActSys_ResetOamBuffersIfReady(void) {
    if (g_ClActSys != NULL && ClActSys_IsOamReady()) {
        ClActOamMan_ResetAll(g_ClActSys->oamMans);
        ClActSys_SetOamReady(FALSE);
    }
}

static void ClActSys_DrawUnits(void) {
    ClActUnit *unit = g_ClActSys->units;

    if (unit != NULL) {
        do {
            if (unit->active && unit->actorList != NULL) {
                renderers[unit->visibleAnimOnly + unit->sharedParams * 2](unit);
            }
            unit = unit->next;
        } while (unit != g_ClActSys->units);
    }
}

static void ClActUnit_Link(ClActUnit *unit) {
    ClActUnit *cur;

    if (g_ClActSys->units == NULL) {
        g_ClActSys->units = unit;
        g_ClActSys->units->next = unit;
        g_ClActSys->units->prev = unit;
        return;
    }
    cur = g_ClActSys->units;
    do {
        if (cur->priority > unit->priority) {
            break;
        }
        cur = cur->next;
    } while (cur != g_ClActSys->units);
    cur = cur->prev;
    unit->prev = cur;
    unit->next = cur->next;
    cur->next->prev = unit;
    cur->next = unit;
}

static void ClActUnit_Unlink(ClActUnit *unit) {
    if (g_ClActSys->units == unit) {
        if (unit->next == unit) {
            g_ClActSys->units = NULL;
        } else {
            g_ClActSys->units = unit->next;
        }
    }
    unit->next->prev = unit->prev;
    unit->prev->next = unit->next;
    unit->next = NULL;
    unit->prev = NULL;
}

static void ClActOamMan_InitAll(ClActOamMan *mans, u8 fromMain, u8 numMain, u8 fromSub, u8 numSub) {
    NNS_G2dInitOamManagerModule();
    ClActOamMan_Init(&mans[0], fromMain, numMain, NNS_G2D_OAMTYPE_MAIN);
    ClActOamMan_Init(&mans[1], fromSub, numSub, NNS_G2D_OAMTYPE_SUB);
}

static void ClActOamMan_ShutdownAll(ClActOamMan *mans) {
    ClActOamMan_ResetAll(mans);
    ClActOamMan_ApplyAll(mans);
    NNS_G2dInitOamManagerModule();
}

static void ClActOamMan_ApplyAll(ClActOamMan *mans) {
    int i;

    for (i = 0; i < 2; i++) {
        ClActOamMan_Apply(&mans[i]);
    }
}

static void ClActOamMan_ResetAll(ClActOamMan *mans) {
    int i;

    for (i = 0; i < 2; i++) {
        ClActOamMan_Reset(&mans[i]);
    }
}

static void ClActOamMan_Init(ClActOamMan *man, u8 from, u8 num, u32 type) {
    if (num == 0) {
        man->active = FALSE;
    } else {
        NNS_G2dGetNewOamManagerInstanceAsFastTransferMode(&man->man, from, num, type);
        man->active = TRUE;
    }
}

static void ClActOamMan_Apply(ClActOamMan *man) {
    if (man->active) {
        NNS_G2dApplyOamManagerToHW(&man->man);
    }
}

static void ClActOamMan_Reset(ClActOamMan *man) {
    if (man->active) {
        NNS_G2dResetOamManagerBuffer(&man->man);
    }
}

static void ClActRenderer_Init(ClActRenderer *renderer, const ClActSurfaceSetup *setups, u16 count, HeapID heapId) {
    u32 i;

    NNS_G2dInitRenderer(&renderer->renderer);
    renderer->renderer.spriteZoffsetStep = 1;
    renderer->surfaces = GFL_HeapAllocate(heapId, count * sizeof(NNSG2dRenderSurface), FALSE, "clact.c", 4124);
    renderer->surfaceCount = count;
    for (i = 0; i < count; i++) {
        ClActSurface_Init(&renderer->surfaces[i], &setups[i]);
        NNS_G2dAddRendererTargetSurface(&renderer->renderer, &renderer->surfaces[i]);
    }
    if (renderer->surfaceCount == 1) {
        renderer->opzHint = NNS_G2D_RDR_OPZHINT_LOCK_PARAMS;
    }
}

static void ClActRenderer_Free(ClActRenderer *renderer) {
    GFL_HeapFree(renderer->surfaces);
}

static void ClActRenderer_SetCullCallback(ClActRenderer *renderer, BOOL cull) {
    int i;
    void *callback;

    if (cull) {
        callback = ClAct_CullOam;
    } else {
        callback = NULL;
        renderer->renderer.rendererCore.bDrawEnable = TRUE;
    }
    for (i = 0; i < renderer->surfaceCount; i++) {
        renderer->surfaces[i].pBeforeDrawOamBackFunc = callback;
    }
}

static void ClActSurface_Init(NNSG2dRenderSurface *surface, const ClActSurfaceSetup *setup) {
    ClActSurfaceFuncs funcs;

    NNS_G2dInitRenderSurface(surface);
    ClAct_SetVec(&surface->viewRect.posTopLeft, setup->x, setup->y);
    ClAct_SetVec(&surface->viewRect.sizeView, setup->w, setup->h);
    ClActSurface_SetType(surface, setup->screen);
    funcs = sSurfaceFuncs[setup->screen];
    funcs.culling = sCullingFuncs[setup->culling];
    surface->pFuncOamRegister = funcs.oamRegister;
    surface->pFuncOamAffineRegister = funcs.affineRegister;
    surface->pFuncVisibilityCulling = funcs.culling;
}

static void ClActSurface_SetType(NNSG2dRenderSurface *surface, u32 screen) {
    surface->type = sSurfaceTypes[screen];
}

static BOOL ClActOamMan_EntryOam(ClActOamMan *man, const GXOamAttr *oam, u16 affineIdx, u16 palette, u32 mode) {
    GXOamAttr attr;

    MI_CpuCopy8(oam, &attr, sizeof(GXOamAttr));
    if (mode == 0) {
        attr.cParam += palette;
    } else if (mode == 1) {
        attr.cParam = palette;
    }
    return NNS_G2dEntryOamManagerOamWithAffineIdx(&man->man, &attr, affineIdx);
}

static BOOL ClActOam_EntryMain(const GXOamAttr *oam, u16 affineIdx, BOOL doubleAffine) {
    return ClActOamMan_EntryOam(&g_ClActSys->oamMans[0], oam, affineIdx, g_ClActSys->palettes[0],
                                g_ClActSys->paletteMode);
}

static BOOL ClActOam_EntrySub(const GXOamAttr *oam, u16 affineIdx, BOOL doubleAffine) {
    return ClActOamMan_EntryOam(&g_ClActSys->oamMans[1], oam, affineIdx, g_ClActSys->palettes[1],
                                g_ClActSys->paletteMode);
}

static u16 ClActAffine_EntryMain(const MtxFx22 *mtx) {
    return NNS_G2dEntryOamManagerAffine(&g_ClActSys->oamMans[0].man, mtx);
}

static u16 ClActAffine_EntrySub(const MtxFx22 *mtx) {
    return NNS_G2dEntryOamManagerAffine(&g_ClActSys->oamMans[1].man, mtx);
}

// Whether a cell's bounding rect, or circle, is in view, its corners transformed by the matrix
static BOOL ClAct_IsCellInViewTransformed(const NNSG2dCellData *cell, const MtxFx32 *mtx, const NNSG2dViewRect *view) {
    fx32 tmp;
    const NNSG2dCellBoundingRectS16 *rect = &((const NNSG2dCellDataWithBR *)cell)->boundingRect;
    u32 r = NNSi_G2dGetCellBoundingSphereR(cell);
    fx32 posX = mtx->_20 - view->posTopLeft.x;
    fx32 posY = mtx->_21 - view->posTopLeft.y;
    fx32 maxY;
    fx32 minX;
    fx32 maxX;
    fx32 minY;
    fx32 top;
    fx32 bottom;
    fx32 left;
    fx32 right;

    if (((cell->cellAttr >> NNSi_G2D_CELLATTR_BOUNDINGRECT_SHIFT) & 1) == 1) {
        minY = rect->minY << FX32_SHIFT;
        maxY = rect->maxY << FX32_SHIFT;
        minX = rect->minX << FX32_SHIFT;
        maxX = rect->maxX << FX32_SHIFT;
    } else {
        minY = -r << FX32_SHIFT;
        maxY = r << FX32_SHIFT;
        minX = minY;
        maxX = maxY;
    }
    top = posY + (FX_MulFunc(minY, mtx->_01) + FX_MulFunc(minY, mtx->_11));
    bottom = posY + (FX_MulFunc(maxY, mtx->_01) + FX_MulFunc(maxY, mtx->_11));
    left = posX + (FX_MulFunc(minX, mtx->_00) + FX_MulFunc(minX, mtx->_10));
    right = posX + (FX_MulFunc(maxX, mtx->_00) + FX_MulFunc(maxX, mtx->_10));
    if (bottom < top) {
        tmp = bottom;
        bottom = top;
        top = tmp;
    }
    if (right < left) {
        tmp = right;
        right = left;
        left = tmp;
    }
    if (bottom > 0 && top < view->sizeView.y && right > 0 && left < view->sizeView.x) {
        return TRUE;
    }
    return FALSE;
}

// The same, untransformed
static BOOL ClAct_IsCellInView(const NNSG2dCellData *cell, const MtxFx32 *mtx, const NNSG2dViewRect *view) {
    const NNSG2dCellBoundingRectS16 *rect = &((const NNSG2dCellDataWithBR *)cell)->boundingRect;
    u32 r = NNSi_G2dGetCellBoundingSphereR(cell);
    fx32 posX = mtx->_20 - view->posTopLeft.x;
    fx32 posY = mtx->_21 - view->posTopLeft.y;
    fx32 minY;
    fx32 maxY;
    fx32 minX;
    fx32 maxX;

    if (((cell->cellAttr >> NNSi_G2D_CELLATTR_BOUNDINGRECT_SHIFT) & 1) == 1) {
        minY = rect->minY << FX32_SHIFT;
        maxY = rect->maxY << FX32_SHIFT;
        minX = rect->minX << FX32_SHIFT;
        maxX = rect->maxX << FX32_SHIFT;
    } else {
        minY = -r << FX32_SHIFT;
        maxY = r << FX32_SHIFT;
        minX = minY;
        maxX = maxY;
    }
    minY += posY;
    maxY += posY;
    minX += posX;
    maxX += posX;
    if (maxY > 0 && minY < view->sizeView.y && maxX > 0 && minX < view->sizeView.x) {
        return TRUE;
    }
    return FALSE;
}

// An OAM's x or y, sign extended from its 9 or 8 bits: one past the right or bottom of the screen wraps around to the
// left or top
static inline s16 ClAct_WrapOamX(s16 x) {
    if (x > 0xff) {
        x |= 0xff00;
    }
    return x;
}

static inline s16 ClAct_WrapOamY(s16 y) {
    if (y > 0x7f) {
        y |= 0xff00;
    }
    return y;
}

// Called by the renderers before each OAM: skips one that is out of view
static void ClAct_CullOam(ClActDrawState *state, const NNSG2dViewRect *view, u32 a2, u32 a3, const MtxFx32 *mtx) {
    GXOamAttr *oam = &state->oam;
    GXOamShape shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
    fx32 left = ClAct_WrapOamX(oam->x) << FX32_SHIFT;
    fx32 right = (ClAct_WrapOamX(oam->x) + NNS_G2dGetOamSizeX(&shape)) << FX32_SHIFT;
    fx32 top = ClAct_WrapOamY(oam->y) << FX32_SHIFT;
    fx32 bottom = (ClAct_WrapOamY(oam->y) + NNS_G2dGetOamSizeY(&shape)) << FX32_SHIFT;

    if (ClAct_IsRectInView(mtx->_20 - view->posTopLeft.x, mtx->_21 - view->posTopLeft.y, left, right, top, bottom, mtx,
                           view)) {
        state->skip = TRUE;
    } else {
        state->skip = FALSE;
    }
}

static inline void ClAct_UpdateMinMax(fx32 value, fx32 *min, fx32 *max) {
    if (value < *min) {
        *min = value;
    } else if (value > *max) {
        *max = value;
    }
}

// Whether a rect is in view, its corners transformed by the matrix
static BOOL ClAct_IsRectInView(fx32 posX, fx32 posY, fx32 left, fx32 right, fx32 top, fx32 bottom, const MtxFx32 *mtx,
                               const NNSG2dViewRect *view) {
    fx32 minX;
    fx32 maxX;
    fx32 minY;
    fx32 maxY;

    minX = FX_MulFunc(left, mtx->_00) + FX_MulFunc(top, mtx->_10);
    maxX = minX;
    ClAct_UpdateMinMax(FX_MulFunc(left, mtx->_00) + FX_MulFunc(bottom, mtx->_10), &minX, &maxX);
    ClAct_UpdateMinMax(FX_MulFunc(right, mtx->_00) + FX_MulFunc(top, mtx->_10), &minX, &maxX);
    ClAct_UpdateMinMax(FX_MulFunc(right, mtx->_00) + FX_MulFunc(bottom, mtx->_10), &minX, &maxX);
    minY = FX_MulFunc(left, mtx->_01) + FX_MulFunc(top, mtx->_11);
    maxY = minY;
    ClAct_UpdateMinMax(FX_MulFunc(left, mtx->_01) + FX_MulFunc(bottom, mtx->_11), &minY, &maxY);
    ClAct_UpdateMinMax(FX_MulFunc(right, mtx->_01) + FX_MulFunc(top, mtx->_11), &minY, &maxY);
    ClAct_UpdateMinMax(FX_MulFunc(right, mtx->_01) + FX_MulFunc(bottom, mtx->_11), &minY, &maxY);
    minX += posX;
    maxX += posX;
    minY += posY;
    maxY += posY;
    if (maxY > 0 && minY < view->sizeView.y && maxX > 0 && minX < view->sizeView.x) {
        return TRUE;
    }
    return FALSE;
}

static void ClActTransfer_Init(ClActTransfer *transfer, u8 count, HeapID heapId) {
    if (count == 0) {
        transfer->active = FALSE;
        return;
    }
    transfer->count = count;
    transfer->states = GFL_HeapAllocate(heapId, count * sizeof(NNSG2dCellTransferState), FALSE, "clact.c", 4976);
    NNS_G2dInitCellTransferStateManager(transfer->states, count, ClActTransfer_Request);
    ClActTransfer_AllocTasks(transfer, count, heapId);
    transfer->active = TRUE;
}

static void ClActTransfer_Free(ClActTransfer *transfer) {
    if (transfer->active) {
        GFL_HeapFree(transfer->states);
        transfer->states = NULL;
        ClActTransfer_FreeTasks(transfer);
        transfer->active = FALSE;
    }
}

static void ClActTransfer_AllocTasks(ClActTransfer *transfer, u32 count, HeapID heapId) {
    u32 i = 0;

    transfer->tasks = GFL_HeapAllocate(heapId, count * sizeof(ClActTransferTask), FALSE, "clact.c", 5044);
    for (; i < transfer->count; i++) {
        MI_CpuFill8(&transfer->tasks[i], 0, sizeof(ClActTransferTask));
    }
}

static void ClActTransfer_FreeTasks(ClActTransfer *transfer) {
    GFL_HeapFree(transfer->tasks);
    transfer->tasks = NULL;
}

static void ClActTransfer_RunTasks(ClActTransfer *transfer) {
    u32 i;

    for (i = 0; i < transfer->count; i++) {
        if (transfer->tasks[i].src != NULL) {
            ClActTransferTask_Run(&transfer->tasks[i]);
            MI_CpuFill8(&transfer->tasks[i], 0, sizeof(ClActTransferTask));
        }
    }
}

static ClActTransferTask *ClActTransfer_FindFreeTask(ClActTransfer *transfer) {
    u32 i;

    for (i = 0; i < transfer->count; i++) {
        if (transfer->tasks[i].src == NULL) {
            return &transfer->tasks[i];
        }
    }
    return NULL;
}

static BOOL ClActTransfer_Request(NNS_GFD_DST_TYPE type, u32 dstAddr, void *src, u32 size) {
    ClActTransferTask *task = ClActTransfer_FindFreeTask(&g_ClActSys->transfer);
    u32 screen;

    if (type == NNS_GFD_DST_2D_OBJ_CHAR_MAIN) {
        screen = 0;
    } else if (type == NNS_GFD_DST_2D_OBJ_CHAR_SUB) {
        screen = 1;
    }
    if (size != 0) {
        ClActTransferTask_Set(task, screen, dstAddr, src, size);
    }
    return TRUE;
}

static void ClActTransferTask_Set(ClActTransferTask *task, u32 screen, u32 dst, void *src, u32 size) {
    task->screen = screen;
    task->dst = dst;
    task->src = src;
    task->size = size;
}

static void ClActTransferTask_Run(ClActTransferTask *task) {
    DC_FlushRange(task->src, task->size);
    sTransferFuncs[task->screen](task->src, task->dst, task->size);
}

static ClActor *ClActUnit_FindFreeActor(ClActUnit *unit) {
    int i;

    for (i = 0; i < unit->count; i++) {
        if (unit->actors[i].next == NULL) {
            return &unit->actors[i];
        }
    }
    GFL_ASSERT(0);
    return NULL;
}

static void ClActUnit_InsertActor(ClActUnit *unit, ClActor *actor) {
    ClActor *pos;
    u8 priority;
    u8 headPriority;

    if (unit->actorList == NULL) {
        unit->actorList = actor;
        actor->next = actor;
        unit->actorList->prev = actor;
        return;
    }
    priority = ClActor_GetPriority(actor);
    headPriority = ClActor_GetPriority(unit->actorList);
    if (priority < headPriority) {
        pos = unit->actorList->prev;
        unit->actorList = actor;
    } else if ((u8)((headPriority + ClActor_GetPriority(unit->actorList->prev)) / 2) >= priority) {
        pos = ClActUnit_FindInsertPosFromHead(unit->actorList, priority);
    } else {
        pos = ClActUnit_FindInsertPosFromTail(unit->actorList->prev, priority);
    }
    actor->prev = pos;
    actor->next = pos->next;
    pos->next->prev = actor;
    pos->next = actor;
}

static void ClActUnit_RemoveActor(ClActUnit *unit, ClActor *actor) {
    if (unit->actorList == actor) {
        if (actor->next == actor) {
            unit->actorList = NULL;
        } else {
            unit->actorList = actor->next;
        }
    }
    actor->next->prev = actor->prev;
    actor->prev->next = actor->next;
    actor->next = NULL;
    actor->prev = NULL;
}

static ClActor *ClActUnit_FindInsertPosFromHead(ClActor *head, u8 priority) {
    ClActor *cur = head;

    do {
        if (ClActor_GetPriority(cur) > priority) {
            return cur->prev;
        }
        cur = cur->next;
    } while (cur != head);
    return head->prev;
}

static ClActor *ClActUnit_FindInsertPosFromTail(ClActor *tail, u8 priority) {
    ClActor *cur = tail;

    do {
        if (ClActor_GetPriority(cur) <= priority) {
            return cur;
        }
        cur = cur->prev;
    } while (cur != tail);
    return tail->next;
}

static void ClActor_Reset(ClActor *actor) {
    MI_CpuFill8(actor, 0, sizeof(ClActor));
    actor->autoAnimSpeed = FX32_ONE;
    NNS_G2dInitImageProxy(&actor->imgProxy);
    NNS_G2dInitImagePaletteProxy(&actor->pltProxy);
    actor->scale.x = FX32_ONE;
    actor->overwrite |= NNS_G2D_RND_OVERWRITE_PRIORITY;
    actor->scale.y = FX32_ONE;
    actor->paletteMode = 0;
    actor->affineMode = 0;
}

static void ClActor_ApplySetup(ClActor *actor, const ClActorSetup *setup, u16 surface) {
    ClActor_SetSurfacePosAxis(actor, setup->x, surface, 0);
    ClActor_SetSurfacePosAxis(actor, setup->y, surface, 1);
    actor->priority = setup->priority;
    ClActor_SetBgPriority(actor, setup->bgPriority);
    ClActor_SetSequence(actor, setup->sequence);
    ClActor_StartAnim(actor);
}

static void ClActor_ApplyRes(ClActor *actor, const ClActResSetup *res) {
    ClActor_SetImageProxy(actor, &res->chars->proxy);
    ClActor_SetPlttProxy(actor, &res->pltt->proxy);
    actor->charIdx = res->charIdx;
    actor->plttIdx = res->plttIdx;
}

static void ClActor_GetSurfaceOffset(ClActor *actor, u32 surface, ClActorPos *offset) {
    if (surface != CLACT_SURFACE_NONE) {
        ClActRenderer_GetSurfacePos(actor->unit->renderer, surface, offset);
    } else {
        offset->x = 0;
        offset->y = 0;
    }
}

static void ClActResSetup_Init(ClActResSetup *res, u16 charIdx, u16 plttIdx, ClActCharRes *chars, ClActPlttRes *pltt,
                               NNSG2dCellDataBank *cells, NNSG2dAnimBankData *anims) {
    res->charIdx = charIdx;
    res->plttIdx = plttIdx;
    res->pltt = pltt;
    res->chars = chars;
    res->cells = cells;
    res->anims = anims;
    res->mcBank = NULL;
    res->mcAnims = NULL;
    res->transferChar = NULL;
}

static ClActor *ClActor_CreateFromRes(ClActUnit *unit, const ClActorSetup *setup, const ClActResSetup *res, u16 surface,
                                      HeapID heapId) {
    ClActor *actor = ClActUnit_FindFreeActor(unit);

    actor->unit = unit;
    ClActAnim_Init(&actor->anim, res, heapId);
    ClActor_ApplySetup(actor, setup, surface);
    ClActor_ApplyRes(actor, res);
    ClActUnit_InsertActor(unit, actor);
    ClActor_SetVisible(actor, TRUE);
    return actor;
}

static ClActor *ClActor_CreateExFromRes(ClActUnit *unit, const ClActorSetupEx *setup, const ClActResSetup *res,
                                        u16 surface, HeapID heapId) {
    ClActor *actor = ClActor_CreateFromRes(unit, &setup->base, res, surface, heapId);

    ClActor_SetAffineCenterAxis(actor, setup->affineCenter.x, 0);
    // BUG: the y of the affine center is set to its x
#ifdef BUGFIX
    ClActor_SetAffineCenterAxis(actor, setup->affineCenter.y, 1);
#else
    ClActor_SetAffineCenterAxis(actor, setup->affineCenter.x, 1);
#endif
    ClActor_SetScaleAxis(actor, setup->scaleX, 0);
    ClActor_SetScaleAxis(actor, setup->scaleY, 1);
    ClActor_SetRotation(actor, setup->rotation);
    ClActor_SetAffineMode(actor, setup->affineMode);
    return actor;
}

static void ClActAnim_Init(ClActAnim *anim, const ClActResSetup *res, HeapID heapId) {
    anim->type = ClActAnim_GetType(res);
    sAnimInitFuncs[anim->type](anim, res, heapId);
}

static void ClActAnim_Free(ClActAnim *anim) {
    sAnimFreeFuncs[anim->type](anim);
    MI_CpuFill8(anim, 0, sizeof(ClActAnim));
}

static u32 ClActAnim_GetType(const ClActResSetup *res) {
    if (res->transferChar != NULL) {
        return CLACT_ANIM_VRAM_TRANSFER;
    }
    if (res->mcBank != NULL) {
        return CLACT_ANIM_MULTICELL;
    }
    return CLACT_ANIM_CELL;
}

static void ClActAnim_InitCell(ClActAnim *anim, const ClActResSetup *res, HeapID heapId) {
    anim->cells = res->cells;
    anim->anims = res->anims;
    NNS_G2dInitCellAnimation(&anim->cellAnim, NNS_G2dGetAnimSequenceByIdx(anim->anims, 0), anim->cells);
}

static void ClActAnim_InitVramTransfer(ClActAnim *anim, const ClActResSetup *res, HeapID heapId) {
    NNSG2dCharacterData *transferChar;
    const NNSG2dAnimSequence *seq;

    anim->cells = res->cells;
    anim->anims = res->anims;
    anim->transferHandle = NNS_G2dGetNewCellTransferStateHandle();
    transferChar = res->transferChar;
    seq = NNS_G2dGetAnimSequenceByIdx(anim->anims, 0);
    NNS_G2dInitCellAnimationVramTransfered(&anim->cellAnim, seq, anim->cells, anim->transferHandle,
                                           NNS_G2D_VRAM_ADDR_NONE,
                                           NNS_G2dGetImageLocation(&res->chars->proxy, NNS_G2D_VRAM_TYPE_2DMAIN),
                                           NNS_G2dGetImageLocation(&res->chars->proxy, NNS_G2D_VRAM_TYPE_2DSUB),
                                           transferChar->rawData, NULL, transferChar->size);
}

static void ClActAnim_InitMultiCell(ClActAnim *anim, const ClActResSetup *res, HeapID heapId) {
    anim->cells = res->cells;
    anim->anims = res->anims;
    anim->mcBank = res->mcBank;
    anim->mcAnims = res->mcAnims;
    anim->mcWork = GFL_HeapAllocate(heapId, NNS_G2dGetMCWorkAreaSize(anim->mcBank, NNS_G2D_MCTYPE_SHARE_CELLANIM),
                                    FALSE, "clact.c", 6175);
    NNS_G2dInitMCAnimationInstance(&anim->mcAnim, anim->mcWork, anim->anims, anim->cells, anim->mcBank,
                                   NNS_G2D_MCTYPE_SHARE_CELLANIM);
    NNS_G2dSetAnimSequenceToMCAnimation(&anim->mcAnim, NNS_G2dGetAnimSequenceByIdx(anim->mcAnims, 0));
}

static void ClActAnim_FreeCell(ClActAnim *anim) {
    MI_CpuFill8(anim, 0, sizeof(ClActAnim));
}

static void ClActAnim_FreeVramTransfer(ClActAnim *anim) {
    NNS_G2dFreeCellTransferStateHandle(anim->transferHandle);
    MI_CpuFill8(anim, 0, sizeof(ClActAnim));
}

static void ClActAnim_FreeMultiCell(ClActAnim *anim) {
    GFL_HeapFree(anim->mcWork);
    MI_CpuFill8(anim, 0, sizeof(ClActAnim));
}

static void ClActAnim_SetSequence(ClActAnim *anim, u16 sequence) {
    sAnimSetSequenceFuncs[anim->type](anim, sequence);
}

static void ClActAnim_SetSequenceCell(ClActAnim *anim, u16 sequence) {
    NNS_G2dSetCellAnimationSequence(&anim->cellAnim, NNS_G2dGetAnimSequenceByIdx(anim->anims, sequence));
}

static void ClActAnim_SetSequenceVramTransfer(ClActAnim *anim, u16 sequence) {
    NNS_G2dSetCellAnimationSequence(&anim->cellAnim, NNS_G2dGetAnimSequenceByIdx(anim->anims, sequence));
}

static void setSeqToMultiCellAnim(ClActAnim *anim, u16 sequence) {
    NNS_G2dSetAnimSequenceToMCAnimation(&anim->mcAnim, NNS_G2dGetAnimSequenceByIdx(anim->mcAnims, sequence));
}

static void ClActAnim_SetTime(ClActAnim *anim, fx32 time) {
    ClActAnim_GetCtrl(anim)->currentTime = time;
    ClActAnim_Tick(anim, 0);
}

void ClActAnim_Tick(ClActAnim *anim, fx32 frames) {
    sAnimTickFuncs[anim->type](anim, frames);
}

static void ClActAnim_TickCell(ClActAnim *anim, fx32 frames) {
    NNS_G2dTickCellAnimation(&anim->cellAnim, frames);
}

static void ClActAnim_TickVramTransfer(ClActAnim *anim, fx32 frames) {
    NNS_G2dTickCellAnimation(&anim->cellAnim, frames);
}

static void ClActAnim_TickMultiCell(ClActAnim *anim, fx32 frames) {
    NNS_G2dTickMCAnimation(&anim->mcAnim, frames);
}

static NNSG2dAnimController *ClActAnim_GetCtrl(ClActAnim *anim) {
    return sAnimGetCtrlFuncs2[anim->type](anim);
}

static NNSG2dAnimController *ClActAnim_GetCtrlCell(ClActAnim *anim) {
    return &anim->cellAnim.animCtrl;
}

static NNSG2dAnimController *ClActAnim_GetCtrlVramTransfer(ClActAnim *anim) {
    return &anim->cellAnim.animCtrl;
}

static NNSG2dAnimController *ClActAnim_GetCtrlMultiCell(ClActAnim *anim) {
    return &anim->mcAnim.animCtrl;
}

static NNSG2dAnimController *ClActAnim_GetCtrl2(ClActAnim *anim) {
    return sAnimGetCtrlFuncs[anim->type](anim);
}

static NNSG2dAnimController *ClActAnim_GetCtrl2Cell(ClActAnim *anim) {
    return &anim->cellAnim.animCtrl;
}

static NNSG2dAnimController *ClActAnim_GetCtrl2VramTransfer(ClActAnim *anim) {
    return &anim->cellAnim.animCtrl;
}

static NNSG2dAnimController *ClActAnim_GetCtrl2MultiCell(ClActAnim *anim) {
    return &anim->mcAnim.animCtrl;
}

static void ClActAnim_SetFrame(ClActAnim *anim, u16 frame) {
    sAnimSetFrameFuncs[anim->type](anim, frame);
}

static void ClActAnim_SetFrameCell(ClActAnim *anim, u16 frame) {
    NNS_G2dSetCellAnimationCurrentFrame(&anim->cellAnim, frame);
}

static void ClActAnim_SetFrameVramTransfer(ClActAnim *anim, u16 frame) {
    NNS_G2dSetCellAnimationCurrentFrame(&anim->cellAnim, frame);
}

static void ClActAnim_SetFrameMultiCell(ClActAnim *anim, u16 frame) {
    NNS_G2dSetMCAnimationCurrentFrame(&anim->mcAnim, frame);
}

void ClActAnim_SetSpeed(ClActAnim *anim, fx32 speed) {
    sAnimSetSpeedFuncs[anim->type](anim, speed);
}

static void ClActAnim_SetSpeedCell(ClActAnim *anim, fx32 speed) {
    NNS_G2dSetCellAnimationSpeed(&anim->cellAnim, speed);
}

static void ClActAnim_SetSpeedVramTransfer(ClActAnim *anim, fx32 speed) {
    NNS_G2dSetCellAnimationSpeed(&anim->cellAnim, speed);
}

static void ClActAnim_SetSpeedMultiCell(ClActAnim *anim, fx32 speed) {
    NNS_G2dSetMCAnimationSpeed(&anim->mcAnim, speed);
}

static void ClActAnim_Restart(ClActAnim *anim) {
    sAnimRestartFuncs[anim->type](anim);
}

static void ClActAnim_RestartCell(ClActAnim *anim) {
    NNS_G2dRestartCellAnimation(&anim->cellAnim);
}

static void ClActAnim_RestartVramTransfer(ClActAnim *anim) {
    NNS_G2dRestartCellAnimation(&anim->cellAnim);
}

static void ClActAnim_RestartMultiCell(ClActAnim *anim) {
    ClActAnim_RestartCell(anim);
    NNS_G2dRestartMCCellAnimations(&anim->mcAnim.multiCellInstance);
}

static const NNSG2dUserExAnimSequenceAttr *ClActAnim_GetUserExSeqAttrCell(ClActAnim *anim, u32 sequence) {
    const NNSG2dUserExAnimAttrBank *bank = NNS_G2dGetUserExAnimAttrBank(anim->anims);

    if (bank != NULL) {
        return NNS_G2dGetUserExAnimSequenceAttr(bank, sequence);
    }
    return NULL;
}

static const NNSG2dUserExAnimSequenceAttr *ClActAnim_GetUserExSeqAttrVramTransfer(ClActAnim *anim, u32 sequence) {
    const NNSG2dUserExAnimAttrBank *bank = NNS_G2dGetUserExAnimAttrBank(anim->anims);

    if (bank != NULL) {
        return NNS_G2dGetUserExAnimSequenceAttr(bank, sequence);
    }
    return NULL;
}

static const NNSG2dUserExAnimSequenceAttr *ClActAnim_GetUserExSeqAttrMultiCell(ClActAnim *anim, u32 sequence) {
    const NNSG2dUserExAnimAttrBank *bank = NNS_G2dGetUserExAnimAttrBank(anim->mcAnims);

    if (bank != NULL) {
        return NNS_G2dGetUserExAnimSequenceAttr(bank, sequence);
    }
    return NULL;
}

static const NNSG2dUserExCellAttrBank *ClActAnim_GetUserExCellAttrBankCell(ClActAnim *anim) {
    return NNS_G2dGetUserExCellAttrBankFromCellBank(anim->cells);
}

static const NNSG2dUserExCellAttrBank *ClActAnim_GetUserExCellAttrBankVramTransfer(ClActAnim *anim) {
    return NNS_G2dGetUserExCellAttrBankFromCellBank(anim->cells);
}

static const NNSG2dUserExCellAttrBank *ClActAnim_GetUserExCellAttrBankMultiCell(ClActAnim *anim) {
    return NNS_G2dGetUserExCellAttrBankFromMCBank(anim->mcBank);
}

static u32 ClActAnim_GetSequenceCount(ClActAnim *anim) {
    return sAnimGetSequenceCountFuncs[anim->type](anim);
}

static u32 ClActAnim_GetSequenceCountCell(ClActAnim *anim) {
    return anim->anims->numSequences;
}

static u32 ClActAnim_GetSequenceCountVramTransfer(ClActAnim *anim) {
    return anim->anims->numSequences;
}

static u32 ClActAnim_GetSequenceCountMultiCell(ClActAnim *anim) {
    return anim->mcAnims->numSequences;
}

static void ClActAnim_SetCallback(ClActAnim *anim, const ClActorCallback *callback) {
    NNSG2dAnimController *animCtrl = ClActAnim_GetCtrl(anim);

    switch (callback->type) {
    case CLACT_CALLBACK_LAST_FRAME:
        NNS_G2dSetAnimCtrlCallBackFunctor(animCtrl, NNS_G2D_ANMCALLBACKTYPE_LAST_FRM, callback->param, callback->func);
        break;
    case CLACT_CALLBACK_EVERY_FRAME:
        NNS_G2dSetAnimCtrlCallBackFunctor(animCtrl, NNS_G2D_ANMCALLBACKTYPE_EVER_FRM, callback->param, callback->func);
        break;
    case CLACT_CALLBACK_FRAME:
        NNS_G2dSetAnimCtrlCallBackFunctorAtAnimFrame(animCtrl, callback->param, callback->func, callback->frame);
        break;
    }
}

static void ClActAnim_ClearCallback(ClActAnim *anim) {
    NNS_G2dSetAnimCtrlCallBackFunctor(ClActAnim_GetCtrl(anim), NNS_G2D_ANMCALLBACKTYPE_NONE, 0, NULL);
}

static void ClActVRAMManager_Init(ClActResMan *man, HeapID heapId, const BGSysVRAMConfig *vramConfig,
                                  const ClActResManSetup *setup) {
    man->vmanMain = GFL_VRAMManagerCreate(heapId, VMAN_TYPE_OBJ, vramConfig->objMain, setup->offsetMain << 5,
                                          vramConfig->objMappingMain);
    man->vmanSub = GFL_VRAMManagerCreate(heapId, VMAN_TYPE_OBJ, vramConfig->objSub, setup->offsetSub << 5,
                                         vramConfig->objMappingSub);
    man->chars = ClActCharRes_AllocArray(heapId, setup->charCount);
    man->pltts = ClActPlttRes_AllocArray(heapId, setup->plttCount);
    man->cellAnims = ClActCellAnimRes_AllocArray(heapId, setup->cellAnimCount);
    man->charCount = setup->charCount;
    man->plttCount = setup->plttCount;
    man->cellAnimCount = setup->cellAnimCount;
    man->unk1A = setup->unk6;
    man->offsetMain = setup->offsetMain;
    man->offsetSub = setup->offsetSub;
}

static void ClActResMan_Free(ClActResMan *man) {
    u32 i;

    for (i = 0; i < man->charCount; i++) {
        if (!man->chars[i].free) {
            ClActSys_FreeChars(i);
        }
    }
    for (i = 0; i < man->cellAnimCount; i++) {
        if (man->cellAnims[i].free == FALSE) {
            ClActSys_FreeCellAnims(i);
        }
    }
    for (i = 0; i < man->plttCount; i++) {
        if (!man->pltts[i].free) {
            ClActSys_FreePltt(i);
        }
    }
    ClActCharRes_FreeArray(man->chars);
    ClActPlttRes_FreeArray(man->pltts);
    ClActCellAnimRes_FreeArray(man->cellAnims);
    GFL_VRAMManagerFree(man->vmanSub);
    GFL_VRAMManagerFree(man->vmanMain);
    man->vmanMain = NULL;
    man->vmanSub = NULL;
    man->chars = NULL;
    man->pltts = NULL;
    man->cellAnims = NULL;
}

static BOOL ClAct_VramTypeIncludes(u32 vramType, u32 which) {
    if (vramType == CLACT_VRAM_BOTH || which == CLACT_VRAM_BOTH) {
        return TRUE;
    }
    if (vramType == which) {
        return TRUE;
    }
    return FALSE;
}

static void *ClAct_ReadCharFile(ArcTool *arc, u32 fileId, BOOL compressed, HeapID heapId) {
    void *data;

    if (compressed) {
        void *file = GFL_ArcToolReadHeapNew(arc, fileId, HEAPID_TAIL(heapId));

        data = GFL_HeapAllocate(heapId, *(u32 *)file >> 8, FALSE, "clact.c", 7335);
        MI_UncompressLZ8(file, data);
        GFL_HeapFree(file);
    } else {
        data = GFL_ArcToolReadHeapNew(arc, fileId, HEAPID_TAIL(heapId));
    }
    return data;
}

static ClActCharRes *ClActCharRes_AllocArray(HeapID heapId, u32 count) {
    ClActCharRes *chars = GFL_HeapAllocate(heapId, count * sizeof(ClActCharRes), FALSE, "clact.c", 7360);
    u32 i = 0;

    for (; i < count; i++) {
        chars[i].free = TRUE;
        GFL_VRAMManagerAllocInit(&chars[i].allocMain);
        GFL_VRAMManagerAllocInit(&chars[i].allocSub);
    }
    return chars;
}

static void ClActResMan_SetupChars(ClActResMan *man, u32 idx, NNSG2dCharacterData *charData,
                                   NNSG2dCellDataBank *transferCells, u32 vramType) {
    ClActCharRes *res = &man->chars[idx];
    u32 size;
    BOOL vramTransfer;

    if (transferCells == NULL) {
        size = charData->size;
        vramTransfer = FALSE;
    } else {
        size = ClAct_GetTransferSize(transferCells);
        vramTransfer = TRUE;
    }
    NNS_G2dInitImageProxy(&res->proxy);
    if (ClAct_VramTypeIncludes(vramType, CLACT_VRAM_MAIN) &&
        GFL_VRAMManagerAlloc(man->vmanMain, size, &res->allocMain)) {
        u32 addr = GFL_VRAMManagerGetAllocAddress(man->vmanMain, &res->allocMain);

        ClAct_SetCharMapping(charData, NNS_G2D_VRAM_TYPE_2DMAIN);
        if (vramTransfer) {
            NNS_G2dLoadImageVramTransfer(charData, addr, NNS_G2D_VRAM_TYPE_2DMAIN, &res->proxy);
        } else if (charData->mappingType != GX_OBJVRAMMODE_CHAR_2D) {
            NNS_G2dLoadImage1DMapping(charData, addr, NNS_G2D_VRAM_TYPE_2DMAIN, &res->proxy);
        } else {
            NNS_G2dLoadImage2DMapping(charData, addr, NNS_G2D_VRAM_TYPE_2DMAIN, &res->proxy);
        }
    }
    if (ClAct_VramTypeIncludes(vramType, CLACT_VRAM_SUB) && GFL_VRAMManagerAlloc(man->vmanSub, size, &res->allocSub)) {
        u32 addr = GFL_VRAMManagerGetAllocAddress(man->vmanSub, &res->allocSub);

        ClAct_SetCharMapping(charData, NNS_G2D_VRAM_TYPE_2DSUB);
        if (vramTransfer) {
            NNS_G2dLoadImageVramTransfer(charData, addr, NNS_G2D_VRAM_TYPE_2DSUB, &res->proxy);
        } else if (charData->mappingType != GX_OBJVRAMMODE_CHAR_2D) {
            NNS_G2dLoadImage1DMapping(charData, addr, NNS_G2D_VRAM_TYPE_2DSUB, &res->proxy);
        } else {
            NNS_G2dLoadImage2DMapping(charData, addr, NNS_G2D_VRAM_TYPE_2DSUB, &res->proxy);
        }
    }
    if (!vramTransfer) {
        charData = NULL;
    }
    res->transferChar = charData;
    res->size = size;
}

static void ClActCharRes_FreeArray(ClActCharRes *chars) {
    GFL_HeapFree(chars);
}

// Makes the characters' mapping the engine's, which loading them follows
static void ClAct_SetCharMapping(NNSG2dCharacterData *charData, NNSG2dVRamType type) {
    u32 mapping = type == NNS_G2D_VRAM_TYPE_2DMAIN ? GX_GetOBJVRamModeChar() : GXS_GetOBJVRamModeChar();

    if (charData->mappingType != mapping) {
        charData->mappingType = mapping;
    }
}

static ClActPlttRes *ClActPlttRes_AllocArray(HeapID heapId, u32 count) {
    ClActPlttRes *pltts = GFL_HeapAllocate(heapId, count * sizeof(ClActPlttRes), FALSE, "clact.c", 7514);
    u32 i = 0;

    for (; i < count; i++) {
        pltts[i].free = TRUE;
    }
    return pltts;
}

static void ClActResMan_SetupPlttRange(ClActResMan *man, u32 idx, void *file, u32 vramType, u16 offset, u16 start,
                                       u16 count) {
    ClActPlttRes *res = &man->pltts[idx];
    u32 limit = 0x200;
    BOOL extended;
    int maxPalettes;
    NNSG2dPaletteData *pltt;
    NNSG2dPaletteCompressInfo *cmpInfo;

    if (offset < limit) {
        extended = FALSE;
        maxPalettes = 16;
    } else {
        offset -= 0x200;
        extended = TRUE;
        maxPalettes = 256;
        limit = 0x2000;
    }
    NNS_G2dGetUnpackedPaletteCompressInfo(file, &cmpInfo);
    if (NNS_G2dGetUnpackedPaletteData(file, &pltt)) {
        u32 banks;

        NNS_G2dInitImagePaletteProxy(&res->proxy);
        if (start != 0 && start * 32 < pltt->size) {
            pltt->rawData = (u8 *)pltt->rawData + start * 32;
            pltt->size -= start * 32;
        }
        if (count != 0 && start + count <= maxPalettes && count * 32 < pltt->size) {
            pltt->size = count * 32;
        }
        if (offset != 0 && pltt->size + offset > limit) {
            pltt->size = limit - offset;
        }
        if (ClAct_VramTypeIncludes(vramType, CLACT_VRAM_MAIN)) {
            banks = GX_GetBankForOBJExtPltt();
            if (extended) {
                GX_SetBankForLCDC(banks);
            }
            NNS_G2dLoadPalette(pltt, offset, NNS_G2D_VRAM_TYPE_2DMAIN, &res->proxy);
            if (extended) {
                GX_SetBankForOBJExtPltt(banks);
            }
        }
        if (ClAct_VramTypeIncludes(vramType, CLACT_VRAM_SUB)) {
            banks = GX_GetBankForSubOBJExtPltt();
            if (extended) {
                GX_SetBankForLCDC(banks);
            }
            NNS_G2dLoadPalette(pltt, offset, NNS_G2D_VRAM_TYPE_2DSUB, &res->proxy);
            if (extended) {
                GX_SetBankForSubOBJExtPltt(banks);
            }
        }
        man->pltts[idx].free = FALSE;
        man->pltts[idx].size = pltt->size;
    }
}

static void ClActResMan_SetupPlttEx(ClActResMan *man, u32 idx, void *file, u32 vramType, u16 offset) {
    ClActPlttRes *res = &man->pltts[idx];
    BOOL extended;
    NNSG2dPaletteData *pltt;
    NNSG2dPaletteCompressInfo *cmpInfo;

    if (offset < 0x200) {
        extended = FALSE;
    } else {
        extended = TRUE;
        offset -= 0x200;
    }
    NNS_G2dGetUnpackedPaletteCompressInfo(file, &cmpInfo);
    if (NNS_G2dGetUnpackedPaletteData(file, &pltt)) {
        NNS_G2dInitImagePaletteProxy(&res->proxy);
        if (ClAct_VramTypeIncludes(vramType, CLACT_VRAM_MAIN)) {
            u32 banks = GX_GetBankForOBJExtPltt();

            if (extended) {
                GX_SetBankForLCDC(banks);
            }
            NNS_G2dLoadPaletteEx(pltt, cmpInfo, offset, NNS_G2D_VRAM_TYPE_2DMAIN, &res->proxy);
            if (extended) {
                GX_SetBankForOBJExtPltt(banks);
            }
        }
        if (ClAct_VramTypeIncludes(vramType, CLACT_VRAM_SUB)) {
            u32 banks = GX_GetBankForSubOBJExtPltt();

            if (extended) {
                GX_SetBankForLCDC(banks);
            }
            NNS_G2dLoadPaletteEx(pltt, cmpInfo, offset, NNS_G2D_VRAM_TYPE_2DSUB, &res->proxy);
            if (extended) {
                GX_SetBankForSubOBJExtPltt(banks);
            }
        }
        man->pltts[idx].free = FALSE;
    }
}

static void ClActPlttRes_FreeArray(ClActPlttRes *pltts) {
    GFL_HeapFree(pltts);
}

static ClActCellAnimRes *ClActCellAnimRes_AllocArray(HeapID heapId, u32 count) {
    ClActCellAnimRes *cellAnims = GFL_HeapAllocate(heapId, count * sizeof(ClActCellAnimRes), FALSE, "clact.c", 7752);
    u32 i = 0;

    for (; i < count; i++) {
        cellAnims[i].free = TRUE;
    }
    return cellAnims;
}

static BOOL ClActResMan_SetupCellAnims(ClActResMan *man, u32 idx, void *cellFile, void *animFile) {
    ClActCellAnimRes *res = &man->cellAnims[idx];

    if (NNS_G2dGetUnpackedCellBank(cellFile, &res->cells) && NNS_G2dGetUnpackedAnimBank(animFile, &res->anims)) {
        res->free = FALSE;
        return TRUE;
    }
    return FALSE;
}

static void ClActCellAnimRes_FreeArray(ClActCellAnimRes *cellAnims) {
    GFL_HeapFree(cellAnims);
}

static void ClActResMan_SetupCharsFromFile(ClActResMan *man, u32 idx, void *file, u32 vramType,
                                           NNSG2dCellDataBank *transferCells) {
    NNSG2dCharacterData *charData;

    if (NNS_G2dGetUnpackedCharacterData(file, &charData)) {
        ClActResMan_SetupChars(man, idx, charData, transferCells, vramType);
        man->chars[idx].free = FALSE;
    }
}
