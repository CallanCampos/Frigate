#include "types.h"
#include "app/ui/frame_list.h"
#include "app/ui/print_msg.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "app/un_select.h"
#include "app/un_select_graphic.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/brightness.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/un_select.h"

// The Unity Tower's floor select, named after the ROM's "un_select.c": a list of the tower's floors, one for each
// country, that scrolls beside a picture of the tower. Only the floors of countries whose visitors the player has met
// can be chosen. The names are ours

// The floors of the list, from the top floor down to floor 2, one for each country of sUnSelectCountries
#define UN_SELECT_FLOOR_COUNT 130
#define UN_SELECT_TOP_FLOOR (UN_SELECT_FLOOR_COUNT + 1)
// The floor of an item of the list
#define UN_SELECT_FLOOR(index) (UN_SELECT_TOP_FLOOR - (index))

// The rows of the list, and the marks of a visitor's floor on each, one for each of the two BGs the rows scroll on
#define UN_SELECT_LIST_ROWS 4
#define UN_SELECT_ROW_MARK_COUNT 7

// The cell actors
enum {
    // The list's place on the picture of the tower
    UN_SELECT_ACTOR_CURSOR = 0,
    // The floors of the latest visitors on the picture of the tower
    UN_SELECT_ACTOR_VISITOR = UN_SELECT_ACTOR_CURSOR + 1,
    // The marks of the visitors' floors in the list's rows, on its first BG and on its second
    UN_SELECT_ACTOR_ROW_MARK = UN_SELECT_ACTOR_VISITOR + UNITY_TOWER_VISITOR_MAX,
    UN_SELECT_ACTOR_ROW_MARK_2 = UN_SELECT_ACTOR_ROW_MARK + UN_SELECT_ROW_MARK_COUNT,
    // The scroll bar
    UN_SELECT_ACTOR_BAR = UN_SELECT_ACTOR_ROW_MARK_2 + UN_SELECT_ROW_MARK_COUNT,
    UN_SELECT_ACTOR_COUNT,
};

// The OBJ resources
enum {
    UN_SELECT_RES_CHARS,
    UN_SELECT_RES_PLTT,
    UN_SELECT_RES_CELLS,
    UN_SELECT_RES_COUNT,
};

#define UN_SELECT_RES_NONE 0xffffffff
// A visitor whose country has no floor
#define UN_SELECT_NO_FLOOR 0xffff

// The steps of the scene: the list, the question whether to go to the floor chosen, and the message that the floor
// chosen is the current one
enum {
    UN_SELECT_STEP_LIST,
    UN_SELECT_STEP_CONFIRM,
    UN_SELECT_STEP_SAME_FLOOR,
    UN_SELECT_STEP_COUNT,
};

// What UnSelect_SetResult sets
enum {
    UN_SELECT_RESULT_SELECTED,
    UN_SELECT_RESULT_CANCEL,
};

// The messages, the windows they are printed in, and the names of the countries of the list
typedef struct {
    HeapID heapId;
    TCBExManager *tcbManager;
    MsgData *msgData;
    Font *font;
    PrintQueue *printQueue;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *expanded;
    PrintStream *stream;
    BmpWin *msgWindow;
    BmpWin *titleWindow;
    StrBuf *names[UN_SELECT_FLOOR_COUNT];
} UnSelectMsg;

typedef struct {
    HeapID heapId;
    UnSelectGraphic *graphic;
    TouchBar *touchBar;
    u32 unk0C;
    u32 unk10;
    u32 unk14;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    UnSelectParam *param;
    UIScene *scene;
    UnSelectMsg *msg;
    FrameList *list;
    // The item chosen
    int selected;
    // For each item, whether a visitor from its country has come
    BOOL visited[UN_SELECT_FLOOR_COUNT];
    TCB *hblankTask;
    ClActor *actors[UN_SELECT_ACTOR_COUNT];
    u32 res[UN_SELECT_RES_COUNT];
    // The item of each visitor's country, or UN_SELECT_NO_FLOOR
    u32 visitorFloors[UNITY_TOWER_VISITOR_MAX];
    // L or R was pressed, which scrolls by a page, so the marks of the rows are hidden before they are printed again
    BOOL pageScrolled;
} UnSelectWork;

// What an actor is created from: where it starts, its resources and its surface
typedef struct {
    ClActorSetup setup;
    u32 chars;
    u32 palette;
    u32 cells;
    u16 unk14;
    u16 surface;
} UnSelectActorTemplate;

