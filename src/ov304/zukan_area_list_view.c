#include "types.h"
#include "app/ui/frame_list.h"
#include "app/ui/print_msg.h"
#include "app/ui/ui_scene.h"
#include "app/zukan_area_list_view.h"
#include "app/zukan_common.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/aeabi.h"
#include "system/app_menu_common.h"
#include "system/blink_palanm.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"

// What the Pokédex's list of areas shows. The ROM doesn't name this file; zukan_area_list_view.c is a guess after
// zukan_area_list.c, whose work holds this file's

// The frames between the steps of the scrolling backgrounds
#define BG_SCROLL_FRAMES 4

// The y of the scroll bar's ends
#define SCROLL_BAR_TOP 12
#define SCROLL_BAR_BOTTOM 156

#define ROW_HEIGHT 24

// The value of the list's items
#define ITEM_AREA 1

static void ZukanAreaListView_InitVRAM(void);
static void ZukanAreaListView_InitBG(void);
static void ZukanAreaListView_InitActorSys(ZukanAreaListView *view);
static void ZukanAreaListView_InitWindows(ZukanAreaListView *view);
static void ZukanAreaListView_CreateActorSys(void);
static void ZukanAreaListView_LoadGraphics(void);
static void ZukanAreaListView_CreateActors(ZukanAreaListView *view);
static void ZukanAreaListView_InitPaletteFade(ZukanAreaListView *view);
static void ZukanAreaListView_InitWindowMask(void);
static void ZukanAreaListView_ExitWindowMask(void);
static void ZukanAreaListView_ExitBG(void);
static void ZukanAreaListView_ExitActors(ZukanAreaListView *view);
static void ZukanAreaListView_ExitPaletteFade(ZukanAreaListView *view);
static void ZukanAreaListView_ExitWindows(ZukanAreaListView *view);
static void ZukanAreaListView_InitBlink(ZukanAreaListView *view);
static void ZukanAreaListView_ExitBlink(ZukanAreaListView *view);
static void ZukanAreaListView_InitBlend(void);
static void ZukanAreaListView_Update(ZukanAreaListView *view);
static void ZukanAreaListView_Blink(ZukanAreaListView *view);
static u32 ZukanAreaListView_GetItemScreen(ZukanHabitatSummary *area);
static void ZukanAreaListView_PrintItem(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void ZukanAreaListView_SelectItem(void *work, u32 index, BOOL moved);
static void ZukanAreaListView_ScrollItems(void *work, s16 delta);
static void ZukanAreaListView_SetRowMarks(ZukanAreaListView *view, u32 index, s16 y, BOOL firstBG);
static void ZukanAreaListView_UpdateScrollBar(ZukanAreaListView *view);
static BOOL ZukanAreaListView_IsAtBottom(ZukanAreaListView *view);
static BOOL ZukanAreaListView_IsActorAnimating(ZukanAreaListView *view, u32 index);
static u16 ZukanAreaListView_GetActorAnim(ZukanAreaListView *view, u32 index);
static void ZukanAreaListView_ShowMarkFrame(ZukanAreaListView *view, BOOL visible);
static void ZukanAreaListView_MoveMarkFrame(ZukanAreaListView *view);
static void ZukanAreaListView_UpdatePageButtons(ZukanAreaListView *view, BOOL force);
static void ZukanAreaListView_UpdateDecideButton(ZukanAreaListView *view);
static void ZukanAreaListView_GetKinds(ZukanHabitatSummary *area, u8 *kind0, u8 *kind1, u8 *kind2);

static const BGSysVRAMConfig sZukanAreaListVRAMConfig = {
    GX_VRAM_A,
    GX_VRAM_NONE,
    GX_VRAM_C,
    GX_VRAM_NONE,
    GX_VRAM_B,
    GX_VRAM_NONE,
    GX_VRAM_D,
    GX_VRAM_NONE,
    GX_VRAM_NONE,
    GX_VRAM_NONE,
    GX_OBJVRAMMODE_CHAR_1D_128K,
    GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const FrameListCallbacks sZukanAreaListCallbacks = {
    ZukanAreaListView_PrintItem,
    ZukanAreaListView_SelectItem,
    ZukanAreaListView_ScrollItems,
};

static const FrameListTouch sZukanAreaListTouch[] = {
    { { 0, 23, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 24, 47, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 72, 95, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 120, 143, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 128, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 136, 159 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 168, 191, 160, 183 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, 0 },
};

static const FrameListSetup sZukanAreaListSetup = {
    { 2, 6, 8, 0, 24, 3, 7, 0, 15, 3, 5, { 24, 12, 8, 6, 4, 3 }, 7, 8, 0 },
    0,
    10,
    0,
    ZUKAN_AREA_LIST_ROWS,
    0,
    sZukanAreaListTouch,
    &sZukanAreaListCallbacks,
    NULL,
};

void ZukanAreaListView_Init(ZukanAreaListView *view, GameData *gameData) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, -16);
    GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysSetDisplayLayout(GX_DISP_SELECT_SUB_MAIN);
    ZukanAreaListView_InitVRAM();
    ZukanAreaListView_InitBG();
    ZukanAreaListView_InitActorSys(view);
    ZukanAreaListView_InitWindows(view);
    ZukanAreaListView_CreateActors(view);
    ZukanAreaListView_LoadGraphics();
    ZukanAreaListView_InitPaletteFade(view);
    ZukanAreaListView_InitBlink(view);
    ZukanAreaListView_InitBlend();
    ZukanAreaListView_InitWindowMask();
    view->listBusy = TRUE;
}

BOOL ZukanAreaListView_Draw(ZukanAreaListView *view) {
    if (FrameList_Draw(view->list) == TRUE) {
        ZukanAreaListView_ShowMarkFrame(view, TRUE);
        ZukanAreaListView_UpdatePageButtons(view, FALSE);
        return FALSE;
    }
    return TRUE;
}

void ZukanAreaListView_Exit(ZukanAreaListView *view) {
    ZukanAreaListView_ExitWindowMask();
    ZukanAreaListView_ExitWindows(view);
    ZukanAreaListView_ExitBlink(view);
    ZukanAreaListView_ExitActors(view);
    ZukanAreaListView_ExitBG();
    ZukanAreaListView_ExitPaletteFade(view);
    G2_BlendNone();
    G2S_BlendNone();
    GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, -16);
    GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, -16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
}

