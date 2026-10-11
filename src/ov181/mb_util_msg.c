#include "types.h"
#include "app/mb_parent/mb_util_msg.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/talkmsgwin.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_menu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/wordset.h"
#include "text/script/global_10520.h"

// The messages, windows and menus of the DS Download Play parent (mb_util_msg.c): one message window that prints
// through a stream or a print queue, the yes/no menus, and the frame of the field's talk window when overlay 36 is
// loaded for it. The names are ours

// The message window's shapes, by place and height
enum {
    MB_UTIL_MSG_WINDOW_NONE,
    MB_UTIL_MSG_WINDOW_MIDDLE,
    MB_UTIL_MSG_WINDOW_TALL,
    MB_UTIL_MSG_WINDOW_TALLER,
    MB_UTIL_MSG_WINDOW_BOTTOM_2,
    MB_UTIL_MSG_WINDOW_BOTTOM_4,
    MB_UTIL_MSG_WINDOW_BOTTOM_6,
    MB_UTIL_MSG_WINDOW_TOP_4,
    // The bottom window with the talk window's frame
    MB_UTIL_MSG_WINDOW_TALK,
};

struct MBUtilMsg {
    HeapID heapId;
    u8 msgBg;
    u8 menuBg;
    u32 windowType;
    // Whether the messages wait for the keys too
    BOOL useKeys;
    TCBExManager *tcbManager;
    BmpWin *window;
    BmpWin *subWindow;
    Font *font;
    PrintStream *stream;
    MsgData *msgData;
    StrBuf *strbuf;
    WordSet *wordSet;
    AppPrintsysCommon printCommon;
    PrintQueue *queue;
    BOOL queued;
    BOOL mapShown;
    BmpWin *queuedSubWindow;
    u8 subQueued;
    AppTaskMenu *menu;
    AppTaskMenuRes *menuRes;
    KeyCursor *cursor;
    ConfirmDialogSetup confirmSetup;
    BmpMenu *confirm;
    TalkMsgWinSys *talkWin;
    // Whether to show the wait icon once the queued message is printed
    BOOL showWaitIcon;
    WaitIcon *waitIcon;
};

static void MBUtilMsg_FreeSubWindow(MBUtilMsg *msg);

MBUtilMsg *MBUtilMsg_Create(HeapID heapId, u8 msgBg, u8 menuBg, u32 fileId, BOOL useTalkWin, BOOL useKeys) {
    MBUtilMsg *msg = GFL_HeapAllocate(heapId, sizeof(MBUtilMsg), TRUE, "mb_util_msg.c", 104);

    msg->heapId = heapId;
    msg->msgBg = msgBg;
    msg->menuBg = menuBg;
    msg->useKeys = useKeys;
    msg->window = NULL;
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, msg->heapId);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, fileId, msg->heapId);
    LoadSysMsgBox(msgBg, 1, 13, 0, msg->heapId);
    if (msgBg < 4) {
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1c0, 0x20, msg->heapId);
    } else {
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, msg->heapId);
    }
    func_020232d8();
    msg->tcbManager = GFL_TCBExMgrCreate(msg->heapId, msg->heapId, 3, 0x100);
    msg->stream = NULL;
    msg->strbuf = NULL;
    msg->wordSet = NULL;
    msg->queue = func_02021998(msg->heapId);
    msg->menuRes = AppTaskMenuRes_Create(msg->menuBg, 10, msg->font, msg->queue, msg->heapId);
    msg->menu = NULL;
    msg->confirm = NULL;
    msg->cursor = KeyCursor_Create(15, useKeys, TRUE, msg->heapId);
    if (useTalkWin == TRUE) {
        TalkMsgWinSetup setup;

        GFL_OvlLoad(OVERLAY_ID(36));
        setup.heapId = heapId;
        setup.unk4 = 0;
        setup.font = msg->font;
        setup.unkC = 10;
        setup.bg = msgBg;
        setup.unk14 = 12;
        setup.unk15 = 14;
        msg->talkWin = func_ov036_0218b1f8(&setup);
    } else {
        msg->talkWin = NULL;
    }
    msg->queued = FALSE;
    msg->mapShown = FALSE;
    msg->showWaitIcon = FALSE;
    msg->waitIcon = NULL;
    return msg;
}

