#include "types.h"
#include "app/ui/frame_list.h"
#include "app/zknsearch_main.h"
#include "app/zknsearch_obj.h"
#include "app/zukan_common.h"
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
// and the shapes. The ROM doesn't name this file; zknsearch_obj.c is a guess after zknsearch_main.c

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

static void ZknSearchObj_InitSystem(void);
static void ZknSearchObj_LoadResources(ZknSearchWork *wk);
static void ZknSearchObj_FreeResources(ZknSearchWork *wk);
static void ZknSearchObj_CreateActors(ZknSearchWork *wk);
static void ZknSearchObj_DeleteActor(ZknSearchWork *wk, u32 id);
static void ZknSearchObj_DeleteActors(ZknSearchWork *wk);

static const ZukanActorTemplate sZknSearchActorTemplates[] = {
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

void ZknSearchObj_Init(ZknSearchWork *wk) {
    ZknSearchObj_InitSystem();
    ZknSearchObj_LoadResources(wk);
    ZknSearchObj_CreateActors(wk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void ZknSearchObj_Exit(ZknSearchWork *wk) {
    ZknSearchObj_DeleteActors(wk);
    ZknSearchObj_FreeResources(wk);
    func_0204b758();
}

void ZknSearchObj_Update(ZknSearchWork *wk) {
    u32 i;

    for (i = 0; i < ZKNSEARCH_OBJ_COUNT; i++) {
        if (wk->actors[i] != NULL && func_0204c534(wk->actors[i]) != TRUE) {
            func_0204c4e0(wk->actors[i], FX32_ONE);
        }
    }
    func_0204b794();
}

void ZknSearchObj_SetAnmSeq(ZknSearchWork *wk, u32 id, u32 anm) {
    func_0204c4d4(wk->actors[id], 0);
    func_0204c488(wk->actors[id], anm);
}

void ZknSearchObj_SetAnm(ZknSearchWork *wk, u32 id, u32 anm) {
    ZknSearchObj_SetAnmSeq(wk, id, anm);
    func_0204c520(wk->actors[id], TRUE);
}

u32 ZknSearchObj_GetAnm(ZknSearchWork *wk, u32 id) {
    return func_0204c4a0(wk->actors[id]);
}

BOOL ZknSearchObj_IsAnmPlaying(ZknSearchWork *wk, u32 id) {
    return func_0204c560(wk->actors[id]);
}

void ZknSearchObj_SetVisible(ZknSearchWork *wk, u32 id, BOOL visible) {
    if (wk->actors[id] != NULL) {
        func_0204c124(wk->actors[id], visible);
    }
}

BOOL ZknSearchObj_IsVisible(ZknSearchWork *wk, u32 id) {
    return func_0204c138(wk->actors[id]);
}

void ZknSearchObj_SetBlend(ZknSearchWork *wk, u32 id, BOOL blend) {
    if (blend == TRUE) {
        func_0204c318(wk->actors[id], GX_OAM_MODE_XLU);
    } else {
        func_0204c318(wk->actors[id], GX_OAM_MODE_NORMAL);
    }
}

void ZknSearchObj_SetPos(ZknSearchWork *wk, u32 id, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(wk->actors[id], &pos, surface);
}

void ZknSearchObj_GetPos(ZknSearchWork *wk, u32 id, s16 *x, s16 *y, u16 surface) {
    ClActorPos pos;

    func_0204c178(wk->actors[id], &pos, surface);
    *x = pos.x;
    *y = pos.y;
}

void ZknSearchObj_SetBGPriority(ZknSearchWork *wk, u32 id, u32 priority) {
    func_0204c468(wk->actors[id], (u8)priority);
}

static void ZknSearchObj_InitSystem(void) {
    ClActSysSetup setup = { 0,
                            0,
                            0,
                            512,
                            4,
                            124,
                            4,
                            124,
                            0,
                            ZKNSEARCH_CHAR_RES_COUNT,
                            ZKNSEARCH_PALETTE_RES_COUNT,
                            ZKNSEARCH_CELL_RES_COUNT,
                            0,
                            16,
                            16 };

    ClActSys_Create(&setup, ZknSearchMain_GetVRAMConfig(), HEAPID_ZUKAN_SEARCH);
}

static void ZknSearchObj_LoadResources(ZknSearchWork *wk) {
    ArcTool *arc;
    u32 i;

    for (i = 0; i < ZKNSEARCH_CHAR_RES_COUNT; i++) {
        wk->charRes[i] = -1;
    }
    for (i = 0; i < ZKNSEARCH_PALETTE_RES_COUNT; i++) {
        wk->paletteRes[i] = -1;
    }
    for (i = 0; i < ZKNSEARCH_CELL_RES_COUNT; i++) {
        wk->cellRes[i] = -1;
    }

    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_LIST] = func_0204b81c(arc, 80, FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_SEARCH);
    wk->paletteRes[CHAR_RES_LIST] = func_0204bba0(arc, 77, CLACT_VRAM_MAIN, 0 * 0x20, HEAPID_ZUKAN_SEARCH);
    wk->cellRes[CELL_RES_LIST] = func_0204bde0(arc, 86, 102, HEAPID_ZUKAN_SEARCH);
    wk->charRes[CHAR_RES_LIST_SUB] = func_0204b81c(arc, 80, FALSE, CLACT_VRAM_SUB, HEAPID_ZUKAN_SEARCH);
    wk->paletteRes[CHAR_RES_LIST_SUB] = func_0204bba0(arc, 77, CLACT_VRAM_SUB, 0 * 0x20, HEAPID_ZUKAN_SEARCH);
    wk->charRes[CHAR_RES_LOADING] = func_0204b81c(arc, 79, FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_SEARCH);
    wk->paletteRes[CHAR_RES_LOADING] = func_0204bba0(arc, 74, CLACT_VRAM_MAIN, 3 * 0x20, HEAPID_ZUKAN_SEARCH);
    wk->cellRes[CELL_RES_LOADING] = func_0204bde0(arc, 85, 101, HEAPID_ZUKAN_SEARCH);
    wk->charRes[CHAR_RES_SHAPE] = func_0204b81c(arc, 84, FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_SEARCH);
    wk->paletteRes[CHAR_RES_SHAPE] = func_0204bba0(arc, 78, CLACT_VRAM_MAIN, 4 * 0x20, HEAPID_ZUKAN_SEARCH);
    wk->cellRes[CELL_RES_SHAPE] = func_0204bde0(arc, 87, 103, HEAPID_ZUKAN_SEARCH);
    wk->charRes[CHAR_RES_SHAPE_SUB] = func_0204b81c(arc, 84, FALSE, CLACT_VRAM_SUB, HEAPID_ZUKAN_SEARCH);
    wk->paletteRes[CHAR_RES_SHAPE_SUB] = func_0204bba0(arc, 78, CLACT_VRAM_SUB, 3 * 0x20, HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    wk->charRes[CHAR_RES_TOUCHBAR] =
        func_0204b81c(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_MAIN, HEAPID_ZUKAN_SEARCH);
    wk->paletteRes[CHAR_RES_TOUCHBAR] =
        func_0204bba0(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_MAIN, 5 * 0x20, HEAPID_ZUKAN_SEARCH);
    wk->cellRes[CELL_RES_TOUCHBAR] = func_0204bde0(arc, AppMenuCommon_GetTouchBarCellFile(2),
                                                   AppMenuCommon_GetTouchBarAnimFile(2), HEAPID_ZUKAN_SEARCH);
    GFL_ArcToolFree(arc);
}

static void ZknSearchObj_FreeResources(ZknSearchWork *wk) {
    u32 i;

    for (i = 0; i < ZKNSEARCH_CHAR_RES_COUNT; i++) {
        if (wk->charRes[i] != -1) {
            func_0204b98c(wk->charRes[i]);
        }
    }
    for (i = 0; i < ZKNSEARCH_PALETTE_RES_COUNT; i++) {
        if (wk->paletteRes[i] != -1) {
            func_0204bcd0(wk->paletteRes[i]);
        }
    }
    for (i = 0; i < ZKNSEARCH_CELL_RES_COUNT; i++) {
        if (wk->cellRes[i] != -1) {
            func_0204be64(wk->cellRes[i]);
        }
    }
}

static void ZknSearchObj_CreateActors(ZknSearchWork *wk) {
    u32 i;

    wk->actorUnit = func_0204bf1c(ZKNSEARCH_OBJ_COUNT, 0, HEAPID_ZUKAN_SEARCH);
    for (i = 0; i < ZKNSEARCH_OBJ_COUNT; i++) {
        wk->actors[i] = NULL;
    }
    for (i = ZKNSEARCH_OBJ_SCROLL_BAR; i <= ZKNSEARCH_OBJ_MARK_WINDOW + 1; i++) {
        ZukanActorTemplate template = sZknSearchActorTemplates[i];

        template.chars = wk->charRes[template.chars];
        template.palette = wk->paletteRes[template.palette];
        template.cells = wk->cellRes[template.cells];
        wk->actors[i] = ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_SEARCH);
    }
    if (GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_POKEDEX_SEARCH) == TRUE) {
        ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_SHORTCUT, 7);
    }
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_DOWN, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_UP, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_BAR, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_RAIL, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_LOADING_BAR, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW + 1, FALSE);
    func_0204c318(wk->actors[ZKNSEARCH_OBJ_MARK_WINDOW], OAM_MODE_OBJWND);
    func_0204c318(wk->actors[ZKNSEARCH_OBJ_MARK_WINDOW + 1], OAM_MODE_OBJWND);
    ZknSearchObj_SetBlend(wk, ZKNSEARCH_OBJ_MARK_SUB, TRUE);
    ZknSearchObj_SetBlend(wk, ZKNSEARCH_OBJ_MARK_SUB + 1, TRUE);

    {
        ZukanActorTemplate template;
        ZukanActorTemplate shape = { { 0, 0, 0, 0, 1 }, CHAR_RES_SHAPE, CHAR_RES_SHAPE, CELL_RES_SHAPE, 0,
                                     CLACT_SURFACE_MAIN };
        ZukanActorTemplate shapeSub = { { 0, 0, 0, 0, 2 }, CHAR_RES_SHAPE_SUB, CHAR_RES_SHAPE_SUB, CELL_RES_SHAPE, 0,
                                        CLACT_SURFACE_SUB };

        for (i = 0; i < ZKNSEARCH_SHAPE_COUNT; i++) {
            template = shape;
            template.chars = wk->charRes[template.chars];
            template.palette = wk->paletteRes[template.palette];
            template.cells = wk->cellRes[template.cells];
            template.setup.sequence = i;
            wk->actors[ZKNSEARCH_OBJ_SHAPE_ICON + i] =
                ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_SEARCH);
            ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON + i, FALSE);

            template = shapeSub;
            template.chars = wk->charRes[template.chars];
            template.palette = wk->paletteRes[template.palette];
            template.cells = wk->cellRes[template.cells];
            template.setup.sequence = i;
            wk->actors[ZKNSEARCH_OBJ_SHAPE_ICON_SUB + i] =
                ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_SEARCH);
            ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_SUB + i, FALSE);
            ZknSearchObj_SetBlend(wk, ZKNSEARCH_OBJ_SHAPE_ICON_SUB + i, TRUE);

            wk->actors[ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + i] =
                ZukanCommon_CreateActor(wk->actorUnit, &template, HEAPID_ZUKAN_SEARCH);
            ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + i, FALSE);
            func_0204c318(wk->actors[ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + i], OAM_MODE_OBJWND);
        }
    }
}

