// The Entralink's monolith: the proc that overlay 12's script command starts, which sets up the graphics, text and
// actors that its screens share and runs them, a top-screen and a bottom-screen proc for each of its menus. The ROM
// gives no name for this file, so monolith_main.c is a guess, after monolith_tool.c, whose name it does give

#include "types.h"
#include "app/monolith.h"
#include "app/monolith/monolith_main.h"
#include "app/monolith/monolith_power_select.h"
#include "app/monolith/monolith_record.h"
#include "app/monolith/monolith_status.h"
#include "app/monolith/monolith_top.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/event_work.h"
#include "save/high_link.h"
#include "save/save_control.h"
#include "system/actor_tool.h"
#include "system/app_menu_common.h"
#include "system/app_printsys_common.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The steps of the proc
enum {
    MONOLITH_SEQ_FADE_IN,
    MONOLITH_SEQ_FADE_IN_NEXT,
    MONOLITH_SEQ_FADE_IN_WAIT,
    MONOLITH_SEQ_MAIN,
    MONOLITH_SEQ_FADE_OUT,
    MONOLITH_SEQ_FADE_OUT_NEXT,
    MONOLITH_SEQ_FADE_OUT_WAIT,
    MONOLITH_SEQ_END,
};

// What GFL_ProcMgrUpdate returns while a proc runs
#define PROCMGR_RUNNING 1

typedef struct {
    const GameProcFunctions *top;
    const GameProcFunctions *bottom;
} MonolithScreenProcs;

static BOOL Monolith_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Monolith_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Monolith_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void Monolith_InitVRAM(void);
static void Monolith_FreeBGSys(void);
static void Monolith_InitBmpWin(void);
static void Monolith_FreeBmpWin(void);
static void Monolith_CreatePaletteFade(MonolithWork *wk);
static void Monolith_FreePaletteFade(MonolithWork *wk);
static void Monolith_CreateBGs(void);
static void Monolith_ReleaseBGs(void);
static void Monolith_LoadBGGraphics(MonolithWork *wk, MonolithWork *res);
static void Monolith_FreeBarChar(MonolithWork *wk);
static void Monolith_CreateText(MonolithWork *wk);
static void Monolith_FreeText(MonolithWork *wk);
static void Monolith_CreateActors(MonolithWork *wk);
static void Monolith_FreeActors(MonolithWork *wk);
static void Monolith_LoadActorResources(MonolithWork *wk);
static void Monolith_FreeActorResources(MonolithWork *wk);
static void Monolith_VBlank(TCB *tcb, void *data);
static void Monolith_UpdateScene(MonolithParam *param);

const GameProcFunctions MONOLITH_PROC_FUNCTIONS = { Monolith_Init, Monolith_Main, Monolith_Exit };

static const BGSysLCDConfig sMonolithLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSysVRAMConfig sMonolithVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE, GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_0_F, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,   GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The procs of each screen, by MONOLITH_SCREEN_*
static const MonolithScreenProcs sMonolithScreens[MONOLITH_SCREEN_COUNT] = {
    { &MONOLITH_STATUS_PROC_FUNCTIONS, &MONOLITH_TOP_PROC_FUNCTIONS },
    { &data_ov143_021a0134, &MONOLITH_POWER_SELECT_PROC_FUNCTIONS },
    { &MONOLITH_STATUS_PROC_FUNCTIONS, &MONOLITH_RECORD_PROC_FUNCTIONS },
    { &data_ov143_021a0388, &data_ov143_021a01e4 },
};

