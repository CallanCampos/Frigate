#ifndef POKEBW2_APP_WIFIBATTLEMATCH_UTIL_H
#define POKEBW2_APP_WIFIBATTLEMATCH_UTIL_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// Overlay 262's wifibattlematch_util.c, named after the ROM's string: a task runner, a message box, a list menu and the
// records that the matches add to. See wifibattlematch_graphic.h for the overlay

typedef struct WifiBattleMatchTask WifiBattleMatchTask;
typedef struct WifiBattleMatchMsgBox WifiBattleMatchMsgBox;
typedef struct WifiBattleMatchMenu WifiBattleMatchMenu;

// A step of a task: seq is the step it is in, and work is the work that the task was created with
typedef void (*WifiBattleMatchTaskFunc)(WifiBattleMatchTask *task, u32 *seq, void *work);

// How the message box prints its message
#define WIFIBATTLEMATCH_MSG_QUEUE 0
#define WIFIBATTLEMATCH_MSG_STREAM_KEY 1
#define WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON 2
#define WIFIBATTLEMATCH_MSG_STREAM 3

// What WifiBattleMatchMenu is made from: a list of messages of msgData, with the cursor and its window
typedef struct {
    MsgData *msgData;
    Font *font;
    PrintQueue *printQueue;
    u32 msgIds[4];
    u32 count;
    u16 bg;
    u16 palette;
    u16 framePalette;
    u16 frameChar;
    // Whether B cancels the menu
    u32 canCancel;
    // What WifiBattleMatchMenu's update returns when B cancels it
    u16 cancelValue;
    u16 cursorRow;
} WifiBattleMatchMenuParam;

// Tasks
WifiBattleMatchTask *WifiBattleMatchTask_Create(void *work, WifiBattleMatchTaskFunc func, HeapID heapId);
void WifiBattleMatchTask_Free(WifiBattleMatchTask *task);
// Runs the current step
void WifiBattleMatchTask_Run(WifiBattleMatchTask *task);
// Is TRUE once the task has ended
BOOL WifiBattleMatchTask_IsEnded(WifiBattleMatchTask *task);
// Makes func the step that runs next
void WifiBattleMatchTask_SetFunc(WifiBattleMatchTask *task, WifiBattleMatchTaskFunc func);
// Ends the task
void WifiBattleMatchTask_End(WifiBattleMatchTask *task);
// Remembers the step to go back to, which WifiBattleMatchTask_ReturnToSeq goes to
void WifiBattleMatchTask_SetReturnSeq(WifiBattleMatchTask *task, u32 seq);
void WifiBattleMatchTask_ReturnToSeq(WifiBattleMatchTask *task);

// The message box, at the bottom of BG bg. The frame is drawn with frameChar and framePalette
WifiBattleMatchMsgBox *WifiBattleMatchMsgBox_Create(u32 bg, u32 palette, u16 framePalette, u16 frameChar,
                                                    PrintQueue *printQueue, Font *font, HeapID heapId);
void WifiBattleMatchMsgBox_Free(WifiBattleMatchMsgBox *box);
void WifiBattleMatchMsgBox_Update(WifiBattleMatchMsgBox *box);
// Prints a message of msgData, with a WIFIBATTLEMATCH_MSG_* way of printing it
void WifiBattleMatchMsgBox_Print(WifiBattleMatchMsgBox *box, MsgData *msgData, u32 msgId, u32 mode);
void WifiBattleMatchMsgBox_PrintStr(WifiBattleMatchMsgBox *box, StrBuf *text, u32 mode);
// Gives the box a key cursor, if it has none
void WifiBattleMatchMsgBox_EnableKeyCursor(WifiBattleMatchMsgBox *box);
// Is TRUE once the message is printed
BOOL WifiBattleMatchMsgBox_IsDone(WifiBattleMatchMsgBox *box);
// Clears the message
void WifiBattleMatchMsgBox_Clear(WifiBattleMatchMsgBox *box);

// The list menu. WifiBattleMatchMenu_CreateBottomRight puts it at the bottom right
WifiBattleMatchMenu *WifiBattleMatchMenu_CreateBottomRight(const WifiBattleMatchMenuParam *param, HeapID heapId);
WifiBattleMatchMenu *WifiBattleMatchMenu_Create(const WifiBattleMatchMenuParam *param, u8 x, u8 y, u8 width, u8 height,
                                                HeapID heapId);
void WifiBattleMatchMenu_Free(WifiBattleMatchMenu *menu);
// The item picked, or -1 while none is
s32 WifiBattleMatchMenu_Update(WifiBattleMatchMenu *menu);

// Count a match in the records. mode is the kind of match
void func_ov262_021c5c94(GameData *gameData, u32 mode);
void func_ov262_021c5cd4(GameData *gameData, u32 mode);

#endif // POKEBW2_APP_WIFIBATTLEMATCH_UTIL_H
