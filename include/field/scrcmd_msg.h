#ifndef POKEBW2_FIELD_SCRCMD_MSG_H
#define POKEBW2_FIELD_SCRCMD_MSG_H

// Overlay 36's scrcmd_msg.c: the script commands of the message windows. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of command 0x230, the sub events'
// finish functions and the helpers swan has no name for

#include "types.h"
#include "field/field_script.h"
#include "gfl/g3d.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/vm.h"

// Where a script's message window opens: across the top or the bottom of the screen, AUTO away from the player for a
// balloon over an actor, and the balloons offset to the left or the right of their actor. The positions are stored as
// u8s (ActorMsgWin_SetPosActual)
enum {
    MSG_WIN_POS_AUTO,
    MSG_WIN_POS_TOP,
    MSG_WIN_POS_BOTTOM,
    MSG_WIN_POS_TOP_LEFT,
    MSG_WIN_POS_BOTTOM_LEFT,
    MSG_WIN_POS_TOP_RIGHT,
    MSG_WIN_POS_BOTTOM_RIGHT,
};

// Where a balloon over an actor goes: the offset and window position of a MSG_WIN_POS_, and the position away from
// where the player stands
void ActorMsgWin_GetBalloonOffset(const VecFx32 *pos, VecFx32 *offset, G3DCamera *g3dCamera, FieldCamera *camera,
                                  u8 winPos);
void ActorMsgWin_GetBalloonTailParams(u32 winPos, u32 *type, u32 *tail);
u8 ActorMsgWin_CalcWinPosAuto(FieldActor *player, const VecFx32 *pos);
// Show a message, and have the script wait for it
BOOL CallFieldActorMessageDispStrBuf(VM *vm, FieldScriptEnv *env, StrBuf *message, u16 actorId, u16 pos, u32 frameType);
BOOL loadMsgBox(VM *vm, FieldScriptEnv *env, StrBuf *message, u32 frameType, u8 pos);

BOOL Scrcmd_Msg_SystemMsg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_SystemMsgAsync(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgSetLoadingSpinner(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_SystemMsgClose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MoneyWinDisp(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MoneyWinUpdate(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MoneyWinClose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_ActorMsg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_ParentActorMsg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_FunfestDispSalesmanMessage(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_ActorMsgGendered(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_ActorMsgVersioned(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_ActorMsgClose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_TrainerSayMessage(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_InfoMsg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_ScreamMsg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_InfoMsgClose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MultiMsg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgWinCloseNo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgPlaceSign(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgPlaceSignFont4(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgPlaceSignClose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_CheckerMsg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_CheckerMsgClose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgWinCloseAll(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgWaitAdvance(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Msg_MsgSetAutoscrolls(VM *vm, FieldScriptEnv *env);

// Close the windows when the script's sub events are finished, in FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS
BOOL FieldScriptSubEventFinish_SystemMsg(FinishScriptSubEventsWork *work, u32 *state);
BOOL FieldScriptSubEventFinish_ActorMsg(FinishScriptSubEventsWork *work, u32 *state);
BOOL FieldScriptSubEventFinish_InfoMsg(FinishScriptSubEventsWork *work, u32 *state);
BOOL FieldScriptSubEventFinish_MultiMsg(FinishScriptSubEventsWork *work, u32 *state);
BOOL FieldScriptSubEventFinish_Sign(FinishScriptSubEventsWork *work, u32 *state);
BOOL FieldScriptSubEventFinish_CheckerMsg(FinishScriptSubEventsWork *work, u32 *state);
BOOL ActorMsgWin_CleanupAutoMode(FinishScriptSubEventsWork *work, u32 *state);

#endif // POKEBW2_FIELD_SCRCMD_MSG_H
