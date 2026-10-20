#include "types.h"
#include "constants/arc.h"
#include "constants/version.h"
#include "demo/the_end.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/version.h"

// The process that shows the picture. It releases the BG 4 of its table, which it does not create

#define BG_PICTURE 1
#define BG_UNUSED 2

// Frames before the picture is shown again, which the steps that use it never reach
#define WAIT_FRAMES 900

// The steps of the process
enum {
    STEP_START,
    STEP_WAIT_FADE_IN_A,
    STEP_WAIT_TIME,
    STEP_WAIT_FADE_OUT,
    STEP_WAIT_FADE_IN,
    STEP_WAIT_INPUT,
    STEP_WAIT_FADE_OUT_END,
    STEP_END,
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} TheEndBGSetup;

typedef struct {
    u16 heapId;
    TheEndGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    void *param;
    TCB *vblankTask;
    s32 counter;
} TheEndWork;

static BOOL TheEnd_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL TheEnd_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL TheEnd_Main(GameProc *proc, u32 *state, void *param, void *work);
static void TheEnd_VBlank(TCB *tcb, void *data);
static void TheEnd_LoadGraphics(TheEndWork *work);
static void TheEnd_Unused(TheEndWork *work);
static void TheEnd_Fade(int start, int end, int slowness);

const GameProcFunctions THE_END_PROC_FUNCTIONS = { TheEnd_Init, TheEnd_Main, TheEnd_Exit };

static const TheEndBGSetup sUnusedBGSetups[1] = {
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x8000), GX_BG_CHARBASE(0x0000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// Counts a frame, and whether the time to wait is over
static inline BOOL TheEnd_CountFrame(TheEndWork *work) {
    BOOL over = FALSE;

    if (++work->counter > WAIT_FRAMES) {
        over = TRUE;
    }
    return over;
}

static BOOL TheEnd_Init(GameProc *proc, u32 *state, void *param, void *work) {
    u16 color[1] = { 0xffff };
    TheEndWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_THE_END, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(TheEndWork), HEAPID_THE_END);
    MI_CpuFill8(wk, 0, sizeof(TheEndWork));
    wk->heapId = HEAPID_THE_END;
    wk->param = param;
    wk->graphic = TheEndGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    GFL_BGSysSetBGPriority(1, 1);
    GFL_BGSysSetBGPriority(2, 0);
    wk->vblankTask = GFL_VBlankTCBAdd(TheEnd_VBlank, wk, 1);
    TheEnd_LoadGraphics(wk);
    if (getGameVersion() == VERSION_WHITE2) {
        GXS_LoadBGPltt(color, 0, 2);
    }
    TheEnd_Fade(16, 0, 0);
    return TRUE;
}

static BOOL TheEnd_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    TheEndWork *wk = work;
    int i;

    for (i = 0; i < NELEMS(sUnusedBGSetups); i++) {
        GFL_BGSysReleaseBG(sUnusedBGSetups[i].bg);
    }
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    TheEnd_Unused(wk);
    GFL_TCBRemove(wk->vblankTask);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    TheEndGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_THE_END);
    return TRUE;
}

static BOOL TheEnd_Main(GameProc *proc, u32 *state, void *param, void *work) {
    TheEndWork *wk = work;

    switch (*state) {
    case STEP_START:
        *state = STEP_WAIT_FADE_IN;
        break;
    case STEP_WAIT_FADE_IN_A:
        if (!GFL_FadeIsRunning()) {
            *state = STEP_WAIT_TIME;
        }
        break;
    case STEP_WAIT_TIME:
        if (TheEnd_CountFrame(wk)) {
            *state = STEP_WAIT_FADE_OUT;
            TheEnd_Fade(0, 16, 0);
        }
        break;
    case STEP_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            TheEnd_LoadGraphics(wk);
            TheEnd_Fade(16, 0, 0);
            *state = STEP_WAIT_FADE_IN;
        }
        break;
    case STEP_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            wk->counter = 0;
            *state = STEP_WAIT_INPUT;
        }
        break;
    case STEP_WAIT_INPUT:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_START)) {
            *state = STEP_WAIT_FADE_OUT_END;
            TheEnd_Fade(0, 16, 0);
        }
        break;
    case STEP_WAIT_FADE_OUT_END:
        if (!GFL_FadeIsRunning()) {
            *state = STEP_END;
        }
        break;
    case STEP_END:
        return TRUE;
    }
    PrintQueue_Main(wk->printQueue);
    TheEndGraphic_Update(wk->graphic);
    return FALSE;
}

static void TheEnd_VBlank(TCB *tcb, void *data) {
}

static void TheEnd_LoadGraphics(TheEndWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_THE_END, work->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 2, PALTYPE_MAIN_BG, 0, 0, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, BG_PICTURE, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 5, BG_PICTURE, 0, 0, FALSE, work->heapId);
    if (getGameVersion() == VERSION_WHITE2) {
        GFL_BGSysSetScrPaletteNo(BG_PICTURE, 0, 0, 32, 24, 1);
    }
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_PICTURE);
    GFL_BGSysSetBGEnabled(BG_PICTURE, TRUE);
    GFL_BGSysSetBGEnabled(BG_UNUSED, FALSE);
}

static void TheEnd_Unused(TheEndWork *work) {
}

// Fades both screens, to black in Black 2 and to white in White 2
static void TheEnd_Fade(int start, int end, int slowness) {
    if (getGameVersion() == VERSION_BLACK2) {
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, start, end, slowness);
    } else {
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, start, end, slowness);
    }
}
