#ifndef POKEBW2_APP_GTSNEGO_LOCAL_H
#define POKEBW2_APP_GTSNEGO_LOCAL_H

#include "types.h"
#include "app/gtsnego.h"
#include "app/ui/touchbar.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/app_taskmenu.h"

// GTS Negotiations' own declarations, shared by the files of overlay 195: the process (gtsnego_proc.c), its display
// (gtsnego_disp.c) and its messages (gtsnego_message.c). Our names

// The process's heap, which shares its number
#define HEAPID_GTSNEGO HEAPID_IRC_BATTLE_MENU

// Where the cursor of the lower screen is: the two buttons of the main menu, the three settings of the search and its
// button, the three rows of the list of people to meet, or nowhere
enum {
    GTSNEGO_CURSOR_ANYONE,
    GTSNEGO_CURSOR_RENDEZVOUS,
    GTSNEGO_CURSOR_LEVEL,
    GTSNEGO_CURSOR_MY_KIND,
    GTSNEGO_CURSOR_WANT_KIND,
    GTSNEGO_CURSOR_SEARCH,
    GTSNEGO_CURSOR_FRIEND_1,
    GTSNEGO_CURSOR_FRIEND_2,
    GTSNEGO_CURSOR_FRIEND_3,
    GTSNEGO_CURSOR_NONE,
};

// What the lower screen's palettes are dimmed for, leaving these lit: nothing; the yes/no menu over the list of
// people; the search's top screen; the yes/no menu over the main menu
enum {
    GTSNEGO_DIM_ALL,
    GTSNEGO_DIM_LIST_MENU,
    GTSNEGO_DIM_SEARCH,
    GTSNEGO_DIM_MAIN_MENU,
};

// What moving in the list of people did: nothing, moved the cursor, or scrolled the list
enum {
    GTSNEGO_SCROLL_NONE,
    GTSNEGO_SCROLL_CURSOR,
    GTSNEGO_SCROLL_LIST,
};

// The way the list of people scrolls
enum {
    GTSNEGO_LIST_STILL,
    GTSNEGO_LIST_DOWN,
    GTSNEGO_LIST_UP,
};

// The rows of the list of people on the screen, and the rows kept, with the hidden ones above and below
#define GTSNEGO_LIST_SHOWN 3
#define GTSNEGO_LIST_ROWS 6

// The list of people to meet: how many there are, the row of the cursor and the person in the first row
typedef struct {
    int count;
    int cursor;
    int top;
} GtsNegoScroll;

typedef struct GtsNegoDisp GtsNegoDisp;
typedef struct GtsNegoMessage GtsNegoMessage;

// gtsnego_proc.c
// The person of the Pal Pad's list at index, or NULL past its ends
PlayerInfo *GtsNego_GetFriend(GameData *gameData, int index);

