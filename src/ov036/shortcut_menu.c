// The Y Button's shortcut menu: the list of the shortcuts registered in the save, which the player scrolls, confirms
// with Y, and reorders with SELECT
#include "types.h"
#include "app/name_entry.h"
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
#include "nitro/gx.h"
#include "save/shortcut.h"
#include "system/bmp_menulist.h"
#include "system/bmp_menuwork.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/shortcut_util.h"

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

// What the list's buttons do: pick a shortcut, or move the one picked to where the cursor goes
enum {
    LIST_MODE_SELECT,
    LIST_MODE_REORDER,
};

// The list of the shortcuts, which SetupMenu makes and FreeMenu clears
typedef struct {
    ListMenuOption *options;
    BmpMenuList *list;
    BmpWin *window;
    PrintQueue *queue;
    PrintWindow printWindow;
    u32 input;
    u16 listTop;
    u16 cursorRow;
    ShortcutSave *shortcut;
    // A LIST_MODE_ value, which Select toggles
    u32 mode;
    ClActor *actors[ACTORS];
    HeapID heapId;
    MsgData *msgData;
    Font *font;
    u32 scrollDir;
    u32 cursorMoved;
    u32 showActors;
} ShortcutMenuList;

struct ShortcutMenuApp {
    ShortcutMenuList menu;
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
    ShortcutMenuPos *listPos;
    u32 scrolling;
    TCB *tcb;
};

// What the event that puts the field back after the menu runs on
typedef struct {
    GameSystem *gsys;
    Field *field;
    void *param;
    BOOL transition;
} ShortcutMenuReturn;

static const BGSetup sBgSetup = {
    0,
    0,
    0x1000,
    0,
    BGRES_256x512,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE_0x0000,
    GX_BG_CHARBASE_0x10000,
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const u32 sActorX[ACTORS] = { 0x80, 0xa, 0xf6 };

static const BmpMenuListHeader sListHeader = {
    NULL, NULL, NULL, 0,    5,    0,    13,   0, 8, 1, 15, 2, 0, 0, BMPMENULIST_SKIP_NONE, 0, BMPMENULIST_CURSOR_HIDE,
    NULL, 12,   16,   NULL, NULL, NULL, NULL, 0,
};

static void FreeList(ShortcutMenuList *menu, u16 *listTop, u16 *cursorRow);
static void ClampListPos(u16 *listTop, u16 *cursorRow, u16 count);
static void CreateList(ShortcutMenuList *menu, u16 listTop, u16 cursorRow, HeapID heapId);
static void SetActorsY(ShortcutMenuList *menu, u32 y);
static void ScrollBgIn(TCB *tcb, void *data);
static BOOL PrintDone(ShortcutMenuList *menu);
static s32 GetInput(ShortcutMenuList *menu);
static void OnListCursor(BmpMenuList *list, s32 value, u8 init);
static void RedrawHighlight(BmpMenuList *list, u8 unused1, u8 unused2);
static void OnCursorUp(ShortcutMenuList *menu, u16 value, u32 count, u32 *handled);
static void OnCursorDown(ShortcutMenuList *menu, u16 value, u32 count, u32 *handled);
static void ReadInput(ShortcutMenuList *menu);
static void SetupMenu(ShortcutMenuList *menu, const ShortcutMenuPos *listPos, MsgData *msgData, Font *font,
                      PrintQueue *queue, ShortcutSave *shortcut, ClActor **actors, HeapID heapId);
static void FreeMenu(ShortcutMenuList *menu, ShortcutMenuPos *listPos);
static GameEventReturnCode ReturnToField_Callback(GameEvent *event, u32 *state, void *data);

ShortcutMenuApp *ShortcutMenu_Create(GameData *gameData, u32 mode, ShortcutMenuPos *listPos, HeapID menuHeapId,
                                     HeapID heapId) {
    ClActorSetup setup;
    ShortcutMenuApp *work;
    ShortcutSave *shortcut;
    ArcTool *arc;
    int i;

    work = GFL_HeapAllocate(heapId, sizeof(*work), FALSE, "shortcut_menu.c", 253);
    MI_CpuFill8(work, 0, sizeof(*work));
    work->code = 0;
    work->item = 0xff;
    work->listPos = listPos;
    GFL_BGSysCreateBG(BG, &sBgSetup, BGMODE_TEXT);
    GFL_BGSysClearBG(BG);
    GFL_BGSysFillChar(BG, 0, 1, 0);
    work->unit = ClActUnit_Create(8, 0, heapId);

    arc = GFL_ArcSysCreateFileHandle(ARCID_SHORTCUT_MENU, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0x160, 0x40, heapId);
    work->palette = ClActSys_LoadPlttRange(arc, 4, 0, 0x40, 0, 1, heapId);
    work->chars = ClActSys_LoadChars(arc, 3, FALSE, 0, heapId);
    work->cellAnims = ClActSys_LoadCellAnims(arc, 2, 1, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0x140, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, BG, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, BG, 0, 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(BG, 0, 0xb, 0x20, 1, 0xa);
    GFL_ArcToolFree(arc);

    MI_CpuFill8(&setup, 0, sizeof(setup));
    setup.y = 0x60;
    for (i = 0; i < ACTORS; i++) {
        setup.sequence = i;
        setup.x = sActorX[i];
        work->clactActors[i] =
            ClActor_Create(work->unit, work->chars, work->palette, work->cellAnims, &setup, 0, heapId);
        ClActor_SetVisible(work->clactActors[i], FALSE);
        ClActor_SetAutoAnim(work->clactActors[i], TRUE);
    }

    work->menuMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_4, heapId);
    work->menuFont = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, menuHeapId);
    work->menuQueue = PrintQueue_Create(0x1000, heapId);
    shortcut = SaveControl_GetShortcutSaveCore(GameData_GetSaveControl(gameData));
    SetupMenu(&work->menu, work->listPos, work->menuMsgData, work->menuFont, work->menuQueue, shortcut,
              work->clactActors, menuHeapId);
    GFL_BGSysQueueScrLoad(BG);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_OBJ, 12, 4);

    switch (mode) {
    case SHORTCUT_MENU_START_CLOSED:
        GFL_BGSysMoveBG(BG, BG_MOVE_SET_Y, -0xc0);
        GFL_BGSysSetBGEnabled(BG, FALSE);
        work->state = STATE_CLOSED;
        break;
    case SHORTCUT_MENU_START_OPEN:
        GFL_BGSysMoveBG(BG, BG_MOVE_SET_Y, 0);
        GFL_BGSysSetBGEnabled(BG, TRUE);
        work->state = STATE_INPUT;
        break;
    }
    return work;
}

