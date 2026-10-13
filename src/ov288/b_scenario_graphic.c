#include "types.h"
#include "battle/b_scenario_graphic.h"
#include "battle/b_scenario_main.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/str_tool.h"

// The scenario screen's graphics (b_scenario_graphic.c, a guessed name; the ROM names only b_scenario_main.c, and
// this file's data is laid out apart from that one's): the BG's files, the windows and their printing, the touch
// rectangles, the actors, and the flash of the choice made.

// The battle's graphics archive, and the one of the actors of the cursor
#define ARCID_BATTLE_SCENARIO 100
#define ARCID_BATTLE_CURSOR 11
// The files of the type icons' palette, cells and animations in the menus' archive
#define FILE_TYPE_PALETTE 0x21
#define FILE_TYPE_CELLS 0x3b
#define FILE_TYPE_ANIMS 0x3e
// The files of the cursor's actors
#define FILE_CURSOR_PALETTE 0x1a5
#define FILE_CURSOR_CHARS 0x1a8
#define FILE_CURSOR_CELLS 0x1a9
#define FILE_CURSOR_ANIMS 0x1aa
// The files of the buttons' actors
#define FILE_BUTTON_PALETTE 10
#define FILE_BUTTON_CHARS 26
#define FILE_BUTTON_CELLS 28
#define FILE_BUTTON_ANIMS 40

#define POSITION_COUNT 0x50

#define BG_PAGES 7

#define TEXT_COLOR 0x440
#define ALIGN_CENTER 2
#define ALIGN_RIGHT 1

#define PALETTE_KEEP (-1)

#define FLASH_ACTIVE 0x1

typedef struct {
    u8 type;
    u8 fileId;
    u8 offset;
    u8 count;
} BScenarioPaletteLoad;

typedef struct {
    u8 bg;
    u8 fileId;
    u8 offset;
    u8 unused;
} BScenarioBGLoad;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} BScenarioWindowSetup;

typedef struct {
    s16 frame;
    s16 palette;
} BScenarioFlashStep;

typedef struct {
    s16 x;
    s16 y;
} BScenarioScroll;

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unused4;
    u8 flags;
    u8 position;
    u8 chars;
} BScenarioActorSetup;

static u16 BScenarioGraphic_GetAlignOffset(StrBuf *str, Font *font, u32 align);
static void BScenarioGraphic_LoadTypeResources(BScenarioWork *work);
static void BScenarioGraphic_LoadIconResources(BScenarioWork *work);
static void BScenarioGraphic_LoadCursorResources(BScenarioWork *work);
static void BScenarioGraphic_LoadButtonResources(BScenarioWork *work);
static void BScenarioGraphic_CreateActors(BScenarioWork *work, u32 surface, const BScenarioActorSetup *setups,
                                          const s16 *resources);
static s16 BScenarioGraphic_GetPosition(BScenarioWork *work, u32 index);
static void BScenarioGraphic_SetCursor(BScenarioWork *work, BOOL show, int place);
static void BScenarioGraphic_SetFlashPalette(BScenarioWork *work, int button, int phase);

// The tables are declared in the order that lays them out as the ROM has them: MWCC sorts them by size, and the
// tables of one size by where every table is declared

// The positions of the marker on the script page's two lines
static const s16 sBScenarioMarkerPositions[2] = { 38, 39 };