static BOOL UnSelect_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL UnSelect_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL UnSelect_Main(GameProc *proc, u32 *state, void *param, void *work);
static void UnSelect_LoadBG(UnSelectWork *wk, HeapID heapId);
static TouchBar *UnSelect_CreateTouchBar(ClActUnit *unit, HeapID heapId);
static void UnSelect_FreeTouchBar(TouchBar *touchBar);
static void UnSelect_UpdateTouchBar(TouchBar *touchBar);
static AppTaskMenu *UnSelect_CreateYesNo(AppTaskMenuRes *res, MsgData *msgData, HeapID heapId);
static void UnSelect_FreeMenu(AppTaskMenu *menu);
static void UnSelect_UpdateMenu(AppTaskMenu *menu);
static void UnSelect_StartDim(void);
static void UnSelect_StartUndim(void);
static UnSelectMsg *UnSelectMsg_Create(HeapID heapId);
static void UnSelectMsg_Free(UnSelectMsg *msg);
static void UnSelectMsg_Update(UnSelectMsg *msg);
static void UnSelectMsg_PrintItem(UnSelectMsg *msg, PrintWindow *window, PrintQueue *queue, Font *font, u32 index);
static void UnSelectMsg_Print(UnSelectMsg *msg, u32 msgId);
static BOOL UnSelectMsg_Wait(UnSelectMsg *msg);
static void UnSelectMsg_ClearFrame(UnSelectMsg *msg);
static Font *UnSelectMsg_GetFont(UnSelectMsg *msg);
static PrintQueue *UnSelectMsg_GetPrintQueue(UnSelectMsg *msg);
static MsgData *UnSelectMsg_GetMsgData(UnSelectMsg *msg);
static void UnSelect_PrintItem(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG);
static void UnSelect_SelectItem(void *work, u32 index, BOOL moved);
static void UnSelect_ScrollList(void *work, s16 delta);
static void UnSelect_CreateList(UnSelectWork *wk);
static void UnSelect_FreeList(UnSelectWork *wk);
static UnSelectWork *UnSelect_Setup(GameProc *proc, UnSelectParam *param);
static void UnSelect_SetResult(UnSelectWork *wk, u32 result);
static BOOL UnSelect_ListBegin(UIScene *scene, void *param);
static BOOL UnSelect_ListRun(UIScene *scene, void *param);
static BOOL UnSelect_ListEnd(UIScene *scene, void *param);
static BOOL UnSelect_ConfirmBegin(UIScene *scene, void *param);
static BOOL UnSelect_ConfirmRun(UIScene *scene, void *param);
static BOOL UnSelect_ConfirmEnd(UIScene *scene, void *param);
static BOOL UnSelect_SameFloorBegin(UIScene *scene, void *param);
static BOOL UnSelect_SameFloorRun(UIScene *scene, void *param);
static BOOL UnSelect_SameFloorEnd(UIScene *scene, void *param);
static void UnSelect_HBlank(TCB *tcb, void *data);
static void UnSelect_LoadObjRes(UnSelectWork *wk);
static void UnSelect_FreeObjRes(UnSelectWork *wk);
static void UnSelect_CreateActors(UnSelectWork *wk);
static void UnSelect_FreeActors(UnSelectWork *wk);
static ClActor *UnSelect_CreateActor(ClActUnit *unit, UnSelectActorTemplate *template);
static void UnSelect_HideRowMarks(UnSelectWork *wk);
static void UnSelect_ScrollRowMarks(UnSelectWork *wk, int delta);
static void UnSelect_PlaceVisitors(UnSelectWork *wk);
static void UnSelect_SetBarPos(UnSelectWork *wk, int pos);
static void UnSelect_SetCursorPos(UnSelectWork *wk, int y);
static void UnSelect_ShowRowMark(UnSelectWork *wk, BOOL firstBG, int y);
static BOOL UnSelect_ShowActor(ClActor **actors, int index, u16 surface, int y);
static BOOL UnSelect_IsVisitorFloor(UnSelectWork *wk, u32 index);
static u32 UnSelect_FindCountry(u32 country);