static void ZknSearchObj_DeleteActor(ZknSearchWork *wk, u32 id) {
    if (wk->actors[id] != NULL) {
        func_0204c108(wk->actors[id]);
        wk->actors[id] = NULL;
    }
}

static void ZknSearchObj_DeleteActors(ZknSearchWork *wk) {
    u32 i;

    for (i = 0; i < ZKNSEARCH_OBJ_COUNT; i++) {
        ZknSearchObj_DeleteActor(wk, i);
    }
    func_0204bf98(wk->actorUnit);
}

void ZknSearchObj_HideList(ZknSearchWork *wk) {
    u32 i;

    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_BAR, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_RAIL, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW + 1, FALSE);
    for (i = 0; i < ZKNSEARCH_SHAPE_COUNT; i++) {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON + i, FALSE);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_SUB + i, FALSE);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + i, FALSE);
    }
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE, FALSE);
}

void ZknSearchObj_SetMain(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_UP, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_DOWN, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_RETURN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_EXIT, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHORTCUT, TRUE);
    if (wk->param->criteria->shape != ZKNSEARCH_NONE) {
        ZknSearchObj_SetAnmSeq(wk, ZKNSEARCH_OBJ_SHAPE, wk->param->criteria->shape);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SHAPE, 192, 116, CLACT_SURFACE_MAIN);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE, TRUE);
    }
}

