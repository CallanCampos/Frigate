// frame_list.c: a list of items that scrolls on one BG or two, by keys, by touching its rows, scroll bar and arrows, or
// by flicking it. The names are ours

#include "app/ui/frame_list.h"
#include "types.h"
#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/mi.h"
#include "system/bgwinfrm.h"
#include "system/blink_palanm.h"
#include "system/printsys.h"
#include "system/scroll_bar.h"

// frame_list.c: A list of items that scrolls, with a scroll bar and arrows. The name is the file's string; the rest
// of the names are ours

// What the list's states do: wait for input, scroll the rows, follow the scroll bar, follow a drag
#define LIST_STATE_INPUT 0
#define LIST_STATE_SCROLL 1
#define LIST_STATE_BAR 2
#define LIST_STATE_DRAG 3
#define LIST_STATE_CURSOR_WAIT 4

// What the keys and the touch screen ask for
enum {
    LIST_EVENT_CURSOR_SHOWN,
    LIST_EVENT_CURSOR_MOVE,
    LIST_EVENT_SCROLL_UP,
    LIST_EVENT_SCROLL_DOWN,
    LIST_EVENT_PAGE_UP,
    LIST_EVENT_PAGE_DOWN,
    LIST_EVENT_CURSOR_TOP,
    LIST_EVENT_CURSOR_BOTTOM,
    LIST_EVENT_JUMP_TOP,
    LIST_EVENT_JUMP_BOTTOM,
    LIST_EVENT_BAR,
    LIST_EVENT_TOUCH_ITEM,
    LIST_EVENT_DECIDE,
    LIST_EVENT_BLANK,
    LIST_EVENT_NONE,
};

// The screen height in tiles, and the BG's screen buffer height
#define LIST_SCREEN_ROWS 24
#define LIST_BG_ROWS 32

static void InitCounts(FrameList *list);
static void DrawAll(FrameList *list, BOOL moved);
static int TouchEvent(FrameList *list);
static int KeyEvent(FrameList *list);
static int HandleEvent(FrameList *list);
static void UpdateKeyRepeat(FrameList *list);
static u8 GetScrollFrames(FrameList *list, s32 delta);
static u32 GetScrollRoom(FrameList *list, s32 dir);
static void StartScroll(FrameList *list, s8 step, u8 count, u16 nextState, BOOL playSE);
static BOOL UpdateScroll(FrameList *list);
static void StartBar(FrameList *list, u32 index);
static u32 GetBarTouchValue(FrameList *list);
static void GetBarRange(FrameList *list, u8 *top, u8 *bottom);

static BOOL UpdateBar(FrameList *list);
static void StartDrag(FrameList *list, u8 item);
static BOOL UpdateDrag(FrameList *list);
static BOOL ScrollToTouched(FrameList *list, int item);
static BOOL StartFlick(FrameList *list);
static u8 GetBgNo(u8 bg);
static void DrawItemFrame(FrameList *list, int item, s8 row);
static void LoadRowScreen(FrameList *list, s8 row, s8 y);
static void ClearRow(FrameList *list, u8 bg, s8 y);
static s8 GetOffscreenRowY(FrameList *list, int dir);
static s8 WrapRow(s8 pos, s8 count, s8 delta);
static void PrintRow(FrameList *list, int item, s8 y);
static void PrintRowOnBg2(FrameList *list, int item, s8 y);
static void SetRowPalette(FrameList *list, u32 row, u32 palette);
static void SetCursorRow(FrameList *list, u16 newCursor, u16 oldCursor);
static void FlushWindows(FrameList *list);
static u32 GetKeyTrigger(FrameList *list);
static u32 GetKeyRepeat(FrameList *list);


// How many rows to scroll by at once, from the speed that a drag or flick started at
static const u8 sFlickRows[6] = { 32, 24, 16, 10, 6, 4 };

// How far a drag has to go, by how long it has been held, for each speed
static const u8 sFlickDistances[6][6] = {
    { 16, 8, 6, 4, 4, 4 },        { 32, 16, 8, 6, 4, 1 },      { 64, 40, 32, 16, 8, 1 },
    { 96, 80, 64, 48, 32, 1 },    { 112, 96, 80, 64, 48, 1 },  { 128, 112, 96, 80, 64, 1 },
};

FrameList *FrameList_Create(const FrameListSetup *setup, HeapID heapId) {
    FrameList *list = GFL_HeapAllocate(heapId, sizeof(FrameList), TRUE, "frame_list.c", 250);
    u32 i;

    list->keyMask = 0x2fff;
    list->setup = *setup;
    list->heapId = heapId;
    list->touchY = -1;
    list->prevTouchY = -1;
    list->printQueue = func_020219a8(0x800, heapId);
    list->blinkAnm = BlinkPalAnm_Create(list->setup.layout.palette << 4, 16, list->setup.layout.bg, list->heapId);
    list->items = GFL_HeapAllocate(list->heapId, list->setup.count * sizeof(FrameListItem), TRUE, "frame_list.c", 265);
    if (list->setup.screenCount != 0) {
        list->screens = GFL_HeapAllocate(list->heapId, list->setup.screenCount * sizeof(u16 *), FALSE, "frame_list.c",
                                         270);
        for (i = 0; i < list->setup.screenCount; i++) {
            list->screens[i] = GFL_HeapAllocate(list->heapId, list->setup.layout.width * list->setup.layout.rowHeight * 2,
                                                FALSE, "frame_list.c", 272);
        }
    }
    list->rowCount = LIST_SCREEN_ROWS / list->setup.layout.rowHeight + 2;
    list->rowsPerBG = list->rowCount;
    if (list->setup.layout.bg2 != 0xff) {
        list->rowCount = list->rowCount * 2;
    }
    list->frames = BGWinFrame_Create(BGWINFRAME_TRANSFER_VBLANK, list->rowCount, list->heapId);
    list->printWindows = GFL_HeapAllocate(list->heapId, list->rowCount * sizeof(PrintWindow), FALSE, "frame_list.c", 284);
    for (i = 0; i < list->rowCount; i++) {
        BmpWin *window;

        if (list->setup.layout.bg2 != 0xff && list->rowsPerBG <= i) {
            BGWinFrame_InitFrame(list->frames, i, list->setup.layout.bg2, list->setup.layout.width,
                                 list->setup.layout.rowHeight);
            window = BmpWin_CreateDynamic(list->setup.layout.bg2, list->setup.layout.windowX,
                                          list->setup.layout.windowY, list->setup.layout.windowWidth,
                                          list->setup.layout.windowHeight, list->setup.layout.windowPalette, TRUE);
        } else {
            BGWinFrame_InitFrame(list->frames, i, list->setup.layout.bg, list->setup.layout.width,
                                 list->setup.layout.rowHeight);
            window = BmpWin_CreateDynamic(list->setup.layout.bg, list->setup.layout.windowX,
                                          list->setup.layout.windowY, list->setup.layout.windowWidth,
                                          list->setup.layout.windowHeight, list->setup.layout.windowPalette, TRUE);
        }
        list->printWindows[i].window = window;
    }
    list->rowOffset = LIST_SCREEN_ROWS;
    do {
        list->rowOffset = list->rowOffset - list->setup.layout.rowHeight;
    } while (list->rowOffset > 0);
    if (list->setup.touch != NULL) {
        u32 n = 0;

        while (list->setup.touch[n].rect.top != TOUCH_RECT_END) {
            n++;
        }
        list->touchRects = GFL_HeapAllocate(list->heapId, (n + 1) * sizeof(TouchRect), FALSE, "frame_list.c", 323);
        for (i = 0; i < n + 1; i++) {
            sys_memcpy32(&list->setup.touch[i], &list->touchRects[i], sizeof(TouchRect));
        }
    }
    return list;
}

