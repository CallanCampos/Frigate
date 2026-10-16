#include "types.h"
#include "app/marine_tube_board.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"

// The Marine Tube board's message window and the printing of its message. The file name is a guess: the ROM has no
// string for it

#define BG_MESSAGE MARINE_TUBE_BOARD_BG_MESSAGE
#define FRAME_CHAR MARINE_TUBE_BOARD_FRAME_CHAR
#define FRAME_PALETTE MARINE_TUBE_BOARD_FRAME_PALETTE
#define BACKGROUND_COLOR 15

void MarineTubeBoardMsgWin_Init(MarineTubeBoardMsgWin *win, Font *font, MsgData *msgData, PrintQueue *printQueue,
                                WordSet *wordSet, HeapID heapId) {
    BmpWin *window;

    MI_CpuFill8(win, 0, sizeof(MarineTubeBoardMsgWin));
    win->color = BACKGROUND_COLOR;
    win->font = font;
    win->wordSet = wordSet;
    win->msgData = msgData;
    win->printQueue = printQueue;
    win->strBuf = GFL_StrBufCreate(512, heapId);
    win->window = BmpWin_CreateDynamic(BG_MESSAGE, 1, 1, 30, 6, BACKGROUND_COLOR, TRUE);
    BmpWin_DrawFrame(win->window, WINFRAME_TRANSFER_NONE, FRAME_CHAR, FRAME_PALETTE);
    win->window2 = win->window;
    win->unk0C = 0;
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    window = win->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
}

void MarineTubeBoardMsgWin_Free(MarineTubeBoardMsgWin *win) {
    BmpWin_Free(win->window);
    GFL_StrBufFree(win->strBuf);
    MI_CpuFill8(win, 0, sizeof(MarineTubeBoardMsgWin));
}

void MarineTubeBoardPrinter_Init(MarineTubeBoardPrinter *printer, HeapID heapId) {
    MI_CpuFill8(printer, 0, sizeof(MarineTubeBoardPrinter));
    printer->tcbMgr = GFL_TCBExMgrCreate(heapId, heapId, 2, 0);
    printer->keyCursor = KeyCursor_Create(BACKGROUND_COLOR, TRUE, TRUE, heapId);
}

void MarineTubeBoardPrinter_Free(MarineTubeBoardPrinter *printer) {
    if (printer->stream != NULL) {
        func_020223cc(printer->stream);
    }
    KeyCursor_Free(printer->keyCursor);
    GFL_TCBExMgrFree(printer->tcbMgr);
    MI_CpuFill8(printer, 0, sizeof(MarineTubeBoardPrinter));
}

void MarineTubeBoardPrinter_Print(MarineTubeBoardPrinter *printer, MarineTubeBoardMsgWin *win, u32 messageId,
                                  HeapID heapId) {
    GFL_MsgDataLoadStrbuf(win->msgData, messageId, win->strBuf);
    if (printer->stream != NULL) {
        func_020223cc(printer->stream);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_FlushChar(win->window);
    printer->stream = func_02022268(win->window, 0, 0, win->strBuf, win->font, func_02017bcc(), printer->tcbMgr, 2,
                                    heapId, win->color);
    BmpWin_FlushMap(win->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win->window));
    AppPrintsysCommon_Init(&printer->printCommon, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    printer->unk0C = 0;
    printer->unk0E = 0;
    printer->unk08 = 0;
}

BOOL MarineTubeBoardPrinter_Update(MarineTubeBoardPrinter *printer, MarineTubeBoardMsgWin *win) {
    GFL_TCBExMgrUpdate(printer->tcbMgr);
    if (printer->stream != NULL && win->window != NULL) {
        KeyCursor_Update(printer->keyCursor, printer->stream, win->window);
        if (AppPrintsysCommon_Update(&printer->printCommon, printer->stream) == TRUE) {
            func_020223cc(printer->stream);
            printer->stream = NULL;
            return TRUE;
        }
    }
    return FALSE;
}