// gtsnego_disp.c
GtsNegoDisp *GtsNegoDisp_Create(HeapID heapId, GameData *gameData);
void GtsNegoDisp_Main(GtsNegoDisp *disp);
void GtsNegoDisp_Free(GtsNegoDisp *disp);
ClActUnit *GtsNegoDisp_GetClActUnit(GtsNegoDisp *disp);
// Dims the lower screen's palettes for a GTSNEGO_DIM_*, or brings them back
void GtsNegoDisp_SetDim(GtsNegoDisp *disp, BOOL dim, u32 type);
void GtsNegoDisp_Dummy(GtsNegoDisp *disp, BOOL list);
// The search's settings on the lower screen, with their arrows
void GtsNegoDisp_ShowSearchMenu(GtsNegoDisp *disp);
void GtsNegoDisp_HideSearchMenu(GtsNegoDisp *disp);
void GtsNegoDisp_CreateTouchBar(GtsNegoDisp *disp, u32 objPalette);
TouchBar *GtsNegoDisp_GetTouchBar(GtsNegoDisp *disp);
void GtsNegoDisp_FreeTouchBar(GtsNegoDisp *disp);
// Puts the cursor at a GTSNEGO_CURSOR_*, the search button being lit instead of it
void GtsNegoDisp_SetCursor(GtsNegoDisp *disp, AppTaskMenuWin *button, int cursor);
// Plays the cursor's animation of a press, and whether it has ended
BOOL GtsNegoDisp_PressCursor(GtsNegoDisp *disp, int cursor);
BOOL GtsNegoDisp_IsCursorPressDone(GtsNegoDisp *disp);
void GtsNegoDisp_FreeArrows(GtsNegoDisp *disp);
void GtsNegoDisp_PressArrow(GtsNegoDisp *disp, int arrow);
// The list of people to meet, with a scroll bar when there are more than its rows
void GtsNegoDisp_ShowFriendList(GtsNegoDisp *disp, BOOL scrollable);
void GtsNegoDisp_FreeListScreen(GtsNegoDisp *disp);
void GtsNegoDisp_DrawListRows(GtsNegoDisp *disp, GameData *gameData, int top);
void GtsNegoDisp_LeaveFriendList(GtsNegoDisp *disp);
void GtsNegoDisp_HideFriendList(GtsNegoDisp *disp);
// Move the cursor in the list, returning a GTSNEGO_SCROLL_*
int GtsNegoDisp_ScrollDown(GtsNegoDisp *disp, GtsNegoScroll *scroll);
int GtsNegoDisp_ScrollUp(GtsNegoDisp *disp, GtsNegoScroll *scroll);
void GtsNegoDisp_SetScrollCursor(GtsNegoDisp *disp, GtsNegoScroll *scroll, u32 row);
void GtsNegoDisp_FreeListFooter(GtsNegoDisp *disp);
void GtsNegoDisp_FreeListActors(GtsNegoDisp *disp);
void GtsNegoDisp_SetKnobAnimating(GtsNegoDisp *disp, BOOL animating);
// Starts scrolling the list by a row (GTSNEGO_LIST_*), stops it, and scrolls it for a frame: the way it still
// scrolls, and in done the way it has just ended
void GtsNegoDisp_StartListScroll(GtsNegoDisp *disp, int dir);
void GtsNegoDisp_StopListScroll(GtsNegoDisp *disp);
int GtsNegoDisp_UpdateListScroll(GtsNegoDisp *disp, int *done);
void GtsNegoDisp_MoveFriendIcons(GtsNegoDisp *disp, BOOL up);
void GtsNegoDisp_ResetList(GtsNegoDisp *disp);
void GtsNegoDisp_CreateFriendIcon(GtsNegoDisp *disp, int row, int avatar);
void GtsNegoDisp_ShowFriendIcon(GtsNegoDisp *disp, int row, int x, int y);
void GtsNegoDisp_SetFriendIcon(GtsNegoDisp *disp, PlayerInfo *friend, int row);
void GtsNegoDisp_MoveFriendIcon(GtsNegoDisp *disp, int row, int y);
// Move the icons a row up or down, the new person's taking the free row
void GtsNegoDisp_ShiftFriendIconsUp(GtsNegoDisp *disp, PlayerInfo *friend);
void GtsNegoDisp_ShiftFriendIconsDown(GtsNegoDisp *disp, PlayerInfo *friend);
void GtsNegoDisp_SetFriendIcons(GtsNegoDisp *disp, GameData *gameData, int top);
// The scroll bar's knob: puts it where a position of count is, and drags it to y, returning the position there
void GtsNegoDisp_SetKnob(GtsNegoDisp *disp, int pos, int count);
int GtsNegoDisp_DragKnob(GtsNegoDisp *disp, int y, int count);
// The top screen while it searches, and with the partner found
void GtsNegoDisp_ShowPartner(GtsNegoDisp *disp, int avatar);
void GtsNegoDisp_ShowSearching(GtsNegoDisp *disp);
// Back to the screens of the main menu
void GtsNegoDisp_Reset(GtsNegoDisp *disp);

