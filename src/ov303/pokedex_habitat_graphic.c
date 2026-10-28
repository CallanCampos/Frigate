#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_habitat.h"
#include "app/pokedex_habitat_graphic.h"
#include "app/townmap.h"
#include "app/townmap/townmap_data.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "field/player_state.h"
#include "field/townmap_util.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "system/app_keycursor.h"
#include "system/app_menu_common.h"
#include "system/app_printsys_common.h"
#include "system/blink_palanm.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/text_speed.h"

// What the Pokédex's Habitat List draws. The ROM has no name for this file; pokedex_habitat_graphic.c is a guess after
// the graphic files of the other screens. The main screen has the list of an area's Pokémon, with the tabs of the
// kinds of habitat and the touch bar, and the sub screen the map of the region with the area on it

// Not in constants/arc.h
#define ARCID_POKEICON 7

#define OVERLAY_UI OVERLAY_ID(139)

// The places of the map
enum {
    PLACE_NONE,
    PLACE_HIDDEN,
    PLACE_SHOWN,
};

static void PokedexHabitatGraphic_UpdateMessage(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_SetVRAMBanks(void);
static void PokedexHabitatGraphic_InitBG(void);
static void PokedexHabitatGraphic_InitObj(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_InitWindows(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_InitClActSys(void);
static void PokedexHabitatGraphic_LoadBG(void);
static void PokedexHabitatGraphic_CreateActors(PokedexHabitatGraphic *graphic);
static ClActor *PokedexHabitatGraphic_CreateIcon(PokedexHabitatGraphic *graphic, u32 species, u32 form, u32 sex,
                                                 u8 cell);
static void PokedexHabitatGraphic_InitPaletteFade(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_HideWindows(void);
static void PokedexHabitatGraphic_FreeBG(void);
static void PokedexHabitatGraphic_FreeObj(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_FreePaletteFade(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_FreeBlink(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_FreeWindows(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_InitScroll(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_InitBlink(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_UpdatePrints(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_UpdateBlink(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_LoadIcon(PokedexHabitatGraphic *graphic, u8 cell);
static void PokedexHabitatGraphic_GetCellPos(u8 cell, ClActorPos *pos);
static void PokedexHabitatGraphic_UpdateGlow(PokedexHabitatGraphic *graphic);
static void PokedexHabitatGraphic_StartGlow(PokedexHabitatGraphic *graphic);
static BmpWin *PokedexHabitatGraphic_GetWindow(PokedexHabitatGraphic *graphic, u8 window);
static void PokedexHabitatGraphic_ReloadBlink(PokedexHabitatGraphic *graphic, u32 type);
static void PokedexHabitatGraphic_CreatePlaces(PokedexHabitatGraphic *graphic);

static const BGSysVRAMConfig sPokedexHabitatVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_0123_H,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

void PokedexHabitatGraphic_Init(PokedexHabitatGraphic *graphic, GameData *gameData) {
    GFL_OvlLoad(OVERLAY_UI);
    GFL_OvlLoad(OVERLAY_TOWNMAP);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    GFL_BGSysSetDisplayLayout(0);
    graphic->townmap = TownMapData_Load(HEAPID_POKEDEX_HABITAT);
    graphic->iconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_POKEDEX_HABITAT);
    graphic->gameData = gameData;
    graphic->blinkStopped = FALSE;
    PokedexHabitatGraphic_SetVRAMBanks();
    PokedexHabitatGraphic_InitBG();
    PokedexHabitatGraphic_InitObj(graphic);
    PokedexHabitatGraphic_InitWindows(graphic);
    PokedexHabitatGraphic_CreateActors(graphic);
    PokedexHabitatGraphic_LoadBG();
    PokedexHabitatGraphic_InitPaletteFade(graphic);
    PokedexHabitatGraphic_InitBlink(graphic);
    PokedexHabitatGraphic_InitScroll(graphic);
    PokedexHabitatGraphic_CreatePlaces(graphic);
}

void PokedexHabitatGraphic_Exit(PokedexHabitatGraphic *graphic) {
    PokedexHabitatGraphic_HideWindows();
    TownMapData_Free(graphic->townmap);
    GFL_ArcToolFree(graphic->iconArc);
    PokedexHabitatGraphic_FreeWindows(graphic);
    PokedexHabitatGraphic_FreeObj(graphic);
    PokedexHabitatGraphic_FreeBG();
    PokedexHabitatGraphic_FreeBlink(graphic);
    PokedexHabitatGraphic_FreePaletteFade(graphic);
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_TOWNMAP);
    GFL_OvlUnload(OVERLAY_UI);
}

void PokedexHabitatGraphic_VBlank(PokedexHabitatGraphic *graphic) {
    int i;

    ClActSys_VBlank();
    for (i = 0; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
        if (graphic->iconRequests[i].requested == TRUE) {
            PokedexHabitatGraphic_LoadIcon(graphic, i);
            graphic->iconRequests[i].requested = FALSE;
        }
    }
    if (graphic->paletteRequested == TRUE) {
        switch (graphic->paletteType) {
        case 0:
            GFL_G2DIOLoadNCLR(ARCID_POKEDEX_GRA, 49, PALTYPE_MAIN_BG, 0x120, 0x120, 0x60, HEAPID_POKEDEX_HABITAT);
            break;
        case 1:
            GFL_G2DIOLoadNCLR(ARCID_POKEDEX_GRA, 50, PALTYPE_MAIN_BG, 0, 0x120, 0x60, HEAPID_POKEDEX_HABITAT);
            break;
        case 2:
            GFL_G2DIOLoadNCLR(ARCID_POKEDEX_GRA, 50, PALTYPE_MAIN_BG, 0x60, 0x120, 0x60, HEAPID_POKEDEX_HABITAT);
            break;
        }
        PokedexHabitatGraphic_ReloadBlink(graphic, graphic->paletteType);
        PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_MAIN_BG, 0, 0x200);
        PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_SUB_BG, 0, 0x200);
        PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0, 0x200);
        PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0, 0x200);
        graphic->paletteRequested = FALSE;
    }
    PaletteFade_Transfer(graphic->paletteFade);
}

BOOL PokedexHabitatGraphic_AreIconsLoaded(PokedexHabitatGraphic *graphic) {
    int i;

    for (i = 0; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
        if (graphic->iconRequests[i].requested == TRUE) {
            return FALSE;
        }
    }
    return TRUE;
}

void PokedexHabitatGraphic_Main(PokedexHabitatGraphic *graphic) {
    PokedexHabitatGraphic_UpdatePrints(graphic);
    if (graphic->blinkStopped == FALSE) {
        PokedexHabitatGraphic_UpdateBlink(graphic);
    }
    PokedexHabitatGraphic_UpdateGlow(graphic);
    if (graphic->stream != NULL) {
        PokedexHabitatGraphic_UpdateMessage(graphic);
    }
    graphic->scrollTimer++;
    if (graphic->scrollTimer == 4) {
        GFL_BGSysMoveBGReq(3, BG_MOVE_RIGHT, 1);
        graphic->scrollTimer = 0;
    }
    ClActSys_Draw();
}

static void PokedexHabitatGraphic_UpdateMessage(PokedexHabitatGraphic *graphic) {
    if (graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE] != NULL) {
        GFL_TCBExMgrUpdate(graphic->tcbManager);
        KeyCursor_Update(graphic->keyCursor, graphic->stream, graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]);
        if (AppPrintsysCommon_Update(&graphic->printCommon, graphic->stream) == TRUE &&
            graphic->messageWaiting == TRUE) {
            KeyCursor_Draw(graphic->keyCursor, BmpWin_GetBitmap(graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]), 15);
            BmpWin_FlushChar(graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]);
        }
    }
}

static void PokedexHabitatGraphic_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sPokedexHabitatVRAMConfig);
}

