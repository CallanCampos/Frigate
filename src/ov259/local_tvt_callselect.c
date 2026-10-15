#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "save/save_control.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/wordset.h"

// The list of the persons the player can call on the Xtransceiver's lower screen, named after the ROM's
// "local_tvt_callselect.c": the plates of the persons, of which the player picks one, the scroll bar of a list
// longer than the screen, and the button that makes the call. Every name here is ours

// The plates that fit on the screen
#define LIST_VISIBLE 5
#define ITEM_HEIGHT 32
// How far the scroll bar's knob moves, and how fast
#define SCROLL_BAR_RANGE 120
#define SCROLL_BAR_SPEED 8

// The sequences of the list's actors
enum {
    LIST_ANIM_PLATE,
    LIST_ANIM_ICON,
    LIST_ANIM_ICON_SELECTED,
    LIST_ANIM_ICON_DARK,
    LIST_ANIM_SCROLL_BAR,
    LIST_ANIM_SCROLL_BAR_HELD,
    LIST_ANIM_PLATE_DECIDE,
    LIST_ANIM_PLATE_CURSOR,
};

// The steps of LocalTvtCallSelect_Main
enum {
    SEQ_SELECT,
    SEQ_CALL,
    SEQ_EXIT,
    SEQ_END,
};

// A person of the list
typedef struct {
    BOOL selected;
    BOOL cursor;
    // Dark while another person is selected
    BOOL bright;
    BOOL disabled;
    ClActor *plate;
    ClActor *icon;
    u32 unk18;
    BmpWin *window;
    int index;
} LocalTvtCallItem;

typedef struct {
    // Only a list longer than the screen has it
    BOOL enabled;
    BOOL held;
    BOOL moved;
    // The knob moves to the target on its own
    BOOL scrolling;
    ClActor *actor;
    int pos;
    int target;
} LocalTvtCallScrollBar;

// The bottom of the screen: a message while nobody is selected, then the button that makes the call
typedef struct {
    BmpWin *window;
    AppTaskMenuWin *button;
    AppTaskMenuRes *res;
    LocalTvtMessage *message;
    StrBuf *callStr;
    StrBuf *promptStr;
    u16 heapId;
    BOOL shown;
    BOOL active;
} LocalTvtCallButton;

typedef struct {
    u16 scroll;
    u16 count;
    // The button's when it is the count
    int cursor;
    BOOL changed;
    LocalTvtCallItem items[LOCAL_TVT_SELECT_MAX];
    LocalTvtCallScrollBar scrollBar;
    LocalTvtCallButton button;
    // The next key moves the cursor to the button
    BOOL cursorToButton;
} LocalTvtCallList;

struct LocalTvtCallSelect {
    u32 palette;
    u32 chars;
    u32 cellAnims;
    int seq;
    u32 result;
    LocalTvtCallSelectSetup setup;
    LocalTvtCallList list;
    LocalTvtReturnBar *returnBar;
    LocalTvtKeyTouch *keyTouch;
};

static void LocalTvtCallSelect_LoadResources(LocalTvtCallSelect *select, HeapID heapId);
static void LocalTvtCallSelect_FreeResources(LocalTvtCallSelect *select);
static int LocalTvtCallSelect_Touch(void *work);
static int LocalTvtCallSelect_Key(void *work);
static void LocalTvtCallSelect_DrawCursor(void *work, BOOL touchMode);
static void LocalTvtCallList_Init(LocalTvtCallList *list, const LocalTvtCallSelect *select, HeapID heapId);
static void LocalTvtCallList_Exit(LocalTvtCallList *list);
static void LocalTvtCallList_Main(LocalTvtCallList *list);
static void LocalTvtCallList_UpdateButton(LocalTvtCallList *list);
static BOOL LocalTvtCallList_Touch(LocalTvtCallList *list);
static BOOL LocalTvtCallList_Key(LocalTvtCallList *list);
static BOOL LocalTvtCallList_GetTouchedItem(LocalTvtCallList *list, int *index);
static int LocalTvtCallList_GetSelected(LocalTvtCallList *list);
static BOOL LocalTvtCallList_IsAnySelected(LocalTvtCallList *list);
static void LocalTvtCallList_DrawCursor(LocalTvtCallList *list, BOOL touchMode);
static BOOL LocalTvtCallList_IsButtonFlashing(LocalTvtCallList *list);
static BOOL LocalTvtCallList_IsButtonFlashFinished(LocalTvtCallList *list);
static BOOL LocalTvtCallList_IsScrolling(LocalTvtCallList *list);
static void LocalTvtCallItem_Init(LocalTvtCallItem *item, const LocalTvtCallSelect *select, ClActUnit *unit, int index,
                                  StrBuf *name, LocalTvtMessage *message, BOOL disabled, HeapID heapId);
