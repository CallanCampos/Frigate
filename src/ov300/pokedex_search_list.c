#include "types.h"
#include "app/pokedex_search_bmp.h"
#include "app/pokedex_search_list.h"
#include "app/pokedex_search_main.h"
#include "app/pokedex_search_obj.h"
#include "app/ui/frame_list.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "system/printsys.h"
#include "text/system/zknsearch_main.h"

// The lists of the criteria of the Pokédex's search screen: the order, the initial, the type, the color and the shape.
// The ROM doesn't name this file; pokedex_search_list.c is a guess after pokedex_search_main.c

#define INITIAL_COUNT 26
#define COLOR_COUNT 10
#define ORDER_COUNT 6

static void PokedexSearchList_OrderPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PokedexSearchList_OrderSelect(void *work, u32 index, BOOL moved);
static void PokedexSearchList_OrderScroll(void *work, s16 delta);
static void PokedexSearchList_InitialPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PokedexSearchList_InitialSelect(void *work, u32 index, BOOL moved);
static void PokedexSearchList_InitialScroll(void *work, s16 delta);
static void PokedexSearchList_TypePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PokedexSearchList_TypeSelect(void *work, u32 index, BOOL moved);
static void PokedexSearchList_TypeScroll(void *work, s16 delta);
static void PokedexSearchList_ColorPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PokedexSearchList_ColorSelect(void *work, u32 index, BOOL moved);
static void PokedexSearchList_ColorScroll(void *work, s16 delta);
static void PokedexSearchList_ShapePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void PokedexSearchList_ShapeSelect(void *work, u32 index, BOOL moved);
static void PokedexSearchList_ShapeScroll(void *work, s16 delta);

static const FrameListTouch sPokedexSearchOrderTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },   { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },  { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE }, { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sPokedexSearchOrderCallbacks = {
    PokedexSearchList_OrderPrint,
    PokedexSearchList_OrderSelect,
    PokedexSearchList_OrderScroll,
};

static const FrameListSetup sPokedexSearchOrderSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    ORDER_COUNT,
    1,
    0,
    6,
    0,
    sPokedexSearchOrderTouch,
    &sPokedexSearchOrderCallbacks,
    NULL,
};

static const FrameListTouch sPokedexSearchInitialTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM },        { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 152, 231 }, FRAMELIST_TOUCH_ITEM },      { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },   { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sPokedexSearchInitialCallbacks = {
    PokedexSearchList_InitialPrint,
    PokedexSearchList_InitialSelect,
    PokedexSearchList_InitialScroll,
};

static const FrameListSetup sPokedexSearchInitialSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    POKEDEX_SEARCH_ITEM_NAME_COUNT,
    1,
    0,
    7,
    0,
    sPokedexSearchInitialTouch,
    &sPokedexSearchInitialCallbacks,
    NULL,
};

static const FrameListTouch sPokedexSearchTypeTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM },        { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 152, 231 }, FRAMELIST_TOUCH_ITEM },      { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },   { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sPokedexSearchTypeCallbacks = {
    PokedexSearchList_TypePrint,
    PokedexSearchList_TypeSelect,
    PokedexSearchList_TypeScroll,
};

static const FrameListSetup sPokedexSearchTypeSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    POKEDEX_SEARCH_ITEM_NAME_COUNT,
    1,
    0,
    7,
    0,
    sPokedexSearchTypeTouch,
    &sPokedexSearchTypeCallbacks,
    NULL,
};

static const FrameListTouch sPokedexSearchColorTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM },        { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 152, 231 }, FRAMELIST_TOUCH_ITEM },      { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },   { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sPokedexSearchColorCallbacks = {
    PokedexSearchList_ColorPrint,
    PokedexSearchList_ColorSelect,
    PokedexSearchList_ColorScroll,
};

static const FrameListSetup sPokedexSearchColorSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    POKEDEX_SEARCH_ITEM_NAME_COUNT,
    1,
    0,
    7,
    0,
    sPokedexSearchColorTouch,
    &sPokedexSearchColorCallbacks,
    NULL,
};

static const FrameListTouch sPokedexSearchShapeTouch[] = {
    { { 0, 39, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 40, 79, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 80, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 159, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },         { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN }, { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sPokedexSearchShapeCallbacks = {
    PokedexSearchList_ShapePrint,
    PokedexSearchList_ShapeSelect,
    PokedexSearchList_ShapeScroll,
};

static const FrameListSetup sPokedexSearchShapeSetup = {
    { 1, 5, 16, 0, 16, 5, 4, 1, 1, 1, 1, { 40, 20, 10, 8, 5, 4 }, 3, 8, 0 },
    POKEDEX_SEARCH_SHAPE_COUNT,
    1,
    0,
    4,
    0,
    sPokedexSearchShapeTouch,
    &sPokedexSearchShapeCallbacks,
    NULL,
};

void PokedexSearchList_InitOrder(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sPokedexSearchOrderSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_POKEDEX_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i < ORDER_COUNT; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
        wk->itemNames[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, ZknsearchMain_Text_Number + i);
    }
}

void PokedexSearchList_InitInitial(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sPokedexSearchInitialSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_POKEDEX_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i < INITIAL_COUNT; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
        wk->itemNames[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, ZknsearchMain_Text_A + i);
    }
}

void PokedexSearchList_InitType(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sPokedexSearchTypeSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_POKEDEX_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i <= POKEDEX_SEARCH_TYPE_COUNT - 1; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
        wk->itemNames[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexSearchMain_GetListPosTypeMsg(wk, i));
    }
}