static void PokedexHabitatGraphic_InitBG(void) {
    u16 backdrop = 0;

    GXS_LoadBGPltt(&backdrop, 0, sizeof(backdrop));
    GFL_BGSysCreate(HEAPID_POKEDEX_HABITAT);
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&config);
    }
    // The main screen's text
    {
        BGSetup setup = { 0,
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
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysClearCharCore(0, 0x20, 0, HEAPID_POKEDEX_HABITAT);
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysSetBGEnabled(0, TRUE);
    }
    // The message
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x08000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(1);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysFillChar(1, 0, 1, 0);
    }
    // The list
    {
        BGSetup setup = { 0,
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
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
        GFL_BGSysQueueScrLoad(2);
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    // The pattern behind it
    {
        BGSetup setup = { 0,
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
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(3, TRUE);
    }
    // The sub screen's text
    {
        BGSetup setup = { 0,
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
                          FALSE };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
        GFL_BGSysClearCharCore(4, 0x20, 0, HEAPID_POKEDEX_HABITAT);
        GFL_BGSysQueueScrLoad(4);
        GFL_BGSysSetBGEnabled(4, TRUE);
    }
    // The area's name
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0x0800),
                          GX_BG_CHARBASE(0x0c000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
        GFL_BGSysQueueScrLoad(5);
        GFL_BGSysSetBGEnabled(5, TRUE);
    }
    // The map
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_256,
                          GX_BG_SCRBASE(0x1000),
                          GX_BG_CHARBASE(0x10000),
                          0x8000,
                          GX_BG_EXTPLTT_23,
                          3,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(6);
        GFL_BGSysQueueScrLoad(6);
        GFL_BGSysSetBGEnabled(6, TRUE);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_256,
                          GX_BG_SCRBASE(0x1800),
                          GX_BG_CHARBASE(0x1c000),
                          0x8000,
                          GX_BG_EXTPLTT_23,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(7);
        GFL_BGSysQueueScrLoad(7);
        GFL_BGSysSetBGEnabled(7, TRUE);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2, TRUE);
}

