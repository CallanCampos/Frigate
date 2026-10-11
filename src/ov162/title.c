#include "types.h"
#include "app/delete_save.h"
#include "app/mic_test.h"
#include "app/start_menu.h"
#include "app/title.h"
#include "constants/arc.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/version.h"
#include "text/system/title_16.h"

// The title screen: a 3D scene under a camera that follows a curve, with 2D layers over it. The main engine draws to
// VRAM D through the display capture, blending each frame with the last for a motion blur, and shows VRAM D.
// A or Start goes on to the start menu, Up, Select and B to deleting the save, and Down, X and Y to the microphone
// test. If none of them is pressed, the title ends after TIMEOUT_FRAMES without starting anything

// The cry of the version's Kyurem, when A or Start is pressed
#ifdef BLACK2
#define KYUREM_FORM 2
#else
#define KYUREM_FORM 1
#endif

#define DELETE_SAVE_KEYS (PAD_KEY_UP | PAD_BUTTON_SELECT | PAD_BUTTON_B)
#define MIC_TEST_KEYS (PAD_KEY_DOWN | PAD_BUTTON_X | PAD_BUTTON_Y)

// After this many frames, the title flashes white, puts the 3D scene on the top screen and hides main BG 3 and the
// sub engine's BG 2 and OBJ. A, B or Start then puts them back
#define HIDE_LAYERS_FRAMES 1740
// The title ends after this many frames
#define TIMEOUT_FRAMES 6780

// Frames of the camera curve: where it goes when a button brings the layers back, and loops from its end after that,
// and where it goes when the title is left
#define CURVE_LOOP_FRAME 7300
#define CURVE_END_FRAME 7780
#define CURVE_LEAVE_FRAME 7001

enum {
    STATE_INIT_DISPLAY,
    STATE_INIT,
    STATE_WAIT_FADE_IN,
    STATE_RUN,
    STATE_WAIT_CRY,
    STATE_FADE_OUT,
    STATE_WAIT_FADE_OUT,
    STATE_FREE,
    STATE_END,
};

// What the title goes on to
enum {
    NEXT_START_MENU,
    NEXT_NONE,
    NEXT_DELETE_SAVE,
    NEXT_MIC_TEST,
};

enum {
    LAYERS_SHOWN,
    LAYERS_HIDDEN,
    LAYERS_BACK,
};

typedef struct {
    TCBExManager *tcbManager;
    TCB *vblankTask;
    // The display capture's blend factors, out of 16, for the new frame and the last
    s16 captureEva;
    s16 captureEvb;
} TitleSys;

typedef struct {
    BmpWin *window;
    GFLBitmap *bitmap;
    PrintWindow print;
    BOOL unk10;
    Font *font;
    PrintQueue *printQueue;
    u32 unk1C;
    MsgData *msgData;
    StrBuf *strbuf;
    // Scrolls sub BG 2, one way in Black 2 and the other in White 2
    s32 scroll;
} TitleBG;

typedef struct {
    G3DManager *manager;
    u16 scene;
    G3DLight *light;
    G3DCurve *curve;
    G3DCamera *camera;
} TitleG3D;

typedef struct {
    ClActUnit *unit;
    ClActor *actor;
    u8 unk8[0x24];
    u32 palette;
    u32 chars;
    u32 cellAnims;
} TitleObj;

typedef struct {
    u16 state;
    u16 heapId;
    u32 next;
    TitleSys sys;
    TitleBG bg;
    TitleG3D g3d;
    TitleObj obj;
    u32 frames;
    u32 cry;
    u32 layers;
    u32 vblankCount;
} TitleWork;