void ShortcutMenu_Delete(ShortcutMenuApp *work) {
    int i;

    G2_BlendNone();
    GFL_BGSysSetBGEnabled(BG, FALSE);
    FreeMenu(&work->menu, work->listPos);
    for (i = 0; i < ACTORS; i++) {
        ClActor_Delete(work->clactActors[i]);
    }
    PrintQueue_Clear(work->menuQueue);
    PrintQueue_Free(work->menuQueue);
    GFL_FontFree(work->menuFont);
    GFL_MsgDataFree(work->menuMsgData);
    ClActSys_FreePltt(work->palette);
    ClActSys_FreeChars(work->chars);
    ClActSys_FreeCellAnims(work->cellAnims);
    ClActUnit_Delete(work->unit);
    GFL_BGSysFreeFilledChar(BG, 1, 0);
    GFL_BGSysReleaseBG(BG);
    GFL_HeapFree(work);
}

void ShortcutMenu_Main(ShortcutMenuApp *work) {
    int i;

    if (!ShortcutMenu_RunPrint(work)) {
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

        ReadInput(&work->menu);
        input = GetInput(&work->menu);
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
            ClActor_SetVisible(work->clactActors[i], FALSE);
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

u32 ShortcutMenu_RunPrint(ShortcutMenuApp *work) {
    PrintQueue_Main(work->menuQueue);
    return PrintDone(&work->menu);
}

void ShortcutMenu_Start(ShortcutMenuApp *work) {
    if (work->state == STATE_CLOSED) {
        work->code = 0;
        work->item = 0xff;
        GFL_BGSysSetBGEnabled(BG, TRUE);
        work->state = STATE_OPENING;
    }
}

void ShortcutMenu_RequestClose(ShortcutMenuApp *work) {
    if (work->state == STATE_INPUT) {
        work->state = STATE_CLOSING;
    }
}

BOOL ShortcutMenu_IsWaitingInput(ShortcutMenuApp *work) {
    BOOL busy = TRUE;

    if (work->state != STATE_SCROLL_IN && work->state != STATE_SCROLL_OUT) {
        busy = FALSE;
    }
    return busy;
}

u32 ShortcutMenu_GetResult(ShortcutMenuApp *work, u32 *item) {
    if (item != NULL) {
        *item = work->item;
    }
    return work->code;
}

static void SetupMenu(ShortcutMenuList *menu, const ShortcutMenuPos *listPos, MsgData *msgData, Font *font,
                      PrintQueue *queue, ShortcutSave *shortcut, ClActor **actors, HeapID heapId) {
    int i;
    BmpWin *window;

    MI_CpuFill8(menu, 0, sizeof(*menu));
    menu->queue = queue;
    menu->shortcut = shortcut;
    menu->heapId = heapId;
    menu->msgData = msgData;
    menu->font = font;
    for (i = 0; i < ACTORS; i++) {
        menu->actors[i] = actors[i];
    }
    window = BmpWin_CreateDynamic(BG, 1, 0xc, 0x1e, 0xc, 0xb, TRUE);
    menu->window = window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    PrintWindow_Init(&menu->printWindow, menu->window);
    CreateList(menu, listPos->listTop, listPos->cursorRow, heapId);
}

static void FreeMenu(ShortcutMenuList *menu, ShortcutMenuPos *listPos) {
    FreeList(menu, &listPos->listTop, &listPos->cursorRow);
    BmpWin_Free(menu->window);
    MI_CpuFill8(menu, 0, sizeof(*menu));
}

static void OnCursorUp(ShortcutMenuList *menu, u16 value, u32 count, u32 *handled) {
    if (value == 0) {
        s32 maxShown = BmpMenuList_GetParam(menu->list, BMPMENULIST_PARAM_MAX_SHOWN);

        FreeList(menu, NULL, NULL);
        CreateList(menu, (count - 1) - (maxShown - 1), maxShown - 1, menu->heapId);
        if (menu->mode == LIST_MODE_REORDER) {
            menu->scrollDir = 1;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        *handled = TRUE;
    } else if (menu->scrollDir == 1 && menu->mode == LIST_MODE_REORDER) {
        u16 cursorRow;
        u16 y;

        BmpMenuList_GetPos(menu->list, NULL, &cursorRow);
        y = BmpMenuList_GetRowY(menu->list, cursorRow) >> 3;
        SetActorsY(menu, (y + 12) * 8);
        menu->scrollDir = 0;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        *handled = TRUE;
    }
}

static void OnCursorDown(ShortcutMenuList *menu, u16 value, u32 count, u32 *handled) {
    if (value == count - 1) {
        if (menu->mode == LIST_MODE_REORDER) {
            u16 cursorRow;
            u16 y;

            BmpMenuList_GetPos(menu->list, NULL, &cursorRow);
            y = BmpMenuList_GetRowY(menu->list, cursorRow + 1) >> 3;
            SetActorsY(menu, (y + 12) * 8);
            menu->scrollDir ^= 1;
        }
        if (menu->scrollDir == 0) {
            FreeList(menu, NULL, NULL);
            CreateList(menu, 0, 0, menu->heapId);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        *handled = TRUE;
    } else {
        menu->scrollDir = 0;
    }
}

static void ReadInput(ShortcutMenuList *menu) {
    s32 result = BMPMENULIST_NULL;
    BOOL toggle = FALSE;
    u32 handled = FALSE;
    u16 cursorIndex;
    u16 buttonIndex;
    u16 selectIndex;
    u16 index;
    u16 cursorRow;
    u16 listTop;
    s32 count;

    GCTX_HIDSetUsingTouch(FALSE);
    BmpMenuList_GetCursorIndex(menu->list, &cursorIndex);
    count = BmpMenuList_GetParam(menu->list, BMPMENULIST_PARAM_COUNT);

    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        OnCursorUp(menu, cursorIndex, count, &handled);
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        OnCursorDown(menu, cursorIndex, count, &handled);
    }
    if (!handled) {
        result = BmpMenuList_Update(menu->list);
    }
    if (result == BMPMENULIST_NULL) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            result = BMPMENULIST_CANCEL;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
            BmpMenuList_GetCursorIndex(menu->list, &buttonIndex);
            result = BmpMenuList_GetOptionValue(menu->list, buttonIndex);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && menu->mode == LIST_MODE_REORDER) {
            BmpMenuList_GetCursorIndex(menu->list, &selectIndex);
            result = BmpMenuList_GetOptionValue(menu->list, selectIndex);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
    if (menu->mode == LIST_MODE_SELECT) {
        menu->input = result;
    } else if (menu->mode == LIST_MODE_REORDER) {
        if (result == BMPMENULIST_CANCEL) {
            toggle = TRUE;
        } else if (result != BMPMENULIST_NULL) {
            result = BmpMenuList_GetOptionValue(menu->list, menu->listTop + menu->cursorRow);
            BmpMenuList_GetCursorIndex(menu->list, &index);
            if (menu->scrollDir != 0) {
                result = ShortcutSave_MoveItemToIndex(menu->shortcut, result, 0xff);
            } else {
                result = ShortcutSave_MoveItemToIndex(menu->shortcut, result, (u8)index);
            }
            FreeList(menu, &listTop, &cursorRow);
            if (index - 1 == result) {
                cursorRow--;
            }
            CreateList(menu, listTop, cursorRow, menu->heapId);
            toggle = TRUE;
        }
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && menu->mode == LIST_MODE_SELECT &&
        (u32)BmpMenuList_GetParam(menu->list, BMPMENULIST_PARAM_COUNT) > 1) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        toggle = TRUE;
    }
    if (toggle) {
        menu->mode ^= 1;
        if (menu->mode == LIST_MODE_SELECT) {
            int i;

            menu->scrollDir = 0;
            for (i = 0; i < ACTORS; i++) {
                ClActor_SetVisible(menu->actors[i], FALSE);
            }
            RedrawHighlight(menu->list, FALSE, FALSE);
        } else if (menu->mode == LIST_MODE_REORDER) {
            BmpMenuList_GetPos(menu->list, &menu->listTop, &menu->cursorRow);
            menu->showActors = TRUE;
        }
    }
}

static BOOL PrintDone(ShortcutMenuList *menu) {
    BOOL done;

    PrintWindow_Flush(&menu->printWindow, menu->queue);
    done = PrintWindow_IsPrinted(&menu->printWindow);
    if (done) {
        int i;

        if (menu->cursorMoved) {
            RedrawHighlight(menu->list, 0, 0);
            menu->cursorMoved = FALSE;
        }
        if (menu->showActors) {
            for (i = 0; i < ACTORS; i++) {
                ClActor_SetVisible(menu->actors[i], TRUE);
            }
            menu->showActors = FALSE;
        }
    }
    return done;
}

static s32 GetInput(ShortcutMenuList *menu) {
    return menu->input;
}

static void OnListCursor(BmpMenuList *list, s32 value, u8 init) {
    ShortcutMenuList *menu = (ShortcutMenuList *)BmpMenuList_GetParam(list, BMPMENULIST_PARAM_WORK);

    menu->cursorMoved = TRUE;
}

// Paints the cursor's row in the highlight's palette, and moves the actors beside it. Both callers pass 0 for the
// arguments, which it doesn't use
static void RedrawHighlight(BmpMenuList *list, u8 unused1, u8 unused2) {
    ShortcutMenuList *menu = (ShortcutMenuList *)BmpMenuList_GetParam(list, BMPMENULIST_PARAM_WORK);
    u16 cursorY = BmpMenuList_GetCursorY(list) >> 3;
    u32 rowHeight = (u32)BmpMenuList_GetParam(list, BMPMENULIST_PARAM_ROW_HEIGHT) >> 3;

    if (menu->mode == LIST_MODE_SELECT) {
        GFL_BGSysSetScrPaletteNo(BG, 1, 0xc, 0x1e, 0xc, 0xb);
        GFL_BGSysSetScrPaletteNo(BG, 1, cursorY + 0xc, 0x1e, rowHeight, 0xc);
        GFL_BGSysSetScrPaletteNo(BG, 0, 0xb, 0x20, 1, 0xa);
        GFL_BGSysQueueScrLoad(BG);
    } else if (menu->mode == LIST_MODE_REORDER) {
        u16 listTop;
        u16 cursorRow;
        u32 listY;
        u32 y;

        GFL_BGSysSetScrPaletteNo(BG, 1, 0xc, 0x1e, 0xc, 0xb);
        BmpMenuList_GetPos(list, &listTop, &cursorRow);
        listY = BmpMenuList_GetParam(list, BMPMENULIST_PARAM_Y);
        y = listY +
            (menu->cursorRow + menu->listTop - listTop) * BmpMenuList_GetParam(list, BMPMENULIST_PARAM_ROW_HEIGHT);
        GFL_BGSysSetScrPaletteNo(BG, 1, y / 8 + 0xc, 0x1e, rowHeight, 0xc);
        GFL_BGSysSetScrPaletteNo(BG, 0, 0xb, 0x20, 1, 0xa);
        GFL_BGSysQueueScrLoad(BG);
    }
    if (menu->scrollDir == 1) {
        u16 maxShown = BmpMenuList_GetParam(menu->list, BMPMENULIST_PARAM_MAX_SHOWN);
        u16 rowY = BmpMenuList_GetRowY(menu->list, maxShown) >> 3;

        SetActorsY(menu, (rowY + 0xc) * 8);
    } else {
        SetActorsY(menu, (cursorY + 0xc) * 8);
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

static void CreateList(ShortcutMenuList *menu, u16 listTop, u16 cursorRow, HeapID heapId) {
    BmpMenuListHeader header;
    u32 count = ShortcutSave_GetShortcutCount(menu->shortcut);
    u32 i;

    menu->options = ListMenuCore_CreateOptionList(count, heapId);
    for (i = 0; i < count; i++) {
        u32 value = ShortcutSave_GetRegistItem(menu->shortcut, (u8)i);

        ListMenuCore_AppendMsgOption(menu->options, menu->msgData, value, value, heapId);
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
    header.options = menu->options;
    header.count = count;
    header.work = menu;
    header.cursorCallback = OnListCursor;
    header.printCallback = NULL;
    header.msgData = menu->msgData;
    header.printWindow = &menu->printWindow;
    header.queue = menu->queue;
    header.font = menu->font;
    menu->list = BmpMenuList_Create(&header, listTop, cursorRow, heapId);
}

static void FreeList(ShortcutMenuList *menu, u16 *listTop, u16 *cursorRow) {
    BmpMenuList_Free(menu->list, listTop, cursorRow);
    ListMenuCore_FreeOptionList(menu->options);
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

static void SetActorsY(ShortcutMenuList *menu, u32 y) {
    ClActorPos pos;
    int i;

    for (i = 0; i < ACTORS; i++) {
        ClActor_GetSurfacePos(menu->actors[i], &pos, 0);
        pos.y = y;
        ClActor_SetSurfacePos(menu->actors[i], &pos, 0);
    }
}

GameEvent *ReturnToField_CreateNameInputEvent(GameSystem *gsys, Field *field, NameEntryParam *param, BOOL transition) {
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

// The steps of the event that leaves the menu for the name entry: call it, and end
enum {
    RETURN_STATE_CALL,
    RETURN_STATE_END,
};

static GameEventReturnCode ReturnToField_Callback(GameEvent *event, u32 *state, void *data) {
    ShortcutMenuReturn *ret = data;

    switch (*state) {
    case RETURN_STATE_CALL:
        if (ret->transition) {
            GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(ret->gsys, ret->field, OVERLAY_ID(280),
                                                                             &NAME_ENTRY_PROC_FUNCTIONS, ret->param));
        } else {
            GameEvent_ChainNext(event, EventFieldSubprocessCall_Create(ret->gsys, ret->field, OVERLAY_ID(280),
                                                                       &NAME_ENTRY_PROC_FUNCTIONS, ret->param));
        }
        (*state)++;
        break;
    case RETURN_STATE_END:
        return TRUE;
    }
    return FALSE;
}
