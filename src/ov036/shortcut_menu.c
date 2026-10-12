// The Y Button's shortcut menu: the list of the shortcuts registered in the save, which the player scrolls, confirms
// with Y, and reorders with SELECT
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/shortcut_menu.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/ui.h"
#include "save/shortcut.h"
#include "system/bmp_menulist.h"
#include "system/bmp_menuwork.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/gf_font.h"
#include "system/printsys.h"

#define BG 1
#define ACTORS 3

// What the menu does next
enum {
    STATE_CLOSED,
    STATE_OPENING,
    STATE_SCROLL_IN,
    STATE_INPUT,
    STATE_CLOSING,
    STATE_SCROLL_OUT,
};

typedef struct ShortcutMenuApp {
    ListMenuOption *options;
    BmpMenuList *list;
    BmpWin *window;
    PrintQueue *queue;
    PrintWindow printWindow;
    u32 input;
    u16 listTop;
    u16 cursorRow;
    ShortcutSave *shortcut;
    u32 mode;
    ClActor *actors[ACTORS];
    HeapID heapId;
    MsgData *msgData;
    Font *font;
    u32 scrollDir;
    u32 cursorMoved;
    u32 showActors;
    ClActUnit *unit;
    u16 palette;
    u16 chars;
    u16 cellAnims;
    ClActor *clactActors[ACTORS];
    MsgData *menuMsgData;
    Font *menuFont;
    PrintQueue *menuQueue;
    u16 state;
    u16 scroll;
    u32 code;
    u32 item;
    u16 *listPos;
    u32 scrolling;
    TCB *tcb;
} ShortcutMenuApp;

// What the event that puts the field back after the menu runs on
typedef struct {
    GameSystem *gsys;
    Field *field;
    void *param;
    BOOL transition;
} ShortcutMenuReturn;

static const BGSetup sBgSetup = {
    0, 0x1000, 2, 0x40000, 0, 0, 0, 0, 0x80, 0, 0, 0, 0,
};

static const u32 sActorX[ACTORS] = { 0x80, 0xa, 0xf6 };

static const BmpMenuListHeader sListHeader = {
    NULL, NULL, NULL, 5, 13, 0, 24, 47, 0, 0, 0, 0, 0, 0, BMPMENULIST_SKIP_LR_KEY, 0, 0, NULL, 12, 16, NULL,
    NULL, NULL, NULL, 0,
};

// The subprocess of overlay 0x118 that the menu returns through, whose work it leaves empty
static GameProcFunctions sReturnFunctions = { NULL, NULL, NULL };

static void FreeList(ShortcutMenuApp *work, u16 *listTop, u16 *cursorRow);
static void ClampListPos(u16 *listTop, u16 *cursorRow, u16 count);
static void CreateList(ShortcutMenuApp *work, u16 listTop, u16 cursorRow, HeapID heapId);
static void SetActorsY(ShortcutMenuApp *work, u32 y);
static void ScrollBgIn(TCB *tcb, void *data);
static BOOL PrintDone(ShortcutMenuApp *work);
s32 GetInput(ShortcutMenuApp *work);
static void OnListCursor(BmpMenuList *list, s32 value, u8 init);
static void RedrawHighlight(BmpMenuList *list, u8 a1, u8 a2);
static void OnCursorUp(ShortcutMenuApp *work, u16 value, u32 count, u32 *handled);
static void OnCursorDown(ShortcutMenuApp *work, u16 value, u32 count, u32 *handled);
static void ReadInput(ShortcutMenuApp *work);
static void SetupMenu(ShortcutMenuApp *work, u16 *listPos, MsgData *msgData, Font *font, PrintQueue *queue,
                      ShortcutSave *shortcut, ClActor **actors, HeapID heapId);
static void FreeMenu(ShortcutMenuApp *work, u16 *listPos);
static GameEventReturnCode ReturnToField_Callback(GameEvent *event, u32 *state, void *data);

