#include "app/pokelist.h"
#include "constants/sound.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/bmp_winframe.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"

// The party list's message window, at the bottom of the screen, which prints at once or a character at a time

struct PokeListMessage {
    // POKELIST_MESSAGE_WINDOW_*, or POKELIST_MESSAGE_WINDOW_NONE
    u32 windowType;
    BmpWin *window;
    TCBExManager *tcbMgr;
    PrintStream *stream;
    StrBuf *str;
    KeyCursor *keyCursor;
    AppPrintsysCommon printWait;
    BOOL waitInput;
    BOOL dirty;
    PrintQueue *printQueue;
    WordSet *wordSet;
    // Whether the wait icon is to show once the message is printed
    BOOL wantWaitIcon;
    WaitIcon *waitIcon;
};

static BOOL PokeListMessage_StreamCallback(u32 event);
static void PokeListMessage_Clear(PokeListWork *wk, PokeListMessage *msg);

PokeListMessage *PokeListMessage_Create(PokeListWork *wk) {
    PokeListMessage *msg = GFL_HeapAllocate(wk->heapId, sizeof(PokeListMessage), FALSE, "plist_message.c", 100);

    LoadSysMsgBox(0, 1, 12, 0, wk->heapId);
    msg->tcbMgr = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 1, 0);
    msg->stream = NULL;
    msg->windowType = POKELIST_MESSAGE_WINDOW_NONE;
    msg->dirty = FALSE;
    msg->waitInput = FALSE;
    msg->wordSet = NULL;
    msg->waitIcon = NULL;
    msg->wantWaitIcon = FALSE;
    msg->keyCursor = KeyCursor_Create(15, 1, 1, wk->heapId);
    msg->printQueue = PrintQueue_CreateDefault(wk->heapId);
    return msg;
}

void PokeListMessage_Free(PokeListWork *wk, PokeListMessage *msg) {
    if (msg->windowType != POKELIST_MESSAGE_WINDOW_NONE) {
        PokeListMessage_Close(wk, msg);
    }
    if (msg->waitIcon != NULL) {
        WaitIcon_Free(msg->waitIcon);
    }
    if (msg->stream != NULL) {
        PrintStream_Delete(msg->stream);
        GFL_StrBufFree(msg->str);
    }
    PrintQueue_Clear(msg->printQueue);
    PrintQueue_Free(msg->printQueue);
    KeyCursor_Free(msg->keyCursor);
    GFL_TCBExMgrFree(msg->tcbMgr);
    GFL_HeapFree(msg);
}

void PokeListMessage_Update(PokeListWork *wk, PokeListMessage *msg) {
    GFL_TCBExMgrUpdate(msg->tcbMgr);
    if (msg->stream != NULL) {
        KeyCursor_Update(msg->keyCursor, msg->stream, msg->window);
        if (AppPrintsysCommon_Update(&msg->printWait, msg->stream) == TRUE) {
            if (msg->waitInput == FALSE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) ||
                func_0203da48() == TRUE) {
                if (msg->waitInput == TRUE) {
                    GFL_SndSEPlay(SEQ_SE_MESSAGE);
                }
                PrintStream_Delete(msg->stream);
                msg->stream = NULL;
                GFL_StrBufFree(msg->str);
            } else {
                KeyCursor_Draw(msg->keyCursor, BmpWin_GetBitmap(msg->window), 15);
                BmpWin_FlushChar(msg->window);
            }
        }
    }
    if (msg->dirty == TRUE && PrintQueue_IsBitmapPending(msg->printQueue, BmpWin_GetBitmap(msg->window)) == FALSE) {
        msg->dirty = FALSE;
        BmpWin_Transfer(msg->window);
        if (msg->wantWaitIcon == TRUE) {
            msg->wantWaitIcon = FALSE;
            msg->waitIcon = WaitIcon_CreateTCBEx(msg->tcbMgr, msg->window, 15, 16, wk->heapId);
        }
    }
    PrintQueue_Main(msg->printQueue);
}

void PokeListMessage_Open(PokeListWork *wk, PokeListMessage *msg, u32 windowType) {
    msg->windowType = windowType;
    switch (windowType) {
    case POKELIST_MESSAGE_WINDOW_SHORT:
        msg->window = BmpWin_CreateDynamic(0, 1, 21, 21, 2, 14, TRUE);
        break;
    case POKELIST_MESSAGE_WINDOW_SHORTER:
        msg->window = BmpWin_CreateDynamic(0, 1, 21, 20, 2, 14, TRUE);
        break;
    case POKELIST_MESSAGE_WINDOW_MENU:
        msg->window = BmpWin_CreateDynamic(0, 2, 19, 14, 4, 14, TRUE);
        break;
    case POKELIST_MESSAGE_WINDOW_WIDE:
        msg->window = BmpWin_CreateDynamic(0, 2, 19, 28, 4, 14, TRUE);
        break;
    }
    BmpWin_DrawFrame(msg->window, WINFRAME_TRANSFER_VBLANK, 1, 12);
    PokeListMessage_Clear(wk, msg);
}

void PokeListMessage_Close(PokeListWork *wk, PokeListMessage *msg) {
    if (msg->windowType != POKELIST_MESSAGE_WINDOW_NONE) {
        if (msg->waitIcon != NULL) {
            WaitIcon_Free(msg->waitIcon);
            msg->waitIcon = NULL;
        }
        BmpWin_ClearFrame(msg->window, WINFRAME_TRANSFER_VBLANK);
        BmpWin_Free(msg->window);
        PrintQueue_Clear(msg->printQueue);
        msg->windowType = POKELIST_MESSAGE_WINDOW_NONE;
    }
}