void MBUtilMsg_Delete(MBUtilMsg *msg) {
    if (msg->talkWin != NULL) {
        func_ov036_0218b320(msg->talkWin);
        GFL_OvlUnload(OVERLAY_ID(36));
    }
    if (msg->menu != NULL) {
        AppTaskMenu_Free(msg->menu);
    }
    if (msg->confirm != NULL) {
        ConfirmDialog_Free(msg->confirm);
    }
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    KeyCursor_Free(msg->cursor);
    AppTaskMenuRes_Free(msg->menuRes);
    func_02021a18(msg->queue);
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
    }
    if (msg->strbuf != NULL) {
        GFL_StrBufFree(msg->strbuf);
    }
    if (msg->wordSet != NULL) {
        MBUtilMsg_FreeWordSet(msg);
    }
    GFL_MsgDataFree(msg->msgData);
    if (msg->window != NULL) {
        BmpWin_Free(msg->window);
    }
    MBUtilMsg_FreeSubWindow(msg);
    GFL_FontFree(msg->font);
    GFL_TCBExMgrFree(msg->tcbManager);
    GFL_HeapFree(msg);
}

void MBUtilMsg_Update(MBUtilMsg *msg) {
    GFL_TCBExMgrUpdate(msg->tcbManager);
    if (msg->stream != NULL) {
        BOOL done = AppPrintsysCommon_Update(&msg->printCommon, msg->stream);

        KeyCursor_Update(msg->cursor, msg->stream, msg->window);
        if (done == TRUE) {
            func_020223cc(msg->stream);
            msg->stream = NULL;
        }
    }
    func_02021a3c(msg->queue);
    if (msg->queuedSubWindow != NULL && msg->subQueued) {
        PrintQueue *queue = msg->queue;

        if (!func_02021c1c(queue, BmpWin_GetBitmap(msg->queuedSubWindow))) {
            BmpWin_FlushChar(msg->queuedSubWindow);
            msg->subQueued = FALSE;
        }
    }
    if (msg->queued == TRUE && !func_02021c1c(msg->queue, BmpWin_GetBitmap(msg->window))) {
        BmpWin_FlushChar(msg->window);
        msg->queued = FALSE;
        if (msg->showWaitIcon == TRUE) {
            msg->showWaitIcon = FALSE;
            msg->waitIcon = WaitIcon_CreateTCBEx(msg->tcbManager, msg->window, 15, 16, msg->heapId);
        }
    }
    if (msg->menu != NULL) {
        AppTaskMenu_Update(msg->menu);
    }
}

void MBUtilMsg_SetWindow(MBUtilMsg *msg, u32 type) {
    if (msg->windowType != type) {
        if (msg->window != NULL) {
            BmpWin_Free(msg->window);
            msg->window = NULL;
        }
        switch (type) {
        case MB_UTIL_MSG_WINDOW_NONE:
            break;
        case MB_UTIL_MSG_WINDOW_MIDDLE:
            msg->window = BmpWin_CreateDynamic(msg->msgBg, 3, 8, 26, 8, 14, TRUE);
            break;
        case MB_UTIL_MSG_WINDOW_TALL:
            msg->window = BmpWin_CreateDynamic(msg->msgBg, 3, 2, 26, 18, 14, TRUE);
            break;
        case MB_UTIL_MSG_WINDOW_TALLER:
            msg->window = BmpWin_CreateDynamic(msg->msgBg, 3, 2, 26, 20, 14, TRUE);
            break;
        case MB_UTIL_MSG_WINDOW_BOTTOM_2:
            msg->window = BmpWin_CreateDynamic(msg->msgBg, 1, 21, 30, 2, 14, TRUE);
            break;
        case MB_UTIL_MSG_WINDOW_BOTTOM_4:
        case MB_UTIL_MSG_WINDOW_TALK:
            msg->window = BmpWin_CreateDynamic(msg->msgBg, 1, 19, 30, 4, 14, TRUE);
            break;
        case MB_UTIL_MSG_WINDOW_BOTTOM_6:
            msg->window = BmpWin_CreateDynamic(msg->msgBg, 1, 17, 30, 6, 14, TRUE);
            break;
        case MB_UTIL_MSG_WINDOW_TOP_4:
            msg->window = BmpWin_CreateDynamic(msg->msgBg, 1, 1, 30, 4, 14, TRUE);
            break;
        }
        msg->windowType = type;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 0);
    BmpWin_FlushChar(msg->window);
    BmpWin_FlushMap(msg->window);
    GFL_BGSysLoadScr(msg->msgBg);
}

