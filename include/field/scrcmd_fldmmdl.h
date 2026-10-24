#ifndef POKEBW2_FIELD_SCRCMD_FLDMMDL_H
#define POKEBW2_FIELD_SCRCMD_FLDMMDL_H

// Overlay 36's scrcmd_fldmmdl.c: the script commands of field actors. PauseEventMModels and EnableAllActorsMovementScr,
// which overlay 12 calls, are in field/field_script.h. Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s0064_ActorCmdExec(VM *vm, FieldScriptEnv *env);
BOOL s024F_ActorWalkRoute(VM *vm, FieldScriptEnv *env);
BOOL s0065_ActorCmdWait(VM *vm, FieldScriptEnv *env);
BOOL s0066_ActorGetMoveCode(VM *vm, FieldScriptEnv *env);
BOOL s0073_ActorSetMoveCode(VM *vm, FieldScriptEnv *env);
BOOL s0067_ActorGetGPos(VM *vm, FieldScriptEnv *env);
BOOL s0068_PlayerGetGPos(VM *vm, FieldScriptEnv *env);
BOOL s006E_PlayerGetDir(VM *vm, FieldScriptEnv *env);
BOOL s0069_ActorNew(VM *vm, FieldScriptEnv *env);
BOOL s006C_ActorDelete(VM *vm, FieldScriptEnv *env);
BOOL s006B_ActorAdd(VM *vm, FieldScriptEnv *env);
BOOL s006D_ActorSetGPos(VM *vm, FieldScriptEnv *env);
BOOL s006A_ActorGetSpawnFlag(VM *vm, FieldScriptEnv *env);
BOOL s0079_ActorGetUserParam(VM *vm, FieldScriptEnv *env);
BOOL ScriptNative_SetMmdlZoneDeleteOff(VM *vm, FieldScriptEnv *env);
BOOL s0074_ActorSetEyeToEye(VM *vm, FieldScriptEnv *env);
BOOL s0070_ActorFindByGPos(VM *vm, FieldScriptEnv *env);
BOOL s006F_PlayerGetActorInFront(VM *vm, FieldScriptEnv *env);
BOOL s0075_PlayerSetSpecialSequence(VM *vm, FieldScriptEnv *env);
BOOL s0076_PlayerMoveToYAsync(VM *vm, FieldScriptEnv *env);
BOOL s0248_PlayerMoveToYAsync_(VM *vm, FieldScriptEnv *env);
BOOL s0077_PlayerTurnByTrigger(VM *vm, FieldScriptEnv *env);
BOOL s0078_PlayerGetExState(VM *vm, FieldScriptEnv *env);
BOOL s007B_ActorJumpToGPos(VM *vm, FieldScriptEnv *env);
BOOL ScriptNative_CheckPlayerFormModel(VM *vm, FieldScriptEnv *env);
BOOL s02BE_ActorMoveLinear(VM *vm, FieldScriptEnv *env);
BOOL s0071_PlayerGetRailPos(VM *vm, FieldScriptEnv *env);
BOOL s0072_ActorGetRailPos(VM *vm, FieldScriptEnv *env);
BOOL s007A_ActorPlayRailSlipdown(VM *vm, FieldScriptEnv *env);
BOOL s007C_PlayerSetRailPos(VM *vm, FieldScriptEnv *env);
BOOL s007D_ActorSetRailPos(VM *vm, FieldScriptEnv *env);
BOOL s007E_ActorPlayTeleportSeq(VM *vm, FieldScriptEnv *env);
BOOL s0234_ActorFallDownToXZ(VM *vm, FieldScriptEnv *env);

// The actors the script waits on to stop, among the common symbols at the end of the .bss
extern u8 g_ScrEventActorFlags;

#endif // POKEBW2_FIELD_SCRCMD_FLDMMDL_H