static void PokedexHabitatGraphic_InitObj(PokedexHabitatGraphic *graphic) {
    PokedexHabitatGraphic_InitClActSys();
    graphic->unit = ClActUnit_Create(94, 0, HEAPID_POKEDEX_HABITAT);
    ClActUnit_UseSystemRenderer(graphic->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PokedexHabitatGraphic_InitWindows(PokedexHabitatGraphic *graphic) {
    BmpWin_InitAllocator(HEAPID_POKEDEX_HABITAT);
    graphic->printQueue = PrintQueue_CreateDefault(HEAPID_POKEDEX_HABITAT);

    graphic->windows[POKEDEX_HABITAT_WIN_MAIN] = BmpWin_CreateDynamic(0, 0, 0, 32, 24, 15, FALSE);
    BmpWin_FlushMap(graphic->windows[POKEDEX_HABITAT_WIN_MAIN]);
    GFL_BitmapFill(BmpWin_GetBitmap(graphic->windows[POKEDEX_HABITAT_WIN_MAIN]), 0);
    BmpWin_FlushChar(graphic->windows[POKEDEX_HABITAT_WIN_MAIN]);
    GFL_BGSysQueueScrLoad(0);
    PrintWindow_Init(&graphic->prints[POKEDEX_HABITAT_WIN_MAIN], graphic->windows[POKEDEX_HABITAT_WIN_MAIN]);

    graphic->windows[POKEDEX_HABITAT_WIN_SUB] = BmpWin_CreateDynamic(4, 0, 0, 32, 24, 4, FALSE);
    BmpWin_FlushMap(graphic->windows[POKEDEX_HABITAT_WIN_SUB]);
    GFL_BitmapFill(BmpWin_GetBitmap(graphic->windows[POKEDEX_HABITAT_WIN_SUB]), 0);
    BmpWin_FlushChar(graphic->windows[POKEDEX_HABITAT_WIN_SUB]);
    GFL_BGSysQueueScrLoad(4);
    PrintWindow_Init(&graphic->prints[POKEDEX_HABITAT_WIN_SUB], graphic->windows[POKEDEX_HABITAT_WIN_SUB]);

    graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE] = NULL;
    graphic->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    AppPrintsysCommon_Init(&graphic->printCommon, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    graphic->tcbManager = GFL_TCBExMgrCreate(HEAPID_POKEDEX_HABITAT, HEAPID_POKEDEX_HABITAT, 1, 0);
    graphic->keyCursor = KeyCursor_Create(15, TRUE, TRUE, HEAPID_POKEDEX_HABITAT);
}

static void PokedexHabitatGraphic_InitClActSys(void) {
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 94, 94, 94, 0, 16, 16 };

    ClActSys_Create(&setup, &sPokedexHabitatVRAMConfig, HEAPID_POKEDEX_HABITAT);
}

static void PokedexHabitatGraphic_LoadBG(void) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, HEAPID_POKEDEX_HABITAT);

    GFL_G2DIOLoadArcNCLRDefault(arc, 49, PALTYPE_MAIN_BG, 0, 0, HEAPID_POKEDEX_HABITAT);
    GFL_G2DIOLoadArcNCLRDefault(arc, 51, PALTYPE_SUB_BG, 0, 0, HEAPID_POKEDEX_HABITAT);
    GFL_BGSysLoadArcNCGRStatic(arc, 58, 2, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 128, 2, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    GFL_BGSysLoadArcNCGRStatic(arc, 58, 3, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 115, 3, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    GFL_BGSysLoadArcNCGRStatic(arc, 59, 5, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 129, 5, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, HEAPID_POKEDEX_HABITAT);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_SUB_BG_EX, 0x4000, 0, HEAPID_POKEDEX_HABITAT);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_SUB_BG_EX, 0x6000, 0, HEAPID_POKEDEX_HABITAT);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 6, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 16, 6, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 7, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 17, 7, 0, 0, FALSE, HEAPID_POKEDEX_HABITAT);
    GFL_ArcToolFree(arc);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x1e0, 0x20, HEAPID_POKEDEX_HABITAT);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x1e0, 0x20, HEAPID_POKEDEX_HABITAT);
}