// The rows of the list, the scroll bar and the buttons that scroll by a page
static const FrameListTouch sUnSelectListTouch[] = {
    { { 0, 39, 84, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },   { { 40, 79, 84, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 80, 119, 84, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE }, { { 120, 159, 84, 231 }, FRAMELIST_TOUCH_ITEM_DECIDE },
    { { 8, 160, 240, 255 }, FRAMELIST_TOUCH_BAR },         { { 168, 191, 16, 39 }, FRAMELIST_TOUCH_PAGE_UP },
    { { 168, 191, 80, 103 }, FRAMELIST_TOUCH_PAGE_DOWN },  { { TOUCH_RECT_END } },
};

static const FrameListCallbacks sUnSelectListCallbacks = {
    UnSelect_PrintItem,
    UnSelect_SelectItem,
    UnSelect_ScrollList,
};

static const UISceneStep sUnSelectSteps[UN_SELECT_STEP_COUNT] = {
    { UnSelect_ListBegin, NULL, UnSelect_ListRun, NULL, UnSelect_ListEnd },
    { UnSelect_ConfirmBegin, NULL, UnSelect_ConfirmRun, NULL, UnSelect_ConfirmEnd },
    { UnSelect_SameFloorBegin, NULL, UnSelect_SameFloorRun, NULL, UnSelect_SameFloorEnd },
};

const GameProcFunctions UN_SELECT_PROC_FUNCTIONS = {
    UnSelect_Init,
    UnSelect_Main,
    UnSelect_Exit,
};

// The country of each floor, from the top floor down
static const u32 sUnSelectCountries[UN_SELECT_FLOOR_COUNT] = {
    1,   2,   3,   6,   8,   9,   12,  13,  15,  16,  17,  18,  20,  21,  22,  23,  25,  27,  28,  29,  31,  33,
    34,  35,  36,  40,  42,  43,  45,  47,  48,  49,  51,  53,  54,  58,  60,  61,  62,  63,  64,  71,  72,  73,
    74,  76,  79,  80,  81,  82,  83,  84,  85,  87,  88,  90,  91,  92,  93,  94,  95,  96,  98,  99,  101, 102,
    103, 105, 106, 109, 111, 115, 117, 118, 121, 125, 128, 130, 132, 134, 138, 139, 141, 145, 147, 148, 149, 150,
    151, 155, 156, 157, 160, 161, 163, 164, 166, 167, 173, 174, 181, 185, 186, 188, 189, 190, 191, 194, 170, 195,
    196, 198, 199, 200, 201, 203, 219, 205, 206, 210, 211, 215, 217, 218, 220, 221, 222, 224, 226, 227,
};

static FrameListSetup sUnSelectListSetup = {
    { 2, 6, 8, 0, 22, 5, 1, 0, 20, 5, 1, 40, 20, 10, 8, 5, 4, 3, 16, 0 },
    UN_SELECT_FLOOR_COUNT,
    2,
    0,
    UN_SELECT_LIST_ROWS,
    0,
    sUnSelectListTouch,
    &sUnSelectListCallbacks,
    NULL,
};

static BOOL UnSelect_Init(GameProc *proc, u32 *state, void *param, void *work) {
    UnSelectWork *wk;

    switch (*state) {
    case 0:
        UnSelect_Setup(proc, param);
        (*state)++;
        break;
    case 1:
        wk = work;
        if (!FrameList_Draw(wk->list)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL UnSelect_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    UnSelectWork *wk = work;
    HeapID heapId;

    switch (*state) {
    case 0:
        GFL_FadeSet(3, 0, 16, 1);
        (*state)++;
        break;
    case 1:
        if (GFL_FadeIsRunning() != TRUE) {
            (*state)++;
        }
        break;
    case 2:
        GFL_TCBRemove(wk->hblankTask);
        G2_BlendNone();
        G2S_BlendNone();
        UnSelect_FreeList(wk);
        UIScene_Free(wk->scene);
        UnSelectMsg_Free(wk->msg);
        UnSelect_FreeActors(wk);
        UnSelect_FreeObjRes(wk);
        AppTaskMenuRes_Free(wk->menuRes);
        UnSelect_FreeTouchBar(wk->touchBar);
        UnSelectGraphic_Free(wk->graphic);
        heapId = wk->heapId;
        GFL_ProcReleaseSubsystem(proc);
        GFL_HeapDelete(heapId);
        GFL_OvlUnload(OVERLAY_APP_UI);
        return TRUE;
    }
    return FALSE;
}

static BOOL UnSelect_Main(GameProc *proc, u32 *state, void *param, void *work) {
    UnSelectWork *wk = work;
    BOOL end = FALSE;

    switch (*state) {
    case 0:
        GFL_FadeSet(3, 16, 0, 1);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        if (UIScene_Main(wk->scene)) {
            end = TRUE;
        }
        break;
    }
    UnSelectMsg_Update(wk->msg);
    UnSelectGraphic_Update(wk->graphic);
    return end ? TRUE : FALSE;
}

// Loads the BGs, and blanks the floors on the picture of the tower whose countries no visitor has come from
static void UnSelect_LoadBG(UnSelectWork *wk, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_UN_SELECT, heapId);
    NNSG2dCharacterData *lowerChars;
    NNSG2dCharacterData *upperChars;
    void *lowerData;
    void *upperData;
    int i;

    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 0, 7 * 0x20, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 7 * 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 12, 4, 0, 0x200, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 7, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 13, 4, 0, 0, FALSE, heapId);
    upperData = GFL_G2DIOReadBGNCGRArc(arc, 0, FALSE, &upperChars, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 5, 3, 0, 0, FALSE, heapId);
    lowerData = GFL_G2DIOReadBGNCGRArc(arc, 0, FALSE, &lowerChars, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 2, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 6, 0, 0, FALSE, heapId);
    for (i = 0; i < UN_SELECT_FLOOR_COUNT; i++) {
        // Each floor is two rows of pixels, four floors to a tile
        int floor = UN_SELECT_FLOOR_COUNT - 1 - i;
        u8 *chars = i < 50 ? upperChars->rawData : lowerChars->rawData;

        if (!wk->visited[i]) {
            u8 tile = floor / 4;
            u8 row = (3 - floor % 4) * 2;
            u8 offset = row * 4;
            int pos = tile * 0x20 + 0x20 + offset;

            chars[pos] = 0xff;
            chars[pos + 1] = 0xff;
            chars[pos + 2] = 0xff;
            chars[pos + 3] = 0xff;
            chars[pos + 4] = 0xff;
            chars[pos + 5] = 0xff;
            chars[pos + 6] = 0xff;
            chars[pos + 7] = 0xff;
        }
    }
    GFL_BGSysLoadChar(7, upperChars->rawData, upperChars->size, 0);
    GFL_HeapFree(upperData);
    GFL_BGSysLoadChar(3, lowerChars->rawData, lowerChars->size, 0);
    GFL_HeapFree(lowerData);
    GFL_ArcToolFree(arc);
}

static TouchBar *UnSelect_CreateTouchBar(ClActUnit *unit, HeapID heapId) {
    TouchBarItem items[] = {
        { 1, { 232, 168 } },
        { 4, { 16, 168 } },
        { 5, { 80, 168 } },
    };
    TouchBarSetup setup;
    TouchBar *touchBar;

    sys_memset(&setup, 0, sizeof(TouchBarSetup));
    setup.items = items;
    setup.count = NELEMS(items);
    setup.unit = unit;
    setup.bg = 1;
    setup.bgPalette = 13;
    setup.objPalette = 0;
    setup.vramType = CLACT_VRAM_MAIN;
    touchBar = TouchBar_Create(&setup, heapId);
    TouchBar_SetBgPriority(touchBar, 1);
    TouchBar_SetIconSe(touchBar, 4, 0);
    TouchBar_SetIconSe(touchBar, 5, 0);
    return touchBar;
}

static void UnSelect_FreeTouchBar(TouchBar *touchBar) {
    TouchBar_Free(touchBar);
}

static void UnSelect_UpdateTouchBar(TouchBar *touchBar) {
    TouchBar_Main(touchBar);
}

static AppTaskMenu *UnSelect_CreateYesNo(AppTaskMenuRes *res, MsgData *msgData, HeapID heapId) {
    AppTaskMenuInit init;
    AppTaskMenuItem items[2];
    AppTaskMenu *menu;
    u32 i;

    for (i = 0; i < 2; i++) {
        items[i].str = GFL_MsgDataLoadStrbufNew(msgData, UnSelect_Text_Yes + i);
        items[i].color = PRINT_COLOR(14, 15, 3);
        items[i].type = 0;
    }
    init.heapId = heapId;
    init.itemCount = 2;
    init.items = items;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 18;
    init.width = 13;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, res);
    for (i = 0; i < 2; i++) {
        GFL_StrBufFree(items[i].str);
    }
    return menu;
}

static void UnSelect_FreeMenu(AppTaskMenu *menu) {
    AppTaskMenu_Free(menu);
}

static void UnSelect_UpdateMenu(AppTaskMenu *menu) {
    AppTaskMenu_Update(menu);
}

// Darkens the lower screen's BGs behind the message, and brightens them again
static void UnSelect_StartDim(void) {
    BrightnessController_Reset(1);
    BrightnessController_StartTransition(
        8, -8, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD, 1);
}

static void UnSelect_StartUndim(void) {
    BrightnessController_Reset(1);
    BrightnessController_StartTransition(
        8, 0, -8, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD, 1);
}

static UnSelectMsg *UnSelectMsg_Create(HeapID heapId) {
    static const u8 backColor = 15;
    UnSelectMsg *msg = GFL_HeapAllocate(heapId, sizeof(UnSelectMsg), TRUE, "un_select.c", 972);
    BmpWin *window;

    msg->heapId = heapId;
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 14 * 0x20, 0x20, heapId);
    LoadSysMsgBox(0, 1, 15, 0, heapId);
    msg->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_UN_SELECT, heapId);
    msg->printQueue = func_02021998(heapId);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    msg->strbuf = GFL_StrBufCreate(1600, heapId);
    msg->expanded = GFL_StrBufCreate(1600, heapId);
    msg->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 1, 0);
    msg->msgWindow = BmpWin_CreateDynamic(0, 1, 19, 30, 4, 14, TRUE);
    window = msg->titleWindow = BmpWin_CreateDynamic(4, 1, 1, 30, 2, 6, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(window), backColor);
    GFL_TextRndUpdateColorIndexLUT(1, 2, backColor);
    GFL_MsgDataLoadStrbuf(msg->msgData, UnSelect_Text_PleaseSelectFloor, msg->strbuf);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 4, 0, msg->strbuf, msg->font);
    BmpWin_Transfer(window);
    return msg;
}