static void LocalTvtCallItem_Exit(LocalTvtCallItem *item);
static void LocalTvtCallItem_Update(LocalTvtCallItem *item, u16 scroll);
static void LocalTvtCallItem_StartDecideAnim(LocalTvtCallItem *item);
static BOOL LocalTvtCallItem_SetSelected(LocalTvtCallItem *item, BOOL selected);
static BOOL LocalTvtCallItem_IsSelected(LocalTvtCallItem *item);
static void LocalTvtCallItem_SetBright(LocalTvtCallItem *item, BOOL bright);
static BOOL LocalTvtCallItem_IsBright(LocalTvtCallItem *item);
static void LocalTvtCallItem_SetCursor(LocalTvtCallItem *item, BOOL cursor);
static void LocalTvtCallScrollBar_Init(LocalTvtCallScrollBar *bar, const LocalTvtCallSelect *select, HeapID heapId);
static void LocalTvtCallScrollBar_Exit(LocalTvtCallScrollBar *bar);
static void LocalTvtCallScrollBar_Main(LocalTvtCallScrollBar *bar);
static BOOL LocalTvtCallScrollBar_Touch(LocalTvtCallScrollBar *bar);
static BOOL LocalTvtCallScrollBar_IsScrolling(LocalTvtCallScrollBar *bar);
static BOOL LocalTvtCallScrollBar_IsMoved(LocalTvtCallScrollBar *bar);
static fx32 LocalTvtCallScrollBar_GetRate(LocalTvtCallScrollBar *bar);
static void LocalTvtCallScrollBar_SetTarget(LocalTvtCallScrollBar *bar, fx32 rate);
static void LocalTvtCallButton_Init(LocalTvtCallButton *button, const LocalTvtCallSelect *select, HeapID heapId);
static void LocalTvtCallButton_Exit(LocalTvtCallButton *button);
static void LocalTvtCallButton_Main(LocalTvtCallButton *button);
static void LocalTvtCallButton_Update(LocalTvtCallButton *button, LocalTvtCallList *list);
static void LocalTvtCallButton_Decide(LocalTvtCallButton *button);
static void LocalTvtCallButton_SetShown(LocalTvtCallButton *button, BOOL shown);
static void LocalTvtCallButton_SetActive(LocalTvtCallButton *button, BOOL active);
static BOOL LocalTvtCallButton_IsShown(LocalTvtCallButton *button);
static BOOL LocalTvtCallButton_IsFlashing(LocalTvtCallButton *button);
static BOOL LocalTvtCallButton_IsFlashFinished(LocalTvtCallButton *button);

LocalTvtCallSelect *LocalTvtCallSelect_Create(const LocalTvtCallSelectSetup *setup, HeapID heapId) {
    LocalTvtCallSelect *select =
        GFL_HeapAllocate(heapId, sizeof(LocalTvtCallSelect), TRUE, "local_tvt_callselect.c", 241);
    LocalTvtKeyTouchSetup keyTouchSetup;

    select->setup = *setup;
    LocalTvtCallSelect_LoadResources(select, heapId);
    select->returnBar = LocalTvtReturnBar_Create(select->setup.unit, heapId);
    LocalTvtCallList_Init(&select->list, select, heapId);
    sys_memset(&keyTouchSetup, 0, sizeof(keyTouchSetup));
    keyTouchSetup.work = select;
    keyTouchSetup.touch = LocalTvtCallSelect_Touch;
    keyTouchSetup.key = LocalTvtCallSelect_Key;
    keyTouchSetup.draw = LocalTvtCallSelect_DrawCursor;
    select->keyTouch = LocalTvtKeyTouch_Create(&keyTouchSetup, heapId);
    return select;
}

void LocalTvtCallSelect_Free(LocalTvtCallSelect *select) {
    LocalTvtKeyTouch_Free(select->keyTouch);
    LocalTvtCallList_Exit(&select->list);
    LocalTvtReturnBar_Free(select->returnBar);
    LocalTvtCallSelect_FreeResources(select);
    GFL_HeapFree(select);
}

