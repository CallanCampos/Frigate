#ifndef POKEBW2_FIELD_FIELD_MSGBG_H
#define POKEBW2_FIELD_FIELD_MSGBG_H

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/bmp_menulist.h"
#include "system/printsys.h"

// The field's message BG (field_msgbg.c, overlay 36): the two BGs the field prints its messages on, and the windows,
// list menus, message balloons over actors and wait-for-input cursors on them. The talk windows of talkmsgwin.h draw
// the balloons and the windows framed like them. Existing names are from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the CheckerMsgWin API names
// are our descriptive guesses.

// A list menu's layout. ListMenuRequest_Set completes it with the number of options, the position and the size, which
// CalcListMenuWidth and CalcListMenuHeight give in tiles. The fields up to rowHeight are BmpMenuListHeader's
typedef struct {
    u16 count;
    u16 maxShown;
    u8 labelX;
    u8 itemX;
    u8 cursorX;
    u8 listY : 4;
    u8 fgColor : 4;
    u8 bgColor : 4;
    u8 shadowColor : 4;
    u16 letterSpacing : 3;
    u16 lineSpacing : 4;
    u16 unk0A_7 : 9;
    s32 pageSkip : 2;
    u16 fontSizeX;
    // The height of a row, in pixels
    u16 rowHeight;
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    BmpMenuListPrintCallback printCallback;
} ListMenuRequest;

// A row of a list menu that MsgBGPrinter_CreateMenuItemsFromTable makes: its message, and the value the menu returns
// when it is chosen. The Union Room's menus return the rows of the menu that a row opens
typedef struct ListMenuRow {
    u32 messageId;
    union {
        s32 value;
        const struct ListMenuRow *sub;
    };
} ListMenuRow;

// What MsgBGPrinter_YesNoMenuMain returns for the yes/no menu: the answer, which is no when the menu is cancelled, or
// WAITING
enum {
    MSGBG_YES_NO_YES,
    MSGBG_YES_NO_NO,
    MSGBG_YES_NO_WAITING,
};

// The message balloons over actors, which ActorMsgWin_CheckAndCreate takes as its type: across the top or the bottom
// of the screen, or where the actor leaves room. Each is the talk window of its index
enum {
    ACTOR_MSG_WIN_TOP,
    ACTOR_MSG_WIN_BOTTOM,
    ACTOR_MSG_WIN_AUTO,
    ACTOR_MSG_WIN_COUNT,
};

// The steps of MsgBGPrinter_LoadWindowGraphicsStep, which loads the frames one at a time: the palettes, the system
// message window's frame, and the list window's
enum {
    MSGBG_LOAD_PALETTES,
    MSGBG_LOAD_SYSTEM_FRAME,
    MSGBG_LOAD_LIST_FRAME,
};

// Creates the system, sets up its BGs, either with the talk windows' textures or loading them in a VBlank task, loads
// the frames' characters and palettes to a BG, all at once or one step at a time, and frees the system
FieldMsgBG *CreateFieldMsgBGSystem(HeapID heapId, G3DCamera *camera);
void MsgBGPrinter_SetupBg(FieldMsgBG *sys);
void MsgBGPrinter_SetupBgDelayed(FieldMsgBG *sys);
void MsgBGPrinter_LoadWindowGraphics(u32 bg, HeapID heapId);
// step is a MSGBG_LOAD_ step
void MsgBGPrinter_LoadWindowGraphicsStep(FieldMsgBG *sys, u32 step);
void MsgBGPrinter_Delete(FieldMsgBG *sys);
// Frees the talk windows and the first BG while an app uses the screen, releases the second BG (returning whether it
// had one), and sets up the first again, with the textures loaded in a VBlank task, and whether that task has finished
void MsgBGPrinter_ReleaseBg(FieldMsgBG *sys);
BOOL MsgBGPrinter_ReleaseSysFrame(FieldMsgBG *sys);
void MsgBGPrinter_SetupBgSingle(FieldMsgBG *sys);
BOOL MsgBGPrinter_IsSetupDone(FieldMsgBG *sys);
// Once a frame: prints the queued messages and runs the windows; then draws the talk windows
void MsgBGPrinter_Main(FieldMsgBG *sys);
void MsgBGPrinter_DrawTalkSys(FieldMsgBG *sys);
// Clears the print queue, whether it has printed everything, the queue, and the font
void MsgBGPrinter_ClearPrintQue(FieldMsgBG *sys);
BOOL MsgBGPrinter_IsPrintQueFinished(FieldMsgBG *sys);
PrintQueue *MsgBGPrinter_GetPrintQue(FieldMsgBG *sys);
Font *MsgBGPrinter_GetFont(FieldMsgBG *sys);
// Loads a file of the system messages, and frees it
MsgData *MsgBGPrinter_CreateMsgData(FieldMsgBG *sys, u32 fileId);
void MsgBGPrinter_DeleteMsgData(MsgData *msgData);
// Sets up the second BG again before the next window on it
void MsgBGPrinter_RequestSysFrame(FieldMsgBG *sys);
// Whether the messages advance on their own, without the A or B button
void MsgBGPrinter_SetAutoMode(FieldMsgBG *sys, BOOL enable);