static void UnSelectMsg_Free(UnSelectMsg *msg) {
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    BmpWin_Free(msg->titleWindow);
    BmpWin_Free(msg->msgWindow);
    GFL_StrBufFree(msg->strbuf);
    GFL_StrBufFree(msg->expanded);
    GFL_MsgDataFree(msg->msgData);
    func_02021a18(msg->printQueue);
    GFL_FontFree(msg->font);
    GFL_TCBExMgrFree(msg->tcbManager);
    GFL_WordSetSystemFree(msg->wordSet);
    GFL_HeapFree(msg);
}

static void UnSelectMsg_Update(UnSelectMsg *msg) {
    func_02021a3c(msg->printQueue);
    GFL_TCBExMgrUpdate(msg->tcbManager);
}

// Prints a row of the list: its floor, and the name of its country, in one line or two
static void UnSelectMsg_PrintItem(UnSelectMsg *msg, PrintWindow *window, PrintQueue *queue, Font *font, u32 index) {
    int pos = index;
    u32 y = 12;
    u32 floor = UN_SELECT_FLOOR(pos);

    GFL_MsgDataLoadStrbuf(msg->msgData, floor == 2 ? UnSelect_Text_F_2 : UnSelect_Text_F, msg->strbuf);
    WordSetNumber(msg->wordSet, 2, floor, 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->expanded, msg->strbuf);
    PrintStrAligned(window, queue, 0, 12, msg->expanded, font, PRINT_COLOR(14, 15, 0), PRINT_ALIGN_LEFT);
    if (func_0202284c(msg->names[pos]) > 1) {
        y = 4;
    }
    PrintStrAligned(window, queue, 30, y, msg->names[pos], font, PRINT_COLOR(14, 15, 0), PRINT_ALIGN_LEFT);
}

static void UnSelectMsg_Print(UnSelectMsg *msg, u32 msgId) {
    static const u8 backColor = 15;
    s32 wait = func_02017bcc();
    BmpWin *window = msg->msgWindow;

    GFL_BitmapFill(BmpWin_GetBitmap(window), backColor);
    GFL_TextRndUpdateColorIndexLUT(1, 2, backColor);
    GFL_MsgDataLoadStrbuf(msg->msgData, msgId, msg->strbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->expanded, msg->strbuf);
    msg->stream = func_02022268(window, 4, 0, msg->expanded, msg->font, wait, msg->tcbManager, 0xffff, msg->heapId, 15);
    BmpWin_DrawFrame(msg->msgWindow, 2, 1, 15);
    BmpWin_Transfer(window);
}

// Whether the message has been printed and the player has gone on
static BOOL UnSelectMsg_Wait(UnSelectMsg *msg) {
    if (msg->stream != NULL) {
        switch (func_020223b4(msg->stream)) {
        case 2:
            func_020223cc(msg->stream);
            msg->stream = NULL;
            return TRUE;
        case 1:
            if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A || GCTX_HIDGetPressedKeys() == PAD_BUTTON_B ||
                func_0203da48()) {
                func_020223bc(msg->stream);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            break;
        case 0:
            if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da2c()) {
                func_020223e0(msg->stream, 0);
            }
            break;
        }
        return FALSE;
    }
    return FALSE;
}

static void UnSelectMsg_ClearFrame(UnSelectMsg *msg) {
    BmpWin_ClearFrame(msg->msgWindow, TRUE);
}