void *func_ov036_021bedd8(GameData *gameData, u32 mode, u16 *listPos, HeapID heapId, u32 unused) {
    ClActorSetup setup;
    ShortcutMenuApp *work;
    ShortcutSave *shortcut;
    ArcTool *arc;
    int i;

    work = GFL_HeapAllocate(heapId, sizeof(*work), FALSE, "shortcut_menu.c", 253);
    sys_memset(work, 0, sizeof(*work));
    work->code = 0;
    work->item = 0xff;
    work->listPos = listPos;
    GFL_BGSysCreateBG(BG, &sBgSetup, 0);
    GFL_BGSysClearBG(BG);
    GFL_BGSysFillChar(BG, 0, 1, 0);
    work->unit = func_0204bf1c(8, 0, heapId);

    arc = GFL_ArcSysCreateFileHandle(0x81, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0x160, 0x40, heapId);
    work->palette = func_0204bbb8(arc, 4, 0, 0x40, 0, 1, heapId);
    work->chars = func_0204b81c(arc, 3, FALSE, 0, heapId);
    work->cellAnims = func_0204bde0(arc, 2, 1, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0x140, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, BG, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, BG, 0, 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(BG, 0, 0xb, 0x20, 1, 0xa);
    GFL_ArcToolFree(arc);

    sys_memset(&setup, 0, sizeof(setup));
    setup.y = 0x60;
    for (i = 0; i < ACTORS; i++) {
        setup.sequence = i;
        setup.x = sActorX[i];
        work->clactActors[i] = func_0204c040(work->unit, work->chars, work->palette, work->cellAnims, &setup, 0,
                                             heapId);
        func_0204c124(work->clactActors[i], FALSE);
        func_0204c520(work->clactActors[i], TRUE);
    }

    work->menuMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_4, heapId);
    work->menuFont = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    work->menuQueue = func_020219a8(0x1000, heapId);
    shortcut = SaveControl_GetShortcutSaveCore(GameData_GetSaveControl(gameData));
    SetupMenu(work, work->listPos, work->menuMsgData, work->menuFont, work->menuQueue, shortcut, work->clactActors,
              heapId);
    GFL_BGSysQueueScrLoad(BG);
    gfxRegSetAlphaBlend(0x4000050, 2, 0x11, 0xc, 4);

    if (mode == 0) {
        GFL_BGSysMoveBG(BG, BG_MOVE_SET_Y, -0xc0);
        GFL_BGSysSetBGEnabled(BG, FALSE);
        work->state = STATE_CLOSED;
    } else if (mode == 1) {
        GFL_BGSysMoveBG(BG, BG_MOVE_SET_Y, 0);
        GFL_BGSysSetBGEnabled(BG, TRUE);
        work->state = STATE_INPUT;
    }
    return work;
}

void func_ov036_021bf004(void *app) {
    ShortcutMenuApp *work = app;
    int i;

    *(u16 *)0x4000050 = 0;
    GFL_BGSysSetBGEnabled(BG, FALSE);
    FreeMenu(work, work->listPos);
    for (i = 0; i < ACTORS; i++) {
        func_0204c108(work->clactActors[i]);
    }
    func_02021c44(work->menuQueue);
    func_02021a18(work->menuQueue);
    GFL_FontFree(work->menuFont);
    GFL_MsgDataFree(work->menuMsgData);
    func_0204bcd0(work->palette);
    func_0204b98c(work->chars);
    func_0204be64(work->cellAnims);
    func_0204bf98(work->unit);
    GFL_BGSysFreeFilledChar(BG, 1, 0);
    GFL_BGSysReleaseBG(BG);
    GFL_HeapFree(work);
}

void func_ov036_021bf088(void *app) {
    ShortcutMenuApp *work = app;
    int i;

    if (!PrintDone(work)) {
        return;
    }
    switch (work->state) {
    case STATE_OPENING:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        work->scroll = 0;
        work->scrolling = FALSE;
        work->tcb = GFL_VBlankTCBAdd(ScrollBgIn, work, 0);
        work->state = STATE_SCROLL_IN;
        break;
    case STATE_SCROLL_IN:
        if (work->scrolling) {
            GFL_TCBRemove(work->tcb);
            work->state = STATE_INPUT;
        }
        break;
    case STATE_INPUT: {
        s32 input;

        ReadInput(work);
        input = GetInput(work);
        switch (input) {
        case BMPMENULIST_NULL:
            work->code = 0;
            break;
        case BMPMENULIST_CANCEL:
            work->code = 2;
            break;
        default:
            work->code = 1;
            work->item = input;
            break;
        }
        break;
    }
    case STATE_CLOSING:
        for (i = 0; i < ACTORS; i++) {
            func_0204c124(work->clactActors[i], FALSE);
        }
        work->scroll = 4;
        work->state = STATE_SCROLL_OUT;
        break;
    case STATE_SCROLL_OUT:
        GFL_BGSysMoveBG(BG, BG_MOVE_SET_Y, work->scroll * 0xc0 / 4 - 0xc0);
        if (work->scroll-- == 0) {
            work->state = STATE_CLOSED;
        }
        break;
    }
}

