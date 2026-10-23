// Game Sync's messages: the message window, the info window below it, the yes/no menu and the Game Sync ID's text.
// The name is the ROM's string, from GFL_HeapAllocate's call. Function names are ours.

#include "types.h"
#include "app/gsync/gsync_message.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/hw.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"

#define GSYNC_MESSAGE_WIN_MAX 12

struct GSyncMessage {
    u32 unk0;
    // The frames of the info window and the message window, as LoadCursorImageEndOfHeap gives them
    u32 infoFrameChars;
    u32 frameChars;
    u8 unkC[0x10];
    ButtonMan *buttonMan;
    MsgData *msgData;
    WordSet *wordSet;
    Font *font;
    StrBuf *strBuf;
    StrBuf *tmpBuf;
    u8 unk34[0xc];
    KeyCursor *keyCursor;
    BmpWin *msgWin;
    BmpWin *infoWin;
    BmpWin *wins[GSYNC_MESSAGE_WIN_MAX];
    WaitIcon *waitIcon;
    PrintStream *printStream;
    TCBExManager *tcbMgr;
    PrintQueue *printQueue;
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *menuRes;
    AppPrintsysCommon printCommon;
    HeapID heapId;
};

static void GSyncMessage_DeleteButtons(GSyncMessage *msg);

// The characters of a Game Sync ID, without I, O, 0 and 1
static const u16 sGSyncIdChars[32] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7', '8', '9',
};

static void GSyncMessage_DeleteButtons(GSyncMessage *msg) {
    if (msg->buttonMan != NULL) {
        GFL_BMN_Delete(msg->buttonMan);
        msg->buttonMan = NULL;
    }
}

GSyncMessage *GSyncMessage_Create(HeapID heapId, u32 msgFile) {
    GSyncMessage *msg = GFL_HeapAllocate(heapId, sizeof(GSyncMessage), TRUE, "gsync_message.c", 150);

    msg->heapId = heapId;
    msg->wordSet = GFL_WordSetSystemCreateDefault(msg->heapId);
    BmpWin_InitAllocator(msg->heapId);
    GFL_TextRndInitColorIndexLUT();
    msg->tcbMgr = GFL_TCBExMgrCreate(msg->heapId, msg->heapId, 2, 0);
    msg->printQueue = PrintQueue_CreateDefault(msg->heapId);
    msg->strBuf = GFL_StrBufCreate(400, msg->heapId);
    msg->tmpBuf = GFL_StrBufCreate(400, msg->heapId);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, msg->heapId);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, msgFile, msg->heapId);
    msg->keyCursor = KeyCursor_Create(15, TRUE, TRUE, msg->heapId);
    msg->menuRes = AppTaskMenuRes_Create(5, 9, msg->font, msg->printQueue, msg->heapId);
    msg->frameChars = LoadCursorImageEndOfHeap(5, 12, 0, msg->heapId);
    msg->infoFrameChars = LoadCursorImageEndOfHeap(6, 12, 0, msg->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 11 * 0x20, 0x20, msg->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 11 * 0x20, 0x20, msg->heapId);
    return msg;
}

void GSyncMessage_Main(GSyncMessage *msg) {
    GFL_TCBExMgrUpdate(msg->tcbMgr);
    PrintQueue_Main(msg->printQueue);
}

void GSyncMessage_Free(GSyncMessage *msg) {
    int i;

    GSyncMessage_ClearMessage(msg);
    if (msg->msgWin != NULL) {
        BmpWin_Free(msg->msgWin);
    }
    if (msg->infoWin != NULL) {
        BmpWin_Free(msg->infoWin);
    }
    for (i = 0; i < GSYNC_MESSAGE_WIN_MAX; i++) {
        if (msg->wins[i] != NULL) {
            BmpWin_Free(msg->wins[i]);
        }
    }
    GFL_BGSysFreeCharMemory(5, CHAR_POS(msg->frameChars), CHAR_SIZE(msg->frameChars));
    GFL_BGSysFreeCharMemory(6, CHAR_POS(msg->infoFrameChars), CHAR_SIZE(msg->infoFrameChars));
    GSyncMessage_DeleteButtons(msg);
    KeyCursor_Free(msg->keyCursor);
    GFL_WordSetSystemFree(msg->wordSet);
    GFL_TextRndResetColorIndexLUT();
    GFL_MsgDataFree(msg->msgData);
    GFL_FontFree(msg->font);
    GFL_StrBufFree(msg->strBuf);
    GFL_StrBufFree(msg->tmpBuf);
    AppTaskMenuRes_Free(msg->menuRes);
    PrintQueue_Clear(msg->printQueue);
    PrintQueue_Free(msg->printQueue);
    GFL_TCBExMgrFree(msg->tcbMgr);
    BmpWin_FreeAllocator();
    GFL_HeapFree(msg);
}