static void PokedexHabitatGraphic_CreateActors(PokedexHabitatGraphic *graphic) {
    UIObjResSetup objSetup;
    UIObjResSetup barSetup;
    UIObjResSetup areaSetup;
    UIObjResSetup mapSetup;
    int i;

    objSetup.vramType = CLACT_SURFACE_MAIN;
    objSetup.flags = 0;
    objSetup.arcId = ARCID_POKEDEX_GRA;
    objSetup.paletteFile = 53;
    objSetup.charFile = 57;
    objSetup.cellFile = 63;
    objSetup.animFile = 73;
    objSetup.paletteOffset = 0;
    objSetup.paletteStart = 0;
    objSetup.paletteCount = 8;
    UIObjRes_Load(&graphic->objRes, &objSetup, graphic->unit, HEAPID_POKEDEX_HABITAT);

    barSetup.vramType = CLACT_SURFACE_MAIN;
    barSetup.flags = 0;
    barSetup.arcId = getUINarcIdx();
    barSetup.paletteFile = AppMenuCommon_GetTouchBarPaletteFile();
    barSetup.charFile = AppMenuCommon_GetTouchBarCharFile();
    barSetup.cellFile = AppMenuCommon_GetTouchBarCellFile(2);
    barSetup.animFile = AppMenuCommon_GetTouchBarAnimFile(2);
    barSetup.paletteOffset = 8;
    barSetup.paletteCount = 3;
    barSetup.paletteStart = 0;
    UIObjRes_Load(&graphic->barRes, &barSetup, graphic->unit, HEAPID_POKEDEX_HABITAT);

    areaSetup.vramType = CLACT_SURFACE_SUB;
    areaSetup.flags = 0;
    areaSetup.arcId = ARCID_POKEDEX_GRA;
    areaSetup.paletteFile = 5;
    areaSetup.charFile = 15;
    areaSetup.cellFile = 29;
    areaSetup.animFile = 46;
    areaSetup.paletteOffset = 0;
    areaSetup.paletteStart = 0;
    areaSetup.paletteCount = 1;
    UIObjRes_Load(&graphic->areaRes, &areaSetup, graphic->unit, HEAPID_POKEDEX_HABITAT);

    mapSetup.vramType = CLACT_SURFACE_SUB;
    mapSetup.flags = 0;
    mapSetup.arcId = ARCID_TOWNMAP_GRA;
    mapSetup.paletteFile = 2;
    mapSetup.charFile = 8;
    mapSetup.cellFile = 12;
    mapSetup.animFile = 19;
    mapSetup.paletteOffset = 1;
    mapSetup.paletteCount = 5;
    mapSetup.paletteStart = 0;
    UIObjRes_Load(&graphic->mapRes, &mapSetup, graphic->unit, HEAPID_POKEDEX_HABITAT);

    graphic->actors[POKEDEX_HABITAT_OBJ_SEASON_LEFT] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 20, 60, 24, HEAPID_POKEDEX_HABITAT);
    ClActor_SetBgPriority(graphic->actors[POKEDEX_HABITAT_OBJ_SEASON_LEFT], 1);
    ClActor_SetAutoAnim(graphic->actors[POKEDEX_HABITAT_OBJ_SEASON_LEFT], TRUE);

    graphic->actors[POKEDEX_HABITAT_OBJ_SEASON_RIGHT] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 236, 60, 22, HEAPID_POKEDEX_HABITAT);
    ClActor_SetBgPriority(graphic->actors[POKEDEX_HABITAT_OBJ_SEASON_RIGHT], 1);
    ClActor_SetAutoAnim(graphic->actors[POKEDEX_HABITAT_OBJ_SEASON_RIGHT], TRUE);

    graphic->actors[POKEDEX_HABITAT_OBJ_FRAME] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 128, 96, 31, HEAPID_POKEDEX_HABITAT);
    ClActor_SetBgPriority(graphic->actors[POKEDEX_HABITAT_OBJ_FRAME], 1);
    ClActor_SetPriority(graphic->actors[POKEDEX_HABITAT_OBJ_FRAME], 1);

    graphic->actors[POKEDEX_HABITAT_OBJ_STAMP] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 224, 20, 27, HEAPID_POKEDEX_HABITAT);
    ClActor_SetAutoAnim(graphic->actors[POKEDEX_HABITAT_OBJ_STAMP], FALSE);
    ClActor_SetVisible(graphic->actors[POKEDEX_HABITAT_OBJ_STAMP], FALSE);
    ClActor_SetAffineMode(graphic->actors[POKEDEX_HABITAT_OBJ_STAMP], 2);

    for (i = POKEDEX_HABITAT_OBJ_NEW; i <= POKEDEX_HABITAT_OBJ_NEW + 2; i++) {
        u8 x = (i - POKEDEX_HABITAT_OBJ_NEW) * 40;

        graphic->actors[i] =
            UIObjRes_CreateActor(&graphic->objRes, graphic->unit, x + 82, 181, 11, HEAPID_POKEDEX_HABITAT);
    }

    graphic->actors[POKEDEX_HABITAT_OBJ_TAB] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 80, 184, 16, HEAPID_POKEDEX_HABITAT);
    graphic->actors[POKEDEX_HABITAT_OBJ_TAB + 1] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 120, 184, 17, HEAPID_POKEDEX_HABITAT);
    graphic->actors[POKEDEX_HABITAT_OBJ_TAB + 2] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 160, 184, 18, HEAPID_POKEDEX_HABITAT);

    for (i = 0; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
        ClActorPos pos;

        PokedexHabitatGraphic_GetCellPos(i, &pos);
        graphic->actors[POKEDEX_HABITAT_OBJ_MARK + i] =
            UIObjRes_CreateActor(&graphic->objRes, graphic->unit, pos.x + 10, pos.y - 2, 9, HEAPID_POKEDEX_HABITAT);
        ClActor_SetBgPriority(graphic->actors[POKEDEX_HABITAT_OBJ_MARK + i], 1);
    }

    graphic->actors[POKEDEX_HABITAT_OBJ_INFO] =
        UIObjRes_CreateActor(&graphic->objRes, graphic->unit, 160, 180, 28, HEAPID_POKEDEX_HABITAT);
    ClActor_SetVisible(graphic->actors[POKEDEX_HABITAT_OBJ_INFO], FALSE);
    ClActor_SetBgPriority(graphic->actors[POKEDEX_HABITAT_OBJ_INFO], 1);

    graphic->actors[POKEDEX_HABITAT_OBJ_PLAYER] =
        UIObjRes_CreateActor(&graphic->mapRes, graphic->unit, 0, 0, 6, HEAPID_POKEDEX_HABITAT);
    ClActor_SetVisible(graphic->actors[POKEDEX_HABITAT_OBJ_PLAYER], FALSE);
    ClActor_SetAutoAnim(graphic->actors[POKEDEX_HABITAT_OBJ_PLAYER], TRUE);

    graphic->actors[POKEDEX_HABITAT_OBJ_AREA] =
        UIObjRes_CreateActor(&graphic->areaRes, graphic->unit, 0, 0, 0, HEAPID_POKEDEX_HABITAT);
    ClActor_SetObjMode(graphic->actors[POKEDEX_HABITAT_OBJ_AREA], 1);

    graphic->actors[POKEDEX_HABITAT_OBJ_AREA_UP] =
        UIObjRes_CreateActor(&graphic->barRes, graphic->unit, 0, 168, 3, HEAPID_POKEDEX_HABITAT);
    graphic->actors[POKEDEX_HABITAT_OBJ_AREA_DOWN] =
        UIObjRes_CreateActor(&graphic->barRes, graphic->unit, 24, 168, 2, HEAPID_POKEDEX_HABITAT);
    graphic->actors[POKEDEX_HABITAT_OBJ_SHORTCUT] =
        UIObjRes_CreateActor(&graphic->barRes, graphic->unit, 184, 172, 6, HEAPID_POKEDEX_HABITAT);
    graphic->actors[POKEDEX_HABITAT_OBJ_EXIT] =
        UIObjRes_CreateActor(&graphic->barRes, graphic->unit, 208, 168, 0, HEAPID_POKEDEX_HABITAT);
    graphic->actors[POKEDEX_HABITAT_OBJ_RETURN] =
        UIObjRes_CreateActor(&graphic->barRes, graphic->unit, 232, 168, 1, HEAPID_POKEDEX_HABITAT);

    for (i = 0; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
        ClActorPos pos;
        ClActor *icon = PokedexHabitatGraphic_CreateIcon(graphic, 0, 0, 0, i);

        PokedexHabitatGraphic_GetCellPos(i, &pos);
        ClActor_SetSurfacePos(icon, &pos, CLACT_SURFACE_MAIN);
    }
}

