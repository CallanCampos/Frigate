#include "types.h"
#include "app/pokedex_common.h"
#include "app/pokedex_search_main.h"
#include "app/pokedex_search_obj.h"
#include "app/ui/frame_list.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/math.h"
#include "system/app_menu_common.h"
#include "system/game_data.h"

// The actors of the Pokédex's search screen: the touch bar's buttons, the scroll bar, the marks of the chosen items
// and the shapes. The ROM doesn't name this file; pokedex_search_obj.c is a guess after pokedex_search_main.c

// The resources
enum {
    CHAR_RES_TOUCHBAR,
    CHAR_RES_LIST,
    CHAR_RES_LIST_SUB,
    CHAR_RES_SHAPE,
    CHAR_RES_SHAPE_SUB,
    CHAR_RES_LOADING,
};

enum {
    CELL_RES_TOUCHBAR,
    CELL_RES_LIST,
    CELL_RES_SHAPE,
    CELL_RES_LOADING,
};

// The window that an actor's shape cuts out of the blending, which lib/nitro's GX_OAM_MODE_* doesn't have yet
#define OAM_MODE_OBJWND 2

// Where the marks are: their x, the rows' heights and the height of the top screen's list
#define MARK_X 140
#define ROW_HEIGHT 24
#define SHAPE_X 204
#define SHAPE_ROW_HEIGHT 40
#define TOP_LIST_HEIGHT 192

static void PokedexSearchObj_InitSystem(void);
static void PokedexSearchObj_LoadResources(PokedexSearchWork *wk);
static void PokedexSearchObj_FreeResources(PokedexSearchWork *wk);
static void PokedexSearchObj_CreateActors(PokedexSearchWork *wk);
static void PokedexSearchObj_DeleteActor(PokedexSearchWork *wk, u32 id);
static void PokedexSearchObj_DeleteActors(PokedexSearchWork *wk);

static const PokedexActorTemplate sPokedexSearchActorTemplates[] = {
    { { 252, 12, 0, 0, 1 }, CHAR_RES_LIST, CHAR_RES_LIST, CELL_RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 252, 84, 1, 10, 1 }, CHAR_RES_LIST, CHAR_RES_LIST, CELL_RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 232, 168, 1, 0, 0 }, CHAR_RES_TOUCHBAR, CHAR_RES_TOUCHBAR, CELL_RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 208, 168, 0, 0, 0 }, CHAR_RES_TOUCHBAR, CHAR_RES_TOUCHBAR, CELL_RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 188, 172, 6, 0, 0 }, CHAR_RES_TOUCHBAR, CHAR_RES_TOUCHBAR, CELL_RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 208, 168, 5, 0, 0 }, CHAR_RES_TOUCHBAR, CHAR_RES_TOUCHBAR, CELL_RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 188, 168, 4, 0, 0 }, CHAR_RES_TOUCHBAR, CHAR_RES_TOUCHBAR, CELL_RES_TOUCHBAR, 0, CLACT_SURFACE_MAIN },
    { { 0, 0, 0, 0, 0 }, CHAR_RES_SHAPE, CHAR_RES_SHAPE, CELL_RES_SHAPE, 0, CLACT_SURFACE_MAIN },
    { { 8, 168, 0, 0, 1 }, CHAR_RES_LOADING, CHAR_RES_LOADING, CELL_RES_LOADING, 0, CLACT_SURFACE_MAIN },
    { { 0, 0, 2, 0, 1 }, CHAR_RES_LIST, CHAR_RES_LIST, CELL_RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 0, 0, 2, 0, 1 }, CHAR_RES_LIST, CHAR_RES_LIST, CELL_RES_LIST, 0, CLACT_SURFACE_MAIN },
    { { 0, 0, 2, 0, 2 }, CHAR_RES_LIST_SUB, CHAR_RES_LIST_SUB, CELL_RES_LIST, 0, CLACT_SURFACE_SUB },
    { { 0, 0, 2, 0, 2 }, CHAR_RES_LIST_SUB, CHAR_RES_LIST_SUB, CELL_RES_LIST, 0, CLACT_SURFACE_SUB },
    { { 0, 0, 2, 0, 2 }, CHAR_RES_LIST_SUB, CHAR_RES_LIST_SUB, CELL_RES_LIST, 0, CLACT_SURFACE_SUB },
    { { 0, 0, 2, 0, 2 }, CHAR_RES_LIST_SUB, CHAR_RES_LIST_SUB, CELL_RES_LIST, 0, CLACT_SURFACE_SUB },
};