static const BScenarioBGLoad sBScenarioCharLoads[2] = {
    { 7, 23, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const BScenarioBGLoad sBScenarioScreenLoads[2] = {
    { 7, 36, 0, 0 },
    { 0xff, 0, 0, 0 },
};

// The palette, an unused entry, the cells, the number and the group of each kind of actor
static const s16 sBScenarioCursorResources[5] = { BSCENARIO_RES_CURSOR, BSCENARIO_RES_CURSOR, BSCENARIO_RES_CURSOR, 4,
                                                  BSCENARIO_ACTORS_CURSOR };

static const s16 sBScenarioTypeResources[5] = { BSCENARIO_RES_TYPE, BSCENARIO_RES_TYPE, BSCENARIO_RES_TYPE, 6,
                                                BSCENARIO_ACTORS_TYPE };

static const s16 sBScenarioIconResources[5] = { BSCENARIO_RES_ICON, BSCENARIO_RES_ICON, BSCENARIO_RES_ICON, 6,
                                                BSCENARIO_ACTORS_ICON };

static const s16 sBScenarioButtonResources[5] = { BSCENARIO_RES_BUTTON, BSCENARIO_RES_BUTTON, BSCENARIO_RES_BUTTON,
                                                  BSCENARIO_BUTTON_COUNT, BSCENARIO_ACTORS_BUTTON };

// Where the pages' BG shows each page
static const BScenarioScroll sBScenarioScrolls[3] = {
    { 0, 0 },
    { 256, 0 },
    { 0, 192 },
};

// The frames of the flash of the choice made, and its phase
static const BScenarioFlashStep sBScenarioFlashSteps[4] = {
    { 1, 1 },
    { 5, 0 },
    { 9, PALETTE_KEEP },
    { -1, -1 },
};

// The palettes of the flash's two phases, by choice
static const s16 sBScenarioFlashPalettes[4 * 2] = {
    1, 7, 2, 7, 1, 7, 1, 7,
};

static const TouchRect sBScenarioSummaryTouchRects[4] = {
    { 166, 182, 8, 88 },
    { 166, 182, 112, 192 },
    { 160, 184, 216, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sBScenarioCastTouchRects[5] = {
    { 166, 182, 8, 88 },    { 160, 184, 120, 152 },      { 160, 184, 160, 192 },
    { 160, 184, 216, 248 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The palette of each series
static const BScenarioPaletteLoad sBScenarioPaletteLoads[5] = {
    { 4, 11, 0, 8 }, { 4, 12, 0, 8 }, { 4, 13, 0, 8 }, { 4, 14, 0, 8 }, { 0xff, 0, 0, 0 },
};

static const TouchRect sBScenarioScriptTouchRects[5] = {
    { 166, 182, 8, 88 },    { 160, 184, 120, 152 },      { 160, 184, 160, 192 },
    { 160, 184, 216, 248 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// Whether each page shows the previous, next and back buttons, -1 for not
static const s16 sBScenarioButtonsVisible[3][4] = {
    { -1, -1, 0, -1 },
    { 0, 0, 0, -1 },
    { 0, 0, 0, -1 },
};

// The cursor's place on each of a page's choices, -1 for none
static const s16 sBScenarioCursorPlaces[3 * 4] = {
    0, 1, 2, -1, 0, 3, 4, 2, 0, 3, 4, 2,
};

// The choices that flash, in the pages' BG, as x, y, width and height: the summary's two, the script's and the cast's
static const s16 sBScenarioFlashRects[4][4] = {
    { 0, 20, 12, 4 },
    { 13, 20, 12, 4 },
    { 32, 20, 12, 4 },
    { 0, 44, 12, 4 },
};

// The positions of the cursor's four corners, by place
static const s16 sBScenarioCursorPositions[5 * 4] = {
    7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
};

static const BScenarioActorSetup sBScenarioCursorSetups[5] = {
    { 0, 40, 1, 0, 0, 1, 0, BSCENARIO_CHARS_CURSOR },
    { 1, 40, 1, 0, 0, 1, 0, BSCENARIO_CHARS_CURSOR },
    { 2, 40, 1, 0, 0, 1, 0, BSCENARIO_CHARS_CURSOR },
    { 3, 40, 1, 0, 0, 1, 0, BSCENARIO_CHARS_CURSOR },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

// The series of each film, which gives the screen its colors
static const u8 sBScenarioGraphicSeries[BSCENARIO_SCENARIO_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
};

static const BScenarioActorSetup sBScenarioButtonSetups[6] = {
    { 3, 40, 1, 0, 0, 1, 34, BSCENARIO_CHARS_BUTTON },  { 6, 40, 1, 0, 0, 1, 35, BSCENARIO_CHARS_BUTTON },
    { 0, 40, 1, 0, 0, 1, 36, BSCENARIO_CHARS_BUTTON },  { 9, 40, 3, 0, 0, 1, 37, BSCENARIO_CHARS_BUTTON },
    { 10, 40, 3, 0, 0, 1, 38, BSCENARIO_CHARS_BUTTON }, { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const BScenarioWindowSetup sBScenarioSummaryWindows[9] = {
    { 5, 2, 1, 28, 2, 10 }, { 5, 2, 5, 28, 2, 11 },  { 5, 2, 12, 28, 2, 11 },
    { 5, 2, 3, 28, 2, 12 }, { 5, 3, 7, 28, 4, 12 },  { 5, 3, 14, 28, 4, 12 },
    { 5, 1, 21, 10, 2, 8 }, { 5, 14, 21, 10, 2, 8 }, { 0xff, 0, 0, 0, 0, 0 },
};

static const BScenarioActorSetup sBScenarioTypeSetups[7] = {
    { 0, 48, 1, 0, 0, 1, 0, 0 },   { 0, 48, 1, 0, 0, 1, 1, 1 }, { 0, 48, 1, 0, 0, 1, 2, 2 },
    { 0, 48, 1, 0, 0, 1, 3, 3 },   { 0, 48, 1, 0, 0, 1, 4, 4 }, { 0, 48, 1, 0, 0, 1, 5, 5 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const BScenarioActorSetup sBScenarioIconSetups[7] = {
    { 1, 40, 1, 0, 0, 0, 6, 6 },   { 1, 40, 1, 0, 0, 0, 6, 7 },  { 1, 40, 1, 0, 0, 0, 6, 8 },
    { 1, 40, 1, 0, 0, 0, 6, 9 },   { 1, 40, 1, 0, 0, 0, 6, 10 }, { 1, 40, 1, 0, 0, 0, 6, 11 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const BScenarioWindowSetup sBScenarioScriptWindows[10] = {
    { 5, 2, 1, 28, 2, 8 },  { 5, 2, 3, 28, 2, 9 },   { 5, 2, 10, 28, 2, 9 }, { 5, 3, 5, 28, 4, 9 },
    { 5, 3, 12, 28, 4, 9 }, { 5, 1, 21, 10, 2, 8 },  { 5, 12, 17, 2, 2, 8 }, { 5, 15, 17, 2, 2, 8 },
    { 5, 14, 17, 1, 2, 8 }, { 0xff, 0, 0, 0, 0, 0 },
};

static const BScenarioWindowSetup sBScenarioCastWindows[13] = {
    { 5, 2, 1, 28, 2, 8 },   { 5, 1, 6, 10, 2, 9 },  { 5, 2, 11, 10, 2, 9 },  { 5, 4, 15, 12, 2, 9 },
    { 5, 13, 15, 12, 2, 9 }, { 5, 16, 4, 12, 2, 9 }, { 5, 16, 7, 12, 2, 9 },  { 5, 16, 10, 12, 2, 9 },
    { 5, 16, 13, 12, 2, 9 }, { 5, 4, 17, 12, 2, 9 }, { 5, 13, 17, 12, 2, 9 }, { 5, 1, 21, 10, 2, 8 },
    { 0xff, 0, 0, 0, 0, 0 },
};

static const BScenarioWindowSetup *sBScenarioWindowTables[3] = { sBScenarioSummaryWindows, sBScenarioScriptWindows,
                                                                 sBScenarioCastWindows };

static const TouchRect *sBScenarioTouchRectTables[3] = { sBScenarioSummaryTouchRects, sBScenarioScriptTouchRects,
                                                         sBScenarioCastTouchRects };

// Loads the palette of the film's series, and the characters and the screen of the pages' BG
void BScenarioGraphic_LoadBG(BScenarioWork *work) {
    int i;
    u32 heapId = work->param->heapId;
    const BScenarioBGLoad *charLoad = sBScenarioCharLoads;
    const BScenarioBGLoad *screenLoad = sBScenarioScreenLoads;
    ArcTool *arc;
    const BScenarioPaletteLoad *paletteLoad;

    arc = GFL_ArcSysCreateFileHandle(ARCID_BATTLE_SCENARIO, HEAPID_TAIL(heapId));
    paletteLoad = &sBScenarioPaletteLoads[(u32)sBScenarioGraphicSeries[work->param->scenario] % 4];
    GFL_G2DIOLoadArcNCLRDefault(arc, paletteLoad->fileId, paletteLoad->type, paletteLoad->offset, 0, heapId);
    for (i = 0; i < 8; i++) {
        if (charLoad->bg == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, charLoad->fileId, charLoad->bg, charLoad->offset, 0, FALSE, heapId);
        charLoad++;
    }
    for (i = 0; i < 8; i++) {
        if (screenLoad->bg == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screenLoad->fileId, screenLoad->bg, screenLoad->offset, 0, FALSE,
                                               heapId);
        screenLoad++;
    }
    GFL_ArcToolFree(arc);
}

void BScenarioGraphic_ScrollBG(BScenarioWork *work, int page) {
    GFL_BGSysMoveBG(BG_PAGES, BG_MOVE_SET_X, sBScenarioScrolls[page].x);
    GFL_BGSysMoveBG(BG_PAGES, BG_MOVE_SET_Y, sBScenarioScrolls[page].y);
}

void BScenarioGraphic_CreateWindows(BScenarioWork *work, int page) {
    int i;
    const BScenarioWindowSetup *setup = sBScenarioWindowTables[page];

    work->shownWindows = 0;
    BScenarioGraphic_FreeWindows(work);
    for (i = 0; i < BSCENARIO_WINDOW_MAX; i++) {
        if (setup->bg == 0xff) {
            break;
        }
        work->windows[i].window =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        setup++;
    }
}

void BScenarioGraphic_Print(BScenarioWork *work, u32 window, int x, int y, StrBuf *str, u32 align, u16 color,
                            BOOL smallFont, BOOL keep) {
    if (!keep) {
        BScenarioGraphic_ClearWindow(work, window);
    }
    if (!smallFont) {
        u16 offset = BScenarioGraphic_GetAlignOffset(str, work->param->font, align);
        PrintQueue *printQueue = work->printQueue;

        func_02021c7c(printQueue, BmpWin_GetBitmap(work->windows[window].window), x - offset, y, str, work->param->font,
                      color);
        work->windows[window].dirty = TRUE;
    } else {
        u16 offset = BScenarioGraphic_GetAlignOffset(str, work->smallFont, align);
        PrintQueue *printQueue = work->printQueue;

        func_02021c7c(printQueue, BmpWin_GetBitmap(work->windows[window].window), x - offset, y, str, work->smallFont,
                      color);
        work->windows[window].dirty = TRUE;
    }
}

void BScenarioGraphic_PrintNumber(BScenarioWork *work, u32 window, int x, int y, int number, u32 digits, u32 align,
                                  BOOL smallFont) {
    StrBuf *str = GFL_StrBufCreate(0x80, work->param->heapId);

    GFL_WordSetFormatNumber(str, number, digits, 2, TRUE);
    BScenarioGraphic_Print(work, window, x, y, str, align, TEXT_COLOR, smallFont, FALSE);
    GFL_StrBufFree(str);
}

void BScenarioGraphic_FreeWindows(BScenarioWork *work) {
    int i;

    for (i = 0; i < BSCENARIO_WINDOW_MAX; i++) {
        if (work->windows[i].window != NULL) {
            GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i].window), 0);
            BmpWin_ClearScreen(work->windows[i].window);
            BmpWin_Free(work->windows[i].window);
            work->windows[i].window = NULL;
        }
    }
}

void BScenarioGraphic_ClearWindow(BScenarioWork *work, u32 window) {
    if (work->windows[window].window != NULL) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[window].window), 0);
        work->windows[window].dirty = TRUE;
    }
}

static inline BOOL BScenarioGraphic_IsWindowPrinted(BScenarioWindow *window) {
    if (!window->dirty) {
        return TRUE;
    }
    return FALSE;
}

// Runs the print queue, copies the characters of the windows it has printed, and shows each window once it has
void BScenarioGraphic_UpdatePrint(BScenarioWork *work) {
    int i;

    func_02021a3c(work->printQueue);
    for (i = 0; i < BSCENARIO_WINDOW_MAX; i++) {
        if (work->windows[i].window != NULL) {
            PrintQueue *printQueue = work->printQueue;

            if (work->windows[i].dirty) {
                if (!func_02021c1c(printQueue, BmpWin_GetBitmap(work->windows[i].window))) {
                    BmpWin_FlushChar(work->windows[i].window);
                    work->windows[i].dirty = FALSE;
                }
            }
            if (BScenarioGraphic_IsWindowPrinted(&work->windows[i]) == TRUE) {
                if (!(work->shownWindows & (1 << i))) {
                    BmpWin_FlushMap(work->windows[i].window);
                    GFL_BGSysLoadScr(BmpWin_GetBGIndex(work->windows[i].window));
                    work->shownWindows |= 1 << i;
                }
            }
        }
    }
}

static u16 BScenarioGraphic_GetAlignOffset(StrBuf *str, Font *font, u32 align) {
    if (align == ALIGN_RIGHT) {
        return GFL_FontGetBlockWidth(str, font, 0);
    }
    if (align == ALIGN_CENTER) {
        return GFL_FontGetBlockWidth(str, font, 0) >> 1;
    }
    return 0;
}

s32 BScenarioGraphic_GetTouched(BScenarioWork *work, int page) {
    return func_0203da0c(sBScenarioTouchRectTables[page]);
}

static void BScenarioGraphic_LoadTypeResources(BScenarioWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(work->param->heapId));
    u32 i;

    work->palettes[BSCENARIO_RES_TYPE] = func_0204bbb8(arc, FILE_TYPE_PALETTE, 1, 0x80, 0, 3, work->param->heapId);
    work->cells[BSCENARIO_RES_TYPE] = func_0204bde0(arc, FILE_TYPE_CELLS, FILE_TYPE_ANIMS, work->param->heapId);
    for (i = 0; i < 6; i++) {
        work->chars[BSCENARIO_CHARS_TYPE + i] = func_0204b81c(arc, func_0202d7f4(0), FALSE, 1, work->param->heapId);
    }
    GFL_ArcToolFree(arc);
}

static void BScenarioGraphic_LoadIconResources(BScenarioWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(work->param->heapId));
    u32 i;

    work->palettes[BSCENARIO_RES_ICON] = func_0204bc48(arc, PokeIcon_GetPaletteFile(), 1, 0, work->param->heapId);
    work->cells[BSCENARIO_RES_ICON] =
        func_0204bde0(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), work->param->heapId);
    for (i = 0; i < 6; i++) {
        if (i < work->castCount) {
            PokeParty_GetPkm(work->param->party, i);
            work->chars[BSCENARIO_CHARS_ICON + i] =
                func_0204b81c(arc, PokeParty_GetIconIndex(work->mons[i].species, 1, work->mons[i].sex, FALSE), FALSE, 1,
                              work->param->heapId);
        } else {
            work->chars[BSCENARIO_CHARS_ICON + i] =
                func_0204b81c(arc, PokeParty_GetIconIndex(0, 0, 0, FALSE), FALSE, 1, work->param->heapId);
        }
    }
    GFL_ArcToolFree(arc);
}

static void BScenarioGraphic_LoadCursorResources(BScenarioWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BATTLE_CURSOR, HEAPID_TAIL(work->param->heapId));

    work->palettes[BSCENARIO_RES_CURSOR] = func_0204bbb8(arc, FILE_CURSOR_PALETTE, 1, 0xe0, 4, 1, work->param->heapId);
    work->cells[BSCENARIO_RES_CURSOR] = func_0204bde0(arc, FILE_CURSOR_CELLS, FILE_CURSOR_ANIMS, work->param->heapId);
    work->chars[BSCENARIO_CHARS_CURSOR] = func_0204b81c(arc, FILE_CURSOR_CHARS, FALSE, 1, work->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BScenarioGraphic_LoadButtonResources(BScenarioWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BATTLE_SCENARIO, HEAPID_TAIL(work->param->heapId));

    work->palettes[BSCENARIO_RES_BUTTON] = func_0204bbb8(arc, FILE_BUTTON_PALETTE, 1, 0x100, 0, 5, work->param->heapId);
    work->cells[BSCENARIO_RES_BUTTON] = func_0204bde0(arc, FILE_BUTTON_CELLS, FILE_BUTTON_ANIMS, work->param->heapId);
    work->chars[BSCENARIO_CHARS_BUTTON] = func_0204b81c(arc, FILE_BUTTON_CHARS, FALSE, 1, work->param->heapId);
    GFL_ArcToolFree(arc);
}

// Loads a type's icon into one of the type actors' characters, and gives the actor the icon's palette
void BScenarioGraphic_SetTypeIcon(BScenarioWork *work, int index, u32 unused, u32 type) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_TAIL(work->param->heapId));
    NNSG2dCharacterData *character;
    void *file = GFL_G2DIOReadOBJNCGRArc(arc, func_0202d7f4(type), FALSE, &character, HEAPID_TAIL(work->param->heapId));

    func_0204ba40(work->chars[BSCENARIO_CHARS_TYPE + index], character);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    func_0204c378(work->typeActors[index], func_0202d7e8(type), 1);
}

void BScenarioGraphic_SetIconPalette(BScenarioWork *work, int index, u32 species, u32 sex) {
    func_0204c378(work->iconActors[index], PokeIcon_GetPalette(species, 0, sex, FALSE), 0);
}

static void BScenarioGraphic_CreateActors(BScenarioWork *work, u32 surface, const BScenarioActorSetup *setups,
                                          const s16 *resources) {
    int i;
    ClActUnit *unit = work->unit;
    ClActorSetup setup;
    ClActor **actors;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    s16 palette = resources[0];
    s16 cells = resources[2];
    s16 count = resources[3];
    s16 group = resources[4];
    if (group == BSCENARIO_ACTORS_TYPE) {
        actors = work->typeActors;
    } else if (group == BSCENARIO_ACTORS_ICON) {
        actors = work->iconActors;
    } else if (group == BSCENARIO_ACTORS_CURSOR) {
        actors = work->cursorActors;
    } else if (group == BSCENARIO_ACTORS_BUTTON) {
        actors = work->buttonActors;
    }
    for (i = 0; i < count; i++) {
        if (setups->sequence == 0xff) {
            break;
        }
        u32 position = setups->position * 2;
        s16 x = BScenarioGraphic_GetPosition(work, position);
        s16 y = BScenarioGraphic_GetPosition(work, position + 1);

        setup.x = x;
        setup.y = y;
        setup.sequence = setups->sequence;
        setup.priority = setups->priority;
        setup.bgPriority = setups->bgPriority;
        actors[i] = func_0204c040(unit, work->chars[setups->chars], work->palettes[palette], work->cells[cells], &setup,
                                  surface, work->param->heapId);
        BOOL visible = FALSE;
        if (setups->visible == 1) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        BOOL animated = FALSE;
        if (setups->flags & 1) {
            animated = TRUE;
        }
        func_0204c520(actors[i], animated);
        if (setups->flags & 2) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (setups->flags & 4) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
        setups++;
    }
}

void BScenarioGraphic_InitActors(BScenarioWork *work) {
    int i;

    work->unit = func_0204bf1c(21, 0, HEAPID_TAIL(work->param->heapId));
    BScenarioGraphic_LoadTypeResources(work);
    BScenarioGraphic_LoadIconResources(work);
    BScenarioGraphic_LoadCursorResources(work);
    BScenarioGraphic_LoadButtonResources(work);
    BScenarioGraphic_CreateActors(work, CLACT_SURFACE_SUB, sBScenarioTypeSetups, sBScenarioTypeResources);
    BScenarioGraphic_CreateActors(work, CLACT_SURFACE_SUB, sBScenarioIconSetups, sBScenarioIconResources);
    BScenarioGraphic_CreateActors(work, CLACT_SURFACE_SUB, sBScenarioCursorSetups, sBScenarioCursorResources);
    BScenarioGraphic_CreateActors(work, CLACT_SURFACE_SUB, sBScenarioButtonSetups, sBScenarioButtonResources);
    for (i = 0; i < 4; i++) {
        func_0204c378(work->cursorActors[i], 0, TRUE);
    }
    func_0204c318(work->buttonActors[BSCENARIO_BUTTON_PLATE], 1);
}

void BScenarioGraphic_FreeActors(BScenarioWork *work) {
    int i;

    for (i = 0; i < 6; i++) {
        func_0204c108(work->typeActors[i]);
    }
    for (i = 0; i < 6; i++) {
        func_0204c108(work->iconActors[i]);
    }
    for (i = 0; i < 4; i++) {
        func_0204c108(work->cursorActors[i]);
    }
    for (i = 0; i < BSCENARIO_BUTTON_COUNT; i++) {
        func_0204c108(work->buttonActors[i]);
    }
    for (i = 0; i < BSCENARIO_RES_COUNT; i++) {
        if (work->palettes[i] != -1) {
            func_0204bcd0(work->palettes[i]);
            work->palettes[i] = -1;
        }
    }
    for (i = 0; i < BSCENARIO_CHARS_COUNT; i++) {
        if (work->chars[i] != -1) {
            func_0204b98c(work->chars[i]);
            work->chars[i] = -1;
        }
    }
    for (i = 0; i < BSCENARIO_RES_COUNT; i++) {
        if (work->cells[i] != -1) {
            func_0204be64(work->cells[i]);
            work->cells[i] = -1;
        }
    }
    func_0204bf98(work->unit);
}

void BScenarioGraphic_SetActorVisible(BScenarioWork *work, int group, int index, BOOL visible) {
    if (group == BSCENARIO_ACTORS_TYPE) {
        if (index < 6) {
            func_0204c124(work->typeActors[index], visible);
        }
    } else if (group == BSCENARIO_ACTORS_ICON) {
        if (index < 6) {
            func_0204c124(work->iconActors[index], visible);
        }
    } else if (group == BSCENARIO_ACTORS_CURSOR) {
        if (index < 4) {
            func_0204c124(work->cursorActors[index], visible);
        }
    } else if (group == BSCENARIO_ACTORS_BUTTON) {
        if (index < BSCENARIO_BUTTON_COUNT) {
            func_0204c124(work->buttonActors[index], visible);
        }
    }
}

static s16 BScenarioGraphic_GetPosition(BScenarioWork *work, u32 index) {
    if (index >= POSITION_COUNT) {
        return 0;
    }
    return work->positions[index];
}

static void BScenarioGraphic_SetCursor(BScenarioWork *work, BOOL show, int place) {
    ClActorPos pos;
    int i;

    for (i = 0; i < 4; i++) {
        BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_CURSOR, i, show);
    }
    if (show == TRUE) {
        for (i = 0; i < 4; i++) {
            u32 corner = place * 4 + i;
            s16 index = sBScenarioCursorPositions[corner];

            pos.x = BScenarioGraphic_GetPosition(work, index * 2);
            pos.y = BScenarioGraphic_GetPosition(work, index * 2 + 1);
            func_0204c140(work->cursorActors[i], &pos, CLACT_SURFACE_SUB);
        }
    }
}

void BScenarioGraphic_UpdateCursor(BScenarioWork *work, BOOL show, int page, int cursor) {
    s16 place = sBScenarioCursorPlaces[page * 4 + cursor];

    if (place != -1) {
        BScenarioGraphic_SetCursor(work, show, place);
    }
}

void BScenarioGraphic_SetButtonsVisible(BScenarioWork *work, int page, BOOL unused) {
    int i;

    for (i = 0; i < 3; i++) {
        BOOL show = TRUE;

        if (sBScenarioButtonsVisible[page][i] == -1) {
            show = FALSE;
        }
        BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_BUTTON, i, show);
    }
}

void BScenarioGraphic_SetButtonAnim(BScenarioWork *work, int button, u32 sequence) {
    ClActor **actors = work->buttonActors;

    func_0204c488(actors[button], sequence);
    func_0204c520(actors[button], TRUE);
}

BOOL BScenarioGraphic_IsButtonAnimating(BScenarioWork *work, int button) {
    BOOL animating = TRUE;

    if (!func_0204c560(work->buttonActors[button])) {
        animating = FALSE;
    }
    return animating;
}

static void BScenarioGraphic_SetFlashPalette(BScenarioWork *work, int button, int phase) {
    GFL_BGSysSetScrPaletteNo(BG_PAGES, sBScenarioFlashRects[button][0], sBScenarioFlashRects[button][1],
                             sBScenarioFlashRects[button][2], sBScenarioFlashRects[button][3],
                             sBScenarioFlashPalettes[button * 2 + phase]);
    GFL_BGSysLoadScr(BG_PAGES);
}

void BScenarioGraphic_StartFlash(BScenarioWork *work, int button) {
    work->flashFlags |= FLASH_ACTIVE;
    work->flashFrame[0] = 0;
    work->flashButton = button;
    work->flashStep = 0;
    BScenarioGraphic_UpdateFlash(work);
}

void BScenarioGraphic_UpdateFlash(BScenarioWork *work) {
    if (!(work->flashFlags & FLASH_ACTIVE)) {
        return;
    }
    work->flashFrame[0]++;
    if (work->flashFrame[0] == sBScenarioFlashSteps[work->flashStep].frame) {
        if (sBScenarioFlashSteps[work->flashStep].palette != PALETTE_KEEP) {
            BScenarioGraphic_SetFlashPalette(work, work->flashButton, sBScenarioFlashSteps[work->flashStep].palette);
        }
        work->flashStep++;
        if (sBScenarioFlashSteps[work->flashStep].frame == -1) {
            work->flashFlags &= ~FLASH_ACTIVE;
            work->flashFrame[0] = 0;
        }
    }
}

BOOL BScenarioGraphic_IsFlashing(BScenarioWork *work) {
    BOOL flashing = FALSE;

    if (work->flashFlags & FLASH_ACTIVE) {
        flashing = TRUE;
    }
    return flashing;
}

// The plate shows on every page but the summary
void BScenarioGraphic_SetPlateVisible(BScenarioWork *work, int page) {
    BOOL visible = TRUE;

    if (page == 0) {
        visible = FALSE;
    }
    BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_BUTTON, BSCENARIO_BUTTON_PLATE, visible);
}

void BScenarioGraphic_SetMarker(BScenarioWork *work, int line, u32 unused, BOOL visible) {
    if (visible == TRUE) {
        ClActorPos pos;
        s16 index = sBScenarioMarkerPositions[line];

        pos.x = BScenarioGraphic_GetPosition(work, index * 2);
        pos.y = BScenarioGraphic_GetPosition(work, index * 2 + 1);
        func_0204c140(work->buttonActors[BSCENARIO_BUTTON_MARKER], &pos, CLACT_SURFACE_SUB);
        BScenarioGraphic_SetButtonAnim(work, BSCENARIO_BUTTON_MARKER, 10);
    }
    BScenarioGraphic_SetActorVisible(work, BSCENARIO_ACTORS_BUTTON, BSCENARIO_BUTTON_MARKER, visible);
}