static ClActor *PokedexHabitatGraphic_CreateIcon(PokedexHabitatGraphic *graphic, u32 species, u32 form, u32 sex,
                                                 u8 cell) {
    UIObjResSetup setup;

    setup.vramType = CLACT_SURFACE_MAIN;
    setup.flags = 1;
    setup.arcId = ARCID_POKEICON;
    setup.paletteFile = PokeIcon_GetPaletteFile();
    setup.charFile = PokeParty_GetIconIndex(species, form, sex, FALSE);
    setup.cellFile = PokeIcon_GetCellFile_Main();
    setup.animFile = getOBJTileMapping_MainEng();
    setup.paletteOffset = 11;
    setup.paletteStart = 0;
    setup.paletteCount = 3;
    UIObjRes_Load(&graphic->iconRes[cell], &setup, graphic->unit, HEAPID_POKEDEX_HABITAT);
    graphic->icons[cell] =
        UIObjRes_CreateActor(&graphic->iconRes[cell], graphic->unit, 0, 0, 1, HEAPID_POKEDEX_HABITAT);
    ClActor_SetPriority(graphic->icons[cell], 2);
    ClActor_SetBgPriority(graphic->icons[cell], 1);
    ClActor_SetAutoAnim(graphic->icons[cell], FALSE);
    ClActor_SetPalette(graphic->icons[cell], PokeIcon_GetPalette(species, form, sex, FALSE), 0);
    return graphic->icons[cell];
}

static void PokedexHabitatGraphic_InitPaletteFade(PokedexHabitatGraphic *graphic) {
    graphic->paletteFade = PaletteFade_Create(HEAPID_POKEDEX_HABITAT);
    PaletteFade_AllocBuffer(graphic->paletteFade, PALFADE_BUFFER_MAIN_BG, 0x200, HEAPID_POKEDEX_HABITAT);
    PaletteFade_AllocBuffer(graphic->paletteFade, PALFADE_BUFFER_SUB_BG, 0x200, HEAPID_POKEDEX_HABITAT);
    PaletteFade_AllocBuffer(graphic->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, HEAPID_POKEDEX_HABITAT);
    PaletteFade_AllocBuffer(graphic->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0x200, HEAPID_POKEDEX_HABITAT);
    PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_MAIN_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_SUB_BG, 0, 0x200);
    PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_MAIN_OBJ, 0, 0x200);
    PaletteFade_LoadFromVRAM(graphic->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0, 0x200);
}

