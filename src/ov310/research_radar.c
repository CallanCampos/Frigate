#include "types.h"
#include "app/research_radar.h"
#include "app/research_radar/research_common.h"
#include "app/research_radar/research_graph.h"
#include "app/research_radar/research_list.h"
#include "app/research_radar/research_list_recovery.h"
#include "app/research_radar/research_top.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The Research Radar's proc, which shows the results of the surveys. The game doesn't name this file; it is named
// after the app. It runs one of three screens at a time, the top screen, the list of surveys and a survey's graph,
// which share the work in research_common.c, and draws the BGs that stay behind all of them

// The pattern's palette cycles: it changes every this many frames, through this many palettes, from this one
#define PATTERN_FRAMES 10
#define PATTERN_PALETTES 6
#define PATTERN_PALETTE_FIRST 0

typedef struct {
    HeapID heapId;
    GameSystem *gsys;
    u32 frames;
    ResearchCommon *common;
    ResearchTop *top;
    ResearchList *list;
    ResearchGraph *graph;
    ResearchListRecovery *recovery;
} ResearchRadarWork;

static BOOL ResearchRadar_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ResearchRadar_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ResearchRadar_Main(GameProc *proc, u32 *state, void *param, void *work);
static void ResearchRadar_UpdateComm(ResearchRadarWork *wk);
static void ResearchRadar_ChangeSeq(ResearchRadarWork *wk, u32 *state, u32 next);
static void ResearchRadar_EndSeq(ResearchRadarWork *wk, u32 *state);
static void ResearchRadar_StartSeq(ResearchRadarWork *wk, u32 *state, u32 next);
static u32 ResearchRadar_SeqInit(ResearchRadarWork *wk);
static u32 ResearchRadar_SeqTop(ResearchRadarWork *wk);
static u32 ResearchRadar_SeqList(ResearchRadarWork *wk);
static u32 ResearchRadar_SeqGraph(ResearchRadarWork *wk);
static u32 ResearchRadar_GetFrames(ResearchRadarWork *wk);
static void ResearchRadar_CountFrame(ResearchRadarWork *wk);
static void ResearchRadar_InitWork(ResearchRadarWork *wk, GameSystem *gsys);
static void ResearchRadar_CreateCommon(ResearchRadarWork *wk);
static void ResearchRadar_DeleteCommon(ResearchRadarWork *wk);
static void ResearchRadar_CreateRecovery(ResearchRadarWork *wk);
static void ResearchRadar_DeleteRecovery(ResearchRadarWork *wk);
static void ResearchRadar_InitBG(HeapID heapId);
static void ResearchRadar_ExitBG(void);
static void ResearchRadar_LoadSubBG(HeapID heapId);
static void ResearchRadar_AnimateSubBG(ResearchRadarWork *wk);
static void ResearchRadar_LoadMainBG(HeapID heapId);

static const BGSysLCDConfig sRadarLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSetup sRadarBGSubBack = { 0,
                                         0,
                                         0x800,
                                         0,
                                         BGRES_256x256,
                                         GX_BG_COLORMODE_16,
                                         GX_BG_SCRBASE(0x0000),
                                         GX_BG_CHARBASE(0x04000),
                                         0x8000,
                                         GX_BG_EXTPLTT_01,
                                         3,
                                         GX_BG_AREAOVER_XLU,
                                         FALSE };

static const BGSetup sRadarBGMainFrame = { 0,
                                           0,
                                           0x800,
                                           0,
                                           BGRES_256x256,
                                           GX_BG_COLORMODE_16,
                                           GX_BG_SCRBASE(0x0000),
                                           GX_BG_CHARBASE(0x04000),
                                           0x8000,
                                           GX_BG_EXTPLTT_01,
                                           3,
                                           GX_BG_AREAOVER_XLU,
                                           FALSE };

static const BGSetup sRadarBGSubPattern = { 0,
                                            0,
                                            0x800,
                                            0,
                                            BGRES_256x256,
                                            GX_BG_COLORMODE_16,
                                            GX_BG_SCRBASE(0x0800),
                                            GX_BG_CHARBASE(0x04000),
                                            0x8000,
                                            GX_BG_EXTPLTT_01,
                                            2,
                                            GX_BG_AREAOVER_XLU,
                                            FALSE };