static BOOL Title_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Title_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Title_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void Title_UpdateTimer(TitleWork *wk);
static void Title_HandleInput(TitleWork *wk);
static void Title_ResetDisplay(void);
static void Title_SetBanks(void);
static void Title_Update(TitleWork *wk);
static void TitleSys_VBlank(TCB *tcb, void *data);
static void TitleSys_Init(TitleSys *sys, HeapID heapId);
static void TitleSys_Update(TitleSys *sys, HeapID heapId);
static void TitleSys_Free(TitleSys *sys);
static void TitleSys_SetCaptureBlend(TitleSys *sys, s16 eva, s16 evb);
static void TitleBG_Init(TitleBG *bg, HeapID heapId);
static void TitleBG_Update(TitleBG *bg, HeapID heapId);
static void TitleBG_Free(TitleBG *bg);
static void TitleG3D_Init(TitleG3D *g3d, HeapID heapId);
static void TitleG3D_StepCurve(TitleG3D *g3d, HeapID heapId);
static void TitleG3D_Draw(TitleG3D *g3d, HeapID heapId);
static void TitleG3D_Free(TitleG3D *g3d);
static void TitleG3D_SetFrame(TitleG3D *g3d, u32 frame);
static void TitleObj_Init(TitleObj *obj, HeapID heapId);
static void TitleObj_Update(TitleObj *obj, HeapID heapId);
static void TitleObj_Free(TitleObj *obj);

// Declared in an order that gives the original layout of the data (tools/decomp/rodata_order.py)

#ifdef BLACK2
#define CURVE_FILE 453
#else
#define CURVE_FILE 461
#endif

static const G3DSceneAnimationSetup sAnimations0[] = { { 1, 0 } };

static const G3DSceneAnimationSetup sAnimations1[] = { { 3, 0 } };

static const G3DSceneAnimationSetup sAnimations2[] = { { 5, 0 }, { 6, 0 } };

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sAnimations0, NELEMS(sAnimations0) },
    { 2, 0, 2, 0, sAnimations1, NELEMS(sAnimations1) },
    { 4, 0, 4, 0, sAnimations2, NELEMS(sAnimations2) },
};

// The model and animations of the version's Kyurem and of the other actors
static const G3DSceneResourceSetup sResources[] = {
#ifdef BLACK2
    { ARCID_DEMO3D_RESOURCE, 447, 0 }, { ARCID_DEMO3D_RESOURCE, 446, 0 }, { ARCID_DEMO3D_RESOURCE, 449, 0 },
    { ARCID_DEMO3D_RESOURCE, 448, 0 }, { ARCID_DEMO3D_RESOURCE, 451, 0 }, { ARCID_DEMO3D_RESOURCE, 452, 0 },
    { ARCID_DEMO3D_RESOURCE, 450, 0 },
#else
    { ARCID_DEMO3D_RESOURCE, 455, 0 }, { ARCID_DEMO3D_RESOURCE, 454, 0 }, { ARCID_DEMO3D_RESOURCE, 457, 0 },
    { ARCID_DEMO3D_RESOURCE, 456, 0 }, { ARCID_DEMO3D_RESOURCE, 459, 0 }, { ARCID_DEMO3D_RESOURCE, 460, 0 },
    { ARCID_DEMO3D_RESOURCE, 458, 0 },
#endif
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

static const BGSetup sBGSetupSub3 = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
    GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sBGSetupSub2 = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x04000), 0x8000,
    GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sBGSetupMain3 = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x04000), 0x4000,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSetup sBGSetupSub1 = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x0c000), 0x4000,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};

static const VecFx32 sCameraPosition = { 0, 0, 0 };

static const VecFx32 sCameraTarget = { 0, 0, 0 };

static const VecFx32 sCameraUpVector = { 0, FX32_ONE, 0 };