static const BGSetup sMonolithBGSetupMain = {
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
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sMonolithBGSetupSub = {
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
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static BOOL Monolith_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithWork *wk;

    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 15, 15);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 15, 15);
    GFL_BGSysSetDisplayLayout(1);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MONOLITH, 0x100000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(MonolithWork), HEAPID_MONOLITH);
    sys_memset(wk, 0, sizeof(MonolithWork));
    wk->state.focusPower = HIGH_LINK_POWER_NONE;
    wk->arc = GFL_ArcSysCreateFileHandle(ARCID_MONOLITH, HEAPID_MONOLITH);
    Monolith_InitVRAM();
    Monolith_InitBmpWin();
    Monolith_CreatePaletteFade(wk);
    Monolith_CreateBGs();
    Monolith_LoadBGGraphics(wk, wk);
    Monolith_CreateText(wk);
    Monolith_CreateActors(wk);
    Monolith_LoadActorResources(wk);
    wk->passPowerData = PassPowerData_Create(HEAPID_MONOLITH);
    wk->topProcMgr = CreateGameProcManager(HEAPID_MONOLITH);
    wk->bottomProcMgr = CreateGameProcManager(HEAPID_MONOLITH);
    AppPrintsysCommon_Init(&wk->printsys, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    wk->vblankTask = GFL_VBlankTCBAdd(Monolith_VBlank, wk, 10);
    func_02042ba8(FALSE, HEAPID_MONOLITH);
    wk->screen.param = param;
    wk->screen.work = wk;
    wk->screen.state = &wk->state;
    return TRUE;
}

static BOOL Monolith_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithParam *monolithParam = param;
    MonolithWork *wk = work;
    u32 bottom;
    u32 top;

    GSYS_GetGameCommSystem(monolithParam->gsys);
    switch (*state) {
    case MONOLITH_SEQ_FADE_IN:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1,
                    HEAPID_TAIL(HEAPID_MONOLITH));
        (*state)++;
        break;
    case MONOLITH_SEQ_FADE_IN_NEXT:
        (*state)++;
        break;
    case MONOLITH_SEQ_FADE_IN_WAIT:
        if (GFL_WipeIsFinished() == TRUE) {
            (*state)++;
        }
        break;
    case MONOLITH_SEQ_MAIN:
        if (wk->screen.exiting == FALSE) {
            if (wk->topStarted == FALSE) {
                QueueGameProc(wk->topProcMgr, OVERLAY_NONE, sMonolithScreens[wk->current].top, &wk->screen);
                wk->topStarted = TRUE;
            }
            if (wk->bottomStarted == FALSE) {
                GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
                QueueGameProc(wk->bottomProcMgr, OVERLAY_NONE, sMonolithScreens[wk->current].bottom, &wk->screen);
                wk->bottomStarted = TRUE;
            }
        }
        bottom = GFL_ProcMgrUpdate(wk->bottomProcMgr);
        if (wk->screen.next == MONOLITH_SCREEN_EXIT || (bottom != PROCMGR_RUNNING && wk->screen.exiting == TRUE)) {
            wk->screen.endTop = TRUE;
        } else if (wk->current != wk->screen.next &&
                   sMonolithScreens[wk->current].top != sMonolithScreens[wk->screen.next].top) {
            wk->screen.endTop = TRUE;
        }
        top = GFL_ProcMgrUpdate(wk->topProcMgr);
        if (top == PROCMGR_RUNNING && bottom == PROCMGR_RUNNING) {
            break;
        }
        if (wk->screen.exiting == TRUE || wk->screen.next == MONOLITH_SCREEN_EXIT) {
            if (top == FALSE && bottom == FALSE) {
                (*state)++;
            }
            break;
        }
        wk->current = wk->screen.next;
        if (top != PROCMGR_RUNNING) {
            wk->topStarted = FALSE;
            wk->screen.endTop = FALSE;
        }
        if (bottom != PROCMGR_RUNNING) {
            wk->bottomStarted = FALSE;
        }
        break;
    case MONOLITH_SEQ_FADE_OUT:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1,
                    HEAPID_TAIL(HEAPID_MONOLITH));
        (*state)++;
        break;
    case MONOLITH_SEQ_FADE_OUT_NEXT:
        (*state)++;
        break;
    case MONOLITH_SEQ_FADE_OUT_WAIT:
        if (GFL_WipeIsFinished() == TRUE) {
            (*state)++;
        }
        break;
    case MONOLITH_SEQ_END:
        return TRUE;
    }
    GFL_TCBExMgrUpdate(wk->tcbMgr);
    func_02021a3c(wk->printQueue);
    func_0204b794();
    return FALSE;
}