void LocalTvtCallSelect_Main(LocalTvtCallSelect *select) {
    switch (select->seq) {
    case SEQ_SELECT:
        LocalTvtCallList_Main(&select->list);
        if (!LocalTvtCallList_IsScrolling(&select->list)) {
            if (select->setup.count != 0) {
                LocalTvtKeyTouch_Main(select->keyTouch);
            }
            LocalTvtReturnBar_Main(select->returnBar);
        }
        if (LocalTvtKeyTouch_IsDecided(select->keyTouch)) {
            select->seq = SEQ_CALL;
        }
        if (LocalTvtReturnBar_IsPressed(select->returnBar)) {
            select->seq = SEQ_EXIT;
        }
        break;
    case SEQ_CALL:
        if (LocalTvtCallList_IsButtonFlashFinished(&select->list)) {
            select->result = LOCAL_TVT_SELECT_CALL;
            select->seq = SEQ_END;
        }
        break;
    case SEQ_EXIT:
        if (LocalTvtReturnBar_IsAnimEnd(select->returnBar)) {
            select->result = LOCAL_TVT_SELECT_EXIT;
            select->seq = SEQ_END;
        }
        break;
    case SEQ_END:
        break;
    }
    LocalTvtCallList_UpdateButton(&select->list);
}

u32 LocalTvtCallSelect_GetResult(LocalTvtCallSelect *select) {
    return select->result;
}

int LocalTvtCallSelect_GetSelected(LocalTvtCallSelect *select) {
    return LocalTvtCallList_GetSelected(&select->list);
}

static void LocalTvtCallSelect_LoadResources(LocalTvtCallSelect *select, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARC_LOCAL_TVT_COMMON, heapId);

    LoadSysMsgBox(4, 0x140, 9, 0, heapId);
    select->palette = func_0204bc48(arc, 3, 1, 0, heapId);
    select->chars = func_0204b81c(arc, 25, FALSE, 1, heapId);
    select->cellAnims = func_0204bde0(arc, 19, 22, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 12, 4, 0, 0x2800, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 18, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadScr(4);
    GFL_ArcToolFree(arc);
}

static void LocalTvtCallSelect_FreeResources(LocalTvtCallSelect *select) {
    GFL_BGSysClearScr(4);
    GFL_BGSysLoadScr(4);
    func_0204be64(select->cellAnims);
    func_0204b98c(select->chars);
    func_0204bcd0(select->palette);
}

static int LocalTvtCallSelect_Touch(void *work) {
    LocalTvtCallSelect *select = work;
    int result = LOCAL_TVT_INPUT_NONE;

    if (LocalTvtCallList_Touch(&select->list)) {
        result = LOCAL_TVT_INPUT_MOVE;
    }
    if (LocalTvtCallList_IsButtonFlashing(&select->list)) {
        result = LOCAL_TVT_INPUT_DECIDE;
    }
    return result;
}

static int LocalTvtCallSelect_Key(void *work) {
    LocalTvtCallSelect *select = work;
    int result = LOCAL_TVT_INPUT_NONE;

    if (LocalTvtCallList_Key(&select->list)) {
        result = LOCAL_TVT_INPUT_MOVE;
    }
    if (LocalTvtCallList_IsButtonFlashing(&select->list)) {
        result = LOCAL_TVT_INPUT_DECIDE;
    }
    return result;
}

static void LocalTvtCallSelect_DrawCursor(void *work, BOOL touchMode) {
    LocalTvtCallSelect *select = work;

    LocalTvtCallList_DrawCursor(&select->list, touchMode);
}

static void LocalTvtCallList_Init(LocalTvtCallList *list, const LocalTvtCallSelect *select, HeapID heapId) {
    int i;
    WordSet *wordSet;

    sys_memset(list, 0, sizeof(LocalTvtCallList));
    list->count = select->setup.count;
    LocalTvtCallScrollBar_Init(&list->scrollBar, select, heapId);
    LocalTvtCallButton_Init(&list->button, select, heapId);
    wordSet = GFL_WordSetSystemCreateDefault(heapId);
    GFL_WordSetLoadStr(wordSet, 1, getPtrToRivalName(select->setup.rival));
    for (i = 0; i < list->count; i++) {
        StrBuf *format = GFL_MsgDataLoadStrbufNew(select->setup.msgData, select->setup.names[i]);
        StrBuf *name = GFL_StrBufCreate(64, heapId);
        BOOL disabled;

        GFL_WordSetFormatStrbuf(wordSet, name, format);
        disabled = select->setup.enabled[i] ? FALSE : TRUE;
        LocalTvtCallItem_Init(&list->items[i], select, select->setup.unit, i, name, select->setup.message, disabled,
                              heapId);
        GFL_StrBufFree(format);
        GFL_StrBufFree(name);
    }
    GFL_WordSetSystemFree(wordSet);
    GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, list->scroll);
}