void ZknSearchObj_SetOrder(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_EXIT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHORTCUT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_RETURN, TRUE);
}

void ZknSearchObj_SetInitial(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_EXIT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHORTCUT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_RETURN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_DOWN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_UP, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_BAR, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_RAIL, TRUE);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
}

void ZknSearchObj_SetType(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_EXIT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHORTCUT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_RETURN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_DOWN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_UP, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_BAR, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_RAIL, TRUE);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
}

void ZknSearchObj_SetColor(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_EXIT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHORTCUT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_RETURN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_DOWN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_UP, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_BAR, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_RAIL, TRUE);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
}

void ZknSearchObj_SetShape(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_EXIT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHORTCUT, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_RETURN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_DOWN, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_PAGE_UP, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_BAR, TRUE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SCROLL_RAIL, TRUE);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SCROLL_BAR, 252, 12, CLACT_SURFACE_MAIN);
    ZknSearchObj_PutShape(wk);
}

void ZknSearchObj_PutShape(ZknSearchWork *wk) {
    if (wk->param->criteria->shape != ZKNSEARCH_NONE) {
        ZknSearchObj_SetAnmSeq(wk, ZKNSEARCH_OBJ_SHAPE, wk->param->criteria->shape);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SHAPE, 64, 104, CLACT_SURFACE_MAIN);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE, TRUE);
    } else {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE, FALSE);
    }
}

