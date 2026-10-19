#include "types.h"
#include "constants/arc.h"
#include "field/wbt.h"
#include "field/wbt_subscreen.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// The Pokémon World Tournament's touch screen, named after the ROM's "wbt_subscreen.c": the tournament's name and
// battle style, centered on the sub screen, over a background from archive 245. The names are ours

// The most entries a table of resources can have before its 0xff terminator
#define RES_MAX 4

#define WINDOW_TOURNAMENT 0
#define WINDOW_STYLE 1
#define WINDOW_MAX 2

typedef void (*WbtSubscreenFunc)(WbtSubscreen *work);

typedef struct {
    BmpWin *window;
    u32 unused;
} WbtSubscreenWindow;

struct WbtSubscreen {
    WbtSubscreenFunc func;
    u32 unk04;
    u16 heapId;
    u32 unk0C;
    u32 unk10;
    // The characters of BG 6 that were allocated, a position and size that CHAR_POS and CHAR_SIZE take apart. Never set
    u32 bg6Chars;
    GameSystem *gsys;
    Font *font;
    WbtSubscreenWindow windows[WINDOW_MAX];
    u32 unk30;
};

// A palette, character or screen file to load: for a palette, a PALTYPE_* and an offset and size in palettes, and
// for characters or a screen, a BG and an offset in characters (size is unused)
typedef struct {
    u8 target;
    u8 fileId;
    u8 offset;
    u8 size;
} WbtSubscreenRes;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} WbtSubscreenWindowSetup;

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} WbtSubscreenBGSetup;

static void WbtSubscreen_SetFunc(WbtSubscreen *work, WbtSubscreenFunc func);
static void WbtSubscreen_Init(WbtSubscreen *work);
static void WbtSubscreen_Exit(WbtSubscreen *work);
static void WbtSubscreen_FuncNone(WbtSubscreen *work);
static void WbtSubscreen_InitBG(WbtSubscreen *work);
static void WbtSubscreen_LoadGraphics(WbtSubscreen *work);
static void WbtSubscreen_CreateWindows(WbtSubscreen *work);
static void WbtSubscreen_PrintCentered(WbtSubscreen *work, u32 window, s32 x, s32 y, StrBuf *strbuf);
static void WbtSubscreen_FreeWindows(WbtSubscreen *work);
static void WbtSubscreen_PrintTournament(WbtSubscreen *work);
static void WbtSubscreen_InitWirelessIcons(WbtSubscreen *work);

static const WbtSubscreenRes sWbtSubscreenPalettes[] = {
    { PALTYPE_SUB_BG, 0, 0, 3 },
    { 0xff },
};

static const WbtSubscreenRes sWbtSubscreenChars[] = {
    { 6, 3, 0 },
    { 0xff },
};

static const WbtSubscreenRes sWbtSubscreenScreens[] = {
    { 5, 8, 0 },
    { 6, 7, 0 },
    { 0xff },
};

static const WbtSubscreenWindowSetup sWbtSubscreenWindows[] = {
    { 4, 2, 4, 28, 2, 2 },
    { 4, 2, 17, 28, 2, 2 },
    { 0xff },
};

static const WbtSubscreenBGSetup sWbtSubscreenBGSetups[] = {
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x04000), 0x2000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x00000), 0x4000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x00000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

static void WbtSubscreen_SetFunc(WbtSubscreen *work, WbtSubscreenFunc func) {
    work->func = func;
}

static void WbtSubscreen_Init(WbtSubscreen *work) {
    WbtSubscreen_InitBG(work);
    WbtSubscreen_LoadGraphics(work);
    WbtSubscreen_CreateWindows(work);
    WbtSubscreen_PrintTournament(work);
    WbtSubscreen_InitWirelessIcons(work);
    WbtSubscreen_SetFunc(work, WbtSubscreen_FuncNone);
}

static void WbtSubscreen_Exit(WbtSubscreen *work) {
    GFL_BGSysFreeCharMemory(6, CHAR_POS(work->bg6Chars), CHAR_SIZE(work->bg6Chars));
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
    WbtSubscreen_FreeWindows(work);
}

static void WbtSubscreen_FuncNone(WbtSubscreen *work) {
}

WbtSubscreen *WbtSubscreen_Create(void *saveData, FieldSubscreen *subscreen, GameSystem *gsys) {
    WbtSubscreen *work = GFL_HeapAllocate(HEAPID_FIELDMAP, sizeof(WbtSubscreen), TRUE, "wbt_subscreen.c", 426);
    work->heapId = HEAPID_FIELDMAP;
    work->gsys = gsys;
    WbtSubscreen_Init(work);
    return work;
}

