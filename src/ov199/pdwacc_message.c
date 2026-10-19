// The Dream World account screens' messages: the message window, an info window, the yes/no menu and the Game Sync
// ID. The name is the ROM's string, from GFL_HeapAllocate's call. Function names are ours.

#include "types.h"
#include "app/gsync/pdwacc_message.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"

#define PDWACC_MESSAGE_WIN_MAX 12

struct PdwAccMessage {
    // The frames of the Game Sync ID's window, the info window and the message window, as LoadCursorImageEndOfHeap
    // gives them
    u32 idFrameChars;
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
    BmpWin *idWin;
    BmpWin *idLabelWin;
    BmpWin *msgWin;
    BmpWin *infoWin;
    BmpWin *wins[PDWACC_MESSAGE_WIN_MAX];
    PrintStream *printStream;
    u32 unk84;
    TCBExManager *tcbMgr;
    PrintQueue *printQueue;
    WaitIcon *waitIcon;
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *menuRes;
    HeapID heapId;
};

static void PdwAccMessage_DeleteButtons(PdwAccMessage *msg);

// The characters of a Game Sync ID, without I, O, 0 and 1
static const u16 sPdwAccIdChars[32] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7', '8', '9',
};

static void PdwAccMessage_DeleteButtons(PdwAccMessage *msg) {
    if (msg->buttonMan != NULL) {
        GFL_BMN_Delete(msg->buttonMan);
        msg->buttonMan = NULL;
    }
}

PdwAccMessage *PdwAccMessage_Create(HeapID heapId, u32 msgFile) {
    PdwAccMessage *msg = GFL_HeapAllocate(heapId, sizeof(PdwAccMessage), TRUE, "pdwacc_message.c", 153);

    msg->heapId = heapId;
    BmpWin_InitAllocator(msg->heapId);
    GFL_TextRndInitColorIndexLUT();
    msg->tcbMgr = GFL_TCBExMgrCreate(msg->heapId, msg->heapId, 2, 0);
    msg->printQueue = PrintQueue_CreateDefault(msg->heapId);
    msg->strBuf = GFL_StrBufCreate(400, msg->heapId);
    msg->tmpBuf = GFL_StrBufCreate(400, msg->heapId);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, msg->heapId);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, msgFile, msg->heapId);
    msg->wordSet = GFL_WordSetSystemCreateDefault(msg->heapId);
    msg->menuRes = AppTaskMenuRes_Create(5, 9, msg->font, msg->printQueue, msg->heapId);
    msg->idFrameChars = LoadCursorImageEndOfHeap(1, 12, 0, msg->heapId);
    msg->frameChars = LoadCursorImageEndOfHeap(5, 12, 0, msg->heapId);
    msg->infoFrameChars = LoadCursorImageEndOfHeap(6, 12, 0, msg->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 11 * 0x20, 0x20, msg->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 11 * 0x20, 0x20, msg->heapId);
    return msg;
}

void PdwAccMessage_Main(PdwAccMessage *msg) {
    GFL_TCBExMgrUpdate(msg->tcbMgr);
    PrintQueue_Main(msg->printQueue);
}

void PdwAccMessage_Free(PdwAccMessage *msg) {
    int i;

    GFL_BGSysFreeCharMemory(1, CHAR_POS(msg->idFrameChars), CHAR_SIZE(msg->idFrameChars));
    GFL_BGSysFreeCharMemory(5, CHAR_POS(msg->frameChars), CHAR_SIZE(msg->frameChars));
    GFL_BGSysFreeCharMemory(6, CHAR_POS(msg->infoFrameChars), CHAR_SIZE(msg->infoFrameChars));
    PdwAccMessage_DeleteButtons(msg);
    GFL_TextRndResetColorIndexLUT();
    GFL_MsgDataFree(msg->msgData);
    GFL_FontFree(msg->font);
    GFL_StrBufFree(msg->strBuf);
    GFL_StrBufFree(msg->tmpBuf);
    GFL_WordSetSystemFree(msg->wordSet);
    AppTaskMenuRes_Free(msg->menuRes);
    PrintQueue_Clear(msg->printQueue);
    PrintQueue_Free(msg->printQueue);
    if (msg->printStream != NULL) {
        PrintStream_Delete(msg->printStream);
    }
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    if (msg->idWin != NULL) {
        BmpWin_Free(msg->idWin);
    }
    if (msg->idLabelWin != NULL) {
        BmpWin_Free(msg->idLabelWin);
    }
    if (msg->msgWin != NULL) {
        BmpWin_Free(msg->msgWin);
    }
    if (msg->infoWin != NULL) {
        BmpWin_Free(msg->infoWin);
    }
    for (i = 0; i < PDWACC_MESSAGE_WIN_MAX; i++) {
        if (msg->wins[i] != NULL) {
            BmpWin_Free(msg->wins[i]);
        }
    }
    GFL_TCBExMgrFree(msg->tcbMgr);
    BmpWin_FreeAllocator();
    GFL_HeapFree(msg);
}