u32 func_ov036_021bf198(void *app) {
    ShortcutMenuApp *work = app;

    func_02021a3c(work->menuQueue);
    return PrintDone(work);
}

void func_ov036_021bf1ac(void *app) {
    ShortcutMenuApp *work = app;

    if (work->state == STATE_CLOSED) {
        work->code = 0;
        work->item = 0xff;
        GFL_BGSysSetBGEnabled(BG, TRUE);
        work->state = STATE_OPENING;
    }
}

void func_ov036_021bf1d0(void *app) {
    ShortcutMenuApp *work = app;

    if (work->state == STATE_INPUT) {
        work->state = STATE_CLOSING;
    }
}

BOOL func_ov036_021bf1e4(void *app) {
    ShortcutMenuApp *work = app;
    BOOL busy = TRUE;

    if (work->state != STATE_SCROLL_IN && work->state != STATE_SCROLL_OUT) {
        busy = FALSE;
    }
    return busy;
}

u32 func_ov036_021bf1f8(void *app, u32 *item) {
    ShortcutMenuApp *work = app;

    if (item != NULL) {
        *item = work->item;
    }
    return work->code;
}

static void SetupMenu(ShortcutMenuApp *work, u16 *listPos, MsgData *msgData, Font *font, PrintQueue *queue,
                      ShortcutSave *shortcut, ClActor **actors, HeapID heapId) {
    int i;

    BmpWin *window;

    sys_memset_fast(work, 0, 0x4c);
    work->queue = queue;
    work->shortcut = shortcut;
    work->heapId = heapId;
    work->msgData = msgData;
    work->font = font;
    for (i = 0; i < ACTORS; i++) {
        work->actors[i] = actors[i];
    }
    window = BmpWin_CreateDynamic(BG, 1, 0xb, 0x1e, 0xc, 0xb, TRUE);
    work->window = window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    work->printWindow.window = window;
    work->printWindow.flushPending = FALSE;
    CreateList(work, listPos[0], listPos[1], heapId);
}

static void FreeMenu(ShortcutMenuApp *work, u16 *listPos) {
    FreeList(work, listPos, listPos + 1);
    BmpWin_Free(work->window);
    sys_memset(work, 0, 0x4c);
}

static void OnCursorUp(ShortcutMenuApp *work, u16 value, u32 count, u32 *handled) {
    if (value == 0) {
        s32 maxShown = BmpMenuList_GetParam(work->list, BMPMENULIST_PARAM_MAX_SHOWN);

        FreeList(work, NULL, NULL);
        CreateList(work, (count - 1) - (maxShown - 1), maxShown - 1, work->heapId);
        if (work->mode == 1) {
            work->scrollDir = 1;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        *handled = TRUE;
    } else if (work->scrollDir == 1 && work->mode == 1) {
        u16 cursorRow;
        u16 y;

        BmpMenuList_GetPos(work->list, NULL, &cursorRow);
        y = BmpMenuList_GetRowY(work->list, cursorRow) >> 3;
        SetActorsY(work, (y + 12) * 8);
        work->scrollDir = 0;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        *handled = TRUE;
    }
}

static void OnCursorDown(ShortcutMenuApp *work, u16 value, u32 count, u32 *handled) {
    if (value == count - 1) {
        if (work->mode == 1) {
            u16 cursorRow;
            u16 y;

            BmpMenuList_GetPos(work->list, NULL, &cursorRow);
            y = BmpMenuList_GetRowY(work->list, cursorRow + 1) >> 3;
            SetActorsY(work, (y + 12) * 8);
            work->scrollDir ^= 1;
        }
        if (work->scrollDir == 0) {
            FreeList(work, NULL, NULL);
            CreateList(work, 0, 0, work->heapId);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        *handled = TRUE;
    } else {
        work->scrollDir = 0;
    }
}

static void ReadInput(ShortcutMenuApp *work) {
    s32 count;
    u16 listTop;
    u16 cursorRow;
    u16 index;
    u16 selectIndex;
    u16 buttonIndex;
    u16 cursorIndex;
    u32 handled = FALSE;
    s32 result = BMPMENULIST_NULL;

    func_0203d564(FALSE);
    BmpMenuList_GetCursorIndex(work->list, &cursorIndex);
    count = BmpMenuList_GetParam(work->list, BMPMENULIST_PARAM_COUNT);

    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        OnCursorUp(work, cursorIndex, count, &handled);
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        OnCursorDown(work, cursorIndex, count, &handled);
    }
    if (!handled) {
        result = BmpMenuList_Update(work->list);
    }
    if (result == BMPMENULIST_NULL) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            result = BMPMENULIST_CANCEL;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
            BmpMenuList_GetCursorIndex(work->list, &buttonIndex);
            result = BmpMenuList_GetOptionValue(work->list, buttonIndex);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && work->mode == 1) {
            BmpMenuList_GetCursorIndex(work->list, &selectIndex);
            result = BmpMenuList_GetOptionValue(work->list, selectIndex);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
    if (work->mode == 0) {
        work->input = result;
    } else if (work->mode == 1 && result != BMPMENULIST_NULL) {
        if (result != 0) {
            u8 newIndex;

            result = BmpMenuList_GetOptionValue(work->list, work->listTop + work->cursorRow);
            BmpMenuList_GetCursorIndex(work->list, &index);
            if (work->scrollDir != 0) {
                newIndex = 0xff;
            } else {
                newIndex = (u8)index;
            }
            result = ShortcutSave_MoveItemToIndex(work->shortcut, result, newIndex);
            FreeList(work, &listTop, &cursorRow);
            if (index - 1 == result) {
                cursorRow--;
            }
            CreateList(work, listTop, cursorRow, work->heapId);
        }
        handled = TRUE;
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && work->mode == 0
        && BmpMenuList_GetParam(work->list, BMPMENULIST_PARAM_COUNT) > 1) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        handled = TRUE;
    }
    if (handled) {
        work->mode ^= 1;
        if (work->mode == 0) {
            int i;

            work->scrollDir = 0;
            for (i = 0; i < ACTORS; i++) {
                func_0204c124(work->actors[i], FALSE);
            }
            RedrawHighlight(work->list, FALSE, FALSE);
        } else {
            BmpMenuList_GetPos(work->list, &work->listTop, &work->cursorRow);
            work->showActors = TRUE;
        }
    }
}

