// The Wi-Fi login and logout's messages and menus: a message window printed a character at a time, a title window and
// a yes/no menu. The name is the ROM's string, from GFL_HeapAllocate's call. Function names are ours.

#include "app/wifilogin_message.h"
#include "types.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/hw.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_menu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"

#define WIFILOGIN_MESSAGE_TITLE_BG 1

static void WifiLoginMessage_PrintWaitingFrom(WifiLoginMessage *msg, MsgData *msgData, u32 msgNo);

WifiLoginMessage *WifiLoginMessage_Create(HeapID heapId, u32 bank, u32 style) {
    WifiLoginMessage *msg = GFL_HeapAllocate(heapId, sizeof(WifiLoginMessage), TRUE, "wifilogin_message.c", 135);

    msg->heapId = heapId;
    msg->style = style;
    BmpWin_InitAllocator(msg->heapId);
    func_020232d0();
    msg->tcbManager = GFL_TCBExMgrCreate(msg->heapId, msg->heapId, 1, 0);
    msg->printQueue = func_02021998(msg->heapId);
    msg->strbuf = GFL_StrBufCreate(400, msg->heapId);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, msg->heapId);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, bank, msg->heapId);
    msg->wordSet = GFL_WordSetSystemCreateDefault(msg->heapId);
    msg->menu.res = AppTaskMenuRes_Create(5, 9, msg->font, msg->printQueue, msg->heapId);
    msg->frameChars = LoadCursorImageEndOfHeap(WifiLoginMessage_GetMessageBG(msg->style), 12, 0, msg->heapId);
    msg->frameCharsOther = LoadCursorImageEndOfHeap(WifiLoginMessage_GetNoticeBG(msg->style), 12, 0, msg->heapId);
    u32 palType = PALTYPE_MAIN_BG;

    if (msg->style == WIFILOGIN_MESSAGE_STYLE_PDWACC) {
        palType = PALTYPE_SUB_BG;
    }
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, palType, 0x160, 0x20, msg->heapId);
    return msg;
}

void WifiLoginMessage_Update(WifiLoginMessage *msg) {
    GFL_TCBExMgrUpdate(msg->tcbManager);
    func_02021a3c(msg->printQueue);
}

void WifiLoginMessage_Free(WifiLoginMessage *msg) {
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    WifiLoginMessage_ClearTitle(msg);
    GFL_BGSysFreeCharMemory(WifiLoginMessage_GetMessageBG(msg->style), CHAR_POS(msg->frameChars), CHAR_SIZE(msg->frameChars));
    GFL_BGSysFreeCharMemory(WifiLoginMessage_GetNoticeBG(msg->style), CHAR_POS(msg->frameCharsOther),
                            CHAR_SIZE(msg->frameCharsOther));
    func_020232d8();
    GFL_WordSetSystemFree(msg->wordSet);
    GFL_MsgDataFree(msg->msgData);
    GFL_FontFree(msg->font);
    GFL_StrBufFree(msg->strbuf);
    AppTaskMenuRes_Free(msg->menu.res);
    GFL_TCBExMgrFree(msg->tcbManager);
    func_02021c44(msg->printQueue);
    func_02021a18(msg->printQueue);
    if (msg->messageWin != NULL) {
        BmpWin_Free(msg->messageWin);
        msg->messageWin = NULL;
    }
    if (msg->unk30 != NULL) {
        BmpWin_Free(msg->unk30);
        msg->unk30 = NULL;
    }
    BmpWin_FreeAllocator();
    GFL_HeapFree(msg);
}

void WifiLoginMessage_Print(WifiLoginMessage *msg, u32 msgNo) {
    WifiLoginMessage_PrintFrom(msg, msg->msgData, msgNo);
}

// The BG of the message window of a message. The const pointer matters: the style read through it is not shared with
// the plain read that picks y, so it is loaded again before the call, as in the original
static inline u8 WifiLoginMessage_GetMessageBGOf(const WifiLoginMessage *msg) {
    return WifiLoginMessage_GetMessageBG(msg->style);
}

void WifiLoginMessage_PrintFrom(WifiLoginMessage *msg, MsgData *msgData, u32 msgNo) {
    BmpWin *window;
    u32 flags;
    u8 y;

    GFL_MsgDataLoadStrbuf(msgData, msgNo, msg->strbuf);
    y = 1;
    if (msg->style != WIFILOGIN_MESSAGE_STYLE_PDWACC) {
        y = 19;
    }
    if (msg->messageWin == NULL) {
        msg->messageWin = BmpWin_CreateDynamic(WifiLoginMessage_GetMessageBGOf(msg), 1, y, 30, 4, 11, TRUE);
    }
    window = msg->messageWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    flags = APP_PRINTSYS_COMMON_KEYS;
    if (msg->style == WIFILOGIN_MESSAGE_STYLE_PDWACC) {
        flags = APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH;
    }
    AppPrintsysCommon_Init(&msg->printsys, flags);
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
    }
    msg->stream =
        func_02022268(window, 0, 0, msg->strbuf, msg->font, func_02017bcc(), msg->tcbManager, 2, msg->heapId, 15);
    BmpWin_DrawFrame(window, WINFRAME_TRANSFER_VBLANK, msg->frameChars, 12);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(WifiLoginMessage_GetMessageBGOf(msg));
}