void ZukanAreaListView_VBlank(ZukanAreaListView *view) {
    func_0204b7c8();
    PaletteFade_Transfer(view->paletteFade);
}

void ZukanAreaListView_HBlank(ZukanAreaListView *view) {
    s32 vcount = GX_GetVCount();

    if (vcount >= 168) {
        G2S_ChangeBlendAlpha(11, 5);
    } else if (vcount >= 144) {
        G2S_ChangeBlendAlpha(10, 6);
    } else if (vcount >= 120) {
        G2S_ChangeBlendAlpha(9, 7);
    } else if (vcount >= 96) {
        G2S_ChangeBlendAlpha(8, 8);
    } else if (vcount >= 72) {
        G2S_ChangeBlendAlpha(6, 10);
    } else {
        G2S_ChangeBlendAlpha(4, 12);
    }
}

void ZukanAreaListView_Main(ZukanAreaListView *view) {
    ZukanAreaListView_Update(view);
    ZukanAreaListView_UpdateScrollBar(view);
    if (view->blinkPaused == FALSE) {
        ZukanAreaListView_Blink(view);
    }
    view->scrollTimer++;
    if (view->scrollTimer == BG_SCROLL_FRAMES) {
        GFL_BGSysMoveBGReq(3, BG_MOVE_RIGHT, 1);
        GFL_BGSysMoveBGReq(7, BG_MOVE_RIGHT, 1);
        view->scrollTimer = 0;
    }
    func_0204b794();
}

u32 ZukanAreaListView_ListMain(ZukanAreaListView *view) {
    u32 result = FrameList_Main(view->list);

    if (result == FRAMELIST_NONE) {
        ZukanAreaListView_ShowMarkFrame(view, TRUE);
        ZukanAreaListView_UpdatePageButtons(view, FALSE);
        view->listBusy = FALSE;
    } else {
        view->listBusy = TRUE;
    }
    return result;
}

static void ZukanAreaListView_InitVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sZukanAreaListVRAMConfig);
}

static void ZukanAreaListView_InitBG(void) {
    GFL_BGSysCreate(HEAPID_ZUKAN_AREA_LIST);
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&config);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            0,
            0,
        };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysClearCharCore(0, BGSYS_TILE_SIZE_16, 0, HEAPID_ZUKAN_AREA_LIST);
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysSetBGEnabled(0, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x512,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x4000,
            GX_BG_EXTPLTT_01,
            1,
            0,
            0,
        };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(1, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            0,
            0,
        };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
        GFL_BGSysQueueScrLoad(2);
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            0,
            0,
        };

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(3, TRUE);
    }
    {
        BGSetup setup = {
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
            0,
            0,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
        GFL_BGSysClearCharCore(4, BGSYS_TILE_SIZE_16, 0, HEAPID_ZUKAN_AREA_LIST);
        GFL_BGSysQueueScrLoad(4);
        GFL_BGSysSetBGEnabled(4, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_512x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x0800),
            GX_BG_CHARBASE(0x0c000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            0,
            0,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
        GFL_BGSysQueueScrLoad(5);
        GFL_BGSysSetBGEnabled(5, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x1000),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            0,
            0,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(6);
        GFL_BGSysQueueScrLoad(6);
        GFL_BGSysSetBGEnabled(6, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x1800),
            GX_BG_CHARBASE(0x1c000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            0,
            0,
        };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(7);
        GFL_BGSysQueueScrLoad(7);
        GFL_BGSysSetBGEnabled(7, TRUE);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, TRUE);
}

static void ZukanAreaListView_InitActorSys(ZukanAreaListView *view) {
    ZukanAreaListView_CreateActorSys();
    view->actorUnit = func_0204bf1c(112, 0, HEAPID_ZUKAN_AREA_LIST);
    func_0204c028(view->actorUnit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

// The windows cover the whole of their screens, the lower and the upper
static void ZukanAreaListView_InitWindows(ZukanAreaListView *view) {
    BmpWin_InitAllocator(HEAPID_ZUKAN_AREA_LIST);
    view->printQueue = PrintQueue_CreateDefault(HEAPID_ZUKAN_AREA_LIST);

    view->windows[ZUKAN_AREA_LIST_WIN_MAIN] = BmpWin_CreateDynamic(0, 0, 0, 32, 24, 15, 0);
    BmpWin_FlushMap(view->windows[ZUKAN_AREA_LIST_WIN_MAIN]);
    GFL_BitmapFill(BmpWin_GetBitmap(view->windows[ZUKAN_AREA_LIST_WIN_MAIN]), 0);
    BmpWin_FlushChar(view->windows[ZUKAN_AREA_LIST_WIN_MAIN]);
    GFL_BGSysQueueScrLoad(0);
    PrintWindow_Init(&view->printWindows[ZUKAN_AREA_LIST_WIN_MAIN], view->windows[ZUKAN_AREA_LIST_WIN_MAIN]);

    view->windows[ZUKAN_AREA_LIST_WIN_SUB] = BmpWin_CreateDynamic(4, 0, 0, 32, 24, 0, 0);
    BmpWin_FlushMap(view->windows[ZUKAN_AREA_LIST_WIN_SUB]);
    GFL_BitmapFill(BmpWin_GetBitmap(view->windows[ZUKAN_AREA_LIST_WIN_SUB]), 0);
    BmpWin_FlushChar(view->windows[ZUKAN_AREA_LIST_WIN_SUB]);
    GFL_BGSysQueueScrLoad(4);
    PrintWindow_Init(&view->printWindows[ZUKAN_AREA_LIST_WIN_SUB], view->windows[ZUKAN_AREA_LIST_WIN_SUB]);

    view->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
}

static void ZukanAreaListView_CreateActorSys(void) {
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 112, 112, 112, 0, 16, 16 };

    ClActSys_Create(&setup, &sZukanAreaListVRAMConfig, HEAPID_ZUKAN_AREA_LIST);
}

static void ZukanAreaListView_LoadGraphics(void) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_AREA_LIST);

    GFL_G2DIOLoadArcNCLRDefault(arc, 49, PALTYPE_MAIN_BG, 0, 0, HEAPID_ZUKAN_AREA_LIST);
    GFL_G2DIOLoadArcNCLRDefault(arc, 51, PALTYPE_SUB_BG, 0, 0, HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 58, 3, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 115, 3, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 59, 5, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 127, 5, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 60, 2, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 61, 6, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysLoadArcNCGRStatic(arc, 59, 7, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 116, 7, 0, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    GFL_ArcToolFree(arc);

    // The touch bar
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysLoadArcNCGRDynamic(arc, AppMenuCommon_GetBarBgCharFile(), 1, 0, FALSE, HEAPID_ZUKAN_AREA_LIST);
    GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), PALTYPE_MAIN_BG, 14 * 0x20, 0x20,
                                HEAPID_ZUKAN_AREA_LIST);
    loadBGScrToVramByFileNoReserveNegAlign(arc, AppMenuCommon_GetBarScreenFile(), 1, 0, 0, FALSE,
                                           HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysSetScrPaletteNo(1, 0, 21, 32, 3, 14);
    GFL_BGSysQueueScrLoad(1);
    GFL_ArcToolFree(arc);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 15 * 0x20, 0x20, HEAPID_ZUKAN_AREA_LIST);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 15 * 0x20, 0x20, HEAPID_ZUKAN_AREA_LIST);
}