static Font *UnSelectMsg_GetFont(UnSelectMsg *msg) {
    return msg->font;
}

static PrintQueue *UnSelectMsg_GetPrintQueue(UnSelectMsg *msg) {
    return msg->printQueue;
}

static MsgData *UnSelectMsg_GetMsgData(UnSelectMsg *msg) {
    return msg->msgData;
}

static void UnSelect_PrintItem(void *work, u32 index, PrintWindow *window, s16 y, BOOL firstBG) {
    UnSelectWork *wk = work;
    PrintQueue *queue;

    UnSelect_HideRowMarks(wk);
    FrameList_GetValue(wk->list, index);
    queue = FrameList_GetPrintQueue(wk->list);
    UnSelectMsg_PrintItem(wk->msg, window, queue, UnSelectMsg_GetFont(wk->msg), index);
    if (UnSelect_IsVisitorFloor(wk, index)) {
        UnSelect_ShowRowMark(wk, firstBG, y + 20);
    }
}

static void UnSelect_SelectItem(void *work, u32 index, BOOL moved) {
    UnSelectWork *wk = work;
    s16 pos = FrameList_GetBarPos(wk->list);

    UnSelect_SetBarPos(wk, pos);
    UnSelect_SetCursorPos(wk, (FrameList_GetSelected(wk->list) - FrameList_GetCursor(wk->list)) * 2 + 92);
}

static void UnSelect_ScrollList(void *work, s16 delta) {
    UnSelectWork *wk = work;
    s16 pos;

    UnSelect_ScrollRowMarks(wk, -delta);
    pos = FrameList_GetBarPos(wk->list);
    UnSelect_SetBarPos(wk, pos);
    UnSelect_SetCursorPos(wk, (FrameList_GetSelected(wk->list) - FrameList_GetCursor(wk->list)) * 2 + 92);
}

// Creates the list with the floor the player is on at its top, or the floor above floor 2 when they are on the ground
// floor
static void UnSelect_CreateList(UnSelectWork *wk) {
    int i;
    int floor;
    ArcTool *arc;
    MsgData *msgData;

    sUnSelectListSetup.work = wk;
    floor = wk->param->floor;
    if (floor <= 1) {
        floor = 2;
    }
    for (i = 0; i < UN_SELECT_FLOOR_COUNT; i++) {
        if (UN_SELECT_FLOOR(i) == floor) {
            if (i > UN_SELECT_FLOOR_COUNT - UN_SELECT_LIST_ROWS) {
                sUnSelectListSetup.cursorPos = i - (UN_SELECT_FLOOR_COUNT - UN_SELECT_LIST_ROWS);
                sUnSelectListSetup.scroll = i - sUnSelectListSetup.cursorPos;
            } else {
                sUnSelectListSetup.scroll = i;
            }
        }
    }
    wk->list = FrameList_Create(&sUnSelectListSetup, wk->heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_UN_SELECT, wk->heapId);
    FrameList_LoadScreen(wk->list, arc, 3, FALSE, 0);
    FrameList_LoadScreen(wk->list, arc, 4, FALSE, 1);
    FrameList_LoadCursorPalette(wk->list, arc, 1, 2, 3);
    GFL_ArcToolFree(arc);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_UN_SELECT, wk->heapId);
    for (i = 0; i < UN_SELECT_FLOOR_COUNT; i++) {
        u32 type = 1;

        if (wk->visited[i] == TRUE) {
            type = 0;
        }
        FrameList_AddItem(wk->list, type, i);
        if (type == 0) {
            wk->msg->names[i] = GFL_MsgDataLoadStrbufNew(msgData, sUnSelectCountries[i]);
        } else {
            wk->msg->names[i] = GFL_MsgDataLoadStrbufNew(msgData, UnSelect_Text_Empty_2);
        }
    }
    GFL_MsgDataFree(msgData);
}

static void UnSelect_FreeList(UnSelectWork *wk) {
    int i;

    for (i = 0; i < UN_SELECT_FLOOR_COUNT; i++) {
        GFL_StrBufFree(wk->msg->names[i]);
    }
    FrameList_Free(wk->list);
}

static UnSelectWork *UnSelect_Setup(GameProc *proc, UnSelectParam *param) {
    UnSelectWork *wk;
    int i;
    Font *font;
    PrintQueue *queue;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_UN_SELECT, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(UnSelectWork), HEAPID_UN_SELECT);
    sys_memset(wk, 0, sizeof(UnSelectWork));
    wk->heapId = HEAPID_UN_SELECT;
    wk->param = param;
    for (i = 0; i < UN_SELECT_FLOOR_COUNT; i++) {
        if (param->countryVisited[sUnSelectCountries[i] - 1]) {
            wk->visited[i] = TRUE;
        } else {
            wk->visited[i] = FALSE;
        }
    }
    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        wk->visitorFloors[i] =
            param->visitorCountries[i] != 0 ? UnSelect_FindCountry(param->visitorCountries[i]) : UN_SELECT_NO_FLOOR;
    }
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    wk->graphic = UnSelectGraphic_Create(0, wk->heapId);
    wk->scene = UIScene_Create(wk->heapId, sUnSelectSteps, UN_SELECT_STEP_COUNT, UN_SELECT_STEP_LIST, wk);
    UnSelect_LoadBG(wk, wk->heapId);
    wk->msg = UnSelectMsg_Create(wk->heapId);
    wk->touchBar = UnSelect_CreateTouchBar(UnSelectGraphic_GetClActUnit(wk->graphic), wk->heapId);
    font = UnSelectMsg_GetFont(wk->msg);
    queue = UnSelectMsg_GetPrintQueue(wk->msg);
    wk->menuRes = AppTaskMenuRes_Create(1, 11, font, queue, wk->heapId);
    UnSelect_LoadObjRes(wk);
    UnSelect_CreateList(wk);
    UnSelect_CreateActors(wk);
    UnSelect_PlaceVisitors(wk);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 6, 10);
    wk->hblankTask = GFL_HBlankTCBAdd(UnSelect_HBlank, wk, 0);
    func_02042ba8(TRUE, wk->heapId);
    return wk;
}