// gtsnego_message.c
GtsNegoMessage *GtsNegoMessage_Create(HeapID heapId, u32 bank);
void GtsNegoMessage_Update(GtsNegoMessage *msg);
void GtsNegoMessage_Free(GtsNegoMessage *msg);
// Print a message in the top screen's window, four lines or two, and whether it is printed
void GtsNegoMessage_Print(GtsNegoMessage *msg, u32 msgNo);
void GtsNegoMessage_PrintShort(GtsNegoMessage *msg, u32 msgNo);
BOOL GtsNegoMessage_IsDone(GtsNegoMessage *msg);
void GtsNegoMessage_ClearWindow(GtsNegoMessage *msg);
// The yes/no menu, at one of two heights
AppTaskMenu *GtsNegoMessage_CreateYesNo(GtsNegoMessage *msg, u32 layout);
void GtsNegoMessage_UpdateButtons(GtsNegoMessage *msg);
// The lower screen's labels
void GtsNegoMessage_FreeLabels(GtsNegoMessage *msg);
int GtsNegoMessage_AddLabel(GtsNegoMessage *msg, u32 msgNo, int x, int y, int width, int bg);
int GtsNegoMessage_AddMenuLabel(GtsNegoMessage *msg, u32 msgNo, int x, int y);
void GtsNegoMessage_SetLabel(GtsNegoMessage *msg, u32 msgNo, int label);
void GtsNegoMessage_QueueLabelsLoad(GtsNegoMessage *msg);
void GtsNegoMessage_ClearLabels(GtsNegoMessage *msg);
// The labels and touch buttons of the main menu and of the search's settings
void GtsNegoMessage_ShowMainMenu(GtsNegoMessage *msg, ButtonManCallback callback, void *work);
void GtsNegoMessage_ShowSearchMenu(GtsNegoMessage *msg, ButtonManCallback callback, void *work, int level, int myKind,
                                   int wantKind);
void GtsNegoMessage_SetLevel(GtsNegoMessage *msg, int level);
void GtsNegoMessage_SetMyKind(GtsNegoMessage *msg, int kind);
void GtsNegoMessage_SetWantKind(GtsNegoMessage *msg, int kind);
void GtsNegoMessage_HideMenu(GtsNegoMessage *msg);
// A button at the bottom of the lower screen
AppTaskMenuWin *GtsNegoMessage_CreateButton(GtsNegoMessage *msg, u32 msgNo);
void GtsNegoMessage_PrintHeading(GtsNegoMessage *msg, u32 msgNo);
void GtsNegoMessage_PrintLocation(GtsNegoMessage *msg, PlayerInfo *partner);
void GtsNegoMessage_ClearLocation(GtsNegoMessage *msg);
// The names of the list of people to meet
int GtsNegoMessage_PrintFriend(GtsNegoMessage *msg, PlayerInfo *friend, int row);
void GtsNegoMessage_ShowFriendList(GtsNegoMessage *msg, GameData *gameData);
void GtsNegoMessage_PrintLastFriend(GtsNegoMessage *msg, GameData *gameData, int index);
void GtsNegoMessage_ShiftFriendsUp(GtsNegoMessage *msg);
void GtsNegoMessage_PrintFriends(GtsNegoMessage *msg, GameData *gameData, int top);
void GtsNegoMessage_PrintFirstFriend(GtsNegoMessage *msg, GameData *gameData, int index);
void GtsNegoMessage_ShiftFriendsDown(GtsNegoMessage *msg);
// The menu that confirms the person to meet
AppTaskMenu *GtsNegoMessage_CreateConfirm(GtsNegoMessage *msg, u32 layout);
void GtsNegoMessage_FreeMenu(AppTaskMenu *menu);
// The partner's greeting
void GtsNegoMessage_CreatePMSDraw(GtsNegoMessage *msg, GtsNegoDisp *disp);
void GtsNegoMessage_FreePMSDraw(GtsNegoMessage *msg);
void GtsNegoMessage_PrintPhrase(GtsNegoMessage *msg, PMSData *phrase);
void GtsNegoMessage_PrintTitle(GtsNegoMessage *msg, u32 msgNo);
// The search's cancel button
void GtsNegoMessage_CreateCancelButton(GtsNegoMessage *msg, ButtonManCallback callback, void *work);
void GtsNegoMessage_PressCancelButton(GtsNegoMessage *msg);
void GtsNegoMessage_FreeCancelButton(GtsNegoMessage *msg);
BOOL GtsNegoMessage_IsCancelPressDone(GtsNegoMessage *msg);
void GtsNegoMessage_PrintTradeCount(GtsNegoMessage *msg, u32 msgNo, int count);
void GtsNegoMessage_PrintPartnerName(GtsNegoMessage *msg, PlayerInfo *partner);
// The partner found, on the top screen: name, trades, place
void GtsNegoMessage_ShowPartner(GtsNegoMessage *msg, PlayerInfo *partner, int count);
void GtsNegoMessage_Reset(GtsNegoMessage *msg);

#endif // POKEBW2_APP_GTSNEGO_LOCAL_H