static void ZukanAreaListView_CreateActors(ZukanAreaListView *view) {
    int i;

    {
        UIObjResSetup setup;

        setup.vramType = 0;
        setup.flags = 0;
        setup.arcId = ARCID_ZUKAN_GRA;
        setup.paletteFile = 53;
        setup.charFile = 57;
        setup.cellFile = 63;
        setup.animFile = 73;
        setup.paletteOffset = 0;
        setup.paletteStart = 0;
        setup.paletteCount = 7;
        UIObjRes_Load(&view->res[ZUKAN_AREA_LIST_RES_MAIN], &setup, view->actorUnit, HEAPID_ZUKAN_AREA_LIST);
    }
    {
        UIObjResSetup setup;

        setup.vramType = 1;
        setup.flags = 0;
        setup.arcId = ARCID_ZUKAN_GRA;
        setup.paletteFile = 53;
        setup.charFile = 57;
        setup.cellFile = 63;
        setup.animFile = 73;
        setup.paletteOffset = 0;
        setup.paletteStart = 0;
        setup.paletteCount = 7;
        UIObjRes_Load(&view->res[ZUKAN_AREA_LIST_RES_SUB], &setup, view->actorUnit, HEAPID_ZUKAN_AREA_LIST);
    }
    {
        UIObjResSetup setup;

        setup.vramType = 0;
        setup.flags = 0;
        setup.arcId = getUINarcIdx();
        setup.paletteFile = AppMenuCommon_GetTouchBarPaletteFile();
        setup.charFile = AppMenuCommon_GetTouchBarCharFile();
        setup.cellFile = AppMenuCommon_GetTouchBarCellFile(2);
        setup.animFile = AppMenuCommon_GetTouchBarAnimFile(2);
        setup.paletteOffset = 10;
        setup.paletteStart = 0;
        setup.paletteCount = 3;
        UIObjRes_Load(&view->res[ZUKAN_AREA_LIST_RES_TOUCHBAR], &setup, view->actorUnit, HEAPID_ZUKAN_AREA_LIST);
    }

    for (i = ZUKAN_AREA_LIST_OBJ_MARK; i < ZUKAN_AREA_LIST_OBJ_MARK + ZUKAN_AREA_LIST_MARK_COUNT; i++) {
        view->actors[i] = UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 0, 0, 12,
                                               HEAPID_ZUKAN_AREA_LIST);
        func_0204c124(view->actors[i], FALSE);
        func_0204c468(view->actors[i], 2);
        func_0204c318(view->actors[i], GX_OAM_MODE_XLU);
    }
    for (i = ZUKAN_AREA_LIST_OBJ_MARK_SUB; i < ZUKAN_AREA_LIST_OBJ_MARK_SUB + ZUKAN_AREA_LIST_MARK_COUNT; i++) {
        view->actors[i] = UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_SUB], view->actorUnit, 0, 0, 12,
                                               HEAPID_ZUKAN_AREA_LIST);
        func_0204c124(view->actors[i], FALSE);
        func_0204c468(view->actors[i], 2);
        func_0204c318(view->actors[i], GX_OAM_MODE_XLU);
    }
    // The marks' windows on the upper screen show the row under them unblended
    for (i = ZUKAN_AREA_LIST_OBJ_MARK_WINDOW; i < ZUKAN_AREA_LIST_OBJ_MARK_WINDOW + ZUKAN_AREA_LIST_MARK_COUNT; i++) {
        view->actors[i] = UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_SUB], view->actorUnit, 0, 0, 12,
                                               HEAPID_ZUKAN_AREA_LIST);
        func_0204c124(view->actors[i], TRUE);
        func_0204c318(view->actors[i], 2);
    }

    view->actors[ZUKAN_AREA_LIST_OBJ_DECIDE] =
        UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 32, 64, 15, HEAPID_ZUKAN_AREA_LIST);
    func_0204c124(view->actors[ZUKAN_AREA_LIST_OBJ_DECIDE], TRUE);
    func_0204c520(view->actors[ZUKAN_AREA_LIST_OBJ_DECIDE], TRUE);

    view->actors[ZUKAN_AREA_LIST_OBJ_MODE_UPPER] = UIObjRes_CreateActor(
        &view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 32, 110, 13, HEAPID_ZUKAN_AREA_LIST);
    func_0204c124(view->actors[ZUKAN_AREA_LIST_OBJ_MODE_UPPER], TRUE);
    func_0204c468(view->actors[ZUKAN_AREA_LIST_OBJ_MODE_UPPER], 2);

    view->actors[ZUKAN_AREA_LIST_OBJ_MODE_LOWER] = UIObjRes_CreateActor(
        &view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 32, 133, 13, HEAPID_ZUKAN_AREA_LIST);
    func_0204c124(view->actors[ZUKAN_AREA_LIST_OBJ_MODE_LOWER], TRUE);
    func_0204c468(view->actors[ZUKAN_AREA_LIST_OBJ_MODE_LOWER], 2);

    view->actors[ZUKAN_AREA_LIST_OBJ_MODE_HABITAT] = UIObjRes_CreateActor(
        &view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 32, 156, 14, HEAPID_ZUKAN_AREA_LIST);
    func_0204c124(view->actors[ZUKAN_AREA_LIST_OBJ_MODE_HABITAT], TRUE);
    func_0204c468(view->actors[ZUKAN_AREA_LIST_OBJ_MODE_HABITAT], 2);

    view->actors[ZUKAN_AREA_LIST_OBJ_MODE] =
        UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 32, 180, 1, HEAPID_ZUKAN_AREA_LIST);
    func_0204c124(view->actors[ZUKAN_AREA_LIST_OBJ_MODE], TRUE);

    view->actors[ZUKAN_AREA_LIST_OBJ_SCROLL_BAR] =
        UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 0, 0, 2, HEAPID_ZUKAN_AREA_LIST);

    view->actors[ZUKAN_AREA_LIST_OBJ_SCROLL_RAIL] =
        UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 252, 84, 3, HEAPID_ZUKAN_AREA_LIST);
    func_0204c468(view->actors[ZUKAN_AREA_LIST_OBJ_SCROLL_RAIL], 1);

    view->actors[ZUKAN_AREA_LIST_OBJ_MARK_FRAME] =
        UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_MAIN], view->actorUnit, 0, 0, 32, HEAPID_ZUKAN_AREA_LIST);
    func_0204c468(view->actors[ZUKAN_AREA_LIST_OBJ_MARK_FRAME], 1);

    view->actors[ZUKAN_AREA_LIST_OBJ_PAGE_UP] = UIObjRes_CreateActor(
        &view->res[ZUKAN_AREA_LIST_RES_TOUCHBAR], view->actorUnit, 132, 168, 4, HEAPID_ZUKAN_AREA_LIST);
    view->actors[ZUKAN_AREA_LIST_OBJ_PAGE_DOWN] = UIObjRes_CreateActor(
        &view->res[ZUKAN_AREA_LIST_RES_TOUCHBAR], view->actorUnit, 156, 168, 5, HEAPID_ZUKAN_AREA_LIST);
    view->actors[ZUKAN_AREA_LIST_OBJ_SHORTCUT] = UIObjRes_CreateActor(
        &view->res[ZUKAN_AREA_LIST_RES_TOUCHBAR], view->actorUnit, 184, 172, 6, HEAPID_ZUKAN_AREA_LIST);
    view->actors[ZUKAN_AREA_LIST_OBJ_EXIT] = UIObjRes_CreateActor(&view->res[ZUKAN_AREA_LIST_RES_TOUCHBAR],
                                                                  view->actorUnit, 208, 168, 0, HEAPID_ZUKAN_AREA_LIST);
    view->actors[ZUKAN_AREA_LIST_OBJ_RETURN] = UIObjRes_CreateActor(
        &view->res[ZUKAN_AREA_LIST_RES_TOUCHBAR], view->actorUnit, 232, 168, 1, HEAPID_ZUKAN_AREA_LIST);
}