static void PokedexHabitatGraphic_HideWindows(void) {
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

static void PokedexHabitatGraphic_FreeBG(void) {
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

static void PokedexHabitatGraphic_FreeObj(PokedexHabitatGraphic *graphic) {
    int i;

    for (i = 0; i < POKEDEX_HABITAT_ICON_COUNT; i++) {
        if (graphic->icons[i] != NULL) {
            UIObjRes_Free(&graphic->iconRes[i]);
        }
    }
    UIObjRes_Free(&graphic->objRes);
    UIObjRes_Free(&graphic->mapRes);
    ClActUnit_Delete(graphic->unit);
    ClActSys_Delete();
}

static void PokedexHabitatGraphic_FreePaletteFade(PokedexHabitatGraphic *graphic) {
    PaletteFade_FreeBuffer(graphic->paletteFade, PALFADE_BUFFER_MAIN_BG);
    PaletteFade_FreeBuffer(graphic->paletteFade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_FreeBuffer(graphic->paletteFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_FreeBuffer(graphic->paletteFade, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_Free(graphic->paletteFade);
}

static void PokedexHabitatGraphic_FreeBlink(PokedexHabitatGraphic *graphic) {
    BlinkPalAnm_Free(graphic->blink[0]);
    BlinkPalAnm_Free(graphic->blink[1]);
}

static void PokedexHabitatGraphic_FreeWindows(PokedexHabitatGraphic *graphic) {
    PrintQueue_Clear(graphic->printQueue);
    PrintQueue_Free(graphic->printQueue);
    GFL_FontFree(graphic->font);
    KeyCursor_Free(graphic->keyCursor);
    GFL_TCBExMgrFree(graphic->tcbManager);
    BmpWin_FreeAllocator();
    BmpWin_Free(graphic->windows[POKEDEX_HABITAT_WIN_MAIN]);
    BmpWin_Free(graphic->windows[POKEDEX_HABITAT_WIN_SUB]);
}

void PokedexHabitatGraphic_StartFade(PokedexHabitatGraphic *graphic, u8 start, u8 end) {
    PaletteFade_StartFade(graphic->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 0xbfff, -14, start, end, 0,
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(graphic->paletteFade, 1 << PALFADE_BUFFER_SUB_BG, 0xffef, -14, start, end, 0,
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(graphic->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0x38b8, -14, start, end, 0,
                          GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(graphic->paletteFade, 1 << PALFADE_BUFFER_SUB_OBJ, 0x3fff, -14, start, end, 0,
                          GFL_VBlankGetTCBMgr());
}

u16 PokedexHabitatGraphic_IsFading(PokedexHabitatGraphic *graphic) {
    return PaletteFade_GetActiveMask(graphic->paletteFade);
}

static void PokedexHabitatGraphic_InitScroll(PokedexHabitatGraphic *graphic) {
    graphic->scrollTimer = 0;
    GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, -24);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, -24);
}

static void PokedexHabitatGraphic_InitBlink(PokedexHabitatGraphic *graphic) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, HEAPID_POKEDEX_HABITAT);

    graphic->blink[0] = BlinkPalAnm_Create(0xa0, 5, 2, HEAPID_POKEDEX_HABITAT);
    BlinkPalAnm_SetPalBufferArcTool(graphic->blink[0], arc, 49, 0x90, 0xa0);
    graphic->blink[1] = BlinkPalAnm_Create(0x10, 5, 0xfffe, HEAPID_POKEDEX_HABITAT);
    BlinkPalAnm_SetPalBufferArcTool(graphic->blink[1], arc, 53, 0x10, 0x20);
    GFL_ArcToolFree(arc);
}

static void PokedexHabitatGraphic_UpdatePrints(PokedexHabitatGraphic *graphic) {
    int i;

    graphic->printed = TRUE;
    PrintQueue_Main(graphic->printQueue);
    for (i = 0; i < POKEDEX_HABITAT_WIN_COUNT; i++) {
        PrintWindow_Flush(&graphic->prints[i], graphic->printQueue);
        if (PrintWindow_IsPrinted(&graphic->prints[i]) == FALSE) {
            graphic->printed = FALSE;
            return;
        }
    }
}

BOOL PokedexHabitatGraphic_IsPrinted(PokedexHabitatGraphic *graphic) {
    return graphic->printed;
}

void PokedexHabitatGraphic_Print(u8 window, StrBuf *strbuf, PokedexHabitatGraphic *graphic, s16 x, s16 y, u16 width,
                                 u16 height, u16 color) {
    GFL_BitmapFillArea(BmpWin_GetBitmap(PokedexHabitatGraphic_GetWindow(graphic, window)), x, y, width, height, 0);
    PrintWindow_Print(&graphic->prints[window], graphic->printQueue, x, y, strbuf, graphic->font, color);
}

void PokedexHabitatGraphic_ClearArea(u8 window, PokedexHabitatGraphic *graphic, s16 x, s16 y, u16 width, u16 height) {
    BmpWin *win = PokedexHabitatGraphic_GetWindow(graphic, window);

    GFL_BitmapFillArea(BmpWin_GetBitmap(win), x, y, width, height, 0);
    BmpWin_FlushChar(win);
}

static void PokedexHabitatGraphic_UpdateBlink(PokedexHabitatGraphic *graphic) {
    BlinkPalAnm_Main(graphic->blink[0]);
    BlinkPalAnm_Main(graphic->blink[1]);
}

void PokedexHabitatGraphic_FillWindow(u8 window, u8 color, PokedexHabitatGraphic *graphic) {
    GFL_BitmapFill(BmpWin_GetBitmap(PokedexHabitatGraphic_GetWindow(graphic, window)), color);
}

ClActor *PokedexHabitatGraphic_GetActor(PokedexHabitatGraphic *graphic, u32 actor) {
    return graphic->actors[actor];
}

void PokedexHabitatGraphic_SetTab(PokedexHabitatGraphic *graphic, u32 tab, u32 state) {
    BOOL visible[] = { TRUE, TRUE, FALSE };
    u32 anims[][3] = {
        { 19, 16, 16 },
        { 20, 17, 17 },
        { 21, 18, 18 },
    };
    ClActor *actor;

    switch (tab) {
    case 0:
        actor = PokedexHabitatGraphic_GetActor(graphic, POKEDEX_HABITAT_OBJ_TAB);
        ClActor_SetVisible(actor, visible[state]);
        ClActor_SetSequence(actor, anims[0][state]);
        ClActor_SetAnimTime(actor, 0);
        break;
    case 1:
        actor = PokedexHabitatGraphic_GetActor(graphic, POKEDEX_HABITAT_OBJ_TAB + 1);
        ClActor_SetVisible(actor, visible[state]);
        ClActor_SetSequence(actor, anims[1][state]);
        ClActor_SetAnimTime(actor, 0);
        break;
    case 2:
        actor = PokedexHabitatGraphic_GetActor(graphic, POKEDEX_HABITAT_OBJ_TAB + 2);
        ClActor_SetVisible(actor, visible[state]);
        ClActor_SetSequence(actor, anims[2][state]);
        ClActor_SetAnimTime(actor, 0);
        break;
    }
    GFL_BGSysQueueScrLoad(2);
}

void PokedexHabitatGraphic_RequestIcon(PokedexHabitatGraphic *graphic, u16 species, u32 form, u32 sex, u8 cell) {
    graphic->iconRequests[cell].requested = TRUE;
    graphic->iconRequests[cell].species = species;
    graphic->iconRequests[cell].form = form;
    graphic->iconRequests[cell].sex = sex;
}

static void PokedexHabitatGraphic_LoadIcon(PokedexHabitatGraphic *graphic, u8 cell) {
    u32 species = graphic->iconRequests[cell].species;
    u8 sex = graphic->iconRequests[cell].sex;
    u8 form = graphic->iconRequests[cell].form;
    NNSG2dCharacterData *character;
    void *file = GFL_G2DIOReadOBJNCGRArc(graphic->iconArc, PokeParty_GetIconIndex(species, form, sex, FALSE), FALSE,
                                         &character, HEAPID_POKEDEX_HABITAT);

    ClActSys_WriteCharData(ClActor_GetCharIdx(graphic->icons[cell]), character->pRawData, 0x400, 0, 0);
    ClActSys_ReplaceCharData(ClActor_GetCharIdx(graphic->icons[cell]), character);
    ClActor_SetPalette(graphic->icons[cell], PokeIcon_GetPalette(species, form, sex, FALSE), 1);
    GFL_HeapFree(file);
    PokedexHabitatGraphic_SetIconVisible(graphic, cell, TRUE);
}

void PokedexHabitatGraphic_AnimateIcon(PokedexHabitatGraphic *graphic, u8 cell, BOOL play) {
    if (graphic->icons[cell] != NULL) {
        ClActor_StartAnim(graphic->icons[cell]);
        ClActor_SetAutoAnim(graphic->icons[cell], play);
    }
}

void PokedexHabitatGraphic_SetIconVisible(PokedexHabitatGraphic *graphic, u8 cell, BOOL visible) {
    ClActor_SetVisible(graphic->icons[cell], visible);
}

void PokedexHabitatGraphic_SetCellPalette(PokedexHabitatGraphic *graphic, u8 cell, u32 state) {
    u8 x = cell % POKEDEX_HABITAT_COLUMNS * 5 + 4;
    u8 y = cell / POKEDEX_HABITAT_COLUMNS * 4 + 6;

    if (state == POKEDEX_HABITAT_CELL_CURSOR) {
        GFL_BGSysSetScrPaletteNo(2, x, y, 5, 4, 10);
    } else if (state == POKEDEX_HABITAT_CELL_FILLED) {
        GFL_BGSysSetScrPaletteNo(2, x, y, 5, 4, 9);
    } else if (state == POKEDEX_HABITAT_CELL_EMPTY) {
        GFL_BGSysSetScrPaletteNo(2, x, y, 5, 4, 11);
    }
    GFL_BGSysQueueScrLoad(2);
}

static void PokedexHabitatGraphic_GetCellPos(u8 cell, ClActorPos *pos) {
    pos->x = cell % POKEDEX_HABITAT_COLUMNS * 40 + 48;
    pos->y = cell / POKEDEX_HABITAT_COLUMNS * 32 + 56;
}

void PokedexHabitatGraphic_SetArea(PokedexHabitatGraphic *graphic, u16 zone) {
    u16 place = TownMapData_GetPlaceByZone(graphic->townmap, TownMap_GetMapZone(graphic->gameData, zone));
    ClActorPos pos;

    ClActor_SetSequence(graphic->actors[POKEDEX_HABITAT_OBJ_AREA],
                        TownMapData_GetParam(graphic->townmap, place, TOWNMAP_PARAM_AREA_ANIM));
    MI_CpuFill8(&pos, 0, sizeof(pos));
    pos.x = TownMapData_GetParam(graphic->townmap, place, TOWNMAP_PARAM_AREA_X);
    pos.y = TownMapData_GetParam(graphic->townmap, place, TOWNMAP_PARAM_AREA_Y);
    pos.y += 24;
    ClActor_SetSurfacePos(graphic->actors[POKEDEX_HABITAT_OBJ_AREA], &pos, CLACT_SURFACE_SUB);
    PokedexHabitatGraphic_StartGlow(graphic);
}

static void PokedexHabitatGraphic_UpdateGlow(PokedexHabitatGraphic *graphic) {
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_NONE,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                           GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       graphic->glowEv, 16 - graphic->glowEv);
    PokedexCommon_StepGlow(&graphic->glowPhase, &graphic->glowEv);
}

static void PokedexHabitatGraphic_StartGlow(PokedexHabitatGraphic *graphic) {
    PokedexCommon_StartGlow(&graphic->glowPhase, &graphic->glowEv);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_NONE,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                           GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       graphic->glowEv, 16 - graphic->glowEv);
}

void PokedexHabitatGraphic_SetMark(PokedexHabitatGraphic *graphic, u16 cell, u32 state) {
    ClActor *actor = graphic->actors[POKEDEX_HABITAT_OBJ_MARK + cell];

    switch (state) {
    case POKEDEX_HABITAT_MARK_CAUGHT:
        ClActor_SetVisible(actor, TRUE);
        ClActor_SetSequence(actor, 9);
        break;
    case POKEDEX_HABITAT_MARK_SEEN:
        ClActor_SetVisible(actor, TRUE);
        ClActor_SetSequence(actor, 10);
        break;
    case POKEDEX_HABITAT_MARK_NONE:
        ClActor_SetVisible(actor, FALSE);
        break;
    }
}

void PokedexHabitatGraphic_CreateMessageWindow(PokedexHabitatGraphic *graphic, u8 y, u8 height) {
    graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE] = BmpWin_CreateDynamic(1, 2, y, 28, height, 15, TRUE);
    BmpWin_FlushMap(graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]);
    GFL_BitmapFill(BmpWin_GetBitmap(graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]), 0);
    BmpWin_FlushChar(graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]);
    GFL_BGSysQueueScrLoad(1);
    PrintWindow_Init(&graphic->prints[POKEDEX_HABITAT_WIN_MESSAGE], graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]);
}