void WbtSubscreen_Update(WbtSubscreen *work) {
    if (work->func != NULL) {
        work->func(work);
    }
    WihDwc_GetStatus(GameData_GetWifiList(GSYS_GetGameData(work->gsys)));
}

void func_ov081_021ea934(WbtSubscreen *work) {
}

void WbtSubscreen_Free(WbtSubscreen *work) {
    WbtSubscreen_Exit(work);
    G2S_BlendNone();
    GFL_HeapFree(work);
}

static void WbtSubscreen_InitBG(WbtSubscreen *work) {
    int i;

    // The sub engine's BGs
    for (i = 4; i <= 7; i++) {
        GFL_BGSysSetBGEnabled(i, FALSE);
    }
    for (i = 0; i < NELEMS(sWbtSubscreenBGSetups); i++) {
        const WbtSubscreenBGSetup *bgSetup = &sWbtSubscreenBGSetups[i];
        u32 bg = bgSetup->bg;
        GFL_BGSysCreateBG(bg, &bgSetup->setup, bgSetup->mode);
        GFL_BGSysClearBG(bg);
        GFL_BGSysSetBGEnabled(bg, bgSetup->enabled);
    }
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
}

static void WbtSubscreen_LoadGraphics(WbtSubscreen *work) {
    int i;
    const WbtSubscreenRes *palette = sWbtSubscreenPalettes;
    const WbtSubscreenRes *chars = sWbtSubscreenChars;
    const WbtSubscreenRes *screens = sWbtSubscreenScreens;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WBT_SUBSCREEN, work->heapId);

    for (i = 0; i < RES_MAX; i++) {
        if (palette->target == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, palette->fileId, palette->target, palette->offset * 32, palette->size * 32,
                                    work->heapId);
        palette++;
    }
    for (i = 0; i < RES_MAX; i++) {
        if (chars->target == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->fileId, chars->target, chars->offset, 0, FALSE, work->heapId);
        chars++;
    }
    for (i = 0; i < RES_MAX; i++) {
        if (screens->target == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screens->fileId, screens->target, screens->offset, 0, FALSE,
                                               work->heapId);
        screens++;
    }
    GFL_ArcToolFree(arc);
}

static void WbtSubscreen_CreateWindows(WbtSubscreen *work) {
    int i;
    const WbtSubscreenWindowSetup *setup = sWbtSubscreenWindows;

    for (i = 0; i < WINDOW_MAX; i++) {
        if (setup->bg == 0xff) {
            break;
        }
        work->windows[i].window =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        setup++;
    }
}

// Prints a string centered on x
static void WbtSubscreen_PrintCentered(WbtSubscreen *work, u32 window, s32 x, s32 y, StrBuf *strbuf) {
    u32 halfWidth = GFL_FontGetBlockWidth(strbuf, work->font, 0) / 2;

    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(work->windows[window].window), x - halfWidth, y, strbuf, work->font,
                                   PRINT_COLOR(15, 14, 0));
    BmpWin_FlushMap(work->windows[window].window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[window].window));
    BmpWin_FlushChar(work->windows[window].window);
}

static void WbtSubscreen_FreeWindows(WbtSubscreen *work) {
    int i;

    for (i = 0; i < WINDOW_MAX; i++) {
        BmpWin_Free(work->windows[i].window);
    }
    GFL_FontFree(work->font);
}

static void WbtSubscreen_PrintTournament(WbtSubscreen *work) {
    StrBuf *strbuf;
    WbtSystem **sys = GameData_GetWbtSystemPtr(GSYS_GetGameData(work->gsys));

    if (sys == NULL || *sys == NULL) {
        return;
    }

    strbuf = GFL_StrBufCreate(128, work->heapId);
    func_ov055_021e5d44(*sys, func_ov055_021e5ca4(*sys), strbuf);
    WbtSubscreen_PrintCentered(work, WINDOW_TOURNAMENT, 112, 0, strbuf);
    GFL_StrBufFree(strbuf);

    strbuf = GFL_StrBufCreate(128, work->heapId);
    func_ov055_021e6488(func_ov055_021e5cac(*sys), strbuf);
    WbtSubscreen_PrintCentered(work, WINDOW_STYLE, 112, 0, strbuf);
    GFL_StrBufFree(strbuf);
}

static void WbtSubscreen_InitWirelessIcons(WbtSubscreen *work) {
    GFL_NetSetIconScreen(FALSE, work->heapId);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}