static void LocalTvtCallList_Exit(LocalTvtCallList *list) {
    int i;

    GFL_BGSysClearScr(6);
    GFL_BGSysLoadScr(6);
    GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, 0);
    for (i = 0; i < list->count; i++) {
        LocalTvtCallItem_Exit(&list->items[i]);
    }
    LocalTvtCallButton_Exit(&list->button);
    LocalTvtCallScrollBar_Exit(&list->scrollBar);
    sys_memset(list, 0, sizeof(LocalTvtCallList));
}

static void LocalTvtCallList_Main(LocalTvtCallList *list) {
    int i;
    BOOL scrolled = FALSE;

    LocalTvtCallButton_Update(&list->button, list);
    if (LocalTvtCallScrollBar_IsMoved(&list->scrollBar)) {
        int range = (list->count - LIST_VISIBLE) * ITEM_HEIGHT;

        list->scroll = (LocalTvtCallScrollBar_GetRate(&list->scrollBar) * range) >> FX32_SHIFT;
        scrolled = TRUE;
    }
    LocalTvtCallScrollBar_Main(&list->scrollBar);
    if (list->changed) {
        list->changed = FALSE;
        if (!LocalTvtCallList_IsAnySelected(list)) {
            for (i = 0; i < list->count; i++) {
                if (!LocalTvtCallItem_IsBright(&list->items[i])) {
                    LocalTvtCallItem_SetBright(&list->items[i], TRUE);
                }
            }
        } else {
            for (i = 0; i < list->count; i++) {
                if (i != LocalTvtCallList_GetSelected(list)) {
                    LocalTvtCallItem_SetBright(&list->items[i], FALSE);
                }
            }
        }
    }
    for (i = 0; i < list->count; i++) {
        LocalTvtCallItem_Update(&list->items[i], list->scroll);
    }
    if (scrolled) {
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, list->scroll);
    }
}

static void LocalTvtCallList_UpdateButton(LocalTvtCallList *list) {
    LocalTvtCallButton_Main(&list->button);
}

// Selects the person touched, or takes the selection back
static BOOL LocalTvtCallList_Touch(LocalTvtCallList *list) {
    u32 x;
    u32 y;
    int i;
    BOOL wasSelected;

    if (func_0203dac8(&x, &y) && LocalTvtCallList_GetTouchedItem(list, &list->cursor)) {
        wasSelected = LocalTvtCallItem_IsSelected(&list->items[list->cursor]);
        if (LocalTvtCallItem_SetSelected(&list->items[list->cursor], wasSelected ? FALSE : TRUE)) {
            for (i = 0; i < list->count; i++) {
                if (list->cursor != i) {
                    LocalTvtCallItem_SetSelected(&list->items[i], FALSE);
                }
            }
            if (!wasSelected) {
                GFL_SndSEPlay(SEQ_SE_SYS_69);
            } else {
                GFL_SndSEPlay(SEQ_SE_SYS_70);
            }
            LocalTvtCallItem_StartDecideAnim(&list->items[list->cursor]);
            list->changed = TRUE;
            return TRUE;
        }
    }
    if (LocalTvtCallScrollBar_Touch(&list->scrollBar)) {
        return TRUE;
    }
    return FALSE;
}