void PokedexHabitatGraphic_StartMessage(StrBuf *strbuf, PokedexHabitatGraphic *graphic) {
    BmpWin *win = PokedexHabitatGraphic_GetWindow(graphic, POKEDEX_HABITAT_WIN_MESSAGE);

    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    BmpWin_FlushMap(win);
    LoadSysMsgBox(1, 1, 12, 0, HEAPID_POKEDEX_HABITAT);
    BmpWin_DrawFrame(win, TRUE, 1, 12);
    graphic->stream =
        PrintStream_Create(graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE], 0, 0, strbuf, graphic->font,
                           TextSpeed_GetCurrentWait(), graphic->tcbManager, 0, HEAPID_POKEDEX_HABITAT, 15);
    graphic->messageWaiting = FALSE;
    GFL_BGSysQueueScrLoad(1);
    BmpWin_FlushChar(win);
}

void PokedexHabitatGraphic_CloseMessage(PokedexHabitatGraphic *graphic) {
    BmpWin *win = PokedexHabitatGraphic_GetWindow(graphic, POKEDEX_HABITAT_WIN_MESSAGE);

    PrintStream_Delete(graphic->stream);
    BmpWin_ClearFrame(win, TRUE);
    BmpWin_ClearScreen(win);
    GFL_BGSysQueueScrLoad(0);
    graphic->stream = NULL;
    BmpWin_Free(graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE]);
    graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE] = NULL;
}

u32 PokedexHabitatGraphic_GetMessageState(PokedexHabitatGraphic *graphic) {
    return PrintStream_GetState(graphic->stream);
}

void PokedexHabitatGraphic_WaitMessage(PokedexHabitatGraphic *graphic) {
    graphic->messageWaiting = TRUE;
}

static BmpWin *PokedexHabitatGraphic_GetWindow(PokedexHabitatGraphic *graphic, u8 window) {
    switch (window) {
    case POKEDEX_HABITAT_WIN_MAIN:
        return graphic->windows[POKEDEX_HABITAT_WIN_MAIN];
    case POKEDEX_HABITAT_WIN_SUB:
        return graphic->windows[POKEDEX_HABITAT_WIN_SUB];
    case POKEDEX_HABITAT_WIN_MESSAGE:
        return graphic->windows[POKEDEX_HABITAT_WIN_MESSAGE];
    }
    return NULL;
}