void MBUtilMsg_Print(MBUtilMsg *msg, u32 msgId, s32 wait) {
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    msg->mapShown = FALSE;
    msg->showWaitIcon = FALSE;
    if (msg->strbuf != NULL) {
        GFL_StrBufFree(msg->strbuf);
        msg->strbuf = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 15);
    msg->strbuf = GFL_MsgDataLoadStrbufNew(msg->msgData, msgId);
    if (msg->wordSet != NULL) {
        StrBuf *strbuf = GFL_StrBufCreate(379, msg->heapId);

        GFL_WordSetFormatStrbuf(msg->wordSet, strbuf, msg->strbuf);
        GFL_StrBufFree(msg->strbuf);
        msg->strbuf = strbuf;
    }
    msg->stream = func_02022268(msg->window, 0, 0, msg->strbuf, msg->font, wait, msg->tcbManager, 2, msg->heapId, 15);
    if (msg->windowType == MB_UTIL_MSG_WINDOW_TALK) {
        func_ov036_0218b5ac(msg->talkWin, msg->window, 0);
    } else {
        BmpWin_DrawFrame(msg->window, TRUE, 1, 13);
    }
    if (msg->useKeys == TRUE) {
        AppPrintsysCommon_Init(&msg->printCommon, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    } else {
        AppPrintsysCommon_Init(&msg->printCommon, APP_PRINTSYS_COMMON_TOUCH);
    }
}

void MBUtilMsg_PrintAtOnce(MBUtilMsg *msg, u32 msgId) {
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    if (msg->strbuf != NULL) {
        GFL_StrBufFree(msg->strbuf);
        msg->strbuf = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 15);
    msg->strbuf = GFL_MsgDataLoadStrbufNew(msg->msgData, msgId);
    if (msg->wordSet != NULL) {
        StrBuf *strbuf = GFL_StrBufCreate(379, msg->heapId);

        GFL_WordSetFormatStrbuf(msg->wordSet, strbuf, msg->strbuf);
        GFL_StrBufFree(msg->strbuf);
        msg->strbuf = strbuf;
    }
    func_02021c54(msg->queue, BmpWin_GetBitmap(msg->window), 0, 0, msg->strbuf, msg->font);
    msg->queued = TRUE;
    if (msg->windowType == MB_UTIL_MSG_WINDOW_TALK) {
        func_ov036_0218b5ac(msg->talkWin, msg->window, 0);
        GFL_BGSysLoadScr(msg->msgBg);
    } else {
        BmpWin_DrawFrame(msg->window, TRUE, 1, 13);
    }
}

void MBUtilMsg_ClearWindow(MBUtilMsg *msg) {
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 0);
    if (msg->windowType == MB_UTIL_MSG_WINDOW_TALK) {
        func_ov036_0218b5bc(msg->talkWin, msg->window);
        GFL_BGSysLoadScr(msg->msgBg);
    } else {
        BmpWin_ClearFrame(msg->window, TRUE);
    }
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
}

void MBUtilMsg_CreateWordSet(MBUtilMsg *msg) {
    if (msg->wordSet == NULL) {
        msg->wordSet = GFL_WordSetSystemCreateDefault(msg->heapId);
    }
}

void MBUtilMsg_FreeWordSet(MBUtilMsg *msg) {
    if (msg->wordSet != NULL) {
        GFL_WordSetSystemFree(msg->wordSet);
        msg->wordSet = NULL;
    }
}

void MBUtilMsg_SetNumber(MBUtilMsg *msg, u32 index, s32 number, u32 digits) {
    WordSetNumber(msg->wordSet, index, number, digits, 0, TRUE);
}

void MBUtilMsg_SetNumberZeroPadded(MBUtilMsg *msg, u32 index, s32 number, u32 digits) {
    WordSetNumber(msg->wordSet, index, number, digits, 2, TRUE);
}