static BOOL Monolith_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithWork *wk = work;

    Monolith_UpdateScene(param);
    FreeGameProcManager(wk->topProcMgr);
    FreeGameProcManager(wk->bottomProcMgr);
    GFL_TCBRemove(wk->vblankTask);
    PassPowerData_Free(wk->passPowerData);
    Monolith_FreeActorResources(wk);
    Monolith_FreeActors(wk);
    Monolith_FreeText(wk);
    Monolith_FreeBarChar(wk);
    Monolith_ReleaseBGs();
    Monolith_FreePaletteFade(wk);
    Monolith_FreeBmpWin();
    Monolith_FreeBGSys();
    GFL_ArcToolFree(wk->arc);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MONOLITH);
    return TRUE;
}

static void Monolith_InitVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sMonolithVRAMConfig);
    GFL_BGSysCreate(HEAPID_MONOLITH);
    GFL_BGSysSetLCDConfig(&sMonolithLCDConfig);
    sys_memset32(0, (void *)HW_BG_VRAM, HW_BG_VRAM_SIZE);
    sys_memset32(0, (void *)HW_DB_BG_VRAM, HW_DB_BG_VRAM_SIZE);
    sys_memset32(0, (void *)HW_OBJ_VRAM, HW_OBJ_VRAM_SIZE);
    sys_memset32(0, (void *)HW_DB_OBJ_VRAM, HW_DB_OBJ_VRAM_SIZE);
}

static void Monolith_FreeBGSys(void) {
    GFL_BGSysFree();
}

static void Monolith_InitBmpWin(void) {
    BmpWin_InitAllocator(HEAPID_MONOLITH);
}

static void Monolith_FreeBmpWin(void) {
    BmpWin_FreeAllocator();
}

static void Monolith_CreatePaletteFade(MonolithWork *wk) {
    wk->fade = PaletteFade_Create(HEAPID_MONOLITH);
    PaletteFade_AllocBuffer(wk->fade, PALFADE_BUFFER_MAIN_OBJ, 14 * 0x20, HEAPID_MONOLITH);
    PaletteFade_AllocBuffer(wk->fade, PALFADE_BUFFER_SUB_OBJ, 14 * 0x20, HEAPID_MONOLITH);
    PaletteFade_AllocBuffer(wk->fade, PALFADE_BUFFER_MAIN_BG, 14 * 0x20, HEAPID_MONOLITH);
    PaletteFade_AllocBuffer(wk->fade, PALFADE_BUFFER_SUB_BG, 14 * 0x20, HEAPID_MONOLITH);
    PaletteFade_SetTransferAll(wk->fade, TRUE);
}

