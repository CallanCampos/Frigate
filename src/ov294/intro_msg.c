#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "demo/intro.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "system/app_keycursor.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"

// The intro's message window and its yes/no menu

typedef struct {
    BmpMenuList *list;
    ListMenuOption *options;
    s32 result;
} IntroMenu;

struct IntroMsg {
    HeapID heapId;
    PrintQueue *printQueue;
    WordSet *wordSet;
    MsgData *msgData;
    Font *font;
    PrintStream *printStream;
    TCBExManager *tcbManager;
    BmpWin *window;
    KeyCursor *keyCursor;
    IntroMenu menu;
    PrintWindow menuPrint;
    BmpWin *menuWindow;
    StrBuf *strbuf;
    StrBuf *expanded;
    WaitIcon *waitIcon;
};

// The two window frames that IntroMsg_Print draws
#define FRAME_0_CHAR 7
#define FRAME_0_PALETTE 9
#define FRAME_1_CHAR 16
#define FRAME_1_PALETTE 8

#define WINDOW_PALETTE 10
#define STRBUF_SIZE 1600

IntroMsg *IntroMsg_Create(HeapID heapId) {
    IntroMsg *msg = GFL_HeapAllocate(heapId, sizeof(IntroMsg), TRUE, "intro_msg.c", 129);
    BmpWin *window;

    msg->heapId = heapId;
    msg->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    msg->printQueue = func_02021998(heapId);
    msg->tcbManager = GFL_TCBExMgrCreate(msg->heapId, msg->heapId, 2, 0);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, WINDOW_PALETTE * 0x20, 0x20, heapId);
    LoadSysMsgBox(1, FRAME_0_CHAR, FRAME_0_PALETTE, 0, heapId);
    LoadSysMsgBox(1, FRAME_1_CHAR, FRAME_1_PALETTE, 1, heapId);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 1, 0, msg->heapId);
    msg->strbuf = GFL_StrBufCreate(STRBUF_SIZE, msg->heapId);
    msg->expanded = GFL_StrBufCreate(STRBUF_SIZE, msg->heapId);
    msg->menuWindow = BmpWin_CreateDynamic(1, 22, 13, 9, 4, WINDOW_PALETTE, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(msg->menuWindow), 0);
    window = msg->menuWindow;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    msg->window = BmpWin_CreateDynamic(1, 1, 19, 30, 4, WINDOW_PALETTE, 1);
    msg->keyCursor = KeyCursor_Create(15, TRUE, TRUE, msg->heapId);
    msg->waitIcon = WaitIcon_Alloc(msg->heapId);
    return msg;
}

void IntroMsg_Free(IntroMsg *msg) {
    IntroMsg_HideWaitIcon(msg);
    KeyCursor_Free(msg->keyCursor);
    GFL_StrBufFree(msg->strbuf);
    GFL_StrBufFree(msg->expanded);
    GFL_FontFree(msg->font);
    if (msg->printStream != NULL) {
        func_020223cc(msg->printStream);
    }
    BmpWin_Free(msg->window);
    if (msg->menuWindow != NULL) {
        BmpWin_Free(msg->menuWindow);
    }
    GFL_TCBExMgrFree(msg->tcbManager);
    func_02021a18(msg->printQueue);
    if (msg->msgData != NULL) {
        GFL_MsgDataFree(msg->msgData);
    }
    GFL_WordSetSystemFree(msg->wordSet);
    GFL_HeapFree(msg);
}

void IntroMsg_LoadMessages(IntroMsg *msg, BOOL preload, u16 fileId) {
    if (msg->msgData != NULL) {
        GFL_MsgDataFree(msg->msgData);
    }
    msg->msgData = GFL_MsgSysLoadData(preload, ARCID_SYSTEM_MESSAGE, fileId, msg->heapId);
}

void IntroMsg_Update(IntroMsg *msg) {
    GFL_TCBExMgrUpdate(msg->tcbManager);
    func_02021a3c(msg->printQueue);
}

void IntroMsg_Print(IntroMsg *msg, u32 messageId, BOOL frame) {
    s32 wait = func_02017bcc();
    BmpWin *window = msg->window;

    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, msg->strbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->expanded, msg->strbuf);
    msg->printStream =
        func_02022268(window, 4, 0, msg->expanded, msg->font, wait, msg->tcbManager, 0xffff, msg->heapId, 15);
    if (!frame) {
        BmpWin_DrawFrame(window, WINFRAME_TRANSFER_VBLANK, FRAME_0_CHAR, FRAME_0_PALETTE);
    } else {
        BmpWin_DrawFrame(window, WINFRAME_TRANSFER_VBLANK, FRAME_1_CHAR, FRAME_1_PALETTE);
    }
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

void IntroMsg_Clear(IntroMsg *msg) {
    BmpWin_ClearFrame(msg->window, WINFRAME_TRANSFER_NOW);
}

u32 IntroMsg_GetPrintState(IntroMsg *msg) {
    if (msg->printStream == NULL) {
        return PRINT_STREAM_DONE;
    }
    return func_020223b4(msg->printStream);
}

