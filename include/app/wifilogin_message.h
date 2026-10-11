#ifndef POKEBW2_APP_WIFILOGIN_MESSAGE_H
#define POKEBW2_APP_WIFILOGIN_MESSAGE_H

#include "types.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/tcbl.h"
#include "struct_decls.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/printsys.h"
#include "system/time_icon.h"

// The Wi-Fi login and logout's messages and menus (wifilogin_message.c, overlay 190): a message window printed a
// character at a time, a title window, and a yes/no menu that is either task menu buttons or a confirm dialog. Also
// used by the Global Trade Station (overlay 214). Our names

// The screen the windows are on, which also picks the BGs: the Dream World account style's or Game Sync's, as in
// wifilogin_disp.h
#define WIFILOGIN_MESSAGE_STYLE_PDWACC 0
#define WIFILOGIN_MESSAGE_STYLE_GSYNC 1

// The yes/no menu, of the style's kind
typedef struct {
    u32 style;
    AppTaskMenuItem items[2];
    AppTaskMenuRes *res;
    AppTaskMenu *taskMenu;
    BmpMenu *dialog;
    // The confirm dialog's result, BMPMENU_NULL until picked
    s32 result;
} WifiLoginMessageMenu;

typedef struct WifiLoginMessage {
    // The frame characters, loaded at the end of the BGs of the message window and of the other one
    u32 frameChars;
    u32 frameCharsOther;
    u8 unk8[0x14];
    MsgData *msgData;
    WordSet *wordSet;
    Font *font;
    StrBuf *strbuf;
    BmpWin *messageWin;
    BmpWin *unk30;
    BmpWin *titleWin;
    PrintStream *stream;
    AppPrintsysCommon printsys;
    TCBExManager *tcbManager;
    PrintQueue *printQueue;
    u16 heapId;
    u32 style;
    WifiLoginMessageMenu menu;
    WaitIcon *waitIcon;
} WifiLoginMessage;

WifiLoginMessage *WifiLoginMessage_Create(HeapID heapId, u32 bank, u32 style);
void WifiLoginMessage_Update(WifiLoginMessage *msg);
void WifiLoginMessage_Free(WifiLoginMessage *msg);
// Print a message of the loaded bank (WifiLoginMessage_PrintFrom: of msgData), and wait for it with WifiLoginMessage_IsDone
void WifiLoginMessage_Print(WifiLoginMessage *msg, u32 msgNo);
void WifiLoginMessage_PrintFrom(WifiLoginMessage *msg, MsgData *msgData, u32 msgNo);
// The same, then with the wait icon on the window
void WifiLoginMessage_PrintWaiting(WifiLoginMessage *msg, u32 msgNo);
BOOL WifiLoginMessage_IsDone(WifiLoginMessage *msg);
// Clears the message window
void WifiLoginMessage_ClearWindow(WifiLoginMessage *msg);
void WifiLoginMessage_PrintTitle(WifiLoginMessage *msg, u32 msgNo);
void WifiLoginMessage_ClearTitle(WifiLoginMessage *msg);
WifiLoginMessageMenu *WifiLoginMessage_CreateMenu(WifiLoginMessage *msg, u32 layout, BOOL dimTopScreen, u32 cursorPos);
void WifiLoginMessage_FreeMenu(WifiLoginMessageMenu *menu);
void WifiLoginMessage_UpdateMenu(WifiLoginMessageMenu *menu);

// The yes/no menu of the message, its window of notices, the BGs of its windows and the confirm dialog
// The yes/no menu: whether the player has answered, and the answer, 0 for yes
BOOL WifiLoginMessage_IsMenuAnswered(WifiLoginMessageMenu *menu);
u8 WifiLoginMessage_GetMenuAnswer(WifiLoginMessageMenu *menu);
// Prints a message in the window of the notices, and clears the window
void WifiLoginMessage_PrintNotice(WifiLoginMessage *msg, u32 msgNo);
void WifiLoginMessage_ClearNotice(WifiLoginMessage *msg);
u8 WifiLoginMessage_GetMessageBG(u32 style);
u8 WifiLoginMessage_GetNoticeBG(u32 style);
BmpMenu *WifiLoginMessage_CreateConfirmDialog(WifiLoginMessage *msg, u32 layout, u32 cursorPos);

#endif // POKEBW2_APP_WIFILOGIN_MESSAGE_H