static void UnSelect_SetResult(UnSelectWork *wk, u32 result) {
    switch (result) {
    case UN_SELECT_RESULT_SELECTED: {
        int selected = wk->selected;

        wk->param->selectedFloor = UN_SELECT_FLOOR(selected);
        wk->param->selectedCountry = sUnSelectCountries[selected];
        wk->param->selected = TRUE;
        break;
    }
    case UN_SELECT_RESULT_CANCEL:
        wk->param->selectedFloor = 0;
        wk->param->selectedCountry = 0;
        wk->param->selected = FALSE;
        break;
    }
}

static BOOL UnSelect_ListBegin(UIScene *scene, void *param) {
    return TRUE;
}

static BOOL UnSelect_ListRun(UIScene *scene, void *param) {
    UnSelectWork *wk = param;
    BOOL pageScrolled;
    int selected;
    u16 next;

    wk->pageScrolled = FALSE;
    pageScrolled = FALSE;
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_L) {
        pageScrolled = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_R) {
        pageScrolled = TRUE;
    }
    if (pageScrolled) {
        wk->pageScrolled = TRUE;
    }
    if (FrameList_Main(wk->list) < UN_SELECT_LIST_ROWS) {
        selected = FrameList_GetSelected(wk->list);
        if (wk->visited[selected] == TRUE) {
            wk->selected = selected;
            FrameList_SetRowPalette(wk->list, (u16)FrameList_GetCursor(wk->list), 3);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            if (wk->param->floor == UN_SELECT_FLOOR(selected)) {
                next = UN_SELECT_STEP_SAME_FLOOR;
            } else {
                next = UN_SELECT_STEP_CONFIRM;
            }
            UIScene_SetNext(scene, next);
            return TRUE;
        }
    }
    UnSelect_UpdateTouchBar(wk->touchBar);
    switch (TouchBar_GetDecided(wk->touchBar)) {
    case -1:
    case 0:
        break;
    case 1:
        UnSelect_SetResult(wk, UN_SELECT_RESULT_CANCEL);
        UIScene_SetNext(scene, UI_SCENE_END);
        return TRUE;
    case 2:
    case 3:
    case 4:
    case 5:
        break;
    }
    return FALSE;
}

static BOOL UnSelect_ListEnd(UIScene *scene, void *param) {
    return TRUE;
}

static BOOL UnSelect_ConfirmBegin(UIScene *scene, void *param) {
    UnSelectWork *wk = param;

    switch (UIScene_GetCounter(scene)) {
    case 0:
        UnSelect_StartDim();
        UIScene_IncCounter(scene);
        break;
    case 1:
        if (GFL_FadeIsRunning() != TRUE && BrightnessController_IsTransitionComplete(1)) {
            loadCountryToStrbuf(wk->msg->wordSet, 0, sUnSelectCountries[wk->selected]);
            UnSelectMsg_Print(wk->msg, UnSelect_Text_WillGoFloor);
            UIScene_IncCounter(scene);
        }
        break;
    case 2:
        if (UnSelectMsg_Wait(wk->msg)) {
            UIScene_IncCounter(scene);
        }
        break;
    case 3:
        wk->menu = UnSelect_CreateYesNo(wk->menuRes, UnSelectMsg_GetMsgData(wk->msg), wk->heapId);
        return TRUE;
    }
    return FALSE;
}