void ZknSearchObj_PutShapeIcons(ZknSearchWork *wk) {
    s16 scroll = FrameList_GetScroll(wk->frameList);
    s16 i;

    for (i = 0; i < ZKNSEARCH_SHAPE_COUNT; i++) {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON + i, FALSE);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_SUB + i, FALSE);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + i, FALSE);
    }
    for (i = 0; i < 5; i++) {
        ZknSearchObj_PutShapeIcon(wk, scroll + i, i * SHAPE_ROW_HEIGHT, TRUE);
        if (scroll - (i + 1) >= 0) {
            ZknSearchObj_PutShapeIcon(wk, scroll - (i + 1), TOP_LIST_HEIGHT - (i + 1) * SHAPE_ROW_HEIGHT, FALSE);
        }
    }
}

void ZknSearchObj_UpdateScrollBar(ZknSearchWork *wk) {
    s16 x, y;
    u32 pos;

    ZknSearchObj_GetPos(wk, ZKNSEARCH_OBJ_SCROLL_BAR, &x, &y, CLACT_SURFACE_MAIN);
    pos = FrameList_ClampBarPos(wk->frameList, y);
    if (pos < 12) {
        pos = 12;
    } else if (pos > 156) {
        pos = 156;
    }
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SCROLL_BAR, 252, pos, CLACT_SURFACE_MAIN);
}

void ZknSearchObj_SetPageButtons(ZknSearchWork *wk, BOOL keep) {
    s16 cursor = FrameList_GetCursor(wk->frameList);

    if (ZknSearchObj_GetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP) != 12 || keep == FALSE) {
        if (FrameList_GetScroll(wk->frameList) == 0 && cursor == 0) {
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 18);
        } else {
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_UP, 4);
        }
    }
    if (ZknSearchObj_GetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN) != 13 || keep == FALSE) {
        if (FrameList_CanScrollDown(wk->frameList) == FALSE && cursor == wk->listRows - 1) {
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 19);
        } else {
            ZknSearchObj_SetAnm(wk, ZKNSEARCH_OBJ_PAGE_DOWN, 5);
        }
    }
}

void ZknSearchObj_PutMark(ZknSearchWork *wk, int index, s16 y, BOOL touchScreen) {
    if (touchScreen == TRUE) {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK + index, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK + index, MARK_X, y + ROW_HEIGHT / 2, CLACT_SURFACE_MAIN);
    } else {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB + index, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_SUB + index, MARK_X, y + ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW + index, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_WINDOW + index, MARK_X, y + ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
    }
}

void ZknSearchObj_HideMarks(ZknSearchWork *wk) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW + 1, FALSE);
}

void ZknSearchObj_PutRowMark(ZknSearchWork *wk, u16 row, BOOL visible) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, visible);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, FALSE);
    if (visible == TRUE) {
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK, MARK_X, row * ROW_HEIGHT + ROW_HEIGHT / 2, CLACT_SURFACE_MAIN);
    }
}