static const LightSetup sLightSetups[] = {
    { 0, { { FX16_ONE - 1, -(FX16_ONE - 1) / 2, -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 1, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 2, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
    { 3, { { -(FX16_ONE - 1), -(FX16_ONE - 1), -(FX16_ONE - 1) }, GX_RGB(31, 31, 31) } },
};

static const LightSetupList sLightSetupList = { sLightSetups, NELEMS(sLightSetups) };

// The animations of each actor
static const s32 sAnimationCounts[] = { 1, 1, 2 };

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_VRAM_D, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

static const SRTMatrix sTransform = {
    { 0, 0, 0 },
    { FX32_ONE, FX32_ONE, FX32_ONE },
    { { { FX32_ONE, 0, 0 }, { 0, FX32_ONE, 0 }, { 0, 0, FX32_ONE } } },
};

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_32_FG,
    GX_VRAM_BGEXTPLTT_NONE,
    GX_VRAM_SUB_BG_128_C,
    GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_NONE,
    GX_VRAM_OBJEXTPLTT_NONE,
    GX_VRAM_SUB_OBJ_16_I,
    GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB,
    GX_VRAM_TEXPLTT_0123_E,
    GX_OBJVRAMMODE_CHAR_1D_64K,
    GX_OBJVRAMMODE_CHAR_1D_32K,
};

const GameProcFunctions TITLE_PROC_FUNCTIONS = { Title_Init, Title_Main, Title_Exit };

static BOOL Title_Init(GameProc *proc, u32 *state, void *param, void *work) {
    TitleWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TITLE, 0x120000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(TitleWork), HEAPID_TITLE);
    sys_memset(wk, 0, sizeof(TitleWork));
    wk->heapId = HEAPID_TITLE;
    GFL_SndStreamInit(HEAPID_TITLE);
    return TRUE;
}

static void Title_UpdateTimer(TitleWork *wk) {
    if (wk->frames == HIDE_LAYERS_FRAMES) {
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, 3);
        GFL_BGSysSetDisplayLayout(1);
        GFL_BGSysSetBGEnabled(6, FALSE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
        wk->layers = LAYERS_HIDDEN;
    }
    if (wk->layers == LAYERS_BACK && GFL_G3DCurveGetNowFrame(wk->g3d.curve) == CURVE_END_FRAME * FX32_ONE) {
        TitleG3D_SetFrame(&wk->g3d, CURVE_LOOP_FRAME);
    }
    if (++wk->frames > TIMEOUT_FRAMES) {
        wk->next = NEXT_NONE;
        wk->state = STATE_FADE_OUT;
    }
}

static void Title_HandleInput(TitleWork *wk) {
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_START)) &&
        wk->layers == LAYERS_HIDDEN) {
        GFL_BGSysSetDisplayLayout(0);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysSetBGEnabled(3, TRUE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
        TitleG3D_SetFrame(&wk->g3d, CURVE_LOOP_FRAME);
        wk->layers = LAYERS_BACK;
        return;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_START)) {
        wk->next = NEXT_START_MENU;
        wk->state = STATE_WAIT_CRY;
        wk->cry = PokeVoice_Play(SPECIES_KYUREM, KYUREM_FORM, 64, 0, 0, 0, 0, 0);
        TitleG3D_SetFrame(&wk->g3d, CURVE_LEAVE_FRAME);
        GFL_BGSysSetDisplayLayout(0);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, 3);
        GFL_SndStreamFadeStop(4);
        TitleSys_SetCaptureBlend(&wk->sys, 8, 8);
        return;
    }
    if (GCTX_HIDGetHeldKeys() == DELETE_SAVE_KEYS) {
        wk->next = NEXT_DELETE_SAVE;
        wk->state = STATE_FADE_OUT;
        return;
    }
    if (GCTX_HIDGetHeldKeys() == MIC_TEST_KEYS) {
        wk->next = NEXT_MIC_TEST;
        wk->state = STATE_FADE_OUT;
    }
}

static BOOL Title_Main(GameProc *proc, u32 *state, void *param, void *work) {
    TitleWork *wk = work;

    GFL_SndStreamUpdate();
    switch (wk->state) {
    case STATE_INIT_DISPLAY:
        Title_ResetDisplay();
        Title_SetBanks();
        wk->state = STATE_INIT;
        break;
    case STATE_INIT:
        TitleSys_Init(&wk->sys, wk->heapId);
        TitleBG_Init(&wk->bg, wk->heapId);
        TitleG3D_Init(&wk->g3d, wk->heapId);
        TitleObj_Init(&wk->obj, wk->heapId);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 2);
        wk->frames = 0;
        wk->state = STATE_WAIT_FADE_IN;
        GFL_SndStreamPlay(0);
        wk->vblankCount = OS_GetVBlankCount();
        break;
    case STATE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            wk->state = STATE_RUN;
        }
        Title_Update(wk);
        break;
    case STATE_RUN:
        Title_Update(wk);
        Title_HandleInput(wk);
        Title_UpdateTimer(wk);
        break;
    case STATE_WAIT_CRY:
        if (!PokeVoice_IsPlaying(wk->cry)) {
            wk->state = STATE_FADE_OUT;
        }
        Title_Update(wk);
        break;
    case STATE_FADE_OUT:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2);
        wk->state = STATE_WAIT_FADE_OUT;
        Title_Update(wk);
        GFL_SndStreamFadeStop(16);
        break;
    case STATE_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning() && !GFL_SndStreamIsPlaying()) {
            GFL_SndStreamStop();
            wk->state = STATE_FREE;
        }
        Title_Update(wk);
        break;
    case STATE_FREE:
        TitleObj_Free(&wk->obj);
        TitleG3D_Free(&wk->g3d);
        TitleBG_Free(&wk->bg);
        TitleSys_Free(&wk->sys);
        return TRUE;
    case STATE_END:
        return TRUE;
    }
    return FALSE;
}