static BOOL UnSelect_ConfirmRun(UIScene *scene, void *param) {
    UnSelectWork *wk = param;

    UnSelect_UpdateMenu(wk->menu);
    if (AppTaskMenu_IsFlashFinished(wk->menu)) {
        switch (AppTaskMenu_GetCursorPos(wk->menu)) {
        case 0:
            UnSelect_SetResult(wk, UN_SELECT_RESULT_SELECTED);
            UnSelect_StartUndim();
            UnSelectMsg_ClearFrame(wk->msg);
            UnSelect_FreeMenu(wk->menu);
            UIScene_SetNext(scene, UI_SCENE_END);
            break;
        case 1:
            UnSelect_StartUndim();
            UnSelectMsg_ClearFrame(wk->msg);
            UnSelect_FreeMenu(wk->menu);
            UIScene_SetNext(scene, UN_SELECT_STEP_LIST);
            break;
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL UnSelect_ConfirmEnd(UIScene *scene, void *param) {
    if (GFL_FadeIsRunning() == TRUE || !BrightnessController_IsTransitionComplete(1)) {
        return FALSE;
    }
    return TRUE;
}

static BOOL UnSelect_SameFloorBegin(UIScene *scene, void *param) {
    UnSelectWork *wk = param;

    switch (UIScene_GetCounter(scene)) {
    case 0:
        UnSelect_StartDim();
        UIScene_IncCounter(scene);
        break;
    case 1:
        if (GFL_FadeIsRunning() != TRUE && BrightnessController_IsTransitionComplete(1)) {
            UnSelectMsg_Print(wk->msg, UnSelect_Text_SelectedCurrentFloor);
            UIScene_IncCounter(scene);
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL UnSelect_SameFloorRun(UIScene *scene, void *param) {
    UnSelectWork *wk = param;

    if (UnSelectMsg_Wait(wk->msg)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL UnSelect_SameFloorEnd(UIScene *scene, void *param) {
    UnSelectWork *wk = param;

    UnSelect_StartUndim();
    UnSelectMsg_ClearFrame(wk->msg);
    UIScene_SetNext(scene, UN_SELECT_STEP_LIST);
    return TRUE;
}

// Blends the lower screen's BGs less toward its bottom
static void UnSelect_HBlank(TCB *tcb, void *data) {
    s32 line = GX_GetVCount();

    if (line >= 152) {
        G2S_ChangeBlendAlpha(11, 5);
    } else if (line >= 112) {
        G2S_ChangeBlendAlpha(10, 6);
    } else if (line >= 72) {
        G2S_ChangeBlendAlpha(9, 7);
    } else if (line >= 32) {
        G2S_ChangeBlendAlpha(8, 8);
    } else {
        G2S_ChangeBlendAlpha(6, 10);
    }
}

static void UnSelect_LoadObjRes(UnSelectWork *wk) {
    HeapID heapId = wk->heapId;
    ArcTool *arc;

    wk->res[UN_SELECT_RES_CHARS] = UN_SELECT_RES_NONE;
    wk->res[UN_SELECT_RES_PLTT] = UN_SELECT_RES_NONE;
    wk->res[UN_SELECT_RES_CELLS] = UN_SELECT_RES_NONE;
    arc = GFL_ArcSysCreateFileHandle(ARCID_UN_SELECT, heapId);
    wk->res[UN_SELECT_RES_CHARS] = func_0204b81c(arc, 10, FALSE, CLACT_VRAM_BOTH, heapId);
    wk->res[UN_SELECT_RES_PLTT] = func_0204bba0(arc, 11, CLACT_VRAM_BOTH, 3 * 0x20, heapId);
    wk->res[UN_SELECT_RES_CELLS] = func_0204bde0(arc, 9, 8, heapId);
    GFL_ArcToolFree(arc);
}

static void UnSelect_FreeObjRes(UnSelectWork *wk) {
    if (wk->res[UN_SELECT_RES_CHARS] != UN_SELECT_RES_NONE) {
        func_0204b98c(wk->res[UN_SELECT_RES_CHARS]);
    }
    if (wk->res[UN_SELECT_RES_PLTT] != UN_SELECT_RES_NONE) {
        func_0204bcd0(wk->res[UN_SELECT_RES_PLTT]);
    }
    if (wk->res[UN_SELECT_RES_CELLS] != UN_SELECT_RES_NONE) {
        func_0204be64(wk->res[UN_SELECT_RES_CELLS]);
    }
}

static void UnSelect_CreateActors(UnSelectWork *wk) {
    ClActUnit *unit = UnSelectGraphic_GetClActUnit(wk->graphic);
    int i;

    for (i = 0; i < UN_SELECT_ACTOR_COUNT; i++) {
        wk->actors[i] = NULL;
    }
    {
        UnSelectActorTemplate template = {
            { 8, 104, 2, 0, 2 },
            wk->res[UN_SELECT_RES_CHARS],
            wk->res[UN_SELECT_RES_PLTT],
            wk->res[UN_SELECT_RES_CELLS],
            3,
            0xffff,
        };

        wk->actors[UN_SELECT_ACTOR_CURSOR] = UnSelect_CreateActor(unit, &template);
    }
    {
        UnSelectActorTemplate template = {
            { 40, 0, 0, 0, 2 },
            wk->res[UN_SELECT_RES_CHARS],
            wk->res[UN_SELECT_RES_PLTT],
            wk->res[UN_SELECT_RES_CELLS],
            3,
            0xffff,
        };

        for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
            wk->actors[UN_SELECT_ACTOR_VISITOR + i] = UnSelect_CreateActor(unit, &template);
        }
    }
    {
        UnSelectActorTemplate template = {
            { 242, 0, 1, 0, 2 },
            wk->res[UN_SELECT_RES_CHARS],
            wk->res[UN_SELECT_RES_PLTT],
            wk->res[UN_SELECT_RES_CELLS],
            3,
            0,
        };

        for (i = 0; i < UN_SELECT_ROW_MARK_COUNT; i++) {
            ClActor *actor = UnSelect_CreateActor(unit, &template);

            func_0204c124(actor, FALSE);
            wk->actors[UN_SELECT_ACTOR_ROW_MARK + i] = actor;
        }
    }
    {
        UnSelectActorTemplate template = {
            { 242, 0, 1, 0, 2 },
            wk->res[UN_SELECT_RES_CHARS],
            wk->res[UN_SELECT_RES_PLTT],
            wk->res[UN_SELECT_RES_CELLS],
            3,
            1,
        };

        for (i = 0; i < UN_SELECT_ROW_MARK_COUNT; i++) {
            ClActor *actor = UnSelect_CreateActor(unit, &template);

            func_0204c318(actor, 1);
            func_0204c124(actor, FALSE);
            wk->actors[UN_SELECT_ACTOR_ROW_MARK_2 + i] = actor;
        }
    }
    {
        UnSelectActorTemplate template = {
            { 246, 152, 3, 0, 2 },
            wk->res[UN_SELECT_RES_CHARS],
            wk->res[UN_SELECT_RES_PLTT],
            wk->res[UN_SELECT_RES_CELLS],
            3,
            0,
        };

        wk->actors[UN_SELECT_ACTOR_BAR] = UnSelect_CreateActor(unit, &template);
    }
}

static void UnSelect_FreeActors(UnSelectWork *wk) {
    int i;

    for (i = 0; i < UN_SELECT_ACTOR_COUNT; i++) {
        if (wk->actors[i] != NULL) {
            func_0204c108(wk->actors[i]);
        }
    }
}

static ClActor *UnSelect_CreateActor(ClActUnit *unit, UnSelectActorTemplate *template) {
    ClActor *actor = func_0204c040(unit, template->chars, template->palette, template->cells, &template->setup,
                                   template->surface, HEAPID_UN_SELECT);

    func_0204c520(actor, TRUE);
    return actor;
}

static void UnSelect_HideRowMarks(UnSelectWork *wk) {
    int i;
    ClActor *actor;

    if (wk->pageScrolled) {
        wk->pageScrolled = FALSE;
        for (i = 0; i < UN_SELECT_ROW_MARK_COUNT; i++) {
            actor = wk->actors[UN_SELECT_ACTOR_ROW_MARK + i];
            if (func_0204c138(actor)) {
                func_0204c124(actor, FALSE);
            }
        }
        for (i = 0; i < UN_SELECT_ROW_MARK_COUNT; i++) {
            actor = wk->actors[UN_SELECT_ACTOR_ROW_MARK_2 + i];
            if (func_0204c138(actor)) {
                func_0204c124(actor, FALSE);
            }
        }
    }
}

// Moves the marks with the rows, and hides those that have scrolled off
static void UnSelect_ScrollRowMarks(UnSelectWork *wk, int delta) {
    int i;
    s16 y;
    ClActorPos pos;

    for (i = 0; i < UN_SELECT_ROW_MARK_COUNT; i++) {
        ClActor *actor = wk->actors[UN_SELECT_ACTOR_ROW_MARK + i];

        if (func_0204c138(actor)) {
            func_0204c178(actor, &pos, 0);
            pos.y += (s16)delta;
            y = pos.y;
            func_0204c140(actor, &pos, 0);
            if (y <= -20 || y >= 220) {
                func_0204c124(actor, FALSE);
            }
        }
    }
    for (i = 0; i < UN_SELECT_ROW_MARK_COUNT; i++) {
        ClActor *actor = wk->actors[UN_SELECT_ACTOR_ROW_MARK_2 + i];

        if (func_0204c138(actor)) {
            func_0204c178(actor, &pos, 1);
            pos.y += (s16)delta;
            y = pos.y;
            func_0204c140(actor, &pos, 1);
            if (y <= -28 || y >= 212) {
                func_0204c124(actor, FALSE);
            }
        }
    }
}

// Marks the floors of the latest visitors on the picture of the tower
static void UnSelect_PlaceVisitors(UnSelectWork *wk) {
    int i;

    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        ClActor *actor = wk->actors[UN_SELECT_ACTOR_VISITOR + i];

        if (wk->visitorFloors[i] == UN_SELECT_NO_FLOOR) {
            func_0204c124(actor, FALSE);
        } else {
            ClActorPos pos;

            func_0204c178(actor, &pos, 0xffff);
            pos.y = wk->visitorFloors[i] * 2 + 92;
            func_0204c140(actor, &pos, 0xffff);
        }
    }
}

static void UnSelect_SetBarPos(UnSelectWork *wk, int pos) {
    s16 y;
    ClActor *actor = wk->actors[UN_SELECT_ACTOR_BAR];
    ClActorPos actorPos;

    y = pos;
    if (y < 16) {
        y = 16;
    } else if (y > 152) {
        y = 152;
    }
    func_0204c178(actor, &actorPos, 0);
    actorPos.y = y;
    func_0204c140(actor, &actorPos, 0);
}

static void UnSelect_SetCursorPos(UnSelectWork *wk, int y) {
    ClActor *actor = wk->actors[UN_SELECT_ACTOR_CURSOR];
    ClActorPos pos;

    func_0204c178(actor, &pos, 0xffff);
    pos.y = y;
    func_0204c140(actor, &pos, 0xffff);
}

// Shows a mark that is hidden at y
static void UnSelect_ShowRowMark(UnSelectWork *wk, BOOL firstBG, int y) {
    int i;
    int count;
    ClActor **actors;
    u16 surface;

    if (firstBG) {
        surface = 0;
        actors = &wk->actors[UN_SELECT_ACTOR_ROW_MARK];
        count = UN_SELECT_ROW_MARK_COUNT;
    } else {
        surface = 1;
        actors = &wk->actors[UN_SELECT_ACTOR_ROW_MARK_2];
        count = UN_SELECT_ROW_MARK_COUNT;
    }
    for (i = 0; i < count; i++) {
        if (UnSelect_ShowActor(actors, i, surface, y)) {
            break;
        }
    }
}

// Shows an actor at y unless it is shown already, and returns whether it did
static BOOL UnSelect_ShowActor(ClActor **actors, int index, u16 surface, int y) {
    ClActorPos pos;
    BOOL shown;
    ClActor *actor = actors[index];

    shown = FALSE;
    if (!func_0204c138(actor)) {
        func_0204c124(actor, TRUE);
        shown = TRUE;
        func_0204c178(actor, &pos, surface);
        pos.y = y;
        func_0204c140(actor, &pos, surface);
    }
    return shown;
}

static BOOL UnSelect_IsVisitorFloor(UnSelectWork *wk, u32 index) {
    int i;

    for (i = 0; i < UNITY_TOWER_VISITOR_MAX; i++) {
        if (index == wk->visitorFloors[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

// The item of a country, or UN_SELECT_NO_FLOOR
static u32 UnSelect_FindCountry(u32 country) {
    int i;

    for (i = 0; i < UN_SELECT_FLOOR_COUNT; i++) {
        if (country == sUnSelectCountries[i]) {
            break;
        }
    }
    if (i >= UN_SELECT_FLOOR_COUNT) {
        return UN_SELECT_NO_FLOOR;
    }
    return i;
}