void PokedexSearchObj_Init(PokedexSearchWork *wk) {
    PokedexSearchObj_InitSystem();
    PokedexSearchObj_LoadResources(wk);
    PokedexSearchObj_CreateActors(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void PokedexSearchObj_Exit(PokedexSearchWork *wk) {
    PokedexSearchObj_DeleteActors(wk);
    PokedexSearchObj_FreeResources(wk);
    ClActSys_Delete();
}

void PokedexSearchObj_Update(PokedexSearchWork *wk) {
    u32 i;

    for (i = 0; i < POKEDEX_SEARCH_OBJ_COUNT; i++) {
        if (wk->actors[i] != NULL && ClActor_IsAutoAnim(wk->actors[i]) != TRUE) {
            ClActor_SetAnimSpeed(wk->actors[i], FX32_ONE);
        }
    }
    ClActSys_Draw();
}

void PokedexSearchObj_SetAnmSeq(PokedexSearchWork *wk, u32 id, u32 anm) {
    ClActor_SetAnimTime(wk->actors[id], 0);
    ClActor_SetSequence(wk->actors[id], anm);
}

void PokedexSearchObj_SetAnm(PokedexSearchWork *wk, u32 id, u32 anm) {
    PokedexSearchObj_SetAnmSeq(wk, id, anm);
    ClActor_SetAutoAnim(wk->actors[id], TRUE);
}

u32 PokedexSearchObj_GetAnm(PokedexSearchWork *wk, u32 id) {
    return ClActor_GetSequence(wk->actors[id]);
}

BOOL PokedexSearchObj_IsAnmPlaying(PokedexSearchWork *wk, u32 id) {
    return ClActor_IsAnimActive(wk->actors[id]);
}

void PokedexSearchObj_SetVisible(PokedexSearchWork *wk, u32 id, BOOL visible) {
    if (wk->actors[id] != NULL) {
        ClActor_SetVisible(wk->actors[id], visible);
    }
}

BOOL PokedexSearchObj_IsVisible(PokedexSearchWork *wk, u32 id) {
    return ClActor_IsVisible(wk->actors[id]);
}

void PokedexSearchObj_SetBlend(PokedexSearchWork *wk, u32 id, BOOL blend) {
    if (blend == TRUE) {
        ClActor_SetObjMode(wk->actors[id], GX_OAM_MODE_XLU);
    } else {
        ClActor_SetObjMode(wk->actors[id], GX_OAM_MODE_NORMAL);
    }
}

void PokedexSearchObj_SetPos(PokedexSearchWork *wk, u32 id, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    ClActor_SetSurfacePos(wk->actors[id], &pos, surface);
}

void PokedexSearchObj_GetPos(PokedexSearchWork *wk, u32 id, s16 *x, s16 *y, u16 surface) {
    ClActorPos pos;

    ClActor_GetSurfacePos(wk->actors[id], &pos, surface);
    *x = pos.x;
    *y = pos.y;
}

void PokedexSearchObj_SetBGPriority(PokedexSearchWork *wk, u32 id, u32 priority) {
    ClActor_SetBgPriority(wk->actors[id], (u8)priority);
}

static void PokedexSearchObj_InitSystem(void) {
    ClActSysSetup setup = { 0,
                            0,
                            0,
                            512,
                            4,
                            124,
                            4,
                            124,
                            0,
                            POKEDEX_SEARCH_CHAR_RES_COUNT,
                            POKEDEX_SEARCH_PALETTE_RES_COUNT,
                            POKEDEX_SEARCH_CELL_RES_COUNT,
                            0,
                            16,
                            16 };

    ClActSys_Create(&setup, PokedexSearchMain_GetVRAMConfig(), HEAPID_POKEDEX_SEARCH);
}

static void PokedexSearchObj_LoadResources(PokedexSearchWork *wk) {
    ArcTool *arc;
    u32 i;

    for (i = 0; i < POKEDEX_SEARCH_CHAR_RES_COUNT; i++) {
        wk->charRes[i] = -1;
    }
    for (i = 0; i < POKEDEX_SEARCH_PALETTE_RES_COUNT; i++) {
        wk->paletteRes[i] = -1;
    }
    for (i = 0; i < POKEDEX_SEARCH_CELL_RES_COUNT; i++) {
        wk->cellRes[i] = -1;
    }

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_LIST] = ClActSys_LoadChars(arc, 80, FALSE, CLACT_VRAM_MAIN, HEAPID_POKEDEX_SEARCH);
    wk->paletteRes[CHAR_RES_LIST] = ClActSys_LoadPltt(arc, 77, CLACT_VRAM_MAIN, 0 * 0x20, HEAPID_POKEDEX_SEARCH);
    wk->cellRes[CELL_RES_LIST] = ClActSys_LoadCellAnims(arc, 86, 102, HEAPID_POKEDEX_SEARCH);
    wk->charRes[CHAR_RES_LIST_SUB] = ClActSys_LoadChars(arc, 80, FALSE, CLACT_VRAM_SUB, HEAPID_POKEDEX_SEARCH);
    wk->paletteRes[CHAR_RES_LIST_SUB] = ClActSys_LoadPltt(arc, 77, CLACT_VRAM_SUB, 0 * 0x20, HEAPID_POKEDEX_SEARCH);
    wk->charRes[CHAR_RES_LOADING] = ClActSys_LoadChars(arc, 79, FALSE, CLACT_VRAM_MAIN, HEAPID_POKEDEX_SEARCH);
    wk->paletteRes[CHAR_RES_LOADING] = ClActSys_LoadPltt(arc, 74, CLACT_VRAM_MAIN, 3 * 0x20, HEAPID_POKEDEX_SEARCH);
    wk->cellRes[CELL_RES_LOADING] = ClActSys_LoadCellAnims(arc, 85, 101, HEAPID_POKEDEX_SEARCH);
    wk->charRes[CHAR_RES_SHAPE] = ClActSys_LoadChars(arc, 84, FALSE, CLACT_VRAM_MAIN, HEAPID_POKEDEX_SEARCH);
    wk->paletteRes[CHAR_RES_SHAPE] = ClActSys_LoadPltt(arc, 78, CLACT_VRAM_MAIN, 4 * 0x20, HEAPID_POKEDEX_SEARCH);
    wk->cellRes[CELL_RES_SHAPE] = ClActSys_LoadCellAnims(arc, 87, 103, HEAPID_POKEDEX_SEARCH);
    wk->charRes[CHAR_RES_SHAPE_SUB] = ClActSys_LoadChars(arc, 84, FALSE, CLACT_VRAM_SUB, HEAPID_POKEDEX_SEARCH);
    wk->paletteRes[CHAR_RES_SHAPE_SUB] = ClActSys_LoadPltt(arc, 78, CLACT_VRAM_SUB, 3 * 0x20, HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_TOUCHBAR] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_MAIN, HEAPID_POKEDEX_SEARCH);
    wk->paletteRes[CHAR_RES_TOUCHBAR] = ClActSys_LoadPltt(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_MAIN,
                                                          5 * 0x20, HEAPID_POKEDEX_SEARCH);
    wk->cellRes[CELL_RES_TOUCHBAR] = ClActSys_LoadCellAnims(
        arc, AppMenuCommon_GetTouchBarCellFile(2), AppMenuCommon_GetTouchBarAnimFile(2), HEAPID_POKEDEX_SEARCH);
    GFL_ArcToolFree(arc);
}