// The marks of both types: on the top screen's rows above the list's scroll, or on the touch screen's
void ZknSearchObj_PutTypeMarks(ZknSearchWork *wk, u8 type1, u8 type2) {
    u8 scroll = FrameList_GetScroll(wk->frameList);
    u8 diff;

    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB + 1, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW + 1, FALSE);
    if (type1 != ZKNSEARCH_NONE) {
        type1 = ZknSearchMain_TypeToListPos(wk, type1);
        diff = MATH_ABS(type1 - scroll);
        if (type1 < scroll) {
            if (diff <= 8) {
                ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, TRUE);
                ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_SUB, MARK_X, (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2,
                                    CLACT_SURFACE_SUB);
                ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, TRUE);
                ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_WINDOW, MARK_X, (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2,
                                    CLACT_SURFACE_SUB);
            }
        } else if (diff <= 7) {
            ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, TRUE);
            ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK, MARK_X, diff * ROW_HEIGHT + ROW_HEIGHT / 2, CLACT_SURFACE_MAIN);
        }
    }
    if (type2 != ZKNSEARCH_NONE) {
        type2 = ZknSearchMain_TypeToListPos(wk, type2);
        diff = MATH_ABS(type2 - scroll);
        if (type2 < scroll) {
            if (diff <= 8) {
                ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB + 1, TRUE);
                ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_SUB + 1, MARK_X, (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2,
                                    CLACT_SURFACE_SUB);
                ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW + 1, TRUE);
                ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_WINDOW + 1, MARK_X, (8 - diff) * ROW_HEIGHT + ROW_HEIGHT / 2,
                                    CLACT_SURFACE_SUB);
            }
        } else if (diff <= 7) {
            ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK + 1, TRUE);
            ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK + 1, MARK_X, diff * ROW_HEIGHT + ROW_HEIGHT / 2,
                                CLACT_SURFACE_MAIN);
        }
    }
}

void ZknSearchObj_PutShapeMark(ZknSearchWork *wk, s16 y, BOOL touchScreen) {
    if (touchScreen == TRUE) {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK, MARK_X, y + SHAPE_ROW_HEIGHT / 2, CLACT_SURFACE_MAIN);
    } else {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_SUB, MARK_X, y + SHAPE_ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_WINDOW, MARK_X, y + SHAPE_ROW_HEIGHT / 2, CLACT_SURFACE_SUB);
    }
}

void ZknSearchObj_PutShapeRowMark(ZknSearchWork *wk, u16 row, BOOL visible) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, visible);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, FALSE);
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, FALSE);
    if (visible == TRUE) {
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK, MARK_X, row * SHAPE_ROW_HEIGHT + SHAPE_ROW_HEIGHT / 2,
                            CLACT_SURFACE_MAIN);
    }
}

void ZknSearchObj_PutItemMark(ZknSearchWork *wk, u32 index, u16 item) {
    u32 scroll = FrameList_GetScroll(wk->frameList);
    s16 top = scroll - 7;
    s16 bottom = scroll + 7;

    if (top < 0) {
        top = 0;
    }
    if (item >= top && item < scroll) {
        ZknSearchObj_PutMark(wk, index, TOP_LIST_HEIGHT - (scroll - item) * ROW_HEIGHT, FALSE);
    } else if (item >= scroll && item < bottom) {
        ZknSearchObj_PutMark(wk, index, (item - scroll) * ROW_HEIGHT, TRUE);
    }
}

void ZknSearchObj_PutShapeItemMark(ZknSearchWork *wk) {
    u32 scroll = FrameList_GetScroll(wk->frameList);
    u8 shape = wk->param->criteria->shape;
    s16 top = shape - 4;
    s16 bottom = shape + 4;

    if (top < 0) {
        top = 0;
    }
    if (shape >= top && shape < scroll) {
        ZknSearchObj_PutShapeMark(wk, TOP_LIST_HEIGHT - (scroll - shape) * SHAPE_ROW_HEIGHT, FALSE);
    } else if (shape >= scroll && shape < bottom) {
        ZknSearchObj_PutShapeMark(wk, (shape - scroll) * SHAPE_ROW_HEIGHT, TRUE);
    }
}

void ZknSearchObj_PutShapeIcon(ZknSearchWork *wk, u16 shape, s16 y, BOOL touchScreen) {
    if (touchScreen == TRUE) {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON + shape, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SHAPE_ICON + shape, SHAPE_X, y + SHAPE_ROW_HEIGHT / 2,
                            CLACT_SURFACE_MAIN);
    } else {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_SUB + shape, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SHAPE_ICON_SUB + shape, SHAPE_X, y + SHAPE_ROW_HEIGHT / 2,
                            CLACT_SURFACE_SUB);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + shape, TRUE);
        ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW + shape, SHAPE_X, y + SHAPE_ROW_HEIGHT / 2,
                            CLACT_SURFACE_SUB);
    }
}

