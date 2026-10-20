#include "types.h"
#include "app/subway_map.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "save/player_info.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// The process that shows the subway map

#define BG_MAIN_MAP 1
#define BG_MAIN_STATIONS 2
#define BG_SUB_MAP 4

// The steps of the process
enum {
    STEP_START,
    STEP_WAIT_FADE_IN,
    STEP_WAIT_INPUT,
    STEP_WAIT_FADE_OUT,
    STEP_END,
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} SubwayMapBGSetup;

typedef struct {
    u16 heapId;
    SubwayMapParam *param;
    SubwayMapGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    TCB *vblankTask;
} SubwayMapWork;

static BOOL SubwayMap_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL SubwayMap_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL SubwayMap_Main(GameProc *proc, u32 *state, void *param, void *work);
static void SubwayMap_VBlank(TCB *tcb, void *data);
static void SubwayMap_LoadGraphics(SubwayMapWork *work);
static void SubwayMap_Unused(SubwayMapWork *work);

const GameProcFunctions SUBWAY_MAP_PROC_FUNCTIONS = { SubwayMap_Init, SubwayMap_Main, SubwayMap_Exit };

static const SubwayMapBGSetup sSubBGSetups[1] = {
    { BG_SUB_MAP,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x8000), GX_BG_CHARBASE(0x0000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

static BOOL SubwayMap_Init(GameProc *proc, u32 *state, void *param, void *work) {
    SubwayMapWork *wk;
    ArcTool *arc;
    HeapID heapId;
    u32 palette;
    int i;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_SUBWAY_MAP, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(SubwayMapWork), HEAPID_SUBWAY_MAP);
    MI_CpuFill8(wk, 0, sizeof(SubwayMapWork));
    wk->heapId = HEAPID_SUBWAY_MAP;
    wk->param = param;
    wk->graphic = SubwayMapGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    GFL_BGSysSetBGPriority(1, 1);
    GFL_BGSysSetBGPriority(2, 0);
    wk->vblankTask = GFL_VBlankTCBAdd(SubwayMap_VBlank, wk, 1);
    SubwayMap_LoadGraphics(wk);
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_DispOn();
    GFL_BGSysDisableAllB();
    GXS_SetGraphicsMode(0);
    GFL_BGSysDisableBGsB();
    for (i = 0; i < NELEMS(sSubBGSetups); i++) {
        GFL_BGSysCreateBG(sSubBGSetups[i].bg, &sSubBGSetups[i].setup, sSubBGSetups[i].mode);
        GFL_BGSysClearBG(sSubBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sSubBGSetups[i].bg, sSubBGSetups[i].enabled);
    }
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    heapId = wk->heapId;
    palette = TRUE;
    if (wk->param->playerInfo->gender != GENDER_FEMALE) {
        palette = FALSE;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, palette, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, BG_SUB_MAP, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, BG_SUB_MAP, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_SUB_MAP);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
    GFL_NetSetIconScreen(FALSE, wk->heapId);
    return TRUE;
}

static BOOL SubwayMap_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    SubwayMapWork *wk = work;
    int i;

    for (i = 0; i < NELEMS(sSubBGSetups); i++) {
        GFL_BGSysReleaseBG(sSubBGSetups[i].bg);
    }
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    SubwayMap_Unused(wk);
    GFL_TCBRemove(wk->vblankTask);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    SubwayMapGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_SUBWAY_MAP);
    return TRUE;
}

static BOOL SubwayMap_Main(GameProc *proc, u32 *state, void *param, void *work) {
    SubwayMapWork *wk = work;
    BOOL done;
    u32 x;
    u32 y;

    switch (*state) {
    case STEP_START:
        *state = STEP_WAIT_FADE_IN;
        break;
    case STEP_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            *state = STEP_WAIT_INPUT;
        }
        break;
    case STEP_WAIT_INPUT:
        done = FALSE;
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GCTX_HIDSetUsingTouch(FALSE);
            done = TRUE;
        } else if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
            done = TRUE;
            GCTX_HIDSetUsingTouch(TRUE);
        }
        if (done) {
            *state = STEP_WAIT_FADE_OUT;
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        }
        break;
    case STEP_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            *state = STEP_END;
        }
        break;
    case STEP_END:
        return TRUE;
    }
    PrintQueue_Main(wk->printQueue);
    SubwayMapGraphic_Update(wk->graphic);
    return FALSE;
}

static void SubwayMap_VBlank(TCB *tcb, void *data) {
}

static void SubwayMap_LoadGraphics(SubwayMapWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_SUBWAY_MAP, work->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, BG_MAIN_MAP, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 2, BG_MAIN_MAP, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, BG_MAIN_STATIONS, 0, 0, FALSE, work->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_MAIN_MAP);
    GFL_BGSysLoadScr(BG_MAIN_STATIONS);
}

static void SubwayMap_Unused(SubwayMapWork *work) {
}