void PdwAccMessage_PrintStream(PdwAccMessage *msg, u32 msgId) {
    BmpWin *win;

    GFL_MsgDataLoadStrbuf(msg->msgData, msgId, msg->strBuf);
    if (msg->msgWin == NULL) {
        msg->msgWin = BmpWin_CreateDynamic(5, 1, 3, 30, 4, 11, TRUE);
    }
    win = msg->msgWin;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    msg->printStream = PrintStream_Create(win, 0, 0, msg->strBuf, msg->font, TextSpeed_GetCurrentWait(), msg->tcbMgr, 2,
                                          msg->heapId, 15);
    BmpWin_DrawFrame(win, TRUE, msg->frameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(5);
}

void PdwAccMessage_StartWaitIcon(PdwAccMessage *msg) {
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    msg->waitIcon = WaitIcon_CreateTCBEx(msg->tcbMgr, msg->msgWin, 15, 16, msg->heapId);
}

// The message goes on only with A
BOOL PdwAccMessage_IsPrintFinished(PdwAccMessage *msg) {
    if (msg->printStream != NULL) {
        switch (PrintStream_GetState(msg->printStream)) {
        case PRINT_STREAM_DONE:
            PrintStream_Delete(msg->printStream);
            msg->printStream = NULL;
            break;
        case PRINT_STREAM_PAUSED:
            if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
                PrintStream_Continue(msg->printStream);
            }
            break;
        }
        return FALSE;
    }
    return TRUE;
}

void PdwAccMessage_ClearMessage(PdwAccMessage *msg) {
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
        msg->waitIcon = NULL;
    }
    if (msg->msgWin != NULL) {
        BmpWin_ClearFrame(msg->msgWin, 2);
        BmpWin_ClearScreen(msg->msgWin);
        GFL_BGSysQueueScrLoad(5);
        BmpWin_Free(msg->msgWin);
        msg->msgWin = NULL;
    }
}

AppTaskMenu *PdwAccMessage_CreateYesNo(PdwAccMessage *msg, int pos) {
    AppTaskMenuInit init;
    AppTaskMenu *menu;

    init.heapId = msg->heapId;
    init.itemCount = 2;
    init.items = msg->yesNoItems;
    switch (pos) {
    case PDWACC_YESNO_POS_UPPER:
        init.x = 32;
        init.y = 14;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        break;
    case PDWACC_YESNO_POS_LOWER:
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
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_OBJ, -8);
    return menu;
}

void PdwAccMessage_PrintInfo(PdwAccMessage *msg, u32 msgId) {
    BmpWin *win;

    GFL_MsgDataLoadStrbuf(msg->msgData, msgId, msg->strBuf);
    if (msg->infoWin == NULL) {
        msg->infoWin = BmpWin_CreateDynamic(6, 1, 3, 30, 16, 11, TRUE);
    }
    win = msg->infoWin;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, msg->strBuf, msg->font);
    BmpWin_DrawFrame(win, TRUE, msg->infoFrameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(6);
}

void PdwAccMessage_ClearInfo(PdwAccMessage *msg) {
    BmpWin_ClearFrame(msg->infoWin, 2);
    BmpWin_ClearScreen(msg->infoWin);
    GFL_BGSysQueueScrLoad(6);
}

void PdwAccMessage_ShowGSyncId(PdwAccMessage *msg, u32 id, u32 unused) {
    StrBuf *strBuf = msg->strBuf;
    // The ID as a signed value, which the code extends to 64 bits
    int value = id;
    u16 crc = getCRC16(&value, 4);
    u16 params[] = { 1 };
    u64 code = value + ((u64)crc << 32);
    int i;
    BmpWin *win;

    GFL_StrBufClear(strBuf);
    for (i = 0; i < 10; i++) {
        u16 index = code & 0x1f;

        code >>= 5;
        GFL_StrBufAppend(strBuf, sPdwAccIdChars[index]);
        GFL_StrCmdBuild(strBuf, 0xbd, 4, 1, params);
    }

    if (msg->idWin == NULL) {
        msg->idWin = BmpWin_CreateDynamic(1, 1, 8, 30, 4, 11, TRUE);
    }
    win = msg->idWin;
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, msg->strBuf, msg->font);
    BmpWin_DrawFrame(win, TRUE, msg->idFrameChars, 12);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);

    if (msg->idLabelWin == NULL) {
        msg->idLabelWin = BmpWin_CreateDynamic(1, 3, 3, 22, 2, 11, TRUE);
    }
    win = msg->idLabelWin;
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    GFL_MsgDataLoadStrbuf(msg->msgData, 10, msg->strBuf);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), 0, 0, msg->strBuf, msg->font);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysQueueScrLoad(5);
}

void PdwAccMessage_ClearGSyncId(PdwAccMessage *msg) {
    if (msg->idWin != NULL) {
        BmpWin_ClearFrame(msg->idWin, 2);
        BmpWin_ClearScreen(msg->idWin);
        BmpWin_ClearFrame(msg->idLabelWin, 2);
        BmpWin_ClearScreen(msg->idLabelWin);
        BmpWin_Free(msg->idWin);
        BmpWin_Free(msg->idLabelWin);
        GFL_BGSysQueueScrLoad(5);
        msg->idWin = NULL;
        msg->idLabelWin = NULL;
    }
}
