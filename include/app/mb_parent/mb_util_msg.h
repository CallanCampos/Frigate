#ifndef POKEBW2_APP_MB_PARENT_MB_UTIL_MSG_H
#define POKEBW2_APP_MB_PARENT_MB_UTIL_MSG_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The messages, windows and menus of the DS Download Play parent (mb_util_msg.c). The names are ours

// msgBg and menuBg are BGs; with useTalkWin, the messages of window MB_UTIL_MSG_WINDOW_TALK get the field's talk
// window frame, and with useKeys they wait for the keys as well as the touch screen
MBUtilMsg *MBUtilMsg_Create(HeapID heapId, u8 msgBg, u8 menuBg, u32 fileId, BOOL useTalkWin, BOOL useKeys);
void MBUtilMsg_Delete(MBUtilMsg *msg);
void MBUtilMsg_Update(MBUtilMsg *msg);
// Shows the message window in a shape, MB_UTIL_MSG_WINDOW_* of mb_util_msg.c, and clears it
void MBUtilMsg_SetWindow(MBUtilMsg *msg, u32 type);
// Prints a message of the message data, formatted by the word set if there is one
void MBUtilMsg_Print(MBUtilMsg *msg, u32 msgId, s32 wait);
void MBUtilMsg_PrintAtOnce(MBUtilMsg *msg, u32 msgId);
void MBUtilMsg_ClearWindow(MBUtilMsg *msg);
void MBUtilMsg_CreateWordSet(MBUtilMsg *msg);
void MBUtilMsg_FreeWordSet(MBUtilMsg *msg);
void MBUtilMsg_SetNumber(MBUtilMsg *msg, u32 index, s32 number, u32 digits);
void MBUtilMsg_SetNumberZeroPadded(MBUtilMsg *msg, u32 index, s32 number, u32 digits);
// What the yes/no menu and dialog answer: nothing yet, yes or no
enum {
    MB_UTIL_MSG_WAIT,
    MB_UTIL_MSG_YES,
    MB_UTIL_MSG_NO,
};

// A yes/no task menu, at one of three heights
void MBUtilMsg_CreateYesNoMenu(MBUtilMsg *msg, u32 pos);
void MBUtilMsg_FreeMenu(MBUtilMsg *msg);
// The item picked, from 1 (MB_UTIL_MSG_YES and MB_UTIL_MSG_NO), or MB_UTIL_MSG_WAIT
int MBUtilMsg_GetMenuResult(MBUtilMsg *msg);
// A yes/no dialog; the argument is not used
void MBUtilMsg_CreateConfirm(MBUtilMsg *msg, u32 unused);
// Drops the dialog without freeing it, once it has freed itself
void MBUtilMsg_ForgetConfirm(MBUtilMsg *msg);
// MB_UTIL_MSG_YES, MB_UTIL_MSG_NO, or MB_UTIL_MSG_WAIT while the dialog waits
int MBUtilMsg_UpdateConfirm(MBUtilMsg *msg);
// Prints the message that the wireless is off, from the script messages
void MBUtilMsg_PrintNoWireless(MBUtilMsg *msg, s32 wait);
MsgData *MBUtilMsg_GetMsgData(MBUtilMsg *msg);
WordSet *MBUtilMsg_GetWordSet(MBUtilMsg *msg);
Font *MBUtilMsg_GetFont(MBUtilMsg *msg);
BOOL MBUtilMsg_IsQueueDone(MBUtilMsg *msg);
BOOL MBUtilMsg_IsPrintDone(MBUtilMsg *msg);
void MBUtilMsg_ShowWindow(MBUtilMsg *msg, BOOL shown);
// Shows the wait icon once the message printed at once is done
void MBUtilMsg_SetShowWaitIcon(MBUtilMsg *msg, BOOL show);

#endif // POKEBW2_APP_MB_PARENT_MB_UTIL_MSG_H