void PokedexHabitatGraphic_StartActorAnim(PokedexHabitatGraphic *graphic, u32 actor, u32 anim) {
    ClActor *act = PokedexHabitatGraphic_GetActor(graphic, actor);

    ClActor_SetSequence(act, anim);
    ClActor_SetAnimTime(act, 0);
    ClActor_SetAutoAnim(act, TRUE);
    ClActor_SetVisible(act, TRUE);
}

void PokedexHabitatGraphic_UpdatePlayerMarker(PokedexHabitatGraphic *graphic) {
    ClActor *actor = graphic->actors[POKEDEX_HABITAT_OBJ_PLAYER];
    u16 place = TownMapData_GetPlaceByZone(
        graphic->townmap,
        TownMap_GetMapZone(graphic->gameData, PlayerState_GetZoneID(GameData_GetPlayerState(graphic->gameData))));

    if (place != TOWNMAP_PLACE_NONE) {
        ClActorPos pos;

        pos.x = TownMapData_GetParam(graphic->townmap, place, TOWNMAP_PARAM_X);
        pos.y = TownMapData_GetParam(graphic->townmap, place, TOWNMAP_PARAM_Y);
        pos.y += 24;
        ClActor_SetSurfacePos(actor, &pos, CLACT_SURFACE_SUB);
        ClActor_SetVisible(actor, TRUE);
    } else {
        ClActor_SetVisible(actor, FALSE);
    }
}

void PokedexHabitatGraphic_Scroll(PokedexHabitatGraphic *graphic, int value, BOOL objects) {
    if (objects) {
        int i;

        for (i = POKEDEX_HABITAT_OBJ_INFO; i <= POKEDEX_HABITAT_OBJ_FRAME; i++) {
            ClActor *actor = graphic->actors[i];
            ClActorPos pos;

            ClActor_GetSurfacePos(actor, &pos, CLACT_SURFACE_MAIN);
            pos.y += value;
            ClActor_SetSurfacePos(actor, &pos, CLACT_SURFACE_MAIN);
        }
    } else {
        GFL_BGSysMoveBGReq(4, BG_MOVE_UP, value);
        GFL_BGSysMoveBGReq(5, BG_MOVE_UP, value);
    }
}

void PokedexHabitatGraphic_RequestPalette(PokedexHabitatGraphic *graphic, u32 type) {
    graphic->paletteRequested = TRUE;
    graphic->paletteType = type;
}

static void PokedexHabitatGraphic_ReloadBlink(PokedexHabitatGraphic *graphic, u32 type) {
    BlinkPalAnm_Free(graphic->blink[0]);
    graphic->blink[0] = BlinkPalAnm_Create(0xa0, 5, 2, HEAPID_POKEDEX_HABITAT);
    switch (type) {
    case 0:
        BlinkPalAnm_SetPalBufferArc(graphic->blink[0], ARCID_POKEDEX_GRA, 49, 0x90, 0xa0);
        break;
    case 1:
        BlinkPalAnm_SetPalBufferArc(graphic->blink[0], ARCID_POKEDEX_GRA, 50, 0, 0x10);
        break;
    case 2:
        BlinkPalAnm_SetPalBufferArc(graphic->blink[0], ARCID_POKEDEX_GRA, 50, 0x30, 0x40);
        break;
    }
}

BOOL PokedexHabitatGraphic_IsPaletteRequested(PokedexHabitatGraphic *graphic) {
    return graphic->paletteRequested;
}

static inline BOOL PokedexHabitatGraphic_IsPlaceHidden(GameData *gameData, u16 flag) {
    if (TownMap_CheckFlag(gameData, flag) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static void PokedexHabitatGraphic_CreatePlaces(PokedexHabitatGraphic *graphic) {
    u8 i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        u16 flag = TownMapData_GetParam(graphic->townmap, i, TOWNMAP_PARAM_FLAG);

        if (flag != TOWNMAP_NO_FLAG) {
            if (PokedexHabitatGraphic_IsPlaceHidden(graphic->gameData, flag)) {
                graphic->places[i].state = PLACE_HIDDEN;
                graphic->places[i].actor = NULL;
            } else {
                u16 type = TownMapData_GetParam(graphic->townmap, i, TOWNMAP_PARAM_TYPE);
                s16 x = TownMapData_GetParam(graphic->townmap, i, TOWNMAP_PARAM_X);
                s16 y = TownMapData_GetParam(graphic->townmap, i, TOWNMAP_PARAM_Y);
                u16 anim = type == TOWNMAP_PLACE_TYPE_TOWN ? 14 : 16;

                y += 24;
                graphic->places[i].state = PLACE_SHOWN;
                graphic->places[i].actor =
                    UIObjRes_CreateActor(&graphic->mapRes, graphic->unit, x, y, anim, HEAPID_POKEDEX_HABITAT);
                ClActor_SetVisible(graphic->places[i].actor, TRUE);
                ClActor_SetAutoAnim(graphic->places[i].actor, TRUE);
                ClActor_SetObjMode(graphic->places[i].actor, 0);
            }
        } else {
            graphic->places[i].state = PLACE_NONE;
            graphic->places[i].actor = NULL;
        }
    }
}

void PokedexHabitatGraphic_SetBlinkStopped(PokedexHabitatGraphic *graphic, BOOL stopped) {
    graphic->blinkStopped = stopped;
}

void PokedexHabitatGraphic_SetNewMarksVisible(PokedexHabitatGraphic *graphic, BOOL visible) {
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(graphic, POKEDEX_HABITAT_OBJ_NEW), visible);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(graphic, POKEDEX_HABITAT_OBJ_NEW + 1), visible);
    ClActor_SetVisible(PokedexHabitatGraphic_GetActor(graphic, POKEDEX_HABITAT_OBJ_NEW + 2), visible);
}