static const BGSysVRAMConfig sRadarVRAMConfig = {
    GX_VRAM_BG_128_A, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_B,  GX_VRAM_TEXPLTT_0_G,     GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

GameProcFunctions RESEARCH_RADAR_PROC_FUNCTIONS = { ResearchRadar_Init, ResearchRadar_Main, ResearchRadar_Exit };

static BOOL ResearchRadar_Init(GameProc *proc, u32 *state, void *param, void *work) {
    ResearchRadarWork *wk = work;
    GameSystem **gsys = param;

    switch (*state) {
    case 0:
        GFL_ProcInitSubsystem(proc, sizeof(ResearchRadarWork), HEAPID_USER);
        (*state)++;
        break;
    case 1:
        ResearchRadar_InitWork(wk, *gsys);
        GFL_BGSysSetVRAMBanks(&sRadarVRAMConfig);
        GFL_BGSysSetDisplayLayout(0);
        ResearchRadar_InitBG(wk->heapId);
        ResearchRadar_LoadMainBG(wk->heapId);
        ResearchRadar_LoadSubBG(wk->heapId);
        ResearchRadar_CreateCommon(wk);
        ResearchRadar_CreateRecovery(wk);
        return TRUE;
    }
    return FALSE;
}

static BOOL ResearchRadar_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    ResearchRadarWork *wk = work;

    ResearchRadar_DeleteRecovery(wk);
    ResearchRadar_DeleteCommon(wk);
    ResearchRadar_ExitBG();
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static BOOL ResearchRadar_Main(GameProc *proc, u32 *state, void *param, void *work) {
    ResearchRadarWork *wk = work;
    u32 next;

    ResearchRadar_UpdateComm(wk);
    if (func_02016bec(wk->gsys)) {
        ResearchCommon_SetForceExit(wk->common);
    }

    switch (*state) {
    case RESEARCH_SEQ_INIT:
        next = ResearchRadar_SeqInit(wk);
        break;
    case RESEARCH_SEQ_TOP:
        next = ResearchRadar_SeqTop(wk);
        break;
    case RESEARCH_SEQ_LIST:
        next = ResearchRadar_SeqList(wk);
        break;
    case RESEARCH_SEQ_GRAPH:
        next = ResearchRadar_SeqGraph(wk);
        break;
    case RESEARCH_SEQ_END:
        return TRUE;
    }

    ResearchRadar_AnimateSubBG(wk);
    ResearchRadar_CountFrame(wk);
    if (*state != next) {
        ResearchRadar_ChangeSeq(wk, state, next);
    }
    return FALSE;
}

static void ResearchRadar_UpdateComm(ResearchRadarWork *wk) {
    func_02012be4(GameData_GetWifiList(GSYS_GetGameData(wk->gsys)));
    if (GSYS_TryBootGameComm(wk->gsys)) {
        func_02042ba8(TRUE, wk->heapId);
    }
}

static void ResearchRadar_ChangeSeq(ResearchRadarWork *wk, u32 *state, u32 next) {
    ResearchRadar_EndSeq(wk, state);
    ResearchRadar_StartSeq(wk, state, next);

    switch (*state) {
    case RESEARCH_SEQ_TOP:
        ResearchCommon_SetSeq(wk->common, RESEARCH_SEQ_TOP);
        break;
    case RESEARCH_SEQ_LIST:
        ResearchCommon_SetSeq(wk->common, RESEARCH_SEQ_LIST);
        break;
    case RESEARCH_SEQ_GRAPH:
        ResearchCommon_SetSeq(wk->common, RESEARCH_SEQ_GRAPH);
        break;
    }
}

static void ResearchRadar_EndSeq(ResearchRadarWork *wk, u32 *state) {
    switch (*state) {
    case RESEARCH_SEQ_INIT:
        break;
    case RESEARCH_SEQ_TOP:
        ResearchTop_Delete(wk->top);
        break;
    case RESEARCH_SEQ_LIST:
        ResearchList_Delete(wk->list);
        break;
    case RESEARCH_SEQ_GRAPH:
        ResearchGraph_Delete(wk->graph);
        break;
    case RESEARCH_SEQ_END:
        break;
    }

    switch (*state) {
    case RESEARCH_SEQ_INIT:
        break;
    case RESEARCH_SEQ_TOP:
        wk->top = NULL;
        break;
    case RESEARCH_SEQ_LIST:
        wk->list = NULL;
        break;
    case RESEARCH_SEQ_GRAPH:
        wk->graph = NULL;
        break;
    case RESEARCH_SEQ_END:
        break;
    }
}

static void ResearchRadar_StartSeq(ResearchRadarWork *wk, u32 *state, u32 next) {
    switch (next) {
    case RESEARCH_SEQ_INIT:
        break;
    case RESEARCH_SEQ_TOP:
        wk->top = ResearchTop_Create(wk->common);
        break;
    case RESEARCH_SEQ_LIST:
        wk->list = ResearchList_Create(wk->common, wk->recovery);
        break;
    case RESEARCH_SEQ_GRAPH:
        wk->graph = ResearchGraph_Create(wk->common);
        break;
    case RESEARCH_SEQ_END:
        break;
    }
    *state = next;
}

static u32 ResearchRadar_SeqInit(ResearchRadarWork *wk) {
    return RESEARCH_SEQ_TOP;
}

static u32 ResearchRadar_SeqTop(ResearchRadarWork *wk) {
    ResearchTop_Main(wk->top);
    if (ResearchTop_IsEnd(wk->top)) {
        switch (ResearchTop_GetNext(wk->top)) {
        case RESEARCH_TOP_NEXT_LIST:
            return RESEARCH_SEQ_LIST;
        case RESEARCH_TOP_NEXT_GRAPH:
            return RESEARCH_SEQ_GRAPH;
        case RESEARCH_TOP_NEXT_EXIT:
            return RESEARCH_SEQ_END;
        }
        return RESEARCH_SEQ_END;
    }
    return RESEARCH_SEQ_TOP;
}

static u32 ResearchRadar_SeqList(ResearchRadarWork *wk) {
    ResearchList_Main(wk->list);
    if (ResearchList_IsEnd(wk->list)) {
        if (ResearchList_GetNext(wk->list) == RESEARCH_LIST_NEXT_TOP) {
            return RESEARCH_SEQ_TOP;
        }
        return RESEARCH_SEQ_END;
    }
    return RESEARCH_SEQ_LIST;
}

static u32 ResearchRadar_SeqGraph(ResearchRadarWork *wk) {
    ResearchGraph_Main(wk->graph);
    if (ResearchGraph_IsEnd(wk->graph)) {
        if (ResearchGraph_GetNext(wk->graph) == RESEARCH_GRAPH_NEXT_TOP) {
            return RESEARCH_SEQ_TOP;
        }
        return RESEARCH_SEQ_END;
    }
    return RESEARCH_SEQ_GRAPH;
}

static u32 ResearchRadar_GetFrames(ResearchRadarWork *wk) {
    return wk->frames;
}

static void ResearchRadar_CountFrame(ResearchRadarWork *wk) {
    wk->frames++;
}

static void ResearchRadar_InitWork(ResearchRadarWork *wk, GameSystem *gsys) {
    wk->heapId = HEAPID_USER;
    wk->gsys = gsys;
    wk->frames = 0;
    wk->common = NULL;
    wk->top = NULL;
    wk->list = NULL;
    wk->graph = NULL;
    wk->recovery = NULL;
}

static void ResearchRadar_CreateCommon(ResearchRadarWork *wk) {
    wk->common = ResearchCommon_Create(wk->heapId, wk->gsys);
}

static void ResearchRadar_DeleteCommon(ResearchRadarWork *wk) {
    ResearchCommon_Delete(wk->common);
    wk->common = NULL;
}

static void ResearchRadar_CreateRecovery(ResearchRadarWork *wk) {
    wk->recovery = ResearchListRecovery_Create(wk->heapId);
    ResearchListRecovery_Init(wk->recovery);
}

static void ResearchRadar_DeleteRecovery(ResearchRadarWork *wk) {
    ResearchListRecovery_Delete(wk->recovery);
    wk->recovery = NULL;
}

static void ResearchRadar_InitBG(HeapID heapId) {
    GFL_BGSysCreate(heapId);
    GFL_BGSysSetLCDConfig(&sRadarLCDConfig);
    GFL_BGSysCreateBG(RESEARCH_BG_SUB_BACK, &sRadarBGSubBack, BGMODE_TEXT);
    GFL_BGSysCreateBG(RESEARCH_BG_SUB_PATTERN, &sRadarBGSubPattern, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_BACK, TRUE);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_SUB_PATTERN, TRUE);
    GFL_BGSysCreateBG(RESEARCH_BG_MAIN_FRAME, &sRadarBGMainFrame, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(RESEARCH_BG_MAIN_FRAME, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void ResearchRadar_ExitBG(void) {
    GFL_BGSysReleaseBG(RESEARCH_BG_MAIN_FRAME);
    GFL_BGSysReleaseBG(RESEARCH_BG_SUB_BACK);
    GFL_BGSysFree();
}

static void ResearchRadar_LoadSubBG(HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, heapId);
    void *file;
    NNSG2dPaletteData *palette;
    NNSG2dCharacterData *character;
    NNSG2dScreenData *screen;
    NNSG2dScreenData *patternScreen;

    file = GFL_ArcToolReadHeapNew(handle, 9, heapId);
    NNS_G2dGetUnpackedPaletteData(file, &palette);
    GFL_BGSysUploadStdPalette(RESEARCH_BG_SUB_BACK, palette->rawData, 0x200, 0);
    GFL_HeapFree(file);

    file = GFL_ArcToolReadHeapNew(handle, 10, heapId);
    NNS_G2dGetUnpackedBGCharacterData(file, &character);
    GFL_BGSysLoadChar(RESEARCH_BG_SUB_BACK, character->rawData, character->size, 0);
    GFL_HeapFree(file);

    file = GFL_ArcToolReadHeapNew(handle, 11, heapId);
    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(RESEARCH_BG_SUB_BACK, screen->rawData, 0, 0, 32, 24);
    GFL_BGSysLoadScr(RESEARCH_BG_SUB_BACK);
    GFL_HeapFree(file);

    file = GFL_ArcToolReadHeapNew(handle, 12, heapId);
    NNS_G2dGetUnpackedScreenData(file, &patternScreen);
    GFL_BGSysLoadScrAreaAll(RESEARCH_BG_SUB_PATTERN, patternScreen->rawData, 0, 0, 32, 24);
    GFL_BGSysLoadScr(RESEARCH_BG_SUB_PATTERN);
    GFL_HeapFree(file);

    GFL_ArcToolFree(handle);
}

static void ResearchRadar_AnimateSubBG(ResearchRadarWork *wk) {
    GFL_BGSysSetScrPaletteNo(
        RESEARCH_BG_SUB_PATTERN, 0, 0, 32, 24,
        (u8)(ResearchRadar_GetFrames(wk) / PATTERN_FRAMES % PATTERN_PALETTES) + PATTERN_PALETTE_FIRST);
    GFL_BGSysLoadScr(RESEARCH_BG_SUB_PATTERN);
}

static void ResearchRadar_LoadMainBG(HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_RESEARCH_RADAR, HEAPID_TAIL(heapId));
    void *file;
    NNSG2dPaletteData *palette;
    NNSG2dCharacterData *character;
    NNSG2dScreenData *screen;

    file = GFL_ArcToolReadHeapNew(handle, 0, heapId);
    NNS_G2dGetUnpackedPaletteData(file, &palette);
    GFL_BGSysUploadStdPalette(RESEARCH_BG_MAIN_FRAME, palette->rawData, 0x200, 0);
    GFL_HeapFree(file);

    file = GFL_ArcToolReadHeapNew(handle, 1, heapId);
    NNS_G2dGetUnpackedBGCharacterData(file, &character);
    GFL_BGSysLoadChar(RESEARCH_BG_MAIN_FRAME, character->rawData, character->size, 0);
    GFL_HeapFree(file);

    file = GFL_ArcToolReadHeapNew(handle, 2, heapId);
    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrAreaAll(RESEARCH_BG_MAIN_FRAME, screen->rawData, 0, 0, 32, 24);
    GFL_BGSysLoadScr(RESEARCH_BG_MAIN_FRAME);
    GFL_HeapFree(file);

    GFL_ArcToolFree(handle);
}