// Moves the cursor over the persons and the button, and selects the person or presses the button at the cursor
static BOOL LocalTvtCallList_Key(LocalTvtCallList *list) {
    BOOL anySelected = LocalTvtCallList_IsAnySelected(list);
    BOOL decided = FALSE;
    BOOL moved = FALSE;
    u32 count = list->count;
    int i;
    BOOL wasSelected;

    if (LocalTvtCallButton_IsShown(&list->button)) {
        count++;
    }
    if (anySelected) {
        LocalTvtCallList_GetSelected(list);
    }
    if (list->cursorToButton) {
        list->cursorToButton = FALSE;
        moved = TRUE;
        list->cursor = list->count;
    }
    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        list->cursor--;
        if (list->cursor < 0) {
            list->cursor = count - 1;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        list->cursor++;
        list->cursor %= count;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        if (list->cursor == list->count) {
            LocalTvtCallButton_Decide(&list->button);
            moved = TRUE;
            decided = TRUE;
            list->changed = TRUE;
        } else {
            wasSelected = LocalTvtCallItem_IsSelected(&list->items[list->cursor]);
            if (LocalTvtCallItem_SetSelected(&list->items[list->cursor], wasSelected ? FALSE : TRUE)) {
                for (i = 0; i < list->count; i++) {
                    if (list->cursor != i) {
                        LocalTvtCallItem_SetSelected(&list->items[i], FALSE);
                    }
                }
                LocalTvtCallItem_StartDecideAnim(&list->items[list->cursor]);
                if (!wasSelected) {
                    list->cursorToButton = TRUE;
                    GFL_SndSEPlay(SEQ_SE_SYS_69);
                } else {
                    GFL_SndSEPlay(SEQ_SE_SYS_70);
                }
                moved = TRUE;
                decided = TRUE;
                list->changed = TRUE;
            }
        }
    }
    if (moved) {
        for (i = 0; i < list->count; i++) {
            if (list->cursor == i) {
                LocalTvtCallItem_SetCursor(&list->items[i], TRUE);
            } else {
                LocalTvtCallItem_SetCursor(&list->items[i], FALSE);
            }
        }
        if (list->count >= LIST_VISIBLE + 1) {
            if (list->cursor <= 0) {
                LocalTvtCallScrollBar_SetTarget(&list->scrollBar, 0);
            } else if (list->cursor >= LIST_VISIBLE) {
                LocalTvtCallScrollBar_SetTarget(&list->scrollBar, FX32_ONE);
            }
        }
        if (list->cursor == list->count) {
            LocalTvtCallButton_SetActive(&list->button, TRUE);
        } else {
            LocalTvtCallButton_SetActive(&list->button, FALSE);
        }
    }
    return decided;
}

static BOOL LocalTvtCallList_GetTouchedItem(LocalTvtCallList *list, int *index) {
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y) && x > 8 && x < 224 && y < 168) {
        int item = (y + list->scroll) / ITEM_HEIGHT;

        if (item >= 0 && item < list->count) {
            *index = item;
            return TRUE;
        }
    }
    return FALSE;
}

static int LocalTvtCallList_GetSelected(LocalTvtCallList *list) {
    int i;

    for (i = 0; i < list->count; i++) {
        if (LocalTvtCallItem_IsSelected(&list->items[i])) {
            return i;
        }
    }
    return 0;
}