// The money window, framed on the second BG: create, free, print a message of its message data, a string, or a string
// in a color and a font, whether it has printed, clear all or an area, and its bitmap window
MoneyWin *FieldMsgBG_CreateMoneyWin(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height);
void MsgBGPrinter_DeleteMsgWin(MoneyWin *win);
void MsgBGPrinter_MsgWinPrintMsg(MoneyWin *win, u16 x, u16 y, u32 messageId);
void MsgBGPrinter_MsgWinPrintStr(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf);
void MsgBGPrinter_MsgWinPrintStrColorFont(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf, u16 color, Font *font);
BOOL MsgBGPrinter_IsMsgWinTransDone(MoneyWin *win);
void MsgBGPrinter_ClearMsgWin(MoneyWin *win);
void MsgBGPrinter_FillMsgWin(MoneyWin *win, u32 x, u32 y, u32 width, u32 height);
BmpWin *MsgBGPrinter_GetMsgWinBmpWin(MoneyWin *win);
// The list window, with the frame of the system message window: create, free, print a string, and clear
MoneyWin *MsgBGPrinter_CreateInfoMsgWin(FieldMsgBG *sys, MsgData *msgData, u16 x, u16 y, u16 width, u16 height);
void MsgBGPrinter_DeleteInfoMsgWin(MoneyWin *win);
void MsgBGPrinter_InfoMsgWinPrintStr(MoneyWin *win, u16 x, u16 y, StrBuf *strbuf);
void MsgBGPrinter_ClearInfoMsgWin(MoneyWin *win);

// A list menu on the second BG: create, with a callback that prints each option or with the request's, or one that
// can't be cancelled, free it with its options, update (BMPMENULIST_NULL until an option is chosen or the menu is
// cancelled), and redraw
ListMenuUI *MsgBGPrinter_CreateMenuFuncCallback(FieldMsgBG *sys, const ListMenuRequest *request,
                                                ListMenuOption *options, u16 listTop, u16 cursorRow,
                                                BmpMenuListCursorCallback cursorCallback,
                                                BmpMenuListPrintCallback printCallback, void *work);
ListMenuUI *MsgBGPrinter_CreateMenuFunc(FieldMsgBG *sys, const ListMenuRequest *request, ListMenuOption *options);
ListMenuUI *ListMenuUI_Create(FieldMsgBG *sys, ListMenuRequest *request, ListMenuOption *options,
                              BmpMenuListCursorCallback callback, void *work, u16 listTop, u16 cursorRow,
                              u32 cancelable);
void MsgBGPrinter_DeleteMenuFunc(ListMenuUI *menu);
s32 ListMenuUI_Update(ListMenuUI *menu);
void MsgBGPrinter_RewriteMenuFunc(ListMenuUI *menu);
// The menu's options: allocate them, add the rows' messages, add a string, free the strings, and count them
ListMenuOption *InitListMenuOptionHeap(u32 count, HeapID heapId);
ListMenuOption *MsgBGPrinter_CreateMenuItemsFromTable(const ListMenuRow *rows, s32 count, MsgData *msgData,
                                                      HeapID heapId);
