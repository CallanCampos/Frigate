#include "types.h"
#include "app/ui/frame_list.h"
#include "app/zknsearch_bmp.h"
#include "app/zknsearch_list.h"
#include "app/zknsearch_main.h"
#include "app/zknsearch_obj.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "system/printsys.h"
#include "text/system/zknsearch_main.h"

// The lists of the criteria of the Pokédex's search screen: the order, the initial, the type, the color and the shape.
// The ROM doesn't name this file; zknsearch_list.c is a guess after zknsearch_main.c

#define INITIAL_COUNT 26
#define COLOR_COUNT 10
#define ORDER_COUNT 6

static void ZknSearchList_OrderPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void ZknSearchList_OrderSelect(void *work, u32 index, BOOL moved);
static void ZknSearchList_OrderScroll(void *work, s16 delta);
static void ZknSearchList_InitialPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void ZknSearchList_InitialSelect(void *work, u32 index, BOOL moved);
static void ZknSearchList_InitialScroll(void *work, s16 delta);
static void ZknSearchList_TypePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void ZknSearchList_TypeSelect(void *work, u32 index, BOOL moved);
static void ZknSearchList_TypeScroll(void *work, s16 delta);
static void ZknSearchList_ColorPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void ZknSearchList_ColorSelect(void *work, u32 index, BOOL moved);
static void ZknSearchList_ColorScroll(void *work, s16 delta);
static void ZknSearchList_ShapePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void ZknSearchList_ShapeSelect(void *work, u32 index, BOOL moved);
static void ZknSearchList_ShapeScroll(void *work, s16 delta);

static const FrameListTouch sZknSearchOrderTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },   { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },  { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE }, { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sZknSearchOrderCallbacks = {
    ZknSearchList_OrderPrint,
    ZknSearchList_OrderSelect,
    ZknSearchList_OrderScroll,
};

static const FrameListSetup sZknSearchOrderSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    ORDER_COUNT,
    1,
    0,
    6,
    0,
    sZknSearchOrderTouch,
    &sZknSearchOrderCallbacks,
    NULL,
};

static const FrameListTouch sZknSearchInitialTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM },        { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 152, 231 }, FRAMELIST_TOUCH_ITEM },      { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },   { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sZknSearchInitialCallbacks = {
    ZknSearchList_InitialPrint,
    ZknSearchList_InitialSelect,
    ZknSearchList_InitialScroll,
};

static const FrameListSetup sZknSearchInitialSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    ZKNSEARCH_ITEM_NAME_COUNT,
    1,
    0,
    7,
    0,
    sZknSearchInitialTouch,
    &sZknSearchInitialCallbacks,
    NULL,
};

static const FrameListTouch sZknSearchTypeTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM },        { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 152, 231 }, FRAMELIST_TOUCH_ITEM },      { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },   { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sZknSearchTypeCallbacks = {
    ZknSearchList_TypePrint,
    ZknSearchList_TypeSelect,
    ZknSearchList_TypeScroll,
};

static const FrameListSetup sZknSearchTypeSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    ZKNSEARCH_ITEM_NAME_COUNT,
    1,
    0,
    7,
    0,
    sZknSearchTypeTouch,
    &sZknSearchTypeCallbacks,
    NULL,
};

static const FrameListTouch sZknSearchColorTouch[] = {
    { { 0, 23, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 24, 47, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 48, 71, 152, 231 }, FRAMELIST_TOUCH_ITEM },        { { 72, 95, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 96, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 143, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 144, 167, 152, 231 }, FRAMELIST_TOUCH_ITEM },      { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },
    { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },   { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN },
    { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sZknSearchColorCallbacks = {
    ZknSearchList_ColorPrint,
    ZknSearchList_ColorSelect,
    ZknSearchList_ColorScroll,
};

static const FrameListSetup sZknSearchColorSetup = {
    { 1, 5, 16, 0, 16, 3, 3, 0, 11, 3, 1, { 24, 12, 8, 6, 4, 3 }, 3, 8, 0 },
    ZKNSEARCH_ITEM_NAME_COUNT,
    1,
    0,
    7,
    0,
    sZknSearchColorTouch,
    &sZknSearchColorCallbacks,
    NULL,
};

static const FrameListTouch sZknSearchShapeTouch[] = {
    { { 0, 39, 152, 231 }, FRAMELIST_TOUCH_ITEM },         { { 40, 79, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 80, 119, 152, 231 }, FRAMELIST_TOUCH_ITEM },       { { 120, 159, 152, 231 }, FRAMELIST_TOUCH_ITEM },
    { { 8, 160, 232, 255 }, FRAMELIST_TOUCH_BAR },         { { 168, 191, 184, 207 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 168, 191, 208, 231 }, FRAMELIST_TOUCH_PAGE_DOWN }, { { TOUCH_RECT_END, 0, 0, 0 }, FRAMELIST_TOUCH_ITEM },
};

static const FrameListCallbacks sZknSearchShapeCallbacks = {
    ZknSearchList_ShapePrint,
    ZknSearchList_ShapeSelect,
    ZknSearchList_ShapeScroll,
};

static const FrameListSetup sZknSearchShapeSetup = {
    { 1, 5, 16, 0, 16, 5, 4, 1, 1, 1, 1, { 40, 20, 10, 8, 5, 4 }, 3, 8, 0 },
    ZKNSEARCH_SHAPE_COUNT,
    1,
    0,
    4,
    0,
    sZknSearchShapeTouch,
    &sZknSearchShapeCallbacks,
    NULL,
};

void ZknSearchList_InitOrder(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sZknSearchOrderSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_ZUKAN_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i < ORDER_COUNT; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
        wk->itemNames[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, ZknsearchMain_Text_Number + i);
    }
}

void ZknSearchList_InitInitial(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sZknSearchInitialSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_ZUKAN_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i < INITIAL_COUNT; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
        wk->itemNames[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, ZknsearchMain_Text_A + i);
    }
}