static BOOL LocalTvtCallList_IsAnySelected(LocalTvtCallList *list) {
    int i;

    for (i = 0; i < list->count; i++) {
        if (LocalTvtCallItem_IsSelected(&list->items[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

// Shows the cursor of the keys, and hides it for the touch screen
static void LocalTvtCallList_DrawCursor(LocalTvtCallList *list, BOOL touchMode) {
    int i;

    if (touchMode == TRUE) {
        for (i = 0; i < list->count; i++) {
            LocalTvtCallItem_SetCursor(&list->items[i], FALSE);
        }
        return;
    }
    for (i = 0; i < list->count; i++) {
        if (list->cursor == i) {
            LocalTvtCallItem_SetCursor(&list->items[i], TRUE);
        } else {
            LocalTvtCallItem_SetCursor(&list->items[i], FALSE);
        }
    }
    if (list->cursor == list->count) {
        LocalTvtCallButton_SetActive(&list->button, TRUE);
    } else {
        LocalTvtCallButton_SetActive(&list->button, FALSE);
    }
    if (list->count >= LIST_VISIBLE + 1) {
        if (list->cursor <= 0) {
            LocalTvtCallScrollBar_SetTarget(&list->scrollBar, 0);
        } else if (list->cursor >= LIST_VISIBLE) {
            LocalTvtCallScrollBar_SetTarget(&list->scrollBar, FX32_ONE);
        }
    }
}

static BOOL LocalTvtCallList_IsButtonFlashing(LocalTvtCallList *list) {
    return LocalTvtCallButton_IsFlashing(&list->button);
}

static BOOL LocalTvtCallList_IsButtonFlashFinished(LocalTvtCallList *list) {
    return LocalTvtCallButton_IsFlashFinished(&list->button);
}

static BOOL LocalTvtCallList_IsScrolling(LocalTvtCallList *list) {
    return LocalTvtCallScrollBar_IsScrolling(&list->scrollBar);
}

static void LocalTvtCallItem_Init(LocalTvtCallItem *item, const LocalTvtCallSelect *select, ClActUnit *unit, int index,
                                  StrBuf *name, LocalTvtMessage *message, BOOL disabled, HeapID heapId) {
    ClActorSetup setup;

    sys_memset(item, 0, sizeof(LocalTvtCallItem));
    item->index = index;
    item->disabled = disabled;
    setup.bgPriority = 3;
    setup.x = 128;
    setup.y = index * ITEM_HEIGHT + 16;
    setup.sequence = LIST_ANIM_PLATE;
    setup.priority = 32;
    item->plate = func_0204c040(unit, select->chars, select->palette, select->cellAnims, &setup, 1, heapId);
    func_0204c124(item->plate, TRUE);
    func_0204c520(item->plate, TRUE);
    setup.x = 18;
    setup.sequence = LIST_ANIM_ICON;
    setup.priority = 16;
    item->icon = func_0204c040(unit, select->chars, select->palette, select->cellAnims, &setup, 1, heapId);
    func_0204c124(item->icon, TRUE);
    item->window = BmpWin_CreateDynamic(6, 3, index * 4 + 1, 25, 2, 15, 1);
    LocalTvtMessage_SetColor(message, 0x3c40);
    LocalTvtMessage_SetPosition(message, 6, 0, LOCAL_TVT_ALIGN_LEFT);
    LocalTvtMessage_Print(message, item->window, name);
    BmpWin_Transfer(item->window);
    LocalTvtCallItem_SetBright(item, TRUE);
}

static void LocalTvtCallItem_Exit(LocalTvtCallItem *item) {
    BmpWin_Free(item->window);
    func_0204c108(item->icon);
    func_0204c108(item->plate);
    sys_memset(item, 0, sizeof(LocalTvtCallItem));
}

// Moves the plate with the list
static void LocalTvtCallItem_Update(LocalTvtCallItem *item, u16 scroll) {
    static const u8 sLocalTvtCallItemPlateSequences[] = { LIST_ANIM_PLATE, LIST_ANIM_PLATE_CURSOR };
    ClActorPos pos;
    s16 y;

    if (!func_0204c560(item->plate)) {
        func_0204c488(item->plate, sLocalTvtCallItemPlateSequences[item->cursor]);
    }
    y = item->index * ITEM_HEIGHT - scroll;
    if (y >= -ITEM_HEIGHT && y <= 160) {
        pos.x = 128;
        pos.y = y + 16;
        func_0204c140(item->plate, &pos, 1);
        func_0204c124(item->plate, TRUE);
        pos.x = 18;
        func_0204c140(item->icon, &pos, 1);
        func_0204c124(item->icon, TRUE);
        // The position of something the plate has no more
        pos.x = 200;
    }
}

static void LocalTvtCallItem_StartDecideAnim(LocalTvtCallItem *item) {
    func_0204c488(item->plate, LIST_ANIM_PLATE_DECIDE);
}

// Returns whether it changed, which a person who cannot be called never does
static BOOL LocalTvtCallItem_SetSelected(LocalTvtCallItem *item, BOOL selected) {
    if (item->disabled == TRUE || item->selected == selected) {
        return FALSE;
    }
    item->selected = selected;
    LocalTvtCallItem_SetBright(item, TRUE);
    return TRUE;
}

static BOOL LocalTvtCallItem_IsSelected(LocalTvtCallItem *item) {
    return item->selected;
}

static void LocalTvtCallItem_SetBright(LocalTvtCallItem *item, BOOL bright) {
    item->bright = bright;
    if (bright && !item->disabled) {
        if (item->selected) {
            func_0204c488(item->icon, LIST_ANIM_ICON_SELECTED);
        } else {
            func_0204c488(item->icon, LIST_ANIM_ICON);
        }
    } else {
        func_0204c488(item->icon, LIST_ANIM_ICON_DARK);
    }
}

static BOOL LocalTvtCallItem_IsBright(LocalTvtCallItem *item) {
    return item->bright;
}

static void LocalTvtCallItem_SetCursor(LocalTvtCallItem *item, BOOL cursor) {
    static const u8 sLocalTvtCallItemCursorSequences[] = { LIST_ANIM_PLATE, LIST_ANIM_PLATE_CURSOR };

    item->cursor = cursor;
    if (func_0204c4a0(item->plate) != LIST_ANIM_PLATE_DECIDE) {
        func_0204c488(item->plate, sLocalTvtCallItemCursorSequences[item->cursor]);
    }
}

static void LocalTvtCallScrollBar_Init(LocalTvtCallScrollBar *bar, const LocalTvtCallSelect *select, HeapID heapId) {
    ClActorSetup setup;

    sys_memset(bar, 0, sizeof(LocalTvtCallScrollBar));
    if (select->setup.count > LIST_VISIBLE) {
        bar->enabled = TRUE;
    }
    setup.priority = 0;
    setup.bgPriority = 0;
    setup.x = 248;
    setup.y = 24;
    setup.sequence = LIST_ANIM_SCROLL_BAR;
    bar->actor = func_0204c040(select->setup.unit, select->chars, select->palette, select->cellAnims, &setup, 1,
                               heapId);
    if (bar->enabled) {
        func_0204c124(bar->actor, TRUE);
    } else {
        func_0204c124(bar->actor, FALSE);
    }
}

static void LocalTvtCallScrollBar_Exit(LocalTvtCallScrollBar *bar) {
    func_0204c108(bar->actor);
    sys_memset(bar, 0, sizeof(LocalTvtCallScrollBar));
}

static void LocalTvtCallScrollBar_Main(LocalTvtCallScrollBar *bar) {
    if (!bar->enabled) {
        return;
    }
    if (bar->moved) {
        ClActorPos pos;

        bar->moved = FALSE;
        pos.x = 248;
        pos.y = bar->pos + 24;
        func_0204c140(bar->actor, &pos, 1);
    }
    if (bar->scrolling) {
        if (bar->pos > bar->target) {
            if (bar->pos - bar->target < SCROLL_BAR_SPEED) {
                bar->pos = bar->target;
            } else {
                bar->pos -= SCROLL_BAR_SPEED;
            }
        } else if (bar->pos < bar->target) {
            if (bar->target - bar->pos < SCROLL_BAR_SPEED) {
                bar->pos = bar->target;
            } else {
                bar->pos += SCROLL_BAR_SPEED;
            }
        }
        if (bar->target == bar->pos) {
            bar->scrolling = FALSE;
        }
        bar->moved = TRUE;
    }
}

// The player takes the knob and drags it
static BOOL LocalTvtCallScrollBar_Touch(LocalTvtCallScrollBar *bar) {
    u32 x;
    u32 y;
    u32 heldX;
    u32 heldY;
    BOOL touched;
    BOOL held;

    if (!bar->enabled) {
        return FALSE;
    }
    touched = func_0203dac8(&x, &y);
    held = func_0203da84(&heldX, &heldY);
    if (touched && x >= 236 && x <= 260 && y >= bar->pos + 12 && y <= bar->pos + 36) {
        bar->held = TRUE;
        func_0204c488(bar->actor, LIST_ANIM_SCROLL_BAR_HELD);
        GFL_SndSEPlay(SEQ_SE_SYS_45);
        return TRUE;
    }
    if (held && bar->held) {
        if (heldX >= 236 && heldX <= 260 && heldY >= 12 && heldY <= 156) {
            int pos = heldY - 24;

            if (pos > SCROLL_BAR_RANGE) {
                pos = SCROLL_BAR_RANGE;
            } else if (pos < 0) {
                pos = 0;
            }
            bar->pos = pos;
            bar->moved = TRUE;
            return TRUE;
        }
    } else {
        bar->held = FALSE;
        func_0204c488(bar->actor, LIST_ANIM_SCROLL_BAR);
    }
    return FALSE;
}

static BOOL LocalTvtCallScrollBar_IsScrolling(LocalTvtCallScrollBar *bar) {
    return bar->scrolling;
}

static BOOL LocalTvtCallScrollBar_IsMoved(LocalTvtCallScrollBar *bar) {
    return bar->moved;
}

// How far down the knob is, up to FX32_ONE
static fx32 LocalTvtCallScrollBar_GetRate(LocalTvtCallScrollBar *bar) {
    return (bar->pos << FX32_SHIFT) / SCROLL_BAR_RANGE;
}

static void LocalTvtCallScrollBar_SetTarget(LocalTvtCallScrollBar *bar, fx32 rate) {
    bar->target = (SCROLL_BAR_RANGE * rate) >> FX32_SHIFT;
    bar->scrolling = TRUE;
}

static void LocalTvtCallButton_Init(LocalTvtCallButton *button, const LocalTvtCallSelect *select, HeapID heapId) {
    MsgData *msgData;

    sys_memset(button, 0, sizeof(LocalTvtCallButton));
    button->message = select->setup.message;
    button->heapId = heapId;
    button->res = AppTaskMenuRes_Create(4, 7, LocalTvtMessage_GetFont(button->message),
                                        LocalTvtMessage_GetPrintQueue(button->message), heapId);
    button->window = BmpWin_CreateDynamic(4, 1, 21, 21, 2, 15, 1);
    msgData = LocalTvtMessage_GetTalkMsgData(button->message);
    button->callStr = GFL_MsgDataLoadStrbufNew(msgData, 1);
    button->promptStr = GFL_MsgDataLoadStrbufNew(msgData, 0);
    LocalTvtMessage_SetColor(button->message, 0x44f);
    LocalTvtMessage_Print(button->message, button->window, button->promptStr);
    BmpWin_Transfer(button->window);
    LocalTvtMessage_DrawFrame(button->message, button->window);
    GFL_BGSysLoadScr(4);
}

static void LocalTvtCallButton_Exit(LocalTvtCallButton *button) {
    if (button->button != NULL) {
        AppTaskMenuWin_Free(button->button);
        button->button = NULL;
    }
    GFL_StrBufFree(button->promptStr);
    GFL_StrBufFree(button->callStr);
    BmpWin_Free(button->window);
    GFL_BGSysClearScr(4);
    GFL_BGSysLoadScr(4);
    GFL_BGSysMoveBGReq(4, BG_MOVE_SET_Y, 0);
    AppTaskMenuRes_Free(button->res);
    sys_memset(button, 0, sizeof(LocalTvtCallButton));
}

static void LocalTvtCallButton_Main(LocalTvtCallButton *button) {
    if (button->button != NULL) {
        if (!AppTaskMenuWin_IsFlashing(button->button) && AppTaskMenuWin_IsTouched(button->button) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            AppTaskMenuWin_SetFlashing(button->button, TRUE);
        }
        AppTaskMenuWin_Update(button->button);
    }
}

// The button is there while a person is selected
static void LocalTvtCallButton_Update(LocalTvtCallButton *button, LocalTvtCallList *list) {
    BOOL shown = LocalTvtCallList_IsAnySelected(list);

    if (button->shown != shown) {
        button->shown = shown;
        LocalTvtCallButton_SetShown(button, shown);
    }
}

static void LocalTvtCallButton_Decide(LocalTvtCallButton *button) {
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    AppTaskMenuWin_SetFlashing(button->button, TRUE);
}

// Takes the window off the screen, which is copied at the next VBlank
static inline void LocalTvtCallButton_ClearWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

static void LocalTvtCallButton_SetShown(LocalTvtCallButton *button, BOOL shown) {
    if (shown) {
        LocalTvtCallButton_ClearWindow(button->window);
        LocalTvtMessage_ClearFrame(button->message, button->window);
        if (button->button == NULL) {
            AppTaskMenuItem item;

            sys_memset(&item, 0, sizeof(item));
            item.str = button->callStr;
            item.color = 0x39e3;
            item.type = 0;
            button->button = AppTaskMenuWin_Create(button->res, &item, 0, 21, 21, button->heapId);
        }
    } else {
        if (button->button != NULL) {
            AppTaskMenuWin_Free(button->button);
            button->button = NULL;
        }
        LocalTvtMessage_SetColor(button->message, 0x44f);
        LocalTvtMessage_Print(button->message, button->window, button->promptStr);
        BmpWin_Transfer(button->window);
        LocalTvtMessage_DrawFrame(button->message, button->window);
    }
    GFL_BGSysLoadScr(4);
}

static void LocalTvtCallButton_SetActive(LocalTvtCallButton *button, BOOL active) {
    if (LocalTvtCallButton_IsShown(button)) {
        button->active = active;
        AppTaskMenuWin_SetActive(button->button, active);
    }
}

static BOOL LocalTvtCallButton_IsShown(LocalTvtCallButton *button) {
    if (button->button != NULL) {
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtCallButton_IsFlashing(LocalTvtCallButton *button) {
    if (button->button != NULL) {
        return AppTaskMenuWin_IsFlashing(button->button);
    }
    return FALSE;
}

static BOOL LocalTvtCallButton_IsFlashFinished(LocalTvtCallButton *button) {
    if (button->button != NULL) {
        return AppTaskMenuWin_IsFlashFinished(button->button);
    }
    return FALSE;
}
