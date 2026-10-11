#ifndef POKEBW2_APP_UI_FRAME_LIST_H
#define POKEBW2_APP_UI_FRAME_LIST_H

#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/touchpanel.h"
#include "struct_decls.h"
#include "system/printsys.h"

// frame_list.c: A list of items that scrolls, with a scroll bar and arrows
typedef struct FrameList FrameList;

// What FrameList_Main returns besides the position of the item chosen
#define FRAMELIST_NONE (-1)

// What an entry of a list's touch table does when it is touched (FrameListTouch's action)
#define FRAMELIST_TOUCH_ITEM 0
#define FRAMELIST_TOUCH_BAR 1
#define FRAMELIST_TOUCH_UP 2
#define FRAMELIST_TOUCH_DOWN 3
#define FRAMELIST_TOUCH_PAGE_UP 4
#define FRAMELIST_TOUCH_PAGE_DOWN 5
#define FRAMELIST_TOUCH_TOP 6
#define FRAMELIST_TOUCH_BOTTOM 7
#define FRAMELIST_TOUCH_ITEM_DECIDE 8

// A rectangle of the list's rows and buttons
typedef struct {
    TouchRect rect;
    u32 action;
} FrameListTouch;

// What the list calls to print an item into its row's window at y (on the first BG, or on the second), when the
// cursor moves to an item (initial is FALSE only for the first draw), and when the items scroll by delta pixels
typedef struct {
    void (*print)(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
    void (*select)(void *work, u32 index, BOOL moved);
    void (*scroll)(void *work, s16 delta);
} FrameListCallbacks;

// The first 20 bytes of the setup: where the list is, the size of its rows and the windows they are printed in
typedef struct {
    u8 bg;
    u8 bg2; // the second BG the rows scroll onto, or 0xff
    u8 x;
    s8 y;
    u8 width;
    u8 rowHeight;
    u8 windowX;
    u8 windowY;
    u8 windowWidth;
    u8 windowHeight;
    u8 windowPalette;
    s8 speed[6]; // pixels a step scrolls by, from the fastest
    u8 palette; // of the cursor
    u8 barSize;
    u8 unk13;
} FrameListLayout;

typedef struct {
    FrameListLayout layout;
    u16 count; // of items it can hold
    u16 screenCount;
    u8 cursorPos;
    u8 visibleRows;
    u16 scroll;
    const FrameListTouch *touch;
    const FrameListCallbacks *callbacks;
    void *work;
} FrameListSetup;

// A list item: the screen it is drawn from, and its value
typedef struct {
    u32 screen;
    u32 value;
} FrameListItem;

struct FrameList {
    FrameListSetup setup;
    FrameListItem *items;
    TouchRect *touchRects;
    BlinkPalAnm *blinkAnm;
    PrintQueue *printQueue;
    PrintWindow *printWindows;
    BGWinFrame *frames;
    u16 **screens;
    u8 rowCount;
    u8 rowsPerBG;
    s8 firstRow;
    s8 firstRow2;
    u8 shownRows;
    u8 shownRows2;
    s8 rowOffset;
    u16 count;
    u16 state;
    u16 nextState;
    s32 touchY;
    s32 prevTouchY;
    s16 cursor;
    u16 rows;
    u16 prevCursor;
    s16 scroll;
    u16 maxScroll;
    s8 bgOffset;
    s8 scrollDir;
    u8 scrollStep;
    u8 scrollFrames;
    BOOL playSE;
    BOOL flicking;
    TouchRect barTouch[2];
    u8 barIndex;
    u8 barTop;
    u8 barBottom;
    u8 barDragging;
    s32 barTouchY;
    u8 dragState;
    s8 dragDelta;
    struct {
        u8 touchFrames : 5;
        u8 scrolledByDrag : 1;
        u8 touching : 1;
        u8 dragging : 1;
    } flags;
    u8 dragItem;
    u8 dragItemPrev;
    u8 speedIndex;
    u32 flickStartY;
    u32 flickEndY;
    u8 repeatTimer;
    u8 repeatStage;
    u8 cursorDelay;
    HeapID heapId;
    u32 keyMask;
    BOOL waiting;
};

FrameList *FrameList_Create(const FrameListSetup *setup, HeapID heapId);
void FrameList_Free(FrameList *list);
// Adds an item
void FrameList_AddItem(FrameList *list, u32 type, u32 value);
// Loads the screen of the list's frame, and its palette
void FrameList_LoadScreen(FrameList *list, ArcTool *arc, u32 fileId, BOOL compressed, u32 index);
// FrameList_LoadScreen, then moves the tiles on by tileOffset and sets the palette
void FrameList_LoadScreenPalette(FrameList *list, ArcTool *arc, u32 fileId, BOOL compressed, u32 index, u16 tileOffset,
                         u8 palette);
void FrameList_LoadCursorPalette(FrameList *list, ArcTool *arc, u32 fileId, u32 palette, u32 count);
// Whether the list is still drawing its items
BOOL FrameList_Draw(FrameList *list);
u32 FrameList_Main(FrameList *list);
// Unused: where the scroll bar is, and the cursor off its row
u32 FrameList_GetBarPos(FrameList *list);
void FrameList_RedrawCursor(FrameList *list);
// Where the scroll bar goes, from its y
int FrameList_ClampBarPos(FrameList *list, int y);
PrintQueue *FrameList_GetPrintQueue(FrameList *list);
// An item's value
u32 FrameList_GetValue(FrameList *list, u32 index);
// The item under the cursor, the cursor's row, and the first row shown
int FrameList_GetSelected(FrameList *list);
s16 FrameList_GetCursor(FrameList *list);
s16 FrameList_GetScroll(FrameList *list);
// Whether the list can scroll down
BOOL FrameList_CanScrollDown(FrameList *list);
void FrameList_SetCursor(FrameList *list, int pos);
void FrameList_ShowCursor(FrameList *list);
void FrameList_SetShownRows(FrameList *list, int a1);
void FrameList_SetKeyMask(FrameList *list, u32 a1);
void FrameList_ScrollBlocking(FrameList *list, int a1);
u32 FrameList_GetScrollFrames(FrameList *list);
BlinkPalAnm *FrameList_GetBlinkAnm(FrameList *list);

#endif // POKEBW2_APP_UI_FRAME_LIST_H