static void PokedexSearchObj_FreeResources(PokedexSearchWork *wk) {
    u32 i;

    for (i = 0; i < POKEDEX_SEARCH_CHAR_RES_COUNT; i++) {
        if (wk->charRes[i] != -1) {
            ClActSys_FreeChars(wk->charRes[i]);
        }
    }
    for (i = 0; i < POKEDEX_SEARCH_PALETTE_RES_COUNT; i++) {
        if (wk->paletteRes[i] != -1) {
            ClActSys_FreePltt(wk->paletteRes[i]);
        }
    }
    for (i = 0; i < POKEDEX_SEARCH_CELL_RES_COUNT; i++) {
        if (wk->cellRes[i] != -1) {
            ClActSys_FreeCellAnims(wk->cellRes[i]);
        }
    }
}

static void PokedexSearchObj_CreateActors(PokedexSearchWork *wk) {
    u32 i;

    wk->actorUnit = ClActUnit_Create(POKEDEX_SEARCH_OBJ_COUNT, 0, HEAPID_POKEDEX_SEARCH);
    for (i = 0; i < POKEDEX_SEARCH_OBJ_COUNT; i++) {
        wk->actors[i] = NULL;
    }
    for (i = POKEDEX_SEARCH_OBJ_SCROLL_BAR; i <= POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1; i++) {
        PokedexActorTemplate template = sPokedexSearchActorTemplates[i];

        template.chars = wk->charRes[template.chars];
        template.palette = wk->paletteRes[template.palette];
        template.cells = wk->cellRes[template.cells];
        wk->actors[i] = PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_SEARCH);
    }
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, 7);
    }
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_RAIL, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_LOADING_BAR, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1, FALSE);
    ClActor_SetObjMode(wk->actors[POKEDEX_SEARCH_OBJ_MARK_WINDOW], OAM_MODE_OBJWND);
    ClActor_SetObjMode(wk->actors[POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1], OAM_MODE_OBJWND);
    PokedexSearchObj_SetBlend(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, TRUE);
    PokedexSearchObj_SetBlend(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + 1, TRUE);

    {
        PokedexActorTemplate template;
        PokedexActorTemplate shape = { { 0, 0, 0, 0, 1 }, CHAR_RES_SHAPE, CHAR_RES_SHAPE, CELL_RES_SHAPE, 0,
                                       CLACT_SURFACE_MAIN };
        PokedexActorTemplate shapeSub = { { 0, 0, 0, 0, 2 }, CHAR_RES_SHAPE_SUB, CHAR_RES_SHAPE_SUB, CELL_RES_SHAPE, 0,
                                          CLACT_SURFACE_SUB };

        for (i = 0; i < POKEDEX_SEARCH_SHAPE_COUNT; i++) {
            template = shape;
            template.chars = wk->charRes[template.chars];
            template.palette = wk->paletteRes[template.palette];
            template.cells = wk->cellRes[template.cells];
            template.setup.sequence = i;
            wk->actors[POKEDEX_SEARCH_OBJ_SHAPE_ICON + i] =
                PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_SEARCH);
            PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON + i, FALSE);

            template = shapeSub;
            template.chars = wk->charRes[template.chars];
            template.palette = wk->paletteRes[template.palette];
            template.cells = wk->cellRes[template.cells];
            template.setup.sequence = i;
            wk->actors[POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + i] =
                PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_SEARCH);
            PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + i, FALSE);
            PokedexSearchObj_SetBlend(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + i, TRUE);

            wk->actors[POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + i] =
                PokedexCommon_CreateActor(wk->actorUnit, &template, HEAPID_POKEDEX_SEARCH);
            PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + i, FALSE);
            ClActor_SetObjMode(wk->actors[POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + i], OAM_MODE_OBJWND);
        }
    }
}