static void Monolith_FreePaletteFade(MonolithWork *wk) {
    PaletteFade_FreeBuffer(wk->fade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(wk->fade, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_FreeBuffer(wk->fade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(wk->fade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_Free(wk->fade);
}

static void Monolith_CreateBGs(void) {
    int i;

    GFL_BGSysCreateBG(MONOLITH_BG_MAIN, &sMonolithBGSetupMain, 0);
    GFL_BGSysCreateBG(MONOLITH_BG_SUB, &sMonolithBGSetupSub, 0);
    GFL_BGSysFillScrArea(MONOLITH_BG_MAIN, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_BG_SUB, 0, 0, 0, 32, 32, 17);
    // Every BG of both engines
    for (i = 0; i <= 7; i++) {
        GFL_BGSysSetBGEnabled(i, FALSE);
    }
    GFL_BGSysSetBGEnabled(MONOLITH_BG_MAIN, TRUE);
    GFL_BGSysSetBGEnabled(MONOLITH_BG_SUB, TRUE);
}

static void Monolith_ReleaseBGs(void) {
    GFL_BGSysSetBGEnabled(MONOLITH_BG_MAIN, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_BG_SUB, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_BG_MAIN);
    GFL_BGSysReleaseBG(MONOLITH_BG_SUB);
}

static void Monolith_LoadBGGraphics(MonolithWork *wk, MonolithWork *res) {
    ArcTool *menuArc;

    PaletteFade_LoadArcNCLREx(res->fade, res->arc, 18, HEAPID_MONOLITH, PALFADE_BUFFER_MAIN_BG, 14 * 0x20, 0, 0);
    PaletteFade_LoadArcNCLREx(res->fade, res->arc, 19, HEAPID_MONOLITH, PALFADE_BUFFER_SUB_BG, 14 * 0x20, 0, 0);
    GFL_BGSysLoadArcNCGRStatic(res->arc, 17, MONOLITH_BG_MAIN, 0, 0, TRUE, HEAPID_MONOLITH);
    loadBGScrToVramByFileNoReserveNegAlign(res->arc, 11, MONOLITH_BG_MAIN, 0, 0, TRUE, HEAPID_MONOLITH);
    GFL_BGSysLoadArcNCGRStatic(res->arc, 16, MONOLITH_BG_SUB, 0, 0, TRUE, HEAPID_MONOLITH);
    loadBGScrToVramByFileNoReserveNegAlign(res->arc, 4, MONOLITH_BG_SUB, 0, 0, TRUE, HEAPID_MONOLITH);
    PaletteFade_LoadNCLREx(res->fade, ARCID_FONT, 5, HEAPID_MONOLITH, PALFADE_BUFFER_MAIN_BG, 0, 13 * 0x10, 0);
    PaletteFade_LoadNCLREx(res->fade, ARCID_FONT, 5, HEAPID_MONOLITH, PALFADE_BUFFER_SUB_BG, 0, 13 * 0x10, 0);
    menuArc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_MONOLITH);
    wk->barChar = GFL_BGSysAllocChar(MONOLITH_BG_SUB, 0x80, TRUE);
    GFL_BGSysLoadArcNCGRStatic(menuArc, 28, MONOLITH_BG_SUB, wk->barChar, 0x80, FALSE, HEAPID_MONOLITH);
    AppMenuCommon_LoadBarScreen(menuArc, MONOLITH_BG_SUB, HEAPID_MONOLITH, wk->barChar, 12);
    PaletteFade_LoadArcNCLR(res->fade, menuArc, 27, HEAPID_MONOLITH, PALFADE_BUFFER_SUB_BG, 0x20, 12 * 0x10);
    GFL_ArcToolFree(menuArc);
    GFL_BGSysLoadScr(MONOLITH_BG_SUB);
    GFL_BGSysLoadScr(MONOLITH_BG_MAIN);
    GFL_BGSysLoadScr(MONOLITH_BG_SUB);
}

static void Monolith_FreeBarChar(MonolithWork *wk) {
    GFL_BGSysFreeCharMemory(MONOLITH_BG_SUB, wk->barChar, 0x80);
}

static void Monolith_CreateText(MonolithWork *wk) {
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    wk->tcbMgr = GFL_TCBExMgrCreate(HEAPID_MONOLITH, HEAPID_MONOLITH, 4, 0x20);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 1, FALSE, HEAPID_MONOLITH);
    wk->printQueue = func_02021998(HEAPID_MONOLITH);
    wk->wordSet = GFL_WordSetSystemCreate(4, 64, HEAPID_MONOLITH);
    wk->msgBlank = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_8, HEAPID_MONOLITH);
    wk->msgPowerNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PASS_POWERS, HEAPID_MONOLITH);
    wk->msgPowerInfo = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0264, HEAPID_MONOLITH);
    wk->msgMonolith = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_TITLE, HEAPID_MONOLITH);
}