static BOOL Title_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    TitleWork *wk = work;
    u32 next = wk->next;

    GFL_SndStreamFree();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_TITLE);
    if (next == NEXT_START_MENU) {
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &START_MENU_PROC_FUNCTIONS, NULL);
    } else if (next == NEXT_DELETE_SAVE) {
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(162), &DELETE_SAVE_PROC_FUNCTIONS, NULL);
    } else if (next == NEXT_MIC_TEST) {
        GCTX_ProcMgrReplaceProc(OVERLAY_ID(253), &MIC_TEST_PROC_FUNCTIONS, NULL);
    }
    return TRUE;
}

static void Title_ResetDisplay(void) {
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

static void Title_SetBanks(void) {
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
}

// Steps the camera once for each VBlank since the last update, so that it keeps its speed when frames are dropped
static void Title_Update(TitleWork *wk) {
    u32 vblankCount = OS_GetVBlankCount();
    s32 frames = vblankCount - wk->vblankCount;

    wk->vblankCount = vblankCount;
    TitleSys_Update(&wk->sys, wk->heapId);
    TitleBG_Update(&wk->bg, wk->heapId);
    do {
        TitleG3D_StepCurve(&wk->g3d, wk->heapId);
    } while (--frames != 0);
    TitleG3D_Draw(&wk->g3d, wk->heapId);
    TitleObj_Update(&wk->obj, wk->heapId);
}

static void TitleSys_VBlank(TCB *tcb, void *data) {
    TitleSys *sys = data;

    func_0204b7c8();
    GX_SetCapture(GX_CAPTURE_SIZE_256x192, GX_CAPTURE_MODE_AB, GX_CAPTURE_SRCA_2D3D, GX_CAPTURE_SRCB_VRAM_0x00000,
                  GX_CAPTURE_DEST_VRAM_D_0x00000, sys->captureEva, sys->captureEvb);
}

static void TitleSys_Init(TitleSys *sys, HeapID heapId) {
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_G3DSysCreate(FALSE, 2, 0, 2, 0x1000, heapId, NULL);
    func_020232d0();
    sys->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 4, 32);
    sys->vblankTask = GFL_VBlankTCBAdd(TitleSys_VBlank, sys, 5);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    sys->captureEva = 12;
    sys->captureEvb = 4;
}

static void TitleSys_Update(TitleSys *sys, HeapID heapId) {
    GFL_TCBExMgrUpdate(sys->tcbManager);
}