void GSyncMessage_PrintLoaded(GSyncMessage *msg, BOOL now) {
    BmpWin *win;

    if (msg->msgWin == NULL) {
        msg->msgWin = BmpWin_CreateDynamic(5, 1, 1, 30, 4, 11, TRUE);
    }
    win = msg->msgWin;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    if (now) {
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, msg->strBuf, msg->font);
    } else {
        msg->printStream = PrintStream_Create(win, 0, 0, msg->strBuf, msg->font, TextSpeed_GetCurrentWait(),
                                              msg->tcbMgr, 2, msg->heapId, 15);
        AppPrintsysCommon_Init(&msg->printCommon, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    }
    BmpWin_DrawFrame(win, TRUE, msg->frameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(5);
}

void GSyncMessage_PrintStream(GSyncMessage *msg, u32 msgId) {
    GFL_MsgDataLoadStrbuf(msg->msgData, msgId, msg->strBuf);
    GSyncMessage_PrintLoaded(msg, FALSE);
}

void GSyncMessage_Print(GSyncMessage *msg, u32 msgId) {
    GFL_MsgDataLoadStrbuf(msg->msgData, msgId, msg->strBuf);
    GSyncMessage_PrintLoaded(msg, TRUE);
}

void GSyncMessage_FormatPokemonNumber(GSyncMessage *msg, u32 msgId, s32 number, PartyPkm *pkm) {
    GFL_MsgDataLoadStrbuf(msg->msgData, msgId, msg->tmpBuf);
    loadPokemonNicknameToStrbuf(msg->wordSet, 0, pkm);
    WordSetNumber(msg->wordSet, 1, number, 3, 1, TRUE);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strBuf, msg->tmpBuf);
}

BOOL GSyncMessage_IsPrintFinished(GSyncMessage *msg) {
    if (msg->printStream != NULL) {
        KeyCursor_Update(msg->keyCursor, msg->printStream, msg->msgWin);
        if (!AppPrintsysCommon_Update(&msg->printCommon, msg->printStream)) {
            return FALSE;
        }
        PrintStream_Delete(msg->printStream);
        msg->printStream = NULL;
    }
    return TRUE;
}

void GSyncMessage_ClearMessage(GSyncMessage *msg) {
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    if (msg->printStream != NULL) {
        PrintStream_Delete(msg->printStream);
        msg->printStream = NULL;
    }
    if (msg->msgWin != NULL) {
        BmpWin_ClearFrame(msg->msgWin, 2);
        BmpWin_ClearScreen(msg->msgWin);
        GFL_BGSysQueueScrLoad(5);
    }
}

AppTaskMenu *GSyncMessage_CreateYesNo(GSyncMessage *msg, int pos) {
    AppTaskMenuInit init;
    AppTaskMenu *menu;

    init.heapId = msg->heapId;
    init.itemCount = 2;
    init.items = msg->yesNoItems;
    switch (pos) {
    case GSYNC_YESNO_POS_UPPER:
        init.x = 32;
        init.y = 12;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        break;
    case GSYNC_YESNO_POS_LOWER:
        init.x = 32;
        init.y = 24;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        break;
    }
    init.width = 13;
    init.height = 3;
    msg->yesNoItems[0].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, 4, msg->yesNoItems[0].str);
    msg->yesNoItems[0].color = PRINT_COLOR(14, 15, 0);
    msg->yesNoItems[1].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, 5, msg->yesNoItems[1].str);
    msg->yesNoItems[1].color = PRINT_COLOR(14, 15, 0);
    menu = AppTaskMenu_Create(&init, msg->menuRes);
    GFL_StrBufFree(msg->yesNoItems[0].str);
    GFL_StrBufFree(msg->yesNoItems[1].str);
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG3, -8);
    return menu;
}

void GSyncMessage_LoadString(GSyncMessage *msg, u32 msgId) {
    GFL_MsgDataLoadStrbuf(msg->msgData, msgId, msg->strBuf);
}

void GSyncMessage_PrintInfoAt(GSyncMessage *msg, int y, int height) {
    BmpWin *win;

    if (msg->infoWin != NULL) {
        BmpWin_Free(msg->infoWin);
    }
    win = BmpWin_CreateDynamic(6, 1, y, 30, height, 11, TRUE);
    msg->infoWin = win;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, msg->strBuf, msg->font);
    BmpWin_DrawFrame(win, TRUE, msg->infoFrameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(6);
}

void GSyncMessage_PrintInfo(GSyncMessage *msg) {
    BmpWin *win;

    if (msg->infoWin != NULL) {
        BmpWin_Free(msg->infoWin);
    }
    win = BmpWin_CreateDynamic(6, 1, 9, 30, 6, 11, TRUE);
    msg->infoWin = win;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, msg->strBuf, msg->font);
    BmpWin_DrawFrame(win, TRUE, msg->infoFrameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(6);
}

void GSyncMessage_ClearInfo(GSyncMessage *msg) {
    BmpWin_ClearFrame(msg->infoWin, 2);
    BmpWin_ClearScreen(msg->infoWin);
    GFL_BGSysQueueScrLoad(6);
}

void GSyncMessage_StartWaitIcon(GSyncMessage *msg) {
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    msg->waitIcon = WaitIcon_CreateTCBEx(msg->tcbMgr, msg->msgWin, 15, 16, msg->heapId);
}

void GSyncMessage_FormatGSyncId(GSyncMessage *msg, u32 id) {
    StrBuf *strBuf = msg->strBuf;
    // The ID as a signed value, which the code extends to 64 bits
    int value = id;
    u16 crc = getCRC16(&value, 4);
    u16 params[] = { 1 };
    u64 code = value + ((u64)crc << 32);
    int i;

    GFL_StrBufClear(strBuf);
    for (i = 0; i < 10; i++) {
        u16 index = code & 0x1f;

        code >>= 5;
        GFL_StrBufAppend(strBuf, sGSyncIdChars[index]);
        GFL_StrCmdBuild(strBuf, STRCMD_CATEGORY_LAYOUT, STRCMD_LAYOUT_MOVE_X, 1, params);
    }
}