static void Monolith_FreeText(MonolithWork *wk) {
    GFL_MsgDataFree(wk->msgBlank);
    GFL_MsgDataFree(wk->msgPowerNames);
    GFL_MsgDataFree(wk->msgPowerInfo);
    GFL_MsgDataFree(wk->msgMonolith);
    GFL_WordSetSystemFree(wk->wordSet);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    GFL_TCBExMgrFree(wk->tcbMgr);
}

static void Monolith_CreateActors(MonolithWork *wk) {
    ClActSysSetup setup = data_02093f08;

    setup.oamStartMain = 4;
    setup.oamCountMain = 124;
    setup.oamStartSub = 4;
    setup.oamCountSub = 124;
    setup.charCount = 96;
    setup.cellAnimCount = 64;
    ClActSys_Create(&setup, &sMonolithVRAMConfig, HEAPID_MONOLITH);
    wk->clactUnit = func_0204bf1c(96, 0, HEAPID_MONOLITH);
    func_0204c028(wk->clactUnit);
    wk->palSlots = ActorTool_CreatePalSlots(HEAPID_MONOLITH, 16, 16);
    wk->bmpOam = BmpOam_Init(HEAPID_MONOLITH, wk->clactUnit);
}

static void Monolith_FreeActors(MonolithWork *wk) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
    BmpOam_Exit(wk->bmpOam);
    func_0204bf98(wk->clactUnit);
    func_0204b758();
    ActorTool_DeletePalSlots(wk->palSlots);
}

static void Monolith_LoadActorResources(MonolithWork *wk) {
    ArcTool *fontArc = GFL_ArcSysCreateFileHandle(ARCID_FONT, HEAPID_MONOLITH);
    int i;

    for (i = 0; i < 2; i++) {
        BOOL sub = FALSE;

        if (i != 0) {
            sub = TRUE;
        }
        wk->actorRes[i].palette =
            ActorTool_LoadPalettesFade(wk->fade, wk->palSlots, wk->arc, 3, sub, 5, HEAPID_MONOLITH);
        wk->actorRes[i].chars = func_0204b81c(wk->arc, 0, TRUE, sub, HEAPID_MONOLITH);
        wk->actorRes[i].cellAnims = func_0204bde0(wk->arc, 1, 2, HEAPID_MONOLITH);
        wk->actorRes[i].fontPalette =
            ActorTool_LoadPaletteFade(wk->fade, wk->palSlots, fontArc, 5, sub, 1, HEAPID_MONOLITH);
    }
    GFL_ArcToolFree(fontArc);
}

static void Monolith_FreeActorResources(MonolithWork *wk) {
    int i;

    for (i = 0; i < 2; i++) {
        BOOL sub = FALSE;

        if (i != 0) {
            sub = TRUE;
        }
        ActorTool_FreePalette(wk->palSlots, wk->actorRes[i].palette, sub);
        func_0204b98c(wk->actorRes[i].chars);
        func_0204be64(wk->actorRes[i].cellAnims);
        ActorTool_FreePalette(wk->palSlots, wk->actorRes[i].fontPalette, sub);
    }
}

static void Monolith_VBlank(TCB *tcb, void *data) {
    MonolithWork *wk = data;

    func_0204b7c8();
    PaletteFade_Transfer(wk->fade);
}

// Moves the Entralink's story on once the player has a pass power
static void Monolith_UpdateScene(MonolithParam *param) {
    GameData *gameData = GSYS_GetGameData(param->gsys);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    HighLinkSave *highLink = getHighLinkBlockAddress(GameData_GetSaveControl(gameData));
    BOOL found = FALSE;
    u16 *scene = EventWork_GetWkPtr(eventWork, MONOLITH_SCENE_WORK);
    int i;

    if (*scene == MONOLITH_SCENE_FIRST_POWER) {
        for (i = 0; i < 3; i++) {
            if (func_0200c678(highLink, i) != HIGH_LINK_POWER_NONE) {
                found = TRUE;
                break;
            }
        }
        if (found == TRUE) {
            *scene = MONOLITH_SCENE_GOT_POWER;
        }
    }
}
