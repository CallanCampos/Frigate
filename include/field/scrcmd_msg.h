#ifndef POKEBW2_FIELD_SCRCMD_MSG_H
#define POKEBW2_FIELD_SCRCMD_MSG_H

// Overlay 36's scrcmd_msg.c: the script commands of the message windows. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
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

BOOL s0034_SystemMsg(VM *vm, FieldScriptEnv *env);
BOOL s0035_SystemMsgAsync(VM *vm, FieldScriptEnv *env);
BOOL s0037_MsgSetLoadingSpinner(VM *vm, FieldScriptEnv *env);
BOOL s0036_InfoMsgClose(VM *vm, FieldScriptEnv *env);
BOOL s0040_MoneyWinDisp(VM *vm, FieldScriptEnv *env);
BOOL s0042_MoneyWinUpdate(VM *vm, FieldScriptEnv *env);
BOOL s0041_MoneyWinClose(VM *vm, FieldScriptEnv *env);
BOOL s003C_ActorMsg(VM *vm, FieldScriptEnv *env);
BOOL s003D_ParentActorMsg(VM *vm, FieldScriptEnv *env);
BOOL s0278_FunfestDispSalesmanMessage(VM *vm, FieldScriptEnv *env);
BOOL s0048_ActorMsgGendered(VM *vm, FieldScriptEnv *env);
BOOL s0049_ActorMsgVersioned(VM *vm, FieldScriptEnv *env);
BOOL s003E_ActorMsgClose(VM *vm, FieldScriptEnv *env);
BOOL s0087_TrainerSayMessage(VM *vm, FieldScriptEnv *env);
BOOL s0038_InfoMsg(VM *vm, FieldScriptEnv *env);
BOOL s004A_ScreamMsg(VM *vm, FieldScriptEnv *env);
BOOL s0039_InfoMsgClose(VM *vm, FieldScriptEnv *env);
BOOL s003A_MultiMsg(VM *vm, FieldScriptEnv *env);
BOOL s003B_MsgWinCloseNo(VM *vm, FieldScriptEnv *env);
BOOL s0043_MsgPlaceSign(VM *vm, FieldScriptEnv *env);
BOOL func_ov036_021a9554(VM *vm, FieldScriptEnv *env);
BOOL s0044_MsgPlaceSignClose(VM *vm, FieldScriptEnv *env);
BOOL s0045_CheckerMsg(VM *vm, FieldScriptEnv *env);
BOOL s0046_CheckerMsgClose(VM *vm, FieldScriptEnv *env);
BOOL s003F_MsgWinCloseAll(VM *vm, FieldScriptEnv *env);
BOOL s004B_MsgWaitAdvance(VM *vm, FieldScriptEnv *env);
BOOL s0033_MsgSetAutoscrolls(VM *vm, FieldScriptEnv *env);

// Close the windows when the script's sub events are finished, in FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS
BOOL func_ov036_021a8844(FinishScriptSubEventsWork *work, u32 *state);
BOOL func_ov036_021a90fc(FinishScriptSubEventsWork *work, u32 *state);
BOOL func_ov036_021a9350(FinishScriptSubEventsWork *work, u32 *state);
BOOL func_ov036_021a9478(FinishScriptSubEventsWork *work, u32 *state);
BOOL func_ov036_021a95ec(FinishScriptSubEventsWork *work, u32 *state);
BOOL func_ov036_021a96ec(FinishScriptSubEventsWork *work, u32 *state);
BOOL func_ov036_021a9808(FinishScriptSubEventsWork *work, u32 *state);

#endif // POKEBW2_FIELD_SCRCMD_MSG_H
