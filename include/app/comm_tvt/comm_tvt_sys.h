#ifndef POKEBW2_APP_COMM_TVT_COMM_TVT_SYS_H
#define POKEBW2_APP_COMM_TVT_COMM_TVT_SYS_H

#include "types.h"
#include "app/comm_tvt.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The Xtransceiver's system, which owns every part of the video chat and hands them to each other

// How the top screen is split between the members of the call. Game Freak's name for the first is in an assert; the
// others are guesses
enum {
    CTDM_SINGLE,
    CTDM_DOUBLE,
    CTDM_QUAD,
};

// The modes of the Xtransceiver, each the work of one file
enum {
    COMM_TVT_MODE_NONE,
    COMM_TVT_MODE_TALK,
    COMM_TVT_MODE_CALL,
    COMM_TVT_MODE_DRAW,
    COMM_TVT_MODE_GAME,
    COMM_TVT_MODE_EXIT,
    COMM_TVT_MODE_EXIT_ERROR,
};

CtvtCamera *CommTvt_GetCamera(CommTvtWork *sys);
CtvtComm *CommTvt_GetComm(CommTvtWork *sys);
CtvtTalk *CommTvt_GetTalk(CommTvtWork *sys);
CtvtMic *CommTvt_GetMic(CommTvtWork *sys);
DrawSystem *CommTvt_GetDrawSystem(CommTvtWork *sys);
CtvtGame *CommTvt_GetGame(CommTvtWork *sys);
CtvtCall *CommTvt_GetCall(CommTvtWork *sys);
CommTvtParam *CommTvt_GetParam(CommTvtWork *sys);
HeapID CommTvt_GetHeapId(CommTvtWork *sys);
ArcTool *CommTvt_GetArc(CommTvtWork *sys);
u32 CommTvt_GetObjResource(CommTvtWork *sys, int index);
ClActUnit *CommTvt_GetClActUnit(CommTvtWork *sys);
BOOL func_ov257_021aaa74(CommTvtWork *sys);
void func_ov257_021aaa78(CommTvtWork *sys, BOOL value);
Font *CommTvt_GetFont(CommTvtWork *sys);
MsgData *CommTvt_GetMsgData(CommTvtWork *sys);
PrintQueue *CommTvt_GetPrintQueue(CommTvtWork *sys);
AppTaskMenuRes *CommTvt_GetTaskMenuRes(CommTvtWork *sys);
u8 CommTvt_GetMode(CommTvtWork *sys);
u8 CommTvt_GetNextMode(CommTvtWork *sys);
void CommTvt_SetErrorShown(CommTvtWork *sys);
BOOL CommTvt_IsErrorShown(CommTvtWork *sys);
u8 CommTvt_GetMemberCount(CommTvtWork *sys);
void CommTvt_SetMemberCount(CommTvtWork *sys, u8 count);
int CommTvt_GetDisplayMode(CommTvtWork *sys);
BOOL CommTvt_IsZoomed(CommTvtWork *sys);
void CommTvt_SendZoom(CommTvtWork *sys, BOOL zoomed);
void CommTvt_ToggleZoom(CommTvtWork *sys);
void CommTvt_SetZoomed(CommTvtWork *sys, BOOL zoomed);
u8 CommTvt_GetSelfIndex(CommTvtWork *sys);
void CommTvt_SetSelfIndex(CommTvtWork *sys, u8 index);
BOOL func_ov257_021aab10(CommTvtWork *sys);
void CommTvt_SetMessageActive(CommTvtWork *sys, BOOL value);
BOOL CommTvt_GetViewSwapped(CommTvtWork *sys);
void CommTvt_ToggleViewSwapped(CommTvtWork *sys);
BOOL CommTvt_GetInputLocked(CommTvtWork *sys);
void func_ov257_021aab30(CommTvtWork *sys, BOOL value);
BOOL CommTvt_GetEndRequested(CommTvtWork *sys);
void func_ov257_021aab38(CommTvtWork *sys, BOOL value);
BOOL CommTvt_IsLaunchModeCall(CommTvtWork *sys);
BOOL CommTvt_CanExchangePhotos(CommTvtWork *sys);
void CommTvt_ClearCanExchangePhotos(CommTvtWork *sys);
BOOL func_ov257_021aab5c(CommTvtWork *sys);
void func_ov257_021aab60(CommTvtWork *sys, BOOL value);
BOOL CommTvt_IsCameraEnabled(void);
// The yes/no menus and the cancel button, at a corner in tiles
AppTaskMenu *CommTvt_OpenConfirmMenu(CommTvtWork *sys);
AppTaskMenu *func_ov257_021aac08(CommTvtWork *sys, u8 right, u8 bottom);
AppTaskMenu *func_ov257_021aac98(CommTvtWork *sys, u8 right, u8 bottom);
void CommTvt_ClearNameWindows(CommTvtWork *sys);
void CommTvt_ClearAndTransferNameWindows(CommTvtWork *sys);
void CommTvt_PlaceSlotMarker(CommTvtWork *sys, u8 talker);
void CommTvt_HideSlotMarker(CommTvtWork *sys);
void CommTvt_SavePeerRecord(CommTvtWork *sys, const CtvtCommMemberInfo *info);
void CommTvt_CreateTimeIcon(CommTvtWork *sys, BmpWin *window);
void CommTvt_DeleteTimeIcon(CommTvtWork *sys);

#endif // POKEBW2_APP_COMM_TVT_COMM_TVT_SYS_H