void MBUtilMsg_CreateYesNoMenu(MBUtilMsg *msg, u32 pos) {
    AppTaskMenuItem items[2];
    AppTaskMenuInit init;

    items[0].str = GFL_MsgDataLoadStrbufNew(msg->msgData, 0x12);
    items[1].str = GFL_MsgDataLoadStrbufNew(msg->msgData, 0x13);
    items[0].color = 0x39e3;
    items[1].color = 0x39e3;
    items[0].type = 0;
    items[1].type = 0;
    init.heapId = msg->heapId;
    init.itemCount = 2;
    init.items = items;
    init.posType = 0;
    switch (pos) {
    case 0:
        init.x = 24;
        init.y = 6;
        break;
    case 1:
        init.x = 24;
        init.y = 12;
        break;
    case 2:
        init.x = 24;
        init.y = 18;
        break;
    }
    init.width = 8;
    init.height = 3;
    msg->menu = AppTaskMenu_Create(&init, msg->menuRes);
    GFL_StrBufFree(items[0].str);
    GFL_StrBufFree(items[1].str);
}

void MBUtilMsg_FreeMenu(MBUtilMsg *msg) {
    AppTaskMenu_Free(msg->menu);
    msg->menu = NULL;
}

// The item picked, from 1, or 0 while the menu waits
int MBUtilMsg_GetMenuResult(MBUtilMsg *msg) {
    if (AppTaskMenu_IsFlashFinished(msg->menu) == TRUE) {
        return AppTaskMenu_GetCursorPos(msg->menu) + 1;
    }
    return 0;
}

void MBUtilMsg_CreateConfirm(MBUtilMsg *msg, u32 unused) {
    msg->confirmSetup.bg = msg->menuBg;
    msg->confirmSetup.x = 24;
    msg->confirmSetup.y = 13;
    msg->confirmSetup.palette = 14;
    msg->confirmSetup.unk4 = 0;
    msg->confirm = ShopUI_CreateConfirmDialog(&msg->confirmSetup, 1, 13, 0, msg->heapId);
}

void MBUtilMsg_ForgetConfirm(MBUtilMsg *msg) {
    msg->confirm = NULL;
}

// 1 for yes, 2 for no, 0 while the dialog waits
int MBUtilMsg_UpdateConfirm(MBUtilMsg *msg) {
    u32 result = ConfirmDialog_Update(msg->confirm);

    if (result == 0) {
        msg->confirm = NULL;
        return 1;
    }
    if (result == -2) {
        msg->confirm = NULL;
        return 2;
    }
    return 0;
}

void MBUtilMsg_PrintNoWireless(MBUtilMsg *msg, s32 wait) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, msg->heapId);

    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    msg->mapShown = FALSE;
    msg->showWaitIcon = FALSE;
    if (msg->strbuf != NULL) {
        GFL_StrBufFree(msg->strbuf);
        msg->strbuf = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 15);
    msg->strbuf = GFL_MsgDataLoadStrbufNew(msgData, Global10520_Text_WirelessCommunicationsTurnedOff);
    msg->stream = func_02022268(msg->window, 0, 0, msg->strbuf, msg->font, wait, msg->tcbManager, 2, msg->heapId, 15);
    BmpWin_DrawFrame(msg->window, TRUE, 1, 13);
    GFL_MsgDataFree(msgData);
    if (msg->useKeys == TRUE) {
        AppPrintsysCommon_Init(&msg->printCommon, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    } else {
        AppPrintsysCommon_Init(&msg->printCommon, APP_PRINTSYS_COMMON_TOUCH);
    }
}

MsgData *MBUtilMsg_GetMsgData(MBUtilMsg *msg) {
    return msg->msgData;
}

WordSet *MBUtilMsg_GetWordSet(MBUtilMsg *msg) {
    return msg->wordSet;
}

Font *MBUtilMsg_GetFont(MBUtilMsg *msg) {
    return msg->font;
}

BOOL MBUtilMsg_IsQueueDone(MBUtilMsg *msg) {
    return func_02021c0c(msg->queue);
}

BOOL MBUtilMsg_IsPrintDone(MBUtilMsg *msg) {
    if (msg->stream == NULL) {
        return TRUE;
    }
    return FALSE;
}

void MBUtilMsg_ShowWindow(MBUtilMsg *msg, BOOL shown) {
    msg->mapShown = shown;
    BmpWin_FlushMap(msg->window);
    GFL_BGSysLoadScr(msg->msgBg);
}

void MBUtilMsg_SetShowWaitIcon(MBUtilMsg *msg, BOOL show) {
    msg->showWaitIcon = show;
}

static void MBUtilMsg_FreeSubWindow(MBUtilMsg *msg) {
    if (msg->subWindow != NULL) {
        BmpWin_Free(msg->subWindow);
        msg->subWindow = NULL;
        msg->queuedSubWindow = NULL;
    }
}