void FrameList_Free(FrameList *list) {
    u32 i;

    if (list->touchRects != NULL) {
        GFL_HeapFree(list->touchRects);
    }
    for (i = 0; i < list->rowCount; i++) {
        BmpWin_Free(list->printWindows[i].window);
    }
    GFL_HeapFree(list->printWindows);
    BGWinFrame_Delete(list->frames);
    if (list->setup.screenCount != 0) {
        for (i = 0; i < list->setup.screenCount; i++) {
            GFL_HeapFree(list->screens[i]);
        }
        GFL_HeapFree(list->screens);
    }
    GFL_HeapFree(list->items);
    BlinkPalAnm_Free(list->blinkAnm);
    func_02021a18(list->printQueue);
    GFL_HeapFree(list);
}

void FrameList_AddItem(FrameList *list, u32 type, u32 value) {
    list->items[list->count].screen = type;
    list->items[list->count].value = value;
    list->count++;
}

void FrameList_LoadScreen(FrameList *list, ArcTool *arc, u32 fileId, BOOL compressed, u32 index) {
    NNSG2dScreenData *screen;
    void *data = GFL_G2DIOReadNSCRArc(arc, fileId, compressed, &screen, list->heapId);

    sys_memcpy16(screen->rawData, list->screens[index], list->setup.layout.width * list->setup.layout.rowHeight * 2);
    GFL_HeapFree(data);
}

void FrameList_LoadScreenPalette(FrameList *list, ArcTool *arc, u32 fileId, BOOL compressed, u32 index, u16 tileOffset,
                         u8 palette) {
    int i;

    FrameList_LoadScreen(list, arc, fileId, compressed, index);
    for (i = 0; i < list->setup.layout.width * list->setup.layout.rowHeight; i++) {
        u16 entry = list->screens[index][i];

        list->screens[index][i] = (palette << 12) + (entry & 0xc00) + (((entry & 0x3ff) + tileOffset) & 0x3ff);
    }
}

void FrameList_LoadCursorPalette(FrameList *list, ArcTool *arc, u32 fileId, u32 palette, u32 count) {
    BlinkPalAnm_SetPalBufferArcTool(list->blinkAnm, arc, fileId, palette << 4, count << 4);
}

BOOL FrameList_Draw(FrameList *list) {
    switch (list->state) {
    case 0:
        InitCounts(list);
        DrawAll(list, FALSE);
        list->state++;
        break;
    case 1:
        if (func_02021c0c(list->printQueue) == TRUE) {
            list->state = 0;
            return FALSE;
        }
        break;
    }
    FlushWindows(list);
    return TRUE;
}

u32 FrameList_Main(FrameList *list) {
    u32 x;
    s32 result = FRAMELIST_NONE;

    if (!func_0203da84(&x, (u32 *)&list->touchY)) {
        list->touchY = -1;
        list->flags.scrolledByDrag = 0;
    }
    switch (list->state) {
    case LIST_STATE_INPUT:
        result = HandleEvent(list);
        break;
    case LIST_STATE_SCROLL:
        if (UpdateScroll(list) == TRUE) {
            result = -10;
        }
        break;
    case LIST_STATE_BAR:
        if (UpdateBar(list) == TRUE) {
            result = -9;
        }
        break;
    case LIST_STATE_DRAG:
        if (UpdateDrag(list) == TRUE) {
            result = -8;
        }
        break;
    case LIST_STATE_CURSOR_WAIT:
        if (!func_0203d554() && (GetKeyTrigger(list) & PAD_BUTTON_A)) {
            result = list->cursor;
        }
        if (list->cursorDelay == 0) {
            list->state = LIST_STATE_INPUT;
        } else {
            list->cursorDelay--;
        }
        break;
    }
    list->prevTouchY = list->touchY;
    BlinkPalAnm_Main(list->blinkAnm);
    FlushWindows(list);
    return result;
}

static void InitCounts(FrameList *list) {
    if (list->setup.visibleRows > list->count) {
        list->rows = list->count;
    } else {
        list->rows = list->setup.visibleRows;
    }
    if (list->count < list->setup.visibleRows) {
        list->maxScroll = 0;
    } else {
        list->maxScroll = list->count - list->setup.visibleRows;
    }
    if (list->setup.cursorPos > list->rows || list->setup.scroll > list->maxScroll) {
        list->cursor = 0;
        list->scroll = 0;
    } else {
        list->cursor = list->setup.cursorPos;
        list->scroll = list->setup.scroll;
    }
}