static void PokedexSearchObj_DeleteActor(PokedexSearchWork *wk, u32 id) {
    if (wk->actors[id] != NULL) {
        ClActor_Delete(wk->actors[id]);
        wk->actors[id] = NULL;
    }
}

static void PokedexSearchObj_DeleteActors(PokedexSearchWork *wk) {
    u32 i;

    for (i = 0; i < POKEDEX_SEARCH_OBJ_COUNT; i++) {
        PokedexSearchObj_DeleteActor(wk, i);
    }
    ClActUnit_Delete(wk->actorUnit);
}

void PokedexSearchObj_HideList(PokedexSearchWork *wk) {
    u32 i;

    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_RAIL, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1, FALSE);
    for (i = 0; i < POKEDEX_SEARCH_SHAPE_COUNT; i++) {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON + i, FALSE);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + i, FALSE);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + i, FALSE);
    }
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE, FALSE);
}

void PokedexSearchObj_SetMain(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_RETURN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_EXIT, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, TRUE);
    if (wk->param->criteria->shape != POKEDEX_SEARCH_NONE) {
        PokedexSearchObj_SetAnmSeq(wk, POKEDEX_SEARCH_OBJ_SHAPE, wk->param->criteria->shape);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SHAPE, 192, 116, CLACT_SURFACE_MAIN);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE, TRUE);
    }
}