void AppendListMenuOption(ListMenuOption *options, StrBuf *text, u32 value, HeapID heapId);
void MsgBGPrinter_DeleteMenuItems(ListMenuOption *options);
u32 ListMenuCore_GetOptionCount(ListMenuOption *options);
// The width and the height in tiles of a menu of the options
u32 CalcListMenuWidth(FieldMsgBG *sys, ListMenuOption *options, u32 fontSizeX, u32 letterSpacing);
u32 CalcListMenuHeight(u32 rows, u32 rowHeight, u32 lineSpacing, BOOL scrolls);
void ListMenuRequest_Set(ListMenuRequest *request, u16 count, u16 x, u16 y, u16 width, u16 height);
// Draws the frame of the system message window around the menu
void MsgBGPrinter_DrawMenuFuncFrame(ListMenuUI *menu);
// The yes/no menu, with the cursor on a row, and its answer (MSGBG_YES_NO_)
ListMenuUI *MsgBGPrinter_CreateYesNoMenu(FieldMsgBG *sys, u32 cursorRow);
u32 MsgBGPrinter_YesNoMenuMain(ListMenuUI *menu);

// The talk window across the bottom of the screen, on the second BG: free, print a message of its message data or a
// string a character at a time, update it (TRUE once printed), create, clear, and its bitmap window
void MsgBGPrinter_DeleteSysWin(FieldMsgWin *win);
void MsgBGPrinter_SysWinPrintMsg(FieldMsgWin *win, u16 x, u16 y, u32 messageId);
void MsgBGPrinter_SysWinPrintStr(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf);
BOOL MsgBGPrinter_SysWinMain(FieldMsgWin *win);
FieldMsgWin *MsgBGPrinter_CreateBottomSysWin(FieldMsgBG *sys, MsgData *msgData);
void MsgBGPrinter_ClearSysWin(FieldMsgWin *win);
BmpWin *MsgBGPrinter_GetSysWinBmpWin(FieldMsgWin *win);
// The system message window at a row of the screen, with the list window's frame: create, free, print a message or a
// string a character at a time, update it (TRUE once printed), draw the wait-for-input cursor, clear, print a string at
// once, and its bitmap window
FieldMsgWin *MsgBGPrinter_CreateInfoSysWin(FieldMsgBG *sys, MsgData *msgData, u32 y);
void MsgBGPrinter_DeleteInfoSysWin(FieldMsgWin *win);
void MsgBGPrinter_InfoSysWinPrintMsg(FieldMsgWin *win, u16 x, u16 y, u32 messageId);
void MsgBGPrinter_InfoSysWinPrintStr(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf);
BOOL MsgBGPrinter_InfoSysWinMain(FieldMsgWin *win);
void MsgBGPrinter_SysWinDrawCursor(FieldMsgWin *win);
void MsgBGPrinter_ClearInfoSysWin(FieldMsgWin *win);
void MsgBGPrinter_InfoSysWinPrintStrNow(FieldMsgWin *win, u16 x, u16 y, StrBuf *strbuf);
BmpWin *MsgBGPrinter_GetInfoSysWinBmpWin(FieldMsgWin *win);

// A message balloon over an actor at pos, of an ACTOR_MSG_WIN_ type: create it, or NULL when that one is in use, close
// it, whether it has closed, free it at once, print another string in it, update it (TRUE once printed), and draw the
// wait-for-input cursor. A frameType is a TALKMSG_WIN_FRAME_ style; the spiky one shakes as it opens. tail is a
// TALKMSG_WIN_TAIL_ corner for the top and bottom balloons
ActorMsgWin *ActorMsgWin_CheckAndCreate(FieldMsgBG *sys, s32 type, const VecFx32 *pos, StrBuf *strbuf, u32 frameType,
                                        u32 tail);
void MsgBGPrinter_DeleteTalkWin(ActorMsgWin *win);
BOOL MsgBGPrinter_WaitTalkWinClose(ActorMsgWin *win);
void MsgBGPrinter_ClearTalkWin(ActorMsgWin *win);
void MsgBGPrinter_TalkWinPrintStr(ActorMsgWin *actorWin, StrBuf *strbuf);
BOOL MsgBGPrinter_TalkWinMain(ActorMsgWin *actorWin);
void MsgBGPrinter_TalkWinDrawCursor(ActorMsgWin *actorWin);

// The info window, framed like the talk windows: create, free, clear, clear its frame, print a message of its message
// data, a string or a string in a color at once, or a string a character at a time, update it (TRUE once printed), draw
// the wait-for-input cursor, and its print stream's state (PRINT_STREAM_), which fld_faceup.c moves a mouth with
InfoMsgWin *MsgBGPrinter_CreatePlainWin(FieldMsgBG *sys, MsgData *msgData, u32 frameType, u16 x, u16 y, u16 width,
                                        u16 height);