void PokedexSearchList_InitColor(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sPokedexSearchColorSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_POKEDEX_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = ZknsearchMain_Text_Red; i <= ZknsearchMain_Text_Pink; i++) {
        FrameList_AddItem(wk->frameList, 0, i - ZknsearchMain_Text_Red);
        wk->itemNames[i - ZknsearchMain_Text_Red] = GFL_MsgDataLoadStrbufNew(wk->msgData, i);
    }
}

void PokedexSearchList_InitShape(PokedexSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_POKEDEX_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sPokedexSearchShapeSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_POKEDEX_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 100, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i < POKEDEX_SEARCH_SHAPE_COUNT; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
    }
}

void PokedexSearchList_Exit(PokedexSearchWork *wk) {
    u32 i;

    FrameList_Free(wk->frameList);
    wk->frameList = NULL;
    for (i = 0; i < POKEDEX_SEARCH_ITEM_NAME_COUNT; i++) {
        if (wk->itemNames[i] != NULL) {
            GFL_StrBufFree(wk->itemNames[i]);
            wk->itemNames[i] = NULL;
        }
    }
}

static void PokedexSearchList_OrderPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PokedexSearchWork *wk = work;

    PokedexSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (wk->param->criteria->order == index) {
        PokedexSearchObj_PutMark(wk, 0, y, firstBG);
    }
}

static void PokedexSearchList_OrderSelect(void *work, u32 index, BOOL moved) {
}

static void PokedexSearchList_OrderScroll(void *work, s16 delta) {
}

static void PokedexSearchList_InitialPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PokedexSearchWork *wk = work;

    PokedexSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (wk->param->criteria->initial == index) {
        PokedexSearchObj_PutMark(wk, 0, y, firstBG);
    }
}

static void PokedexSearchList_InitialSelect(void *work, u32 index, BOOL moved) {
}

static void PokedexSearchList_InitialScroll(void *work, s16 delta) {
    PokedexSearchWork *wk = work;

    PokedexSearchObj_ScrollMarks(wk, -delta);
    PokedexSearchObj_SetPageButtons(wk, TRUE);
}

static void PokedexSearchList_TypePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PokedexSearchWork *wk = work;

    PokedexSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (index == PokedexSearchMain_TypeToListPos(wk, wk->param->criteria->type1)) {
        PokedexSearchObj_PutMark(wk, 0, y, firstBG);
    }
    if (index == PokedexSearchMain_TypeToListPos(wk, wk->param->criteria->type2)) {
        PokedexSearchObj_PutMark(wk, 1, y, firstBG);
    }
}

static void PokedexSearchList_TypeSelect(void *work, u32 index, BOOL moved) {
}

static void PokedexSearchList_TypeScroll(void *work, s16 delta) {
    PokedexSearchWork *wk = work;

    PokedexSearchObj_ScrollMarks(wk, -delta);
    PokedexSearchObj_SetPageButtons(wk, TRUE);
}

static void PokedexSearchList_ColorPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PokedexSearchWork *wk = work;

    PokedexSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (wk->param->criteria->color == index) {
        PokedexSearchObj_PutMark(wk, 0, y, firstBG);
    }
}

static void PokedexSearchList_ColorSelect(void *work, u32 index, BOOL moved) {
}

static void PokedexSearchList_ColorScroll(void *work, s16 delta) {
    PokedexSearchWork *wk = work;

    PokedexSearchObj_ScrollMarks(wk, -delta);
    PokedexSearchObj_SetPageButtons(wk, TRUE);
}

static void PokedexSearchList_ShapePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    PokedexSearchWork *wk = work;

    PokedexSearchBmp_FlushItem(wk, window);
    if (wk->param->criteria->shape == index) {
        PokedexSearchObj_PutShapeMark(wk, y, firstBG);
    }
    PokedexSearchObj_PutShapeIcon(wk, index, y, firstBG);
}

static void PokedexSearchList_ShapeSelect(void *work, u32 index, BOOL moved) {
}

static void PokedexSearchList_ShapeScroll(void *work, s16 delta) {
    PokedexSearchWork *wk = work;

    PokedexSearchObj_ScrollShapeIcons(wk, -delta);
    PokedexSearchObj_SetPageButtons(wk, TRUE);
}