void PokedexSearchObj_SetOrder(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_EXIT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_RETURN, TRUE);
}

void PokedexSearchObj_SetInitial(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_EXIT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_RETURN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_RAIL, TRUE);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
}

void PokedexSearchObj_SetType(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_EXIT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_RETURN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_RAIL, TRUE);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
}

void PokedexSearchObj_SetColor(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_EXIT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_RETURN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_RAIL, TRUE);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
}

void PokedexSearchObj_SetShape(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_EXIT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHORTCUT, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_RETURN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, TRUE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SCROLL_RAIL, TRUE);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
    PokedexSearchObj_PutShape(wk);
}

void PokedexSearchObj_PutShape(PokedexSearchWork *wk) {
    if (wk->param->criteria->shape != POKEDEX_SEARCH_NONE) {
        PokedexSearchObj_SetAnmSeq(wk, POKEDEX_SEARCH_OBJ_SHAPE, wk->param->criteria->shape);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SHAPE, 64, 104, CLACT_SURFACE_MAIN);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE, TRUE);
    } else {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE, FALSE);
    }
}

void PokedexSearchObj_PutShapeIcons(PokedexSearchWork *wk) {
    s16 scroll = FrameList_GetScroll(wk->frameList);
    s16 i;

    for (i = 0; i < POKEDEX_SEARCH_SHAPE_COUNT; i++) {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON + i, FALSE);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + i, FALSE);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + i, FALSE);
    }
    for (i = 0; i < 5; i++) {
        PokedexSearchObj_PutShapeIcon(wk, scroll + i, i * SHAPE_ROW_HEIGHT, TRUE);
        if (scroll - (i + 1) >= 0) {
            PokedexSearchObj_PutShapeIcon(wk, scroll - (i + 1), TOP_LIST_HEIGHT - (i + 1) * SHAPE_ROW_HEIGHT, FALSE);
        }
    }
}

void PokedexSearchObj_UpdateScrollBar(PokedexSearchWork *wk) {
    s16 x, y;
    u32 pos;

    PokedexSearchObj_GetPos(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, &x, &y, CLACT_SURFACE_MAIN);
    pos = FrameList_ClampBarPos(wk->frameList, y);
    if (pos < 12) {
        pos = 12;
    } else if (pos > 156) {
        pos = 156;
    }
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SCROLL_BAR, 252, pos, CLACT_SURFACE_MAIN);
}

void PokedexSearchObj_SetPageButtons(PokedexSearchWork *wk, BOOL keep) {
    s16 cursor = FrameList_GetCursor(wk->frameList);

    if (PokedexSearchObj_GetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP) != 12 || keep == FALSE) {
        if (FrameList_GetScroll(wk->frameList) == 0 && cursor == 0) {
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 18);
        } else {
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_UP, 4);
        }
    }
    if (PokedexSearchObj_GetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN) != 13 || keep == FALSE) {
        if (FrameList_CanScrollDown(wk->frameList) == FALSE && cursor == wk->listRows - 1) {
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 19);
        } else {
            PokedexSearchObj_SetAnm(wk, POKEDEX_SEARCH_OBJ_PAGE_DOWN, 5);
        }
    }
}

void PokedexSearchObj_PutMark(PokedexSearchWork *wk, int index, s16 y, BOOL touchScreen) {
    if (touchScreen == TRUE) {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK + index, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK + index, MARK_X, y + ROW_HEIGHT / 2, CLACT_SURFACE_MAIN);
    } else {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + index, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + index, MARK_X, y + ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + index, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + index, MARK_X, y + ROW_HEIGHT / 2,
                                CLACT_SURFACE_SUB);
    }
}

void PokedexSearchObj_HideMarks(PokedexSearchWork *wk) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1, FALSE);
}

void PokedexSearchObj_PutRowMark(PokedexSearchWork *wk, u16 row, BOOL visible) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, visible);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, FALSE);
    if (visible == TRUE) {
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK, MARK_X, row * ROW_HEIGHT + ROW_HEIGHT / 2,
                                CLACT_SURFACE_MAIN);
    }
}