static BOOL PrintDone(ShortcutMenuApp *work) {
    PrintQueue *queue = work->queue;
    BOOL done = TRUE;

    if (work->printWindow.flushPending && !func_02021c1c(queue, BmpWin_GetBitmap(work->printWindow.window))) {
        BmpWin_FlushChar(work->printWindow.window);
        work->printWindow.flushPending = FALSE;
    }
    done = (work->printWindow.flushPending == 0);
    if (done) {
        int i;

        if (work->cursorMoved) {
            RedrawHighlight(work->list, 0, 0);
            work->cursorMoved = 0;
        }
        if (work->showActors) {
            for (i = 0; i < ACTORS; i++) {
                func_0204c124(work->actors[i], TRUE);
            }
            work->showActors = 0;
        }
    }
    return done;
}

s32 GetInput(ShortcutMenuApp *work) {
    return work->input;
}

static void OnListCursor(BmpMenuList *list, s32 value, u8 init) {
    ShortcutMenuApp *work = BmpMenuList_GetParam(list, BMPMENULIST_PARAM_WORK);

    work->cursorMoved = 1;
}

static void RedrawHighlight(BmpMenuList *list, u8 a1, u8 a2) {
    ShortcutMenuApp *work = BmpMenuList_GetParam(list, BMPMENULIST_PARAM_WORK);
    u16 cursorY = BmpMenuList_GetCursorY(list) >> 3;
    u32 rowHeight = (u32)BmpMenuList_GetParam(list, BMPMENULIST_PARAM_ROW_HEIGHT) >> 3;

    if (work->mode == 0) {
        GFL_BGSysSetScrPaletteNo(BG, 1, 0xc, 0x1e, 0xc, 0xb);
        GFL_BGSysSetScrPaletteNo(BG, 1, cursorY + 0xc, 0x1e, rowHeight, 0xc);
        GFL_BGSysSetScrPaletteNo(BG, 0, 0xb, 0x20, 1, 0xa);
        GFL_BGSysQueueScrLoad(BG);
    } else if (work->mode == 1) {
        u16 listTop;
        u16 cursorRow;
        u32 listY;
        u32 rowH;
        u32 y;

        GFL_BGSysSetScrPaletteNo(BG, 1, 0xc, 0x1e, 0xc, 0xb);
        BmpMenuList_GetPos(list, &listTop, &cursorRow);
        listY = BmpMenuList_GetParam(list, BMPMENULIST_PARAM_Y);
        rowH = BmpMenuList_GetParam(list, BMPMENULIST_PARAM_ROW_HEIGHT);
        y = listY + rowH * (work->cursorRow + work->listTop - listTop);
        GFL_BGSysSetScrPaletteNo(BG, 1, y / 8 + 0xc, 0x1e, rowHeight, 0xc);
        GFL_BGSysSetScrPaletteNo(BG, 0, 0xb, 0x20, 1, 0xa);
        GFL_BGSysQueueScrLoad(BG);
    }
    if (work->scrollDir == 1) {
        u16 rowY = BmpMenuList_GetRowY(list, BmpMenuList_GetParam(list, BMPMENULIST_PARAM_MAX_SHOWN)) >> 3;

        SetActorsY(work, (rowY + 0xc) * 8);
    } else {
        SetActorsY(work, (cursorY + 0xc) * 8);
    }
}