static void ZukanAreaListView_InitPaletteFade(ZukanAreaListView *view) {
    view->paletteFade = PaletteFade_Create(HEAPID_ZUKAN_AREA_LIST);
    PaletteFade_AllocBuffer(view->paletteFade, PALFADE_BUFFER_MAIN_BG, 0x200, HEAPID_ZUKAN_AREA_LIST);
    PaletteFade_AllocBuffer(view->paletteFade, PALFADE_BUFFER_SUB_BG, 0x200, HEAPID_ZUKAN_AREA_LIST);
    PaletteFade_AllocBuffer(view->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, HEAPID_ZUKAN_AREA_LIST);
    PaletteFade_AllocBuffer(view->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0x200, HEAPID_ZUKAN_AREA_LIST);
    PaletteFade_LoadFromVRAM(view->paletteFade, PALFADE_BUFFER_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(view->paletteFade, PALFADE_BUFFER_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(view->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0, 0x200);
    PaletteFade_LoadFromVRAM(view->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0, 0x200);
}

// The marks' windows on the upper screen show the row under them unblended
static void ZukanAreaListView_InitWindowMask(void) {
    G2S_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                             TRUE);
    G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                               GX_WND_PLANEMASK_OBJ,
                           TRUE);
    GXS_SetVisibleWnd(GX_WNDMASK_OW);
}