static void TitleSys_Free(TitleSys *sys) {
    GFL_BGSysSetDisplayLayout(1);
    GFL_BGSysEnableEngines();
    GFL_TCBRemove(sys->vblankTask);
    GFL_TCBExMgrFree(sys->tcbManager);
    GFL_G3DSysFree();
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void TitleSys_SetCaptureBlend(TitleSys *sys, s16 eva, s16 evb) {
    sys->captureEva = eva;
    sys->captureEvb = evb;
}

// The sub engine's BG 1 has a line of text, blank in this version, which TitleBG_Update hides
static void TitleBG_Init(TitleBG *bg, HeapID heapId) {
    s32 width;

    GFL_BGSysCreateBG(3, &sBGSetupMain3, BGMODE_TEXT);
    GFL_BGSysCreateBG(5, &sBGSetupSub1, BGMODE_TEXT);
    GFL_BGSysCreateBG(7, &sBGSetupSub3, BGMODE_TEXT);
    GFL_BGSysCreateBG(6, &sBGSetupSub2, BGMODE_TEXT);
    GFL_BGSysLoadNCGRStatic(ARCID_TITLE, 9, 3, 0, 0, TRUE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TITLE, 10, 3, 0, 0, TRUE, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_TITLE, 8, 0, 0, 0, heapId);
    GFL_BGSysLoadScr(3);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysLoadNCGRStatic(ARCID_TITLE, 0, 7, 0, 0x8000, TRUE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TITLE, 1, 7, 0, 0, TRUE, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_TITLE, 2, 4, 0, 0, heapId);
    GFL_BGSysLoadScr(7);
    GFL_BGSysSetBGEnabled(7, TRUE);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TITLE, 3, 6, 0, 0, TRUE, heapId);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysFillChar(5, 0, 1, 0);
    GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, 17);
    bg->window = BmpWin_CreateDynamic(5, 0, 19, 32, 2, 10, 0);
    bg->bitmap = BmpWin_GetBitmap(bg->window);
    GFL_BitmapFill(bg->bitmap, 0);
    BmpWin_FlushMap(bg->window);
    BmpWin_FlushChar(bg->window);
    bg->print.window = bg->window;
    bg->print.flushPending = FALSE;
    GFL_BGSysLoadScr(5);
    bg->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, heapId);
    bg->printQueue = func_02021998(heapId);
    bg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_16, heapId);
    bg->strbuf = GFL_StrBufCreate(64, heapId);
    GFL_MsgDataLoadStrbuf(bg->msgData, Title16_Text_Empty_2, bg->strbuf);
    width = GFL_FontGetBlockWidth(bg->strbuf, bg->font, 0);
    func_02021c7c(bg->printQueue, bg->bitmap, 128 - width / 2, 0, bg->strbuf, bg->font, PRINT_COLOR(1, 2, 0));
    bg->unk10 = TRUE;
}

static void TitleBG_Update(TitleBG *bg, HeapID heapId) {
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysMoveBGReq(6, BG_MOVE_SET_X, bg->scroll / 2);
    if (getGameVersion() == VERSION_BLACK2) {
        bg->scroll++;
    } else {
        bg->scroll--;
    }
}

static void TitleBG_Free(TitleBG *bg) {
    GFL_StrBufFree(bg->strbuf);
    GFL_MsgDataFree(bg->msgData);
    func_02021a18(bg->printQueue);
    GFL_FontFree(bg->font);
    BmpWin_Free(bg->window);
}

// The camera's frustum, for a 60 degree field of view and a 4:3 aspect
#define CAMERA_NEAR FX32_CONST(0.1)
#define CAMERA_ASPECT FX32_CONST(4.0 / 3.0)

static void TitleG3D_Init(TitleG3D *g3d, HeapID heapId) {
    u16 firstActor;
    u16 i;
    s32 j;
    s32 count;
    fx32 sin;
    fx32 cos;
    fx32 top;
    fx32 bottom;
    fx32 left;
    fx32 right;

    g3d->manager = GFL_G3DMgrCreate(7, 3, heapId);
    g3d->scene = GFL_G3DMgrNewScene(g3d->manager, &sSceneSetup);
    firstActor = GFL_G3DMgrGetSceneFirstActorIdx(g3d->manager, g3d->scene);
    for (i = 0; i < 3; i++) {
        count = sAnimationCounts[i];
        for (j = 0; j < count; j++) {
            GFL_G3DActorBindAnm(GFL_G3DMgrGetActor(g3d->manager, firstActor + i), j);
        }
    }
    sin = FX_SinIdx(DEG_TO_IDX(30));
    cos = FX_CosIdx(DEG_TO_IDX(30));
    top = FX_Div(FX_Mul(sin, CAMERA_NEAR), cos);
    bottom = -FX_Div(FX_Mul(sin, CAMERA_NEAR), cos);
    // The original divides near * aspect by FX32_ONE at run time
    right = FX_Div(FX_Mul(FX_Div(FX_Mul(CAMERA_NEAR, CAMERA_ASPECT), FX32_ONE), sin), cos);
    left = -FX_Div(FX_Mul(FX_Div(FX_Mul(CAMERA_NEAR, CAMERA_ASPECT), FX32_ONE), sin), cos);
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_FRUSTUM, top, bottom, left, right, CAMERA_NEAR,
                                      FX32_CONST(2048), CAMERA_NEAR, &sCameraPosition, &sCameraUpVector,
                                      &sCameraTarget, heapId);
    g3d->curve = GFL_G3DCurveLoadFileAll(heapId, ARCID_DEMO3D_RESOURCE, CURVE_FILE);
    g3d->light = GFL_G3DLightCreate(&sLightSetupList, heapId);
    GFL_G3DLightFlush(g3d->light);
    GFL_BGSysSet3DBGPriority(1);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
}