void ZknSearchObj_ScrollMarks(ZknSearchWork *wk, s8 delta) {
    u32 i;
    s16 x, y;

    for (i = ZKNSEARCH_OBJ_MARK; i <= ZKNSEARCH_OBJ_MARK + 1; i++) {
        if (ZknSearchObj_IsVisible(wk, i)) {
            ZknSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            ZknSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_MAIN);
            if (y + delta <= -ROW_HEIGHT / 2 || y + delta >= TOP_LIST_HEIGHT + ROW_HEIGHT / 2) {
                ZknSearchObj_SetVisible(wk, i, FALSE);
            }
        }
    }
    for (i = ZKNSEARCH_OBJ_MARK_SUB; i <= ZKNSEARCH_OBJ_MARK_SUB + 1; i++) {
        if (ZknSearchObj_IsVisible(wk, i)) {
            ZknSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_SUB);
            ZknSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_SUB);
            ZknSearchObj_SetPos(wk, i + 2, x, y + delta, CLACT_SURFACE_SUB);
            if (y + delta <= -ROW_HEIGHT / 2 || y + delta >= TOP_LIST_HEIGHT + ROW_HEIGHT / 2) {
                ZknSearchObj_SetVisible(wk, i, FALSE);
                ZknSearchObj_SetVisible(wk, i + 2, FALSE);
            }
        }
    }
}

void ZknSearchObj_ScrollShapeIcons(ZknSearchWork *wk, s8 delta) {
    u32 i;
    s16 x, y;

    for (i = ZKNSEARCH_OBJ_SHAPE_ICON; i < ZKNSEARCH_OBJ_SHAPE_ICON_SUB; i++) {
        if (ZknSearchObj_IsVisible(wk, i)) {
            ZknSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_MAIN);
            ZknSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_MAIN);
            if (y + delta <= -SHAPE_ROW_HEIGHT / 2 || y + delta >= 220) {
                ZknSearchObj_SetVisible(wk, i, FALSE);
            }
        }
    }
    ZknSearchObj_GetPos(wk, ZKNSEARCH_OBJ_MARK, &x, &y, CLACT_SURFACE_MAIN);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK, x, y + delta, CLACT_SURFACE_MAIN);
    if (y + delta <= -SHAPE_ROW_HEIGHT / 2 || y + delta >= 220) {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK, FALSE);
    }

    for (i = ZKNSEARCH_OBJ_SHAPE_ICON_SUB; i < ZKNSEARCH_OBJ_SHAPE_ICON_WINDOW; i++) {
        if (ZknSearchObj_IsVisible(wk, i)) {
            ZknSearchObj_GetPos(wk, i, &x, &y, CLACT_SURFACE_SUB);
            ZknSearchObj_SetPos(wk, i, x, y + delta, CLACT_SURFACE_SUB);
            ZknSearchObj_SetPos(wk, i + ZKNSEARCH_SHAPE_COUNT, x, y + delta, CLACT_SURFACE_SUB);
            if (y + delta <= -28 || y + delta >= 212) {
                ZknSearchObj_SetVisible(wk, i, FALSE);
                ZknSearchObj_SetVisible(wk, i + ZKNSEARCH_SHAPE_COUNT, FALSE);
            }
        }
    }
    ZknSearchObj_GetPos(wk, ZKNSEARCH_OBJ_MARK_SUB, &x, &y, CLACT_SURFACE_SUB);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_SUB, x, y + delta, CLACT_SURFACE_SUB);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_MARK_WINDOW, x, y + delta, CLACT_SURFACE_SUB);
    if (y + delta <= -28 || y + delta >= 212) {
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_SUB, FALSE);
        ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_MARK_WINDOW, FALSE);
    }
}

void ZknSearchObj_MoveLoadingBar(ZknSearchWork *wk, u32 count) {
    ZknSearchObj_SetVisible(wk, ZKNSEARCH_OBJ_LOADING_BAR, TRUE);
    ZknSearchObj_SetPos(wk, ZKNSEARCH_OBJ_LOADING_BAR, count * 2 + 8, 168, CLACT_SURFACE_MAIN);
}