BOOL PokeListMessage_IsOpen(PokeListWork *wk, PokeListMessage *msg) {
    if (msg->windowType == POKELIST_MESSAGE_WINDOW_NONE) {
        return FALSE;
    }
    return TRUE;
}

// Prints a message at once, expanded with the word set if there is one
void PokeListMessage_Print(PokeListWork *wk, PokeListMessage *msg, u32 msgId) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    if (msg->wordSet != NULL) {
        StrBuf *expanded = GFL_StrBufCreate(256, wk->heapId);

        GFL_WordSetFormatStrbuf(msg->wordSet, expanded, str);
        GFL_StrBufFree(str);
        str = expanded;
    }
    PokeListMessage_Clear(wk, msg);
    PrintQueue_Clear(msg->printQueue);
    PrintQueue_PrintColor(msg->printQueue, BmpWin_GetBitmap(msg->window), 1, 1, str, wk->font, 0x440);
    msg->dirty = TRUE;
    GFL_StrBufFree(str);
}

// Prints a message a character at a time, and waits for A, B or a touch at its end if waitInput is set
void PokeListMessage_PrintStream(PokeListWork *wk, PokeListMessage *msg, u32 msgId, BOOL waitInput) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    if (msg->wordSet != NULL) {
        StrBuf *expanded = GFL_StrBufCreate(256, wk->heapId);

        GFL_WordSetFormatStrbuf(msg->wordSet, expanded, str);
        GFL_StrBufFree(str);
        str = expanded;
    }
    PokeListMessage_Clear(wk, msg);
    PrintQueue_Clear(msg->printQueue);
    if (msg->stream != NULL) {
        PrintStream_Delete(msg->stream);
        GFL_StrBufFree(msg->str);
    }
    msg->waitInput = waitInput;
    msg->str = str;
    AppPrintsysCommon_Init(&msg->printWait, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    msg->stream = PrintStream_CreateWithCallback(msg->window, 0, 0, msg->str, wk->font, TextSpeed_GetCurrentWait(),
                                                 msg->tcbMgr, 0, wk->heapId, 15, PokeListMessage_StreamCallback);
}

BOOL PokeListMessage_IsDone(PokeListWork *wk, PokeListMessage *msg) {
    if (msg->stream == NULL && msg->dirty == FALSE) {
        return TRUE;
    }
    return FALSE;
}

// The events of the messages' control codes: a fanfare, a sound, and waiting for them
static BOOL PokeListMessage_StreamCallback(u32 event) {
    switch (event) {
    case 2:
        GFL_SndBGMSetPaused(TRUE);
        GFL_SndBGMPush();
        GFL_SndBGMPlay(SEQ_ME_LVUP, 0xffff);
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_KON);
        break;
    case 5:
        if (GFL_SndBGMIsPlaying() == FALSE) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            return FALSE;
        }
        return TRUE;
    case 6:
        return GFL_SndPlayerIsActiveAny();
    }
    return FALSE;
}

void PokeListMessage_CreateWordSet(PokeListWork *wk, PokeListMessage *msg) {
    msg->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
}

void PokeListMessage_FreeWordSet(PokeListWork *wk, PokeListMessage *msg) {
    GFL_WordSetSystemFree(msg->wordSet);
    msg->wordSet = NULL;
}

void PokeListMessage_SetPkmName(PokeListWork *wk, PokeListMessage *msg, u32 index, PartyPkm *pkm) {
    loadPokemonNicknameToStrbuf(msg->wordSet, index, pkm);
}

void PokeListMessage_SetItemName(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 item) {
    loadItemNameToStrbuf(msg->wordSet, index, item);
}

void PokeListMessage_SetItemTextName(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 item) {
    loadItemTextNameToStrbuf(msg->wordSet, index, item);
}

void PokeListMessage_SetMoveName(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 move) {
    loadMoveNameToStrbuf(msg->wordSet, index, move);
}

void PokeListMessage_SetStatName(PokeListWork *wk, PokeListMessage *msg, u32 index, u32 stat) {
    loadStatNameToStrbuf(msg->wordSet, index, stat);
}

void PokeListMessage_SetNumber(PokeListWork *wk, PokeListMessage *msg, u32 index, u16 number, u8 digits) {
    WordSetNumber(msg->wordSet, index, number, digits, 0, TRUE);
}

void PokeListMessage_SetString(PokeListWork *wk, PokeListMessage *msg, u32 index, const StrBuf *str, u32 a4) {
    func_0202437c(msg->wordSet, index, str, a4, 1, 2);
}

static void PokeListMessage_Clear(PokeListWork *wk, PokeListMessage *msg) {
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 15);
    BmpWin_Transfer(msg->window);
}

void PokeListMessage_LoadFrame(PokeListWork *wk, PokeListMessage *msg) {
    LoadSysMsgBox(0, 1, 12, 0, wk->heapId);
}

// Shows the wait icon in the window once the message is printed
void PokeListMessage_ShowWaitIcon(PokeListWork *wk, PokeListMessage *msg) {
    msg->wantWaitIcon = TRUE;
}

void PokeListMessage_DrawKeyCursor(PokeListWork *wk, PokeListMessage *msg) {
    KeyCursor_Erase(msg->keyCursor, BmpWin_GetBitmap(msg->window), 15);
    BmpWin_FlushChar(msg->window);
}