static void ZukanAreaListView_ExitWindowMask(void) {
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

static void ZukanAreaListView_ExitBG(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

static void ZukanAreaListView_ExitActors(ZukanAreaListView *view) {
    UIObjRes_Free(&view->res[ZUKAN_AREA_LIST_RES_MAIN]);
    UIObjRes_Free(&view->res[ZUKAN_AREA_LIST_RES_SUB]);
    UIObjRes_Free(&view->res[ZUKAN_AREA_LIST_RES_TOUCHBAR]);
    func_0204bf98(view->actorUnit);
    func_0204b758();
}

static void ZukanAreaListView_ExitPaletteFade(ZukanAreaListView *view) {
    PaletteFade_FreeBuffer(view->paletteFade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(view->paletteFade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_FreeBuffer(view->paletteFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(view->paletteFade, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_Free(view->paletteFade);
}

static void ZukanAreaListView_ExitWindows(ZukanAreaListView *view) {
    PrintQueue_Clear(view->printQueue);
    PrintQueue_Free(view->printQueue);
    GFL_FontFree(view->font);
    BmpWin_FreeAllocator();
    BmpWin_Free(view->windows[ZUKAN_AREA_LIST_WIN_MAIN]);
    BmpWin_Free(view->windows[ZUKAN_AREA_LIST_WIN_SUB]);
}

void ZukanAreaListView_StartFade(ZukanAreaListView *view, u8 start, u8 end) {
    PaletteFade_StartFade(view->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xbfff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(view->paletteFade, 1 << PALFADE_BUFFER_SUB_BG, 0x3ffe, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(view->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0x3f, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(view->paletteFade, 1 << PALFADE_BUFFER_SUB_OBJ, 0x3fff, -14, start, end, GX_RGB(0, 0, 0),
                          GFL_VBlankGetTCBMgr());
}

// The cursor's row blinks
static void ZukanAreaListView_InitBlink(ZukanAreaListView *view) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_AREA_LIST);

    view->blinkAnm = BlinkPalAnm_Create(16, 5, 0xfffe, HEAPID_ZUKAN_AREA_LIST);
    BlinkPalAnm_SetPalBufferArcTool(view->blinkAnm, arc, 53, 16, 32);
    view->blinkPaused = TRUE;
    GFL_ArcToolFree(arc);
}

static void ZukanAreaListView_ExitBlink(ZukanAreaListView *view) {
    BlinkPalAnm_Free(view->blinkAnm);
}

static void ZukanAreaListView_InitBlend(void) {
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 6, 10);
}

u32 ZukanAreaListView_IsFading(ZukanAreaListView *view) {
    return PaletteFade_GetActiveMask(view->paletteFade);
}

// Prints what the queue holds, and gives the marks' actors what the list set for them
static void ZukanAreaListView_Update(ZukanAreaListView *view) {
    int i;
    int window;

    view->printed = FALSE;
    PrintQueue_Main(view->printQueue);
    for (i = 0; i < ZUKAN_AREA_LIST_MARK_COUNT; i++) {
        ClActor *actor = view->actors[ZUKAN_AREA_LIST_OBJ_MARK + i];
        ZukanAreaListMark *mark = &view->marks[0][i];

        if (mark->changed == TRUE) {
            func_0204c124(actor, mark->visible);
            func_0204c488(actor, mark->anim);
            func_0204c140(actor, &mark->pos, 0);
            mark->changed = FALSE;
        }
    }
    for (i = ZUKAN_AREA_LIST_OBJ_MARK_SUB; i < ZUKAN_AREA_LIST_OBJ_MARK_SUB + ZUKAN_AREA_LIST_MARK_COUNT; i++) {
        ClActor *actor = view->actors[i];
        ZukanAreaListMark *mark = &view->subMarks[0][i - ZUKAN_AREA_LIST_OBJ_MARK_SUB];
        ClActor *window = view->actors[i + ZUKAN_AREA_LIST_OBJ_MARK_WINDOW - ZUKAN_AREA_LIST_OBJ_MARK_SUB];

        if (mark->changed == TRUE) {
            func_0204c124(actor, mark->visible);
            func_0204c488(actor, mark->anim);
            func_0204c140(actor, &mark->pos, 1);
            mark->changed = FALSE;
            func_0204c124(window, mark->visible);
            func_0204c140(window, &mark->pos, 1);
            func_0204c488(window, mark->anim);
        }
    }
    for (window = 0; window < ZUKAN_AREA_LIST_WIN_COUNT; window++) {
        PrintWindow_Flush(&view->printWindows[window], view->printQueue);
        view->printed = PrintWindow_IsPrinted(&view->printWindows[window]);
    }
}

BOOL ZukanAreaListView_IsPrinted(ZukanAreaListView *view) {
    return view->printed;
}

void ZukanAreaListView_Print(u32 window, StrBuf *strbuf, ZukanAreaListView *view, s16 x, s16 y, u16 width, u16 height,
                             u16 color) {
    BmpWin *bmpWin;

    switch (window) {
    case ZUKAN_AREA_LIST_WIN_MAIN:
        bmpWin = view->windows[ZUKAN_AREA_LIST_WIN_MAIN];
        break;
    case ZUKAN_AREA_LIST_WIN_SUB:
        bmpWin = view->windows[ZUKAN_AREA_LIST_WIN_SUB];
        break;
    }
    GFL_BitmapFillArea(BmpWin_GetBitmap(bmpWin), x, y, width, height, 0);
    PrintWindow_Print(&view->printWindows[window], view->printQueue, x, y, strbuf, view->font, color);
}

static void ZukanAreaListView_Blink(ZukanAreaListView *view) {
    BlinkPalAnm_Main(view->blinkAnm);
}

void ZukanAreaListView_FillWindow(u32 window, u8 color, ZukanAreaListView *view) {
    BmpWin *bmpWin;

    switch (window) {
    case ZUKAN_AREA_LIST_WIN_MAIN:
        bmpWin = view->windows[ZUKAN_AREA_LIST_WIN_MAIN];
        break;
    case ZUKAN_AREA_LIST_WIN_SUB:
        bmpWin = view->windows[ZUKAN_AREA_LIST_WIN_SUB];
        break;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(bmpWin), color);
}

ClActor *ZukanAreaListView_GetActor(ZukanAreaListView *view, u32 index) {
    return view->actors[index];
}

void ZukanAreaListView_InitList(ZukanAreaListView *view, u32 count, ZukanHabitatSummary *areas, u16 zone,
                                u16 cursorPos) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_AREA_LIST);
    FrameListSetup setup = sZukanAreaListSetup;
    MsgData *msgData;
    u32 i;

    setup.count = ZUKAN_AREA_LIST_NAME_MAX;
    setup.work = view;
    setup.cursorPos = cursorPos;
    for (i = 0; i < count; i++) {
        ZukanHabitatSummary *area = &areas[i];

        if (area->zone == zone) {
            if (i > count - ZUKAN_AREA_LIST_ROWS) {
                setup.cursorPos = i - (count - ZUKAN_AREA_LIST_ROWS);
                setup.scroll = i - setup.cursorPos;
            } else if (i < ZUKAN_AREA_LIST_ROWS) {
                setup.cursorPos = i;
            } else {
                setup.scroll = i;
            }
            break;
        }
    }
    view->list = FrameList_Create(&setup, HEAPID_ZUKAN_AREA_LIST);
    // The rows' screens: the kinds of habitat an area has, and the same with every one of them complete
    FrameList_LoadScreen(view->list, arc, 122, FALSE, 0);
    FrameList_LoadScreen(view->list, arc, 123, FALSE, 1);
    FrameList_LoadScreen(view->list, arc, 124, FALSE, 2);
    FrameList_LoadScreen(view->list, arc, 125, FALSE, 3);
    FrameList_LoadScreen(view->list, arc, 126, FALSE, 4);
    FrameList_LoadScreen(view->list, arc, 117, FALSE, 5);
    FrameList_LoadScreen(view->list, arc, 118, FALSE, 6);
    FrameList_LoadScreen(view->list, arc, 119, FALSE, 7);
    FrameList_LoadScreen(view->list, arc, 120, FALSE, 8);
    FrameList_LoadScreen(view->list, arc, 121, FALSE, 9);
    GFL_ArcToolFree(arc);

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, HEAPID_ZUKAN_AREA_LIST);
    for (i = 0; i < count; i++) {
        view->names[i] = GFL_MsgDataLoadStrbufNew(msgData, ZoneData_GetPlaceNameID(areas[i].zone));
        FrameList_AddItem(view->list, ZukanAreaListView_GetItemScreen(&areas[i]), ITEM_AREA);
    }
    GFL_MsgDataFree(msgData);

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_AREA_LIST);
    FrameList_LoadCursorPalette(view->list, arc, 49, 7, 7);
    GFL_ArcToolFree(arc);

    if (count <= ZUKAN_AREA_LIST_ROWS) {
        func_0204c124(ZukanAreaListView_GetActor(view, ZUKAN_AREA_LIST_OBJ_SCROLL_RAIL), FALSE);
        func_0204c124(ZukanAreaListView_GetActor(view, ZUKAN_AREA_LIST_OBJ_SCROLL_BAR), FALSE);
    } else {
        ZukanAreaListView_UpdateScrollBar(view);
    }
    if (count < ZUKAN_AREA_LIST_ROWS) {
        view->shownRows = count;
    } else {
        view->shownRows = ZUKAN_AREA_LIST_ROWS;
    }
    view->count = count;
    view->areas = areas;
}

// The screen of an area's row: one of five by the kinds of habitat the area has, and of five more once every one
// of them is complete
static u32 ZukanAreaListView_GetItemScreen(ZukanHabitatSummary *area) {
    int screen = -1;
    BOOL partial = FALSE;
    u32 kinds[5][ZUKAN_AREA_LIST_KINDS] = {
        { 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 0, 1, 1 }, { 1, 1, 1 },
    };
    u8 kind0;
    u8 kind1;
    u8 kind2;
    BOOL complete;
    int i;

    ZukanAreaListView_GetKinds(area, &kind0, &kind1, &kind2);
    for (i = 0; i < 5; i++) {
        if (kind0 == kinds[i][0] && kind1 == kinds[i][1] && kind2 == kinds[i][2]) {
            screen = i;
            break;
        }
    }
    if (screen < 0) {
        screen = 0;
    }
    complete = TRUE;
    for (i = 0; i < ZUKAN_AREA_LIST_KINDS; i++) {
        if (area->conditions[i] == 1 && area->states[i] != 2) {
            complete = FALSE;
            break;
        }
    }
    for (i = 0; i < ZUKAN_AREA_LIST_KINDS; i++) {
        if (area->display[i] == 1) {
            partial = TRUE;
            break;
        }
    }
    if (complete == TRUE && partial == FALSE) {
        screen += 5;
    }
    return screen;
}

void ZukanAreaListView_ExitList(ZukanAreaListView *view) {
    int i;

    for (i = 0; i < view->count; i++) {
        GFL_StrBufFree(view->names[i]);
    }
    FrameList_Free(view->list);
}

static void ZukanAreaListView_PrintItem(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    ZukanAreaListView *view = work;

    PrintStrAligned(window, FrameList_GetPrintQueue(view->list), 0, 4, view->names[index], view->font,
                    PRINT_COLOR(14, 15, 0), PRINT_ALIGN_LEFT);
    if (FrameList_GetValue(view->list, index) == ITEM_AREA) {
        ZukanAreaListView_SetRowMarks(view, index, y, firstBG);
    }
}

static void ZukanAreaListView_SelectItem(void *work, u32 index, BOOL moved) {
    ZukanAreaListView *view = work;

    ZukanAreaListView_MoveMarkFrame(view);
    if (func_0203da48()) {
        ZukanAreaListView_UpdateDecideButton(view);
    }
}

// Moves the marks with the rows, and hides those that leave the screen
static void ZukanAreaListView_ScrollItems(void *work, s16 delta) {
    ZukanAreaListView *view = work;
    int i;

    for (i = 0; i < ZUKAN_AREA_LIST_MARK_COUNT; i++) {
        ZukanAreaListMark *mark = &view->marks[0][i];
        ZukanAreaListMark *subMark = &view->subMarks[0][i];

        mark->pos.y -= delta;
        subMark->pos.y -= delta;
        if (mark->pos.y <= -12 || mark->pos.y >= 204) {
            mark->visible = FALSE;
        }
        if (subMark->pos.y <= -12 || subMark->pos.y >= 204) {
            subMark->visible = FALSE;
        }
        mark->changed = TRUE;
        subMark->changed = TRUE;
    }
    ZukanAreaListView_ShowMarkFrame(view, FALSE);
}

// Sets the marks of a row that the list drew at y, on its first BG, the lower screen's, or on its second: one for
// each kind of habitat the area has, from the right
static void ZukanAreaListView_SetRowMarks(ZukanAreaListView *view, u32 index, s16 y, BOOL firstBG) {
    int place = 0;
    ZukanHabitatSummary *area = &view->areas[index];
    u16 anims[5] = { 12, 11, 7, 8, 0 };
    u8 kind0;
    u8 kind1;
    u8 kind2;
    ClActorPos pos;
    ZukanAreaListMark *marks;

    ZukanAreaListView_GetKinds(area, &kind0, &kind1, &kind2);
    if (firstBG == TRUE) {
        marks = view->marks[index % ZUKAN_AREA_LIST_DRAWN_ROWS];
    } else {
        marks = view->subMarks[index % ZUKAN_AREA_LIST_DRAWN_ROWS];
    }
    pos.x = 76;
    y += 12;
    pos.y = y;
    marks[2].pos = pos;
    pos.x += 16;
    marks[1].pos = pos;
    pos.x += 16;
    marks[0].pos = pos;
    marks[0].visible = FALSE;
    marks[1].visible = FALSE;
    marks[2].visible = FALSE;
    if (kind2 == 1) {
        if (area->display[2] != 4) {
            marks[place].anim = anims[area->display[2]];
            marks[place].visible = TRUE;
        }
        place++;
    }
    if (kind1 == 1) {
        if (area->display[1] != 4) {
            marks[place].anim = anims[area->display[1]];
            marks[place].visible = TRUE;
        }
        place++;
    }
    if (kind0 == 1) {
        if (area->display[0] != 4) {
            marks[place].anim = anims[area->display[0]];
            marks[place].visible = TRUE;
        }
        place++;
    }
    marks[0].changed = TRUE;
    marks[1].changed = TRUE;
    marks[2].changed = TRUE;
}

static void ZukanAreaListView_UpdateScrollBar(ZukanAreaListView *view) {
    ClActor *actor = ZukanAreaListView_GetActor(view, ZUKAN_AREA_LIST_OBJ_SCROLL_BAR);
    ClActorPos pos;
    u32 y;

    func_0204c178(actor, &pos, 0);
    y = FrameList_ClampBarPos(view->list, pos.y);
    if (y < SCROLL_BAR_TOP) {
        y = SCROLL_BAR_TOP;
    } else if (y > SCROLL_BAR_BOTTOM) {
        y = SCROLL_BAR_BOTTOM;
    }
    pos.x = 252;
    pos.y = y;
    func_0204c140(actor, &pos, 0);
}

void ZukanAreaListView_HideUndrawnMarks(ZukanAreaListView *view) {
    int i;

    for (i = 0; i < ZUKAN_AREA_LIST_MARK_COUNT; i++) {
        ZukanAreaListMark *mark = &view->marks[0][i];
        ZukanAreaListMark *subMark = &view->subMarks[0][i];

        if (mark->changed == FALSE) {
            mark->changed = TRUE;
            mark->visible = FALSE;
        }
        if (subMark->changed == FALSE) {
            subMark->changed = TRUE;
            subMark->visible = FALSE;
        }
    }
}

int ZukanAreaListView_GetListSelected(ZukanAreaListView *view) {
    return FrameList_GetSelected(view->list);
}

s16 ZukanAreaListView_GetListCursor(ZukanAreaListView *view) {
    return FrameList_GetCursor(view->list);
}

// Whether the cursor is on the last area
static BOOL ZukanAreaListView_IsAtBottom(ZukanAreaListView *view) {
    if (!FrameList_CanScrollDown(view->list) && view->shownRows - 1 == ZukanAreaListView_GetListCursor(view)) {
        return TRUE;
    }
    return FALSE;
}

int ZukanAreaListView_GetListScroll(ZukanAreaListView *view) {
    return FrameList_GetScroll(view->list);
}

void ZukanAreaListView_SetListCursor(ZukanAreaListView *view, int pos) {
    FrameList_SetCursor(view->list, pos);
}

void ZukanAreaListView_SetCursorPalette(ZukanAreaListView *view, u32 palette) {
    s16 cursor = FrameList_GetCursor(view->list);

    FrameList_SetRowPalette(view->list, (u16)cursor, palette);
    GFL_BGSysQueueScrLoad(2);
}

ZukanHabitatSummary *ZukanAreaListView_GetArea(ZukanAreaListView *view, u32 index) {
    return &view->areas[index];
}

void ZukanAreaListView_SetActorAnim(ZukanAreaListView *view, u32 index, u32 anim) {
    ClActor *actor = ZukanAreaListView_GetActor(view, index);

    func_0204c488(actor, anim);
    func_0204c4d4(actor, 0);
    func_0204c520(actor, TRUE);
}

static BOOL ZukanAreaListView_IsActorAnimating(ZukanAreaListView *view, u32 index) {
    return func_0204c560(view->actors[index]);
}

static u16 ZukanAreaListView_GetActorAnim(ZukanAreaListView *view, u32 index) {
    return func_0204c4a0(view->actors[index]);
}

static void ZukanAreaListView_ShowMarkFrame(ZukanAreaListView *view, BOOL visible) {
    func_0204c124(view->actors[ZUKAN_AREA_LIST_OBJ_MARK_FRAME], visible);
}

// Puts the frame around the marks of the cursor's row, as wide as the marks it has
static void ZukanAreaListView_MoveMarkFrame(ZukanAreaListView *view) {
    ClActor *actor = view->actors[ZUKAN_AREA_LIST_OBJ_MARK_FRAME];
    int count = -1;
    s16 cursor = FrameList_GetCursor(view->list);
    int scroll = ZukanAreaListView_GetListScroll(view);
    u32 anims[ZUKAN_AREA_LIST_KINDS] = { 32, 34, 36 };
    ClActorPos pos;
    int i;

    pos.y = cursor * ROW_HEIGHT + 12;
    pos.x = 116;
    for (i = 0; i < ZUKAN_AREA_LIST_KINDS; i++) {
        if (view->areas[cursor + scroll].conditions[i] == 1 && view->areas[cursor + scroll].conditionsMet[i] == 1) {
            count++;
            pos.x -= 16;
        }
    }
    func_0204c140(actor, &pos, 0);
    ZukanAreaListView_SetActorAnim(view, ZUKAN_AREA_LIST_OBJ_MARK_FRAME, anims[count]);
}

// Shows the page buttons as usable or not, unless one is playing its animation of being pressed
static void ZukanAreaListView_UpdatePageButtons(ZukanAreaListView *view, BOOL force) {
    int scroll = ZukanAreaListView_GetListScroll(view);
    s16 cursor = ZukanAreaListView_GetListCursor(view);

    if (!ZukanAreaListView_IsActorAnimating(view, ZUKAN_AREA_LIST_OBJ_PAGE_UP) ||
        ZukanAreaListView_GetActorAnim(view, ZUKAN_AREA_LIST_OBJ_PAGE_UP) != 12 || force == TRUE) {
        if (scroll == 0 && cursor == 0) {
            ZukanAreaListView_SetActorAnim(view, ZUKAN_AREA_LIST_OBJ_PAGE_UP, 18);
        } else {
            ZukanAreaListView_SetActorAnim(view, ZUKAN_AREA_LIST_OBJ_PAGE_UP, 4);
        }
    }
    if (!ZukanAreaListView_IsActorAnimating(view, ZUKAN_AREA_LIST_OBJ_PAGE_DOWN) ||
        ZukanAreaListView_GetActorAnim(view, ZUKAN_AREA_LIST_OBJ_PAGE_DOWN) != 13 || force == TRUE) {
        if (ZukanAreaListView_IsAtBottom(view)) {
            ZukanAreaListView_SetActorAnim(view, ZUKAN_AREA_LIST_OBJ_PAGE_DOWN, 19);
        } else {
            ZukanAreaListView_SetActorAnim(view, ZUKAN_AREA_LIST_OBJ_PAGE_DOWN, 5);
        }
    }
}

void ZukanAreaListView_Slide(ZukanAreaListView *view, int offset, BOOL touchbar) {
    if (touchbar) {
        int i;

        GFL_BGSysMoveBGReq(1, BG_MOVE_UP, offset);
        for (i = ZUKAN_AREA_LIST_OBJ_MODE; i <= ZUKAN_AREA_LIST_OBJ_EXIT; i++) {
            ClActor *actor = view->actors[i];
            ClActorPos pos;

            func_0204c178(actor, &pos, 0);
            pos.y += (s16)offset;
            func_0204c140(actor, &pos, 0);
        }
    } else {
        GFL_BGSysMoveBGReq(4, BG_MOVE_UP, offset);
        GFL_BGSysMoveBGReq(5, BG_MOVE_UP, offset);
    }
}

static void ZukanAreaListView_UpdateDecideButton(ZukanAreaListView *view) {
    ClActor *actor = view->actors[ZUKAN_AREA_LIST_OBJ_DECIDE];

    if (view->listBusy == FALSE) {
        func_0204c4d4(actor, 0);
        func_0204c488(actor, 40);
    } else {
        func_0204c488(actor, 15);
    }
}

void ZukanAreaListView_SetBlinkPaused(ZukanAreaListView *view, BOOL paused) {
    view->blinkPaused = paused;
}

// Whether an area has each kind of habitat, and it is shown
static void ZukanAreaListView_GetKinds(ZukanHabitatSummary *area, u8 *kind0, u8 *kind1, u8 *kind2) {
    *kind0 = area->conditions[0] & area->conditionsMet[0];
    *kind1 = area->conditions[1] & area->conditionsMet[1];
    *kind2 = area->conditions[2] & area->conditionsMet[2];
}