static void DrawAll(FrameList *list, BOOL moved) {
    s16 item;
    s8 y;

    GFL_BGSysMoveBGReq(list->setup.layout.bg, 3, 0);
    list->bgOffset = 0;
    item = list->scroll;
    y = list->setup.layout.y;
    while (TRUE) {
        if (item == 0 || y <= 0) {
            break;
        }
        item--;
        y = y - list->setup.layout.rowHeight;
    }
    list->firstRow = 0;
    while (TRUE) {
        if (y >= LIST_SCREEN_ROWS) {
            break;
        }
        PrintRow(list, item, y);
        list->firstRow++;
        item++;
        y = y + list->setup.layout.rowHeight;
    }
    list->shownRows = list->firstRow;
    if (list->setup.layout.bg2 != 0xff) {
        GFL_BGSysMoveBGReq(list->setup.layout.bg2, 3, 0);
        y = list->setup.layout.y + LIST_SCREEN_ROWS - list->setup.layout.rowHeight;
        item = list->scroll - 1;
        while (TRUE) {
            if (y <= 0) {
                break;
            }
            item--;
            y = y - list->setup.layout.rowHeight;
        }
        list->firstRow2 = 0;
        while (TRUE) {
            if (y >= LIST_SCREEN_ROWS) {
                break;
            }
            PrintRowOnBg2(list, item, y);
            list->firstRow2++;
            item++;
            y = y + list->setup.layout.rowHeight;
        }
        list->shownRows2 = list->firstRow2;
        SetCursorRow(list, list->cursor, 0xff);
        list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, moved);
    }
}

// What the touch screen asks for
static int TouchEvent(FrameList *list) {
    s32 touched;
    BOOL touchMode;

    if (list->touchRects == NULL) {
        return LIST_EVENT_NONE;
    }
    touched = func_0203da0c(list->touchRects);
    if (touched == TOUCH_RECT_NONE) {
        return LIST_EVENT_NONE;
    }
    touchMode = func_0203d554();
    func_0203d564(TRUE);
    switch (list->setup.touch[touched].action) {
    case FRAMELIST_TOUCH_ITEM_DECIDE:
        if (touched >= list->rows) {
            return LIST_EVENT_BLANK;
        }
        list->prevCursor = list->cursor;
        list->cursor = touched;
        return LIST_EVENT_DECIDE;
    case FRAMELIST_TOUCH_ITEM:
        if (touched >= list->rows) {
            return LIST_EVENT_BLANK;
        }
        list->prevCursor = list->cursor;
        list->cursor = touched;
        return LIST_EVENT_TOUCH_ITEM;
    case FRAMELIST_TOUCH_BAR:
        if (list->count > list->rows) {
            list->barIndex = touched;
            list->prevCursor = list->cursor;
            return LIST_EVENT_BAR;
        }
        func_0203d564(touchMode);
        break;
    case FRAMELIST_TOUCH_UP:
        if (list->scroll != 0) {
            list->repeatStage = 0;
            return LIST_EVENT_SCROLL_UP;
        }
        break;
    case FRAMELIST_TOUCH_DOWN:
        if (list->scroll < list->maxScroll) {
            list->repeatStage = 0;
            return LIST_EVENT_SCROLL_DOWN;
        }
        break;
    case FRAMELIST_TOUCH_PAGE_UP:
        if (list->scroll != 0) {
            return LIST_EVENT_PAGE_UP;
        }
        if (list->cursor != 0) {
            list->prevCursor = list->cursor;
            list->cursor = 0;
            return LIST_EVENT_CURSOR_TOP;
        }
        break;
    case FRAMELIST_TOUCH_PAGE_DOWN:
        if (list->scroll < list->maxScroll) {
            return LIST_EVENT_PAGE_DOWN;
        }
        if (list->cursor < list->rows - 1) {
            list->prevCursor = list->cursor;
            list->cursor = list->rows - 1;
            return LIST_EVENT_CURSOR_BOTTOM;
        }
        break;
    case FRAMELIST_TOUCH_TOP:
        if (list->cursor != 0 || list->scroll != 0) {
            return LIST_EVENT_JUMP_TOP;
        }
        break;
    case FRAMELIST_TOUCH_BOTTOM:
        if (list->cursor != list->rows - 1 || list->scroll != list->maxScroll) {
            return LIST_EVENT_JUMP_BOTTOM;
        }
        break;
    }
    return LIST_EVENT_NONE;
}

// What the keys ask for
static int KeyEvent(FrameList *list) {
    if ((GetKeyTrigger(list) & 0x3f1) && func_0203d554() == TRUE) {
        func_0203d564(FALSE);
        list->prevCursor = 0xff;
        list->cursorDelay = 8;
        return LIST_EVENT_CURSOR_SHOWN;
    }
    if (GetKeyRepeat(list) == 0) {
        list->repeatTimer = 0;
        list->repeatStage = 6;
    }
    if (GetKeyRepeat(list) & PAD_KEY_UP) {
        func_0203d564(FALSE);
        if (list->cursor != 0) {
            list->prevCursor = list->cursor;
            list->cursor = list->cursor - 1;
            list->cursorDelay = 8;
            return LIST_EVENT_CURSOR_MOVE;
        }
        if (list->scroll != 0) {
            list->cursorDelay = 0;
            UpdateKeyRepeat(list);
            return LIST_EVENT_SCROLL_UP;
        }
        return LIST_EVENT_NONE;
    }
    if (GetKeyRepeat(list) & PAD_KEY_DOWN) {
        func_0203d564(FALSE);
        if (list->cursor < list->rows - 1) {
            list->prevCursor = list->cursor;
            list->cursor = list->cursor + 1;
            list->cursorDelay = 8;
            return LIST_EVENT_CURSOR_MOVE;
        }
        if (list->scroll < list->maxScroll) {
            list->cursorDelay = 0;
            UpdateKeyRepeat(list);
            return LIST_EVENT_SCROLL_DOWN;
        }
        return LIST_EVENT_NONE;
    }
    if (GetKeyRepeat(list) & PAD_KEY_LEFT) {
        func_0203d564(FALSE);
        if (list->scroll != 0) {
            list->cursorDelay = 4;
            return LIST_EVENT_PAGE_UP;
        }
        if (list->cursor != 0) {
            list->prevCursor = list->cursor;
            list->cursor = 0;
            list->cursorDelay = 4;
            return LIST_EVENT_CURSOR_TOP;
        }
        return LIST_EVENT_NONE;
    }
    if (GetKeyRepeat(list) & PAD_KEY_RIGHT) {
        func_0203d564(FALSE);
        if (list->scroll < list->maxScroll) {
            list->cursorDelay = 4;
            return LIST_EVENT_PAGE_DOWN;
        }
        if (list->cursor < list->rows - 1) {
            list->prevCursor = list->cursor;
            list->cursor = list->rows - 1;
            list->cursorDelay = 4;
            return LIST_EVENT_CURSOR_BOTTOM;
        }
        return LIST_EVENT_NONE;
    }
    if (GetKeyTrigger(list) & PAD_BUTTON_A) {
        return LIST_EVENT_DECIDE;
    }
    if (GetKeyTrigger(list) & PAD_BUTTON_L) {
        func_0203d564(FALSE);
        if (list->cursor != 0 || list->scroll != 0) {
            return LIST_EVENT_JUMP_TOP;
        }
        return LIST_EVENT_NONE;
    }
    if (GetKeyTrigger(list) & PAD_BUTTON_R) {
        func_0203d564(FALSE);
        if (list->cursor != list->rows - 1 || list->scroll != list->maxScroll) {
            return LIST_EVENT_JUMP_BOTTOM;
        }
        return LIST_EVENT_NONE;
    }
    return LIST_EVENT_NONE;
}