static void TitleG3D_StepCurve(TitleG3D *g3d, HeapID heapId) {
    GFL_G3DCurveFrameStep(g3d->curve, FX32_ONE);
}

static void TitleG3D_Draw(TitleG3D *g3d, HeapID heapId) {
    u16 firstActor;
    u32 i;
    u32 k;
    s32 count;
    s32 j;
    G3DActor *actor;

    firstActor = GFL_G3DMgrGetSceneFirstActorIdx(g3d->manager, g3d->scene);
    GFL_G3DCurveApplyCamera(g3d->camera, g3d->curve);
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    for (i = 0; i < 3; i++) {
        GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(g3d->manager, firstActor + i), &sTransform);
    }
    GFL_G3DSysReqSwapBuffers();
    for (k = 0; k < 3; k++) {
        count = sAnimationCounts[k];
        actor = GFL_G3DMgrGetActor(g3d->manager, firstActor + k);
        for (j = 0; j < count; j++) {
            GFL_G3DActorStepAnmFrameLoop(actor, j, FX32_ONE);
        }
    }
}

static void TitleG3D_Free(TitleG3D *g3d) {
    GFL_G3DCurveFree(g3d->curve);
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DLightFree(g3d->light);
    GFL_G3DMgrDeleteScene(g3d->manager, g3d->scene);
    GFL_G3DMgrFree(g3d->manager);
}

// Moves the camera and every animation to a frame
static void TitleG3D_SetFrame(TitleG3D *g3d, u32 frame) {
    fx32 frameFx = frame * FX32_ONE;
    u16 firstActor;
    u32 i;
    s32 count;
    s32 j;
    G3DActor *actor;

    GFL_G3DCurveFrameSet(g3d->curve, frameFx);
    firstActor = GFL_G3DMgrGetSceneFirstActorIdx(g3d->manager, g3d->scene);
    for (i = 0; i < 3; i++) {
        count = sAnimationCounts[i];
        actor = GFL_G3DMgrGetActor(g3d->manager, firstActor + i);
        for (j = 0; j < count; j++) {
            GFL_G3DActorSetAnmFrame(actor, j, &frameFx);
        }
    }
}

static void TitleObj_Init(TitleObj *obj, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_TITLE, heapId);
    ClActorSetup setup;

    ClActSys_Create(&data_02093f08, &sVRAMConfig, heapId);
    obj->unit = func_0204bf1c(64, 0, heapId);
    func_0204c028(obj->unit);
    obj->chars = func_0204b81c(arc, 5, 1, 1, heapId);
    obj->palette = func_0204bba0(arc, 4, 1, 0, heapId);
    obj->cellAnims = func_0204bde0(arc, 6, 7, heapId);
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 0;
    setup.bgPriority = 0;
    setup.priority = 0;
    obj->actor = func_0204c040(obj->unit, obj->chars, obj->palette, obj->cellAnims, &setup, 1, heapId);
    func_0204c520(obj->actor, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_ArcToolFree(arc);
}

static void TitleObj_Update(TitleObj *obj, HeapID heapId) {
    func_0204b794();
}

static void TitleObj_Free(TitleObj *obj) {
    func_0204b98c(obj->chars);
    func_0204bcd0(obj->palette);
    func_0204be64(obj->cellAnims);
    func_0204bf98(obj->unit);
    func_0204b758();
}
