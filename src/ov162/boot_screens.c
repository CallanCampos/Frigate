#include "types.h"
#include "app/boot_screens.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "nitro/hw.h"

// The screens that the game shows when it starts: The Pokémon Company and Nintendo logos, which a button skips, then
// the copyright notice

enum {
    STATE_INIT,
    STATE_LOAD_LOGOS,
    STATE_WAIT_LOGOS_FADE_IN,
    STATE_SHOW_LOGOS,
    STATE_FADE_OUT_LOGOS,
    STATE_WAIT_LOGOS_FADE_OUT,
    STATE_PAUSE_AFTER_LOGOS,
    STATE_LOAD_COPYRIGHT,
    STATE_WAIT_COPYRIGHT_FADE_IN,
    STATE_SHOW_COPYRIGHT,
    STATE_FADE_OUT_COPYRIGHT,
    STATE_WAIT_COPYRIGHT_FADE_OUT,
    STATE_PAUSE_AFTER_COPYRIGHT,
    STATE_END,
};

typedef struct {
    u16 state;
    s32 timer;
} BootScreensWork;

static BOOL BootScreens_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BootScreens_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BootScreens_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions BOOT_SCREENS_PROC_FUNCTIONS = { BootScreens_Init, BootScreens_Main, BootScreens_Exit };

static const BGSysLCDConfig sBootLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

static const BGSetup sBootBGSetupMain = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x0000),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sBootBGSetupSub = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x0000),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSysVRAMConfig sBootVRAMConfig = {
    GX_VRAM_BG_128_D,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static BOOL BootScreens_Init(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TITLE, 0x70000);
    MI_CpuFill8(GFL_ProcInitSubsystem(proc, sizeof(BootScreensWork), HEAPID_TITLE), 0, sizeof(BootScreensWork));
    return TRUE;
}

static BOOL BootScreens_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BootScreensParam *bootParam = param;
    BootScreensWork *wk = work;
    enum { BRIGHTNESS_NORMAL = 0 } brightnessEnd;

    switch (wk->state) {
    case STATE_INIT:
        GFL_BGSysSetEnabledBGsA(0);
        GFL_BGSysSetEnabledBGsB(0);
        G2_BlendNone();
        G2S_BlendNone();
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
        GXS_SetVisibleWnd(GX_WNDMASK_NONE);
        GFL_BGSysSetVRAMBanks(&sBootVRAMConfig);
        GFL_BGSysCreate(HEAPID_TITLE);
        GFL_BGSysSetLCDConfig(&sBootLCDConfig);
        GFL_BGSysCreateBG(1, &sBootBGSetupMain, BGMODE_TEXT);
        GFL_BGSysCreateBG(5, &sBootBGSetupSub, BGMODE_TEXT);
        if (bootParam->skipLogos == TRUE) {
            GX_SetMasterBrightness(-16);
            GXS_SetMasterBrightness(-16);
            wk->state = STATE_LOAD_COPYRIGHT;
        } else {
            GX_SetMasterBrightness(16);
            GXS_SetMasterBrightness(16);
            wk->state = STATE_LOAD_LOGOS;
        }
        break;
    case STATE_LOAD_LOGOS:
        brightnessEnd = BRIGHTNESS_NORMAL;
        GFL_BGSysLoadNCGRStatic(ARCID_TITLE, 12, 1, 0, 0, TRUE, HEAPID_TITLE);
        loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TITLE, 13, 1, 0, 0, TRUE, HEAPID_TITLE);
        GFL_BGSysLoadNCLRDefault(ARCID_TITLE, 11, 0, 0, 0, HEAPID_TITLE);
        GFL_BGSysLoadNCGRStatic(ARCID_TITLE, 12, 5, 0, 0, TRUE, HEAPID_TITLE);
        loadBGScrToVramByNarcNoReserveNegAlign(ARCID_TITLE, 14, 5, 0, 0, TRUE, HEAPID_TITLE);
        GFL_BGSysLoadNCLRDefault(ARCID_TITLE, 11, 4, 0, 0, HEAPID_TITLE);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysSetDisplayLayout(1);
        GFL_BGSysEnableEngines();
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, brightnessEnd, 2);
        wk->state = STATE_WAIT_LOGOS_FADE_IN;
        break;
    case STATE_WAIT_LOGOS_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            wk->state = STATE_SHOW_LOGOS;
            wk->timer = 0;
        }
        break;
    case STATE_SHOW_LOGOS:
        if (++wk->timer > 90) {
            wk->state = STATE_FADE_OUT_LOGOS;
        }
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_START)) {
            wk->state = STATE_FADE_OUT_LOGOS;
        }
        break;
    case STATE_FADE_OUT_LOGOS:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2);
        wk->state = STATE_WAIT_LOGOS_FADE_OUT;
        break;
    case STATE_WAIT_LOGOS_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            wk->state = STATE_PAUSE_AFTER_LOGOS;
            wk->timer = 0;
        }
        break;
    case STATE_PAUSE_AFTER_LOGOS:
        if (++wk->timer > 30) {
            wk->state = STATE_LOAD_COPYRIGHT;
            wk->timer = 0;
        }
        break;
    case STATE_LOAD_COPYRIGHT:
        GFL_BGSysLoadNCGRStatic(ARCID_COPYRIGHT, 1, 1, 0, 0x8000, FALSE, HEAPID_TITLE);
        loadBGScrToVramByNarcNoReserveNegAlign(ARCID_COPYRIGHT, 2, 1, 0, 0, FALSE, HEAPID_TITLE);
        GFL_BGSysLoadNCLRDefault(ARCID_COPYRIGHT, 0, 0, 0, 0, HEAPID_TITLE);
        GFL_BGSysLoadNCGRStatic(ARCID_COPYRIGHT, 3, 5, 0, 0x8000, FALSE, HEAPID_TITLE);
        loadBGScrToVramByNarcNoReserveNegAlign(ARCID_COPYRIGHT, 4, 5, 0, 0, FALSE, HEAPID_TITLE);
        GFL_BGSysLoadNCLRDefault(ARCID_COPYRIGHT, 0, 4, 0, 0, HEAPID_TITLE);
        GFL_BGSysLoadScr(1);
        GFL_BGSysLoadScr(5);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysSetDisplayLayout(1);
        GFL_BGSysEnableEngines();
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 2);
        wk->state = STATE_WAIT_COPYRIGHT_FADE_IN;
        break;
    case STATE_WAIT_COPYRIGHT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            wk->state = STATE_SHOW_COPYRIGHT;
        }
        break;
    case STATE_SHOW_COPYRIGHT:
        if (++wk->timer > 90) {
            wk->state = STATE_FADE_OUT_COPYRIGHT;
            wk->timer = 0;
        }
        break;
    case STATE_FADE_OUT_COPYRIGHT:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2);
        wk->state = STATE_WAIT_COPYRIGHT_FADE_OUT;
        break;
    case STATE_WAIT_COPYRIGHT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            wk->state = STATE_PAUSE_AFTER_COPYRIGHT;
        }
        break;
    case STATE_PAUSE_AFTER_COPYRIGHT:
        if (++wk->timer > 30) {
            wk->state = STATE_END;
        }
        break;
    case STATE_END:
        GFL_BGSysFree();
        return TRUE;
    }
    return FALSE;
}

static BOOL BootScreens_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_TITLE);
    return TRUE;
}