// Does what the input asked for, and returns what the list's user has to know about it
static int HandleEvent(FrameList *list) {
    s16 rows;

    if (func_02021c0c(list->printQueue) == FALSE) {
        return FRAMELIST_NONE;
    }
    {
        int event = TouchEvent(list);

        if (event == LIST_EVENT_NONE) {
            event = KeyEvent(list);
        }
        switch (event) {
        case LIST_EVENT_CURSOR_SHOWN:
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            SetCursorRow(list, list->cursor, list->prevCursor);
            list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
            list->state = LIST_STATE_CURSOR_WAIT;
            return -12;
        case LIST_EVENT_CURSOR_MOVE:
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            SetCursorRow(list, list->cursor, list->prevCursor);
            list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
            list->state = LIST_STATE_CURSOR_WAIT;
            return -11;
        case LIST_EVENT_SCROLL_UP:
            StartScroll(list, -list->setup.layout.speed[list->repeatStage], 1, LIST_STATE_CURSOR_WAIT, TRUE);
            return -10;
        case LIST_EVENT_SCROLL_DOWN:
            StartScroll(list, list->setup.layout.speed[list->repeatStage], 1, LIST_STATE_CURSOR_WAIT, TRUE);
            return -10;
        case LIST_EVENT_PAGE_UP:
            rows = list->rows;
            list->prevCursor = list->cursor;
            if (list->scroll - rows < 0) {
                list->cursor = 0;
                rows = list->scroll;
            }
            SetCursorRow(list, 0xff, list->prevCursor);
            StartScroll(list, -list->setup.layout.speed[0], rows, LIST_STATE_CURSOR_WAIT, FALSE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return -7;
        case LIST_EVENT_PAGE_DOWN:
            rows = list->rows;
            list->prevCursor = list->cursor;
            if (list->scroll + rows > list->maxScroll) {
                list->cursor = list->rows - 1;
                rows = list->maxScroll - list->scroll;
            }
            SetCursorRow(list, 0xff, list->prevCursor);
            StartScroll(list, list->setup.layout.speed[0], rows, LIST_STATE_CURSOR_WAIT, FALSE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return -6;
        case LIST_EVENT_CURSOR_TOP:
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            SetCursorRow(list, list->cursor, list->prevCursor);
            list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
            list->state = LIST_STATE_CURSOR_WAIT;
            return -5;
        case LIST_EVENT_CURSOR_BOTTOM:
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            SetCursorRow(list, list->cursor, list->prevCursor);
            list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
            list->state = LIST_STATE_CURSOR_WAIT;
            return -4;
        case LIST_EVENT_JUMP_TOP:
            list->cursor = 0;
            list->scroll = 0;
            DrawAll(list, TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return -3;
        case LIST_EVENT_JUMP_BOTTOM:
            list->cursor = list->rows - 1;
            list->scroll = list->maxScroll;
            DrawAll(list, TRUE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return -2;
        case LIST_EVENT_BAR:
            SetCursorRow(list, 0xff, list->prevCursor);
            StartBar(list, list->barIndex);
            GFL_SndSEPlay(SEQ_SE_SYS_06);
            return -9;
        case LIST_EVENT_TOUCH_ITEM:
            SetCursorRow(list, 0xff, list->prevCursor);
            list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
            StartDrag(list, list->cursor);
            GFL_SndSEPlay(SEQ_SE_SYS_06);
            return -8;
        case LIST_EVENT_DECIDE:
            if (func_0203d554() == TRUE) {
                SetCursorRow(list, list->cursor, list->prevCursor);
                list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
            }
            return list->cursor;
        default:
            return FRAMELIST_NONE;
        }
    }
}

// Speeds the cursor's repeat up the longer a key is held
static void UpdateKeyRepeat(FrameList *list) {
    if (list->repeatTimer < 20) {
        list->repeatTimer++;
    }
    if (list->repeatTimer == 20) {
        list->repeatStage = 0;
    } else if (list->repeatTimer >= 16) {
        list->repeatStage = 1;
    } else if (list->repeatTimer >= 12) {
        list->repeatStage = 2;
    } else if (list->repeatTimer >= 8) {
        list->repeatStage = 3;
    } else if (list->repeatTimer >= 4) {
        list->repeatStage = 4;
    } else {
        list->repeatStage = 5;
    }
}

// How many frames a step of delta pixels waits to move a row
static u8 GetScrollFrames(FrameList *list, s32 delta) {
    if (delta < 0) {
        delta = -delta;
    }
    return (list->setup.layout.rowHeight * 8) / delta;
}

// How many rows the list can scroll in a direction
static u32 GetScrollRoom(FrameList *list, s32 dir) {
    if (dir < 0) {
        return list->scroll;
    }
    return list->maxScroll - list->scroll;
}

// Starts scrolling count rows, step pixels a frame, and goes on to nextState
static void StartScroll(FrameList *list, s8 step, u8 count, u16 nextState, BOOL playSE) {
    list->scrollDir = step;
    list->scrollStep = count;
    list->scrollFrames = 0;
    list->nextState = nextState;
    list->state = LIST_STATE_SCROLL;
    list->playSE = playSE;
}

static BOOL UpdateScroll(FrameList *list) {
    s32 touched;

    if (func_02021c0c(list->printQueue) == FALSE) {
        return TRUE;
    }
    if (list->nextState == LIST_STATE_DRAG && list->flicking == TRUE) {
        touched = func_0203d9c8(list->touchRects);
        if (list->flags.touching == TRUE) {
            if (!list->flags.dragging) {
                if (touched == TOUCH_RECT_NONE) {
                    int delta;

                    list->dragDelta = list->flickEndY - list->flickStartY;
                    delta = list->dragDelta;
                    if (delta < 0) {
                        delta = -delta;
                    }
                    if (delta >= 16) {
                        list->flags.touchFrames = 0;
                        list->flags.touching = 0;
                        list->flags.dragging = 1;
                    }
                } else if (list->flags.touchFrames != 3) {
                    list->flickStartY = list->touchY;
                    list->flags.touchFrames++;
                }
            }
        } else if (touched != TOUCH_RECT_NONE && list->setup.touch[touched].action == FRAMELIST_TOUCH_ITEM) {
            list->cursor = touched;
            list->flags.touching = 1;
            list->flags.touchFrames = 0;
            list->flickStartY = list->touchY;
            list->flickEndY = list->touchY;
        }
    }
    if (list->scrollFrames == 0) {
        if (list->scrollStep == 0) {
            touched = func_0203d9c8(list->touchRects);
            if (touched != TOUCH_RECT_NONE && list->setup.touch[touched].action == FRAMELIST_TOUCH_ITEM &&
                list->flags.scrolledByDrag == 1 && list->setup.touch[touched].action == FRAMELIST_TOUCH_ITEM &&
                ((list->dragDelta < 0 && list->dragItem < touched) ||
                 (list->dragDelta > 0 && list->dragItem > touched))) {
                u32 available;

                list->flags.scrolledByDrag = 0;
                available = GetScrollRoom(list, list->scrollDir);
                if (available != 0) {
                    int diff = list->dragItem - touched;

                    if (diff < 0) {
                        diff = -diff;
                    }
                    list->scrollStep = diff;
                    if (list->scrollStep > available) {
                        list->scrollStep = available;
                    }
                    list->dragItem = touched;
                } else {
                    list->flickStartY = list->touchY;
                    list->flickEndY = list->touchY;
                    list->flags.touching = 0;
                    SetCursorRow(list, list->cursor, 0xff);
                    list->state = list->nextState;
                    return FALSE;
                }
            } else {
                list->flickStartY = list->touchY;
                list->flickEndY = list->touchY;
                list->flags.scrolledByDrag = 0;
                list->flags.touching = 0;
                SetCursorRow(list, list->cursor, 0xff);
                list->state = list->nextState;
                return FALSE;
            }
        }
        SetCursorRow(list, 0xff, list->cursor);
        if (list->nextState == LIST_STATE_DRAG) {
            if (list->flicking == 0) {
                if (list->scrollDir < 0) {
                    list->cursor = list->cursor + 1;
                } else {
                    list->cursor = list->cursor - 1;
                }
            } else {
                if (list->flags.dragging == 1) {
                    if (StartFlick(list) == TRUE) {
                        list->flags.touchFrames = 0;
                    }
                    list->flags.dragging = 0;
                }
                if (list->flags.touching == 1 && list->flags.touchFrames == 3) {
                    touched = func_0203d9c8(list->touchRects);
                    if (touched != TOUCH_RECT_NONE && list->setup.touch[touched].action == FRAMELIST_TOUCH_ITEM) {
                        list->cursor = touched;
                    }
                    list->flicking = 0;
                    list->dragItem = list->cursor;
                    list->flags.touching = 0;
                    list->flags.touchFrames = 0;
                    list->flickStartY = list->touchY;
                    list->flickEndY = list->touchY;
                    list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
                    list->state = list->nextState;
                    return FALSE;
                }
                if (list->scrollStep == 1) {
                    u32 available = GetScrollRoom(list, list->scrollDir);

                    list->speedIndex++;
                    if (list->speedIndex != 6) {
                        if (list->scrollDir < 0) {
                            list->scrollDir = -list->setup.layout.speed[list->speedIndex];
                        } else {
                            list->scrollDir = list->setup.layout.speed[list->speedIndex];
                        }
                        list->scrollStep = sFlickRows[list->speedIndex];
                    }
                    if (list->scrollStep > available) {
                        list->scrollStep = available;
                    }
                }
            }
        }
        list->scrollFrames = GetScrollFrames(list, list->scrollDir);
        list->scrollStep--;
        if (list->scrollDir < 0) {
            list->scroll--;
            PrintRow(list, list->scroll, GetOffscreenRowY(list, -1));
            list->firstRow = WrapRow(list->firstRow, list->shownRows, -1);
            PrintRowOnBg2(list, list->scroll - list->shownRows2,
                                list->rowOffset + GetOffscreenRowY(list, -1));
            list->firstRow2 = WrapRow(list->firstRow2, list->shownRows2, -1);
        } else {
            list->scroll++;
            PrintRow(list, list->scroll + (list->shownRows - 1), GetOffscreenRowY(list, 1));
            list->firstRow = WrapRow(list->firstRow, list->shownRows, 1);
            PrintRowOnBg2(list, list->scroll - 1, list->rowOffset + GetOffscreenRowY(list, 1));
            list->firstRow2 = WrapRow(list->firstRow2, list->shownRows2, 1);
        }
        list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
        if (list->playSE == TRUE) {
            if ((u16)(list->nextState + 0xfffe) <= 1) {
                GFL_SndSEPlay(SEQ_SE_SYS_06);
            } else {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
        }
    }
    list->bgOffset = list->bgOffset + list->scrollDir;
    GFL_BGSysMoveBGReq(list->setup.layout.bg, 3, list->bgOffset);
    if (list->setup.layout.bg2 != 0xff) {
        GFL_BGSysMoveBGReq(list->setup.layout.bg2, 3, list->bgOffset);
    }
    list->setup.callbacks->scroll(list->setup.work, list->scrollDir);
    list->scrollFrames--;
    return TRUE;
}

// Starts following the scroll bar from the rectangle that was touched
static void StartBar(FrameList *list, u32 index) {
    u32 x;
    u32 top;
    int bottom;
    int left;
    int right;

    list->barTop = list->touchRects[index].top;
    list->barBottom = list->touchRects[index].bottom;
    list->barTouch[0].top = list->touchRects[index].top;
    list->barTouch[0].bottom = list->touchRects[index].bottom;
    list->barTouch[0].left = list->touchRects[index].left;
    list->barTouch[0].right = list->touchRects[index].right;
    // The area around the bar that still follows it
    top = list->barTouch[0].top;
    if (top >= 32) {
        top = top - 32;
    } else {
        top = 0;
    }
    list->barTouch[0].top = top;
    bottom = list->barTouch[0].bottom + 32;
    bottom = bottom <= 191 ? bottom : 191;
    list->barTouch[0].bottom = bottom;
    left = list->barTouch[0].left;
    list->barTouch[0].left = left;
    right = list->barTouch[0].right;
    right = right <= 255 ? right : 255;
    list->barTouch[0].right = right;
    list->barTouch[1].top = TOUCH_RECT_END;
    if (!func_0203da84(&x, (u32 *)&list->barTouchY)) {
        list->barTouchY = -1;
    }
    list->barDragging = TRUE;
    list->state = LIST_STATE_BAR;
}

// The item that the scroll bar's position points at
static u32 GetBarTouchValue(FrameList *list) {
    u32 x;
    u32 y;

    if (!func_0203da84(&x, &y)) {
        y = list->barTouchY;
    }
    list->barTouchY = -1;
    return ScrollBar_GetValue(list->maxScroll, y, list->barTop, list->barBottom, list->setup.layout.barSize);
}

// The scroll bar's top and bottom, from the first rectangle of the touch table that is the bar
static void GetBarRange(FrameList *list, u8 *top, u8 *bottom) {
    u8 i = 0;
    const FrameListTouch *touch = list->setup.touch;

    while (TRUE) {
        const FrameListTouch *entry = &touch[i];

        if (entry->rect.top == TOUCH_RECT_END) {
            break;
        }
        if (entry->action == FRAMELIST_TOUCH_BAR) {
            *top = entry->rect.top;
            *bottom = list->setup.touch[i].rect.bottom;
            return;
        }
        i++;
    }
}

u32 FrameList_GetBarPos(FrameList *list) {
    u8 top;
    u8 bottom;

    GetBarRange(list, &top, &bottom);
    return ScrollBar_GetPos(list->maxScroll, list->scroll, top, bottom, list->setup.layout.barSize);
}

int FrameList_ClampBarPos(FrameList *list, int y) {
    u8 top;
    u8 bottom;
    u32 x;
    u32 touchY;
    u32 start;
    u32 end;

    if (list->barDragging == TRUE) {
        if (func_0203d9c8(&list->barTouch[0]) != TOUCH_RECT_NONE) {
            func_0203da84(&x, &touchY);
            return touchY;
        }
        return y;
    }
    GetBarRange(list, &top, &bottom);
    start = ScrollBar_GetPos(list->maxScroll, list->scroll, top, bottom, list->setup.layout.barSize);
    if (list->maxScroll == list->scroll) {
        end = bottom;
    } else {
        end = ScrollBar_GetPos(list->maxScroll, list->scroll + 1, top, bottom, list->setup.layout.barSize);
    }
    if (y >= start && y < end) {
        return y;
    }
    return start;
}

// Follows the scroll bar
static BOOL UpdateBar(FrameList *list) {
    u32 target;
    u32 diff;

    if (func_02021c0c(list->printQueue) == FALSE) {
        return TRUE;
    }
    if (func_0203d9c8(&list->barTouch[0]) == TOUCH_RECT_NONE && list->barTouchY == -1) {
        list->barDragging = FALSE;
        list->state = LIST_STATE_INPUT;
        return FALSE;
    }
    target = GetBarTouchValue(list);
    diff = list->scroll - target;
    if ((s32)diff < 0) {
        diff = -diff;
    }
    if (diff > list->shownRows) {
        diff = list->shownRows;
    }
    if (list->scroll > target) {
        list->scroll = target + diff;
        StartScroll(list, -list->setup.layout.speed[0], diff, LIST_STATE_BAR, TRUE);
    } else if (list->scroll < target) {
        list->scroll = target - diff;
        StartScroll(list, list->setup.layout.speed[0], diff, LIST_STATE_BAR, TRUE);
    }
    return TRUE;
}

// Starts following a touch that began on an item
static void StartDrag(FrameList *list, u8 item) {
    list->dragState = 0;
    list->dragItem = item;
    list->flicking = FALSE;
    list->flags.touchFrames = 0;
    list->flickStartY = list->touchY;
    list->flickEndY = list->touchY;
    list->state = LIST_STATE_DRAG;
}

// Follows a touch that began on an item, and scrolls the list when it is dragged or flicked
static BOOL UpdateDrag(FrameList *list) {
    s32 touched;

    if (func_02021c0c(list->printQueue) == FALSE) {
        return TRUE;
    }
    touched = func_0203d9c8(list->touchRects);
    switch (list->dragState) {
    case 0:
        if (touched == TOUCH_RECT_NONE) {
            if (list->flickStartY != -1 && list->flickEndY != -1) {
                list->dragDelta = list->flickEndY - list->flickStartY;
                list->flags.touchFrames = 0;
                if (StartFlick(list) == TRUE) {
                    break;
                }
            }
            list->flicking = FALSE;
            list->state = LIST_STATE_INPUT;
            return FALSE;
        }
        if (list->setup.touch[touched].action != FRAMELIST_TOUCH_ITEM) {
            if (list->flickStartY != -1 && list->flickEndY != -1) {
                list->dragDelta = list->flickEndY - list->flickStartY;
                list->flags.touchFrames = 0;
                if (StartFlick(list) == TRUE) {
                    break;
                }
            }
            list->flicking = FALSE;
            list->state = LIST_STATE_INPUT;
            return FALSE;
        }
        if (list->dragItem != touched) {
            list->dragItemPrev = touched;
            list->dragDelta = list->dragItem - touched;
            list->flags.touchFrames = 0;
            list->dragState = 1;
        }
        list->flickEndY = list->flickStartY;
        list->flickStartY = list->touchY;
        break;
    case 1:
        if (touched == TOUCH_RECT_NONE) {
            list->dragState = 0;
            return StartFlick(list);
        }
        if (list->setup.touch[touched].action != FRAMELIST_TOUCH_ITEM) {
            list->dragState = 0;
            return StartFlick(list);
        }
        if (list->flags.touchFrames == 5) {
            list->flags.touchFrames = 0;
            list->dragState = 0;
            if (ScrollToTouched(list, touched) == TRUE) {
                list->flags.scrolledByDrag = 1;
            }
        } else {
            list->dragItemPrev = touched;
            list->flickStartY = list->touchY;
            list->flags.touchFrames++;
        }
        break;
    }
    return TRUE;
}

// Scrolls toward the item that a drag has moved to
static BOOL ScrollToTouched(FrameList *list, int item) {
    int diff = list->dragItem - item;
    int step;

    if ((diff < 0 && list->dragDelta > 0) || (diff > 0 && list->dragDelta < 0)) {
        return FALSE;
    }
    if ((list->dragDelta < 0 && list->scroll == 0) || (list->dragDelta > 0 && list->scroll == list->maxScroll)) {
        return FALSE;
    }
    if (diff < 0) {
        diff = -diff;
    }
    if (diff == 0) {
        return FALSE;
    }
    if (diff == 1) {
        step = list->setup.layout.speed[3];
    } else if (diff == 2) {
        step = list->setup.layout.speed[2];
    } else if (diff == 3) {
        step = list->setup.layout.speed[1];
    } else {
        step = list->setup.layout.speed[0];
    }
    if (item > list->dragItem) {
        step = -1 * step;
    }
    {
        u32 available = GetScrollRoom(list, step);

        if (available != 0) {
            if (available < diff) {
                diff = available;
            }
            StartScroll(list, step, diff, LIST_STATE_DRAG, TRUE);
            list->dragItem = item;
            return TRUE;
        }
    }
    return FALSE;
}

// Keeps scrolling after a flick, as far and as fast as it was
static BOOL StartFlick(FrameList *list) {
    int diff;
    u32 i;
    int step;

    if ((list->dragDelta < 0 && list->flickEndY > list->flickStartY) ||
        (list->dragDelta > 0 && list->flickEndY < list->flickStartY)) {
        return FALSE;
    }
    diff = list->flickEndY - list->flickStartY;
    if (diff < 0) {
        diff = -diff;
    }
    if (diff == 0) {
        return FALSE;
    }
    list->speedIndex = 0;
    for (i = list->speedIndex; i < 6; i = list->speedIndex) {
        if (diff >= sFlickDistances[list->flags.touchFrames][i]) {
            step = list->setup.layout.speed[i];
            diff = sFlickRows[i];
            break;
        }
        list->speedIndex++;
    }
    if (i == 6) {
        return FALSE;
    }
    if (list->flickStartY > list->flickEndY) {
        step = -1 * step;
    }
    {
        u32 available = GetScrollRoom(list, step);

        if (available != 0) {
            if (available < diff) {
                diff = available;
            }
            StartScroll(list, step, diff, LIST_STATE_DRAG, TRUE);
            list->flicking = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

static u8 GetBgNo(u8 bg) {
    if (bg == 0) {
        return 0;
    }
    if (bg == 1) {
        return 1;
    }
    if (bg == 2) {
        return 2;
    }
    if (bg == 3) {
        return 3;
    }
    if (bg == 4) {
        return 4;
    }
    if (bg == 5) {
        return 5;
    }
    if (bg == 6) {
        return 6;
    }
    if (bg == 7) {
        return 7;
    }
    return 0;
}

// Draws an item's frame, with its text, into the row's window
static void DrawItemFrame(FrameList *list, int item, s8 row) {
    BmpWin *window = list->printWindows[row].window;
    u16 *screen = list->screens[list->items[item].screen];
    u8 *tiles;
    u8 *pixels;
    u8 screenWidth;
    u8 width;
    u8 height;
    u8 x;
    u8 y;
    u8 i;
    u8 j;

    tiles = func_020503f4(GetBgNo(BGWinFrame_GetBG(list->frames, row)));
    BGWinFrame_SetScreen(list->frames, row, screen);
    BGWinFrame_WriteBmpWin(list->frames, row, window);
    screenWidth = list->setup.layout.width;
    width = BmpWin_GetWidth1(window);
    height = BmpWin_GetHeight2(window);
    x = BmpWin_GetPosX(window);
    y = BmpWin_GetPosY(window);
    pixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(window));
    for (j = 0; j < height; j++) {
        u16 *line = screen + x + (j + y) * screenWidth;

        for (i = 0; i < width; i++) {
            u16 entry = line[i];

            sys_memcpy32(tiles + (entry & 0x3ff) * 32, pixels + (i + j * width) * 32, 32);
        }
    }
}

// Puts a row's frame on its BG at y
static void LoadRowScreen(FrameList *list, s8 row, s8 y) {
    u16 *screen = BGWinFrame_GetScreen(list->frames, row);
    u32 bg = BGWinFrame_GetBG(list->frames, row);
    u16 i;

    for (i = 0; i < list->setup.layout.rowHeight; i++) {
        if (y < 0) {
            y += LIST_BG_ROWS;
        } else if (y >= LIST_BG_ROWS) {
            y -= LIST_BG_ROWS;
        }
        GFL_BGSysLoadScrArea(bg, list->setup.layout.x, y, list->setup.layout.width, 1, screen, 0, i,
                             list->setup.layout.width, list->setup.layout.rowHeight);
        y++;
    }
    if (!list->waiting) {
        GFL_BGSysQueueScrLoad(bg);
    }
}

// Clears a row of a BG
static void ClearRow(FrameList *list, u8 bg, s8 y) {
    u32 i;

    for (i = 0; i < list->setup.layout.rowHeight; i++) {
        if (y < 0) {
            y += LIST_BG_ROWS;
        } else if (y >= LIST_BG_ROWS) {
            y -= LIST_BG_ROWS;
        }
        GFL_BGSysFillScrArea(bg, 0, list->setup.layout.x, y, list->setup.layout.width, 1, 0);
        y++;
    }
    if (!list->waiting) {
        GFL_BGSysQueueScrLoad(bg);
    }
}

// Where the row that scrolls in goes
static s8 GetOffscreenRowY(FrameList *list, int dir) {
    s8 start = list->setup.layout.y;
    s8 step = dir * list->setup.layout.rowHeight;
    s8 pos = start;

    do {
        pos = pos + step;
    } while (pos >= start && pos < LIST_SCREEN_ROWS);
    return pos + list->bgOffset / 8;
}

// Moves a ring of rows on by delta
static s8 WrapRow(s8 pos, s8 count, s8 delta) {
    s8 next = pos + delta;

    if (next < 0) {
        return count;
    }
    if (next > count) {
        return 0;
    }
    return next;
}

static void PrintRow(FrameList *list, int item, s8 y) {
    if (item >= 0 && item < list->count) {
        DrawItemFrame(list, item, list->firstRow);
        LoadRowScreen(list, list->firstRow, y);
        list->setup.callbacks->print(list->setup.work, item, &list->printWindows[list->firstRow], y * 8 - list->bgOffset, TRUE);
    } else {
        ClearRow(list, list->setup.layout.bg, y);
    }
}

static void PrintRowOnBg2(FrameList *list, int item, s8 y) {
    if (list->setup.layout.bg2 != 0xff) {
        if (item >= 0) {
            int row = list->firstRow2 + list->rowsPerBG;

            DrawItemFrame(list, item, row);
            LoadRowScreen(list, row, y);
            list->setup.callbacks->print(list->setup.work, item, &list->printWindows[row], y * 8 - list->bgOffset, FALSE);
        } else {
            ClearRow(list, list->setup.layout.bg2, y);
        }
    }
}

// Sets the palette of an item's row
static void SetRowPalette(FrameList *list, u32 row, u32 palette) {
    s16 y = list->bgOffset / 8 + (list->setup.layout.y + row * list->setup.layout.rowHeight);
    u16 i;

    for (i = 0; i < list->setup.layout.rowHeight; i++) {
        if (y < 0) {
            y += LIST_BG_ROWS;
        } else if (y >= LIST_BG_ROWS) {
            y -= LIST_BG_ROWS;
        }
        GFL_BGSysSetScrPaletteNo(list->setup.layout.bg, list->setup.layout.x, y, list->setup.layout.width, 1,
                                 palette);
        y++;
    }
    if (!list->waiting) {
        GFL_BGSysQueueScrLoad(list->setup.layout.bg);
    }
}

// Gives the row of the cursor its palette, and the row that it left its own
static void SetCursorRow(FrameList *list, u16 newCursor, u16 oldCursor) {
    if (oldCursor < list->rows) {
        SetRowPalette(list, oldCursor,
                            list->screens[list->items[list->scroll + oldCursor].screen][0] >> 12);
    }
    if (newCursor < list->rows && !func_0203d554()) {
        BlinkPalAnm_InitAnime(list->blinkAnm);
        SetRowPalette(list, newCursor, list->setup.layout.palette);
    }
}

static void FlushWindows(FrameList *list) {
    u32 i;

    func_02021a3c(list->printQueue);
    for (i = 0; i < list->rowCount; i++) {
        PrintWindow_Flush(&list->printWindows[i], list->printQueue);
    }
}

static u32 GetKeyTrigger(FrameList *list) {
    return GCTX_HIDGetPressedKeys() & list->keyMask;
}

static u32 GetKeyRepeat(FrameList *list) {
    return GCTX_HIDGetHeldKeys() & list->keyMask;
}

PrintQueue *FrameList_GetPrintQueue(FrameList *list) {
    return list->printQueue;
}

u32 FrameList_GetValue(FrameList *list, u32 index) {
    return list->items[index].value;
}

int FrameList_GetSelected(FrameList *list) {
    return list->cursor + list->scroll;
}

s16 FrameList_GetCursor(FrameList *list) {
    return list->cursor;
}

s16 FrameList_GetScroll(FrameList *list) {
    return list->scroll;
}

BOOL FrameList_CanScrollDown(FrameList *list) {
    if (list->scroll < list->maxScroll) {
        return TRUE;
    }
    return FALSE;
}

void FrameList_SetCursor(FrameList *list, int pos) {
    list->prevCursor = list->cursor;
    list->cursor = pos;
    SetCursorRow(list, list->cursor, list->prevCursor);
    list->setup.callbacks->select(list->setup.work, list->cursor + list->scroll, TRUE);
}

void FrameList_ShowCursor(FrameList *list) {
    SetCursorRow(list, list->cursor, 0xff);
}

void FrameList_RedrawCursor(FrameList *list) {
    SetCursorRow(list, 0xff, list->cursor);
}

void FrameList_SetShownRows(FrameList *list, int a1) {
    list->shownRows = a1;
    if (list->firstRow > a1) {
        list->firstRow = 0;
    }
}

void FrameList_SetKeyMask(FrameList *list, u32 a1) {
    list->keyMask = a1;
}

void FrameList_ScrollBlocking(FrameList *list, int a1) {
    if (a1 > 0) {
        list->waiting = TRUE;
        StartScroll(list, list->setup.layout.speed[0], a1, LIST_STATE_CURSOR_WAIT, FALSE);
        while (FrameList_Main(list) == -10) {
        }
        list->waiting = FALSE;
    }
}

u32 FrameList_GetScrollFrames(FrameList *list) {
    return list->scrollFrames;
}

BlinkPalAnm *FrameList_GetBlinkAnm(FrameList *list) {
    return list->blinkAnm;
}