void WifiLoginMessage_PrintWaiting(WifiLoginMessage *msg, u32 msgNo) {
    WifiLoginMessage_PrintWaitingFrom(msg, msg->msgData, msgNo);
}

// Prints a message and shows the wait icon on its window
static void WifiLoginMessage_PrintWaitingFrom(WifiLoginMessage *msg, MsgData *msgData, u32 msgNo) {
    WifiLoginMessage_PrintFrom(msg, msgData, msgNo);
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    msg->waitIcon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), msg->messageWin, 15, 16, msg->heapId);
}

// Whether the message is printed and was continued
BOOL WifiLoginMessage_IsDone(WifiLoginMessage *msg) {
    BOOL done = TRUE;

    if (msg->stream != NULL) {
        done = AppPrintsysCommon_Update(&msg->printsys, msg->stream);
        if (done) {
            func_020223cc(msg->stream);
            msg->stream = NULL;
        }
    }
    return done;
}

void WifiLoginMessage_ClearWindow(WifiLoginMessage *msg) {
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    if (msg->messageWin != NULL) {
        BmpWin_ClearFrame(msg->messageWin, WINFRAME_TRANSFER_NONE);
        BmpWin_ClearScreen(msg->messageWin);
        GFL_BGSysQueueScrLoad(WifiLoginMessage_GetMessageBG(msg->style));
    }
}

// Prints a title in the title window
void WifiLoginMessage_PrintTitle(WifiLoginMessage *msg, u32 msgNo) {
    BmpWin *window;

    if (msg->titleWin == NULL) {
        msg->titleWin = BmpWin_CreateDynamic(WIFILOGIN_MESSAGE_TITLE_BG, 4, 7, 24, 2, 11, TRUE);
    }
    window = msg->titleWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->strbuf);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), 0, 0, msg->strbuf, msg->font, PRINT_COLOR(15, 6, 0));
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(WIFILOGIN_MESSAGE_TITLE_BG);
}

void WifiLoginMessage_ClearTitle(WifiLoginMessage *msg) {
    if (msg->titleWin != NULL) {
        BmpWin_ClearScreen(msg->titleWin);
        BmpWin_Free(msg->titleWin);
        msg->titleWin = NULL;
        GFL_BGSysQueueScrLoad(WIFILOGIN_MESSAGE_TITLE_BG);
    }
}

// Creates the yes/no menu: task menu buttons or a confirm dialog, as the style says. layout 0 or 1 puts it higher or
// lower on the screen
WifiLoginMessageMenu *WifiLoginMessage_CreateMenu(WifiLoginMessage *msg, u32 layout, BOOL dimTopScreen, u32 cursorPos) {
    WifiLoginMessageMenu *menu = &msg->menu;

    menu->style = msg->style;
    switch (menu->style) {
    case WIFILOGIN_MESSAGE_STYLE_PDWACC: {
        AppTaskMenuInit init;

        init.heapId = msg->heapId;
        init.itemCount = 2;
        init.items = menu->items;
        switch (layout) {
        case 0:
            init.x = 32;
            init.y = 13;
            init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
            break;
        case 1:
            init.x = 32;
            init.y = 24;
            init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
            break;
        }
        init.width = 13;
        init.height = 3;
        menu->items[0].str = GFL_StrBufCreate(100, msg->heapId);
        GFL_MsgDataLoadStrbuf(msg->msgData, 28, menu->items[0].str);
        menu->items[0].color = PRINT_COLOR(14, 15, 0);
        menu->items[1].str = GFL_StrBufCreate(100, msg->heapId);
        GFL_MsgDataLoadStrbuf(msg->msgData, 29, menu->items[1].str);
        menu->items[1].color = PRINT_COLOR(14, 15, 0);
        menu->taskMenu = AppTaskMenu_Create(&init, menu->res);
        GFL_StrBufFree(menu->items[0].str);
        GFL_StrBufFree(menu->items[1].str);
        if (dimTopScreen) {
            gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_OBJ, -8);
        }
        AppTaskMenu_SetCursorPos(menu->taskMenu, cursorPos);
        break;
    }
    case WIFILOGIN_MESSAGE_STYLE_GSYNC:
        menu->dialog = WifiLoginMessage_CreateConfirmDialog(msg, layout, cursorPos);
        menu->result = BMPMENU_NULL;
        break;
    }
    return menu;
}