void MsgBGPrinter_DeletePlainWin(InfoMsgWin *win);
void MsgBGPrinter_ClearPlainWin(InfoMsgWin *win);
void MsgBGPrinter_OpenPlainWin(InfoMsgWin *win);
void MsgBGPrinter_PlainWinPrintMsg(InfoMsgWin *win, u16 x, u16 y, u32 messageId);
void MsgBGPrinter_PlainWinPrintStr(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf);
void MsgBGPrinter_PlainWinPrintStrColor(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf, u16 color);
void MsgBGPrinter_PlainWinPrintStream(InfoMsgWin *win, u16 x, u16 y, StrBuf *strbuf);
BOOL MsgBGPrinter_PlainWinMain(InfoMsgWin *win);
void MsgBGPrinter_PlainWinDrawCursor(InfoMsgWin *win);
u32 MsgBGPrinter_GetPlainWinStreamState(InfoMsgWin *win);

// The size in tiles of a window for a message
void CalcMsgWindowDimensions(FieldMsgBG *sys, StrBuf *str, u8 *width, u8 *height);
// The numbered talk windows, up to eight, each with a user's ID: create one with a message or a string, the latter in
// a frame style (TALKMSG_WIN_FRAME_), close the user's, close all, and whether any is open
void MsgBGPrinter_AddSubMsgWinMsg(FieldMsgBG *sys, MsgData *msgData, u32 msgId, u16 userId, u8 x, u8 y, u8 width,
                                  u8 height);
void MsgBGPrinter_AddSubMsgWinStr(FieldMsgBG *sys, StrBuf *strbuf, u16 userId, u8 x, u8 y, u8 width, u8 height);
void MsgBGPrinter_AddSubMsgWinStrFrame(FieldMsgBG *sys, StrBuf *strbuf, u16 userId, u8 x, u8 y, u8 width, u8 height,
                                       u32 frameType);
void MsgBGPrinter_DeleteSubMsgWin(FieldMsgBG *sys, u16 userId);
void MsgBGPrinter_DeleteAllSubMsgWin(FieldMsgBG *sys);
BOOL MsgBGPrinter_ExistsSubMsgWin(FieldMsgBG *sys);

// The sign window in one of four styles, which scrolls in from below: create it with the system's font or another,
// free it, and print, which returns TRUE once the player has read every page
SignMsgWin *MsgBGPrinter_CreateBgWin(FieldMsgBG *sys, u16 style);
SignMsgWin *MsgBGPrinter_CreateBgWinFont(FieldMsgBG *sys, u16 style, Font *font);
void MsgBGPrinter_DeleteBgWin(SignMsgWin *win);
BOOL MsgBGPrinter_BgWinMain(SignMsgWin *win, StrBuf *text);

// The checker window draws its frame in its own bitmap. Position and dimensions are in tiles;
// style is retained by the interface but does not change the frame.
CheckerMsgWin *CheckerMsgWin_Create(FieldMsgBG *sys, u16 style, u16 x, u16 y, u16 width, u16 height);
void CheckerMsgWin_Free(CheckerMsgWin *win);
// Prints every line immediately at pixel coordinates.
void CheckerMsgWin_PrintStrBuf(CheckerMsgWin *win, u16 x, u16 y, StrBuf *text);
// Returns TRUE for newly pressed A/B; otherwise advances the wait-for-input cursor.
BOOL CheckerMsgWin_Update(CheckerMsgWin *win);
// Returns the required width or height in tiles; margin is in pixels on each side.
u32 CheckerMsgWin_CalcWidth(FieldMsgBG *sys, StrBuf *str, u32 margin);
u32 CheckerMsgWin_CalcHeight(FieldMsgBG *sys, StrBuf *str, u32 margin);

// A wait-for-input cursor: create, free, and draw its next frame in a bitmap over the color
MsgBGCursor *MsgBGPrinter_CreateCursor(FieldMsgBG *sys);
void MsgBGPrinter_DeleteCursor(MsgBGCursor *cursor);
void MsgBGPrinter_AnimateCursor(MsgBGCursor *cursor, GFLBitmap *bitmap, u32 color);

// Blends the second BG over the others when on is TRUE
void setAlphaBlend_wrapper(BOOL on);
// Loads the first BG's frames again, and the talk windows'
void MsgBGPrinter_ReloadBg(FieldMsgBG *sys);

#endif // POKEBW2_FIELD_FIELD_MSGBG_H