// Prints the message on, with A, B or a touch hurrying it and going past its pauses. Returns TRUE once it has ended
BOOL IntroMsg_UpdatePrint(IntroMsg *msg) {
    if (msg->printStream != NULL) {
        KeyCursor_Update(msg->keyCursor, msg->printStream, msg->window);
        switch (func_020223b4(msg->printStream)) {
        case PRINT_STREAM_DONE:
            func_020223cc(msg->printStream);
            msg->printStream = NULL;
            return TRUE;
        case PRINT_STREAM_PAUSED:
            if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A || GCTX_HIDGetPressedKeys() == PAD_BUTTON_B ||
                func_0203da48()) {
                func_020223bc(msg->printStream);
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            break;
        case PRINT_STREAM_RUNNING:
            if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da2c()) {
                func_020223e0(msg->printStream, 0);
            }
            break;
        }
        return FALSE;
    }
    return FALSE;
}

static PrintWindow *IntroMsg_InitMenuPrint(IntroMsg *msg, BmpWin *window) {
    msg->menuPrint.window = window;
    msg->menuPrint.flushPending = FALSE;
    return &msg->menuPrint;
}

void IntroMsg_OpenMenu(IntroMsg *msg, const IntroMenuItem *items, u32 count, BOOL a3) {
    IntroMenu *menu;
    BmpWin *window;
    const IntroMenuItem *item;
    HeapID heapId;
    u32 i;
    BmpMenuListHeader header;

    menu = &msg->menu;
    heapId = msg->heapId;
    window = msg->menuWindow;
    sys_memset(menu, 0, sizeof(IntroMenu));
    menu->options = ListMenuCore_CreateOptionList(count, heapId);
    for (i = 0; i < count; i++) {
        item = &items[i];
        GFL_MsgDataLoadStrbuf(msg->msgData, item->message, msg->strbuf);
        ListMenuCore_AppendStrBufOption(&menu->options[i], msg->strbuf, item->value, heapId);
    }
    sys_memset(&header, 0, sizeof(BmpMenuListHeader));
    header.options = menu->options;
    header.count = count;
    header.maxShown = 5;
    header.labelX = 0;
    header.itemX = 16;
    header.cursorX = 0;
    header.y = 2;
    header.fgColor = 1;
    header.bgColor = 15;
    header.shadowColor = 2;
    header.letterSpacing = 0;
    header.lineSpacing = 0;
    header.pageSkip = BMPMENULIST_SKIP_LR_KEY;
    header.fontId = 0;
    header.cursorDisplay = BMPMENULIST_CURSOR_SHOW;
    header.work = NULL;
    header.fontSizeX = 16;
    header.fontSizeY = 16;
    header.msgData = NULL;
    header.printWindow = IntroMsg_InitMenuPrint(msg, window);
    header.queue = msg->printQueue;
    header.font = msg->font;
    header.wait = 20;
    menu->list = BmpMenuList_Create(&header, 0, 0, heapId);
    BmpMenuList_LoadCursor(menu->list, heapId);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    BmpWin_DrawFrame(window, WINFRAME_TRANSFER_VBLANK, FRAME_0_CHAR, FRAME_0_PALETTE);
    if (a3) {
        BmpMenuList_SetCancelDisabled(menu->list, 0);
    } else {
        BmpMenuList_SetCancelDisabled(menu->list, 1);
    }
}

void IntroMsg_CloseMenu(IntroMsg *msg) {
    IntroMenu *menu = &msg->menu;

    BmpWin_ClearFrame(msg->menuWindow, WINFRAME_TRANSFER_NOW);
    GFL_BitmapFill(BmpWin_GetBitmap(msg->menuWindow), 0);
    BmpWin_FlushChar(msg->menuWindow);
    BmpMenuList_Free(msg->menu.list, NULL, NULL);
    ListMenuCore_FreeOptionList(menu->options);
    sys_memset(menu, 0, sizeof(IntroMenu));
}

void IntroMsg_UpdateMenu(IntroMsg *msg) {
    IntroMenu *menu = &msg->menu;

    menu->result = BmpMenuList_Update(menu->list);
}

u32 IntroMsg_GetMenuResult(IntroMsg *msg, s32 *value) {
    s32 result = msg->menu.result;

    switch (result) {
    case BMPMENULIST_NULL:
        return INTRO_MENU_NONE;
    case BMPMENULIST_CANCEL:
        return INTRO_MENU_CANCELLED;
    }
    if (value != NULL) {
        *value = result;
    }
    return INTRO_MENU_CHOSEN;
}

WordSet *IntroMsg_GetWordSet(IntroMsg *msg) {
    return msg->wordSet;
}

void IntroMsg_ShowWaitIcon(IntroMsg *msg) {
    WaitIcon_Start(msg->waitIcon, GFL_VBlankGetTCBMgr(), msg->window, 15, 16);
}

void IntroMsg_HideWaitIcon(IntroMsg *msg) {
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
}