static void ClampListPos(u16 *listTop, u16 *cursorRow, u16 count) {
    s32 total = *listTop + *cursorRow;

    if (count <= 5) {
        *cursorRow = total;
        *listTop = 0;
    } else if (total <= 3) {
        *cursorRow = total;
        *listTop = 0;
    } else if (total <= (s32)count - 2 && total > 3) {
        *listTop = total - 3;
        *cursorRow = 3;
    } else if (total == count - 1) {
        *listTop = total - 4;
        *cursorRow = 4;
    }
}

static void CreateList(ShortcutMenuApp *work, u16 listTop, u16 cursorRow, HeapID heapId) {
    BmpMenuListHeader header;
    u32 count = ShortcutSave_GetShortcutCount(work->shortcut);
    u32 i;

    work->options = ListMenuCore_CreateOptionList(count, heapId);
    for (i = 0; i < count; i++) {
        u32 value = ShortcutSave_GetRegistItem(work->shortcut, i);

        ListMenuCore_AppendMsgOption(work->options, work->msgData, value, value, heapId);
    }
    {
        u16 row = cursorRow;
        u16 top = listTop;

        while (row + top >= count) {
            if (row != 0) {
                row--;
            } else if (top != 0) {
                top--;
            }
        }
        listTop = top;
        cursorRow = row;
    }
    ClampListPos(&listTop, &cursorRow, count);

    header = sListHeader;
    header.options = work->options;
    header.count = count;
    header.work = work;
    header.cursorCallback = OnListCursor;
    header.printCallback = NULL;
    header.msgData = work->msgData;
    header.printWindow = &work->printWindow;
    header.queue = work->queue;
    header.font = work->font;
    work->list = BmpMenuList_Create(&header, listTop, cursorRow, heapId);
}

static void FreeList(ShortcutMenuApp *work, u16 *listTop, u16 *cursorRow) {
    BmpMenuList_Free(work->list, listTop, cursorRow);
    ListMenuCore_FreeOptionList(work->options);
}

static void ScrollBgIn(TCB *tcb, void *data) {
    ShortcutMenuApp *work = data;
    s32 y = work->scroll * 0xc0 / 4 - 0xc0;

    if (work->scroll++ >= 4) {
        y = 0;
        work->scrolling = TRUE;
    }
    GFL_BGSysMoveBG(BG, BG_MOVE_SET_Y, y);
}

static void SetActorsY(ShortcutMenuApp *work, u32 y) {
    ClActorPos pos;
    int i;

    for (i = 0; i < ACTORS; i++) {
        func_0204c178(work->actors[i], &pos, 0);
        pos.y = y;
        func_0204c140(work->actors[i], &pos, 0);
    }
}

GameEvent *func_ov036_021bf890(GameSystem *gsys, Field *field, void *param, BOOL transition) {
    ShortcutMenuReturn *ret;
    GameEvent *event;

    event = GameEvent_Create(gsys, NULL, ReturnToField_Callback, sizeof(*ret));
    ret = GameEvent_GetData(event);
    ret->gsys = gsys;
    ret->field = field;
    ret->param = param;
    ret->transition = transition;
    return event;
}

static GameEventReturnCode ReturnToField_Callback(GameEvent *event, u32 *state, void *data) {
    ShortcutMenuReturn *ret = data;

    switch (*state) {
    case 0:
        if (ret->transition) {
            GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(ret->gsys, ret->field, OVERLAY_ID(280),
                                                                             &sReturnFunctions, ret->param));
        } else {
            GameEvent_ChainNext(event, EventFieldSubprocessCall_Create(ret->gsys, ret->field, OVERLAY_ID(280),
                                                                       &sReturnFunctions, ret->param));
        }
        (*state)++;
        break;
    case 1:
        return TRUE;
    }
    return FALSE;
}