// The marks of both types: on the top screen's rows above the list's scroll, or on the touch screen's
void PokedexSearchObj_PutTypeMarks(PokedexSearchWork *wk, u8 type1, u8 type2) {
    u8 scroll = FrameList_GetScroll(wk->frameList);
    u8 diff;

    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + 1, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1, FALSE);
    if (type1 != POKEDEX_SEARCH_NONE) {
        type1 = PokedexSearchMain_TypeToListPos(wk, type1);
        diff = MATH_ABS(type1 - scroll);
        if (type1 < scroll) {
            if (diff <= 8) {
                PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, TRUE);
                PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, MARK_X,
                                        (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
                PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, TRUE);
                PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, MARK_X,
                                        (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
            }
        } else if (diff <= 7) {
            PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, TRUE);
            PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK, MARK_X, diff * ROW_HEIGHT + ROW_HEIGHT / 2,
                                    CLACT_SURFACE_MAIN);
        }
    }
    if (type2 != POKEDEX_SEARCH_NONE) {
        type2 = PokedexSearchMain_TypeToListPos(wk, type2);
        diff = MATH_ABS(type2 - scroll);
        if (type2 < scroll) {
            if (diff <= 8) {
                PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + 1, TRUE);
                PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_SUB + 1, MARK_X,
                                        (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
                PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1, TRUE);
                PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW + 1, MARK_X,
                                        (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
            }
        } else if (diff <= 7) {
            PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK + 1, TRUE);
            PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK + 1, MARK_X, diff * ROW_HEIGHT + ROW_HEIGHT / 2,
                                    CLACT_SURFACE_MAIN);
        }
    }
}

void PokedexSearchObj_PutShapeMark(PokedexSearchWork *wk, s16 y, BOOL touchScreen) {
    if (touchScreen == TRUE) {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK, MARK_X, y + SHAPE_ROW_HEIGHT / 2, CLACT_SURFACE_MAIN);
    } else {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, MARK_X, y + SHAPE_ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, MARK_X, y + SHAPE_ROW_HEIGHT / 2,
                                CLACT_SURFACE_SUB);
    }
}

void PokedexSearchObj_PutShapeRowMark(PokedexSearchWork *wk, u16 row, BOOL visible) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, visible);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, FALSE);
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, FALSE);
    if (visible == TRUE) {
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK, MARK_X, row * SHAPE_ROW_HEIGHT + SHAPE_ROW_HEIGHT / 2,
                                CLACT_SURFACE_MAIN);
    }
}

void PokedexSearchObj_PutItemMark(PokedexSearchWork *wk, u32 index, u16 item) {
    u32 scroll = FrameList_GetScroll(wk->frameList);
    s16 top = scroll - 7;
    s16 bottom = scroll + 7;

    if (top < 0) {
        top = 0;
    }
    if (item >= top && item < scroll) {
        PokedexSearchObj_PutMark(wk, index, TOP_LIST_HEIGHT - (scroll - item) * ROW_HEIGHT, FALSE);
    } else if (item >= scroll && item < bottom) {
        PokedexSearchObj_PutMark(wk, index, (item - scroll) * ROW_HEIGHT, TRUE);
    }
}

void PokedexSearchObj_PutShapeItemMark(PokedexSearchWork *wk) {
    u32 scroll = FrameList_GetScroll(wk->frameList);
    u8 shape = wk->param->criteria->shape;
    s16 top = shape - 4;
    s16 bottom = shape + 4;

    if (top < 0) {
        top = 0;
    }
    if (shape >= top && shape < scroll) {
        PokedexSearchObj_PutShapeMark(wk, TOP_LIST_HEIGHT - (scroll - shape) * SHAPE_ROW_HEIGHT, FALSE);
    } else if (shape >= scroll && shape < bottom) {
        PokedexSearchObj_PutShapeMark(wk, (shape - scroll) * SHAPE_ROW_HEIGHT, TRUE);
    }
}

void PokedexSearchObj_PutShapeIcon(PokedexSearchWork *wk, u16 shape, s16 y, BOOL touchScreen) {
    if (touchScreen == TRUE) {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON + shape, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON + shape, SHAPE_X, y + SHAPE_ROW_HEIGHT / 2,
                                CLACT_SURFACE_MAIN);
    } else {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + shape, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB + shape, SHAPE_X, y + SHAPE_ROW_HEIGHT / 2,
                                CLACT_SURFACE_SUB);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + shape, TRUE);
        PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW + shape, SHAPE_X, y + SHAPE_ROW_HEIGHT / 2,
                                CLACT_SURFACE_SUB);
    }
}