void WifiLoginMessage_FreeMenu(WifiLoginMessageMenu *menu) {
    switch (menu->style) {
    case WIFILOGIN_MESSAGE_STYLE_PDWACC:
        AppTaskMenu_Free(menu->taskMenu);
        break;
    case WIFILOGIN_MESSAGE_STYLE_GSYNC:
        break;
    }
}

void WifiLoginMessage_UpdateMenu(WifiLoginMessageMenu *menu) {
    switch (menu->style) {
    case WIFILOGIN_MESSAGE_STYLE_PDWACC:
        AppTaskMenu_Update(menu->taskMenu);
        break;
    case WIFILOGIN_MESSAGE_STYLE_GSYNC:
        if (!ConfirmDialog_IsPrinted(menu->dialog)) {
            menu->result = BMPMENU_NULL;
        } else {
            menu->result = ConfirmDialog_Update(menu->dialog);
        }
        break;
    }
}

// Whether the player has answered the yes/no menu
BOOL WifiLoginMessage_IsMenuAnswered(WifiLoginMessageMenu *menu) {
    switch (menu->style) {
    case WIFILOGIN_MESSAGE_STYLE_PDWACC:
        return AppTaskMenu_IsFlashFinished(menu->taskMenu);
    case WIFILOGIN_MESSAGE_STYLE_GSYNC:
        if (menu->result != BMPMENU_NULL) {
            return TRUE;
        }
        return FALSE;
    default:
        GFL_ASSERT(0);
        return FALSE;
    }
}

// The answer to the yes/no menu, 0 for yes
u8 WifiLoginMessage_GetMenuAnswer(WifiLoginMessageMenu *menu) {
    switch (menu->style) {
    case WIFILOGIN_MESSAGE_STYLE_PDWACC:
        return AppTaskMenu_GetCursorPos(menu->taskMenu);
    case WIFILOGIN_MESSAGE_STYLE_GSYNC:
        return menu->result;
    default:
        GFL_ASSERT(0);
        return 0;
    }
}

// Prints a message in the window of the notices, which it creates the first time
void WifiLoginMessage_PrintNotice(WifiLoginMessage *msg, u32 msgNo) {
    StrBuf *formatted;
    BmpWin *window;

    GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->strbuf);
    formatted = GFL_StrBufClone(msg->strbuf, msg->heapId);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, formatted);
    if (msg->unk30 == NULL) {
        msg->unk30 = BmpWin_CreateDynamic(WifiLoginMessage_GetNoticeBG(msg->style), 1, 1, 30, 16, 11, TRUE);
    }
    window = msg->unk30;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, msg->strbuf, msg->font);
    BmpWin_DrawFrame(window, WINFRAME_TRANSFER_VBLANK, msg->frameCharsOther, 12);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(WifiLoginMessage_GetNoticeBG(msg->style));
    GFL_StrBufFree(formatted);
}

// Clears the window of the notices
void WifiLoginMessage_ClearNotice(WifiLoginMessage *msg) {
    if (msg->unk30 != NULL) {
        BmpWin_ClearFrame(msg->unk30, WINFRAME_TRANSFER_NONE);
        BmpWin_ClearScreen(msg->unk30);
        GFL_BGSysQueueScrLoad(WifiLoginMessage_GetNoticeBG(msg->style));
    }
}

// The BG of the message window
u8 WifiLoginMessage_GetMessageBG(u32 style) {
    switch (style) {
    case WIFILOGIN_MESSAGE_STYLE_GSYNC:
        return 1;
    case WIFILOGIN_MESSAGE_STYLE_PDWACC:
        return 5;
    default:
        GFL_ASSERT(0);
        return 0;
    }
}

// The BG of the window of the notices
u8 WifiLoginMessage_GetNoticeBG(u32 style) {
    switch (style) {
    case WIFILOGIN_MESSAGE_STYLE_GSYNC:
        return 2;
    case WIFILOGIN_MESSAGE_STYLE_PDWACC:
        return 5;
    default:
        GFL_ASSERT(0);
        return 0;
    }
}

// The confirm dialog of the GSYNC style, with its place on the screen by layout
BmpMenu *WifiLoginMessage_CreateConfirmDialog(WifiLoginMessage *msg, u32 layout, u32 cursorPos) {
    ConfirmDialogSetup setup = {1, 0, 0, 11, 0};

    setup.unk4 = msg->frameChars;
    switch (layout) {
    case 0:
        setup.x = 24;
        setup.y = 13;
        break;
    case 1:
        setup.x = 24;
        setup.y = 19;
        break;
    }
    return ShopUI_CreateConfirmDialog(&setup, setup.unk4, 12, cursorPos, msg->heapId);
}