void ZknSearchList_InitType(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sZknSearchTypeSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_ZUKAN_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i <= ZKNSEARCH_TYPE_COUNT - 1; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
        wk->itemNames[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, ZknSearchMain_GetListPosTypeMsg(wk, i));
    }
}

void ZknSearchList_InitColor(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sZknSearchColorSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_ZUKAN_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 99, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = ZknsearchMain_Text_Red; i <= ZknsearchMain_Text_Pink; i++) {
        FrameList_AddItem(wk->frameList, 0, i - ZknsearchMain_Text_Red);
        wk->itemNames[i - ZknsearchMain_Text_Red] = GFL_MsgDataLoadStrbufNew(wk->msgData, i);
    }
}

void ZknSearchList_InitShape(ZknSearchWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_SEARCH | HEAPID_TAIL_BIT);
    FrameListSetup setup = sZknSearchShapeSetup;
    u32 i;

    setup.cursorPos = 0;
    setup.scroll = 0;
    setup.work = wk;
    wk->frameList = FrameList_Create(&setup, HEAPID_ZUKAN_SEARCH);
    wk->listRows = setup.visibleRows;
    FrameList_LoadScreen(wk->frameList, arc, 100, FALSE, 0);
    FrameList_LoadCursorPalette(wk->frameList, arc, 75, 2, 3);
    GFL_ArcToolFree(arc);
    for (i = 0; i < ZKNSEARCH_SHAPE_COUNT; i++) {
        FrameList_AddItem(wk->frameList, 0, i);
    }
}

void ZknSearchList_Exit(ZknSearchWork *wk) {
    u32 i;

    FrameList_Free(wk->frameList);
    wk->frameList = NULL;
    for (i = 0; i < ZKNSEARCH_ITEM_NAME_COUNT; i++) {
        if (wk->itemNames[i] != NULL) {
            GFL_StrBufFree(wk->itemNames[i]);
            wk->itemNames[i] = NULL;
        }
    }
}

static void ZknSearchList_OrderPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    ZknSearchWork *wk = work;

    ZknSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (wk->param->criteria->order == index) {
        ZknSearchObj_PutMark(wk, 0, y, firstBG);
    }
}

static void ZknSearchList_OrderSelect(void *work, u32 index, BOOL moved) {
}

static void ZknSearchList_OrderScroll(void *work, s16 delta) {
}

static void ZknSearchList_InitialPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    ZknSearchWork *wk = work;

    ZknSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (wk->param->criteria->initial == index) {
        ZknSearchObj_PutMark(wk, 0, y, firstBG);
    }
}

static void ZknSearchList_InitialSelect(void *work, u32 index, BOOL moved) {
}

static void ZknSearchList_InitialScroll(void *work, s16 delta) {
    ZknSearchWork *wk = work;

    ZknSearchObj_ScrollMarks(wk, -delta);
    ZknSearchObj_SetPageButtons(wk, TRUE);
}

static void ZknSearchList_TypePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    ZknSearchWork *wk = work;

    ZknSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (index == ZknSearchMain_TypeToListPos(wk, wk->param->criteria->type1)) {
        ZknSearchObj_PutMark(wk, 0, y, firstBG);
    }
    if (index == ZknSearchMain_TypeToListPos(wk, wk->param->criteria->type2)) {
        ZknSearchObj_PutMark(wk, 1, y, firstBG);
    }
}

static void ZknSearchList_TypeSelect(void *work, u32 index, BOOL moved) {
}

static void ZknSearchList_TypeScroll(void *work, s16 delta) {
    ZknSearchWork *wk = work;

    ZknSearchObj_ScrollMarks(wk, -delta);
    ZknSearchObj_SetPageButtons(wk, TRUE);
}

static void ZknSearchList_ColorPrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    ZknSearchWork *wk = work;

    ZknSearchBmp_PrintItem(wk, window, wk->itemNames[index]);
    if (wk->param->criteria->color == index) {
        ZknSearchObj_PutMark(wk, 0, y, firstBG);
    }
}

static void ZknSearchList_ColorSelect(void *work, u32 index, BOOL moved) {
}

static void ZknSearchList_ColorScroll(void *work, s16 delta) {
    ZknSearchWork *wk = work;

    ZknSearchObj_ScrollMarks(wk, -delta);
    ZknSearchObj_SetPageButtons(wk, TRUE);
}

static void ZknSearchList_ShapePrint(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    ZknSearchWork *wk = work;

    ZknSearchBmp_FlushItem(wk, window);
    if (wk->param->criteria->shape == index) {
        ZknSearchObj_PutShapeMark(wk, y, firstBG);
    }
    ZknSearchObj_PutShapeIcon(wk, index, y, firstBG);
}

static void ZknSearchList_ShapeSelect(void *work, u32 index, BOOL moved) {
}

static void ZknSearchList_ShapeScroll(void *work, s16 delta) {
    ZknSearchWork *wk = work;

    ZknSearchObj_ScrollShapeIcons(wk, -delta);
    ZknSearchObj_SetPageButtons(wk, TRUE);
}