void PokedexSearchObj_ScrollMarks(PokedexSearchWork *wk, s8 delta) {
    u32 i;
    s16 x, y;

    for (i = POKEDEX_SEARCH_OBJ_MARK; i <= POKEDEX_SEARCH_OBJ_MARK + 1; i++) {
        if (PokedexSearchObj_IsVisible(wk, i)) {
            PokedexSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            PokedexSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_MAIN);
            if (y + delta <= -ROW_HEIGHT / 2 || y + delta >= TOP_LIST_HEIGHT + ROW_HEIGHT / 2) {
                PokedexSearchObj_SetVisible(wk, i, FALSE);
            }
        }
    }
    for (i = POKEDEX_SEARCH_OBJ_MARK_SUB; i <= POKEDEX_SEARCH_OBJ_MARK_SUB + 1; i++) {
        if (PokedexSearchObj_IsVisible(wk, i)) {
            PokedexSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_SUB);
            PokedexSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_SUB);
            PokedexSearchObj_SetPos(wk, i + 2, x, y + delta, CLACT_SURFACE_SUB);
            if (y + delta <= -ROW_HEIGHT / 2 || y + delta >= TOP_LIST_HEIGHT + ROW_HEIGHT / 2) {
                PokedexSearchObj_SetVisible(wk, i, FALSE);
                PokedexSearchObj_SetVisible(wk, i + 2, FALSE);
            }
        }
    }
}

void PokedexSearchObj_ScrollShapeIcons(PokedexSearchWork *wk, s8 delta) {
    u32 i;
    s16 x, y;

    for (i = POKEDEX_SEARCH_OBJ_SHAPE_ICON; i < POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB; i++) {
        if (PokedexSearchObj_IsVisible(wk, i)) {
            PokedexSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            PokedexSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_MAIN);
            if (y + delta <= -SHAPE_ROW_HEIGHT / 2 || y + delta >= 220) {
                PokedexSearchObj_SetVisible(wk, i, FALSE);
            }
        }
    }
    PokedexSearchObj_GetPos(wk, POKEDEX_SEARCH_OBJ_MARK, &x, &y, CLACT_SURFACE_MAIN);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK, x, y + delta, CLACT_SURFACE_MAIN);
    if (y + delta <= -SHAPE_ROW_HEIGHT / 2 || y + delta >= 220) {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK, FALSE);
    }

    for (i = POKEDEX_SEARCH_OBJ_SHAPE_ICON_SUB; i < POKEDEX_SEARCH_OBJ_SHAPE_ICON_WINDOW; i++) {
        if (PokedexSearchObj_IsVisible(wk, i)) {
            PokedexSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_SUB);
            PokedexSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_SUB);
            PokedexSearchObj_SetPos(wk, i + POKEDEX_SEARCH_SHAPE_COUNT, x, y + delta, CLACT_SURFACE_SUB);
            if (y + delta <= -28 || y + delta >= 212) {
                PokedexSearchObj_SetVisible(wk, i, FALSE);
                PokedexSearchObj_SetVisible(wk, i + POKEDEX_SEARCH_SHAPE_COUNT, FALSE);
            }
        }
    }
    PokedexSearchObj_GetPos(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, &x, &y, CLACT_SURFACE_SUB);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, x, y + delta, CLACT_SURFACE_SUB);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, x, y + delta, CLACT_SURFACE_SUB);
    if (y + delta <= -28 || y + delta >= 212) {
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_SUB, FALSE);
        PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_MARK_WINDOW, FALSE);
    }
}

void PokedexSearchObj_MoveLoadingBar(PokedexSearchWork *wk, u32 count) {
    PokedexSearchObj_SetVisible(wk, POKEDEX_SEARCH_OBJ_LOADING_BAR, TRUE);
    PokedexSearchObj_SetPos(wk, POKEDEX_SEARCH_OBJ_LOADING_BAR, count * 2 + 8, 168, CLACT_SURFACE_MAIN);
}
