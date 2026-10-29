#ifndef POKEBW2_FIELD_SCRCMD_FIELD_ACTOR_H
#define POKEBW2_FIELD_SCRCMD_FIELD_ACTOR_H

// Overlay 36's scrcmd_field_actor.c: the script commands of field actors. PauseEventMModels and
// EnableAllActorsMovementScr, which overlay 12 calls, are in field/field_script.h. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_FieldActor_ActorCmdExec(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorWalkRoute(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorCmdWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorGetMoveCode(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorSetMoveCode(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorGetGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerGetGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerGetDir(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorNew(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorDelete(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorSetGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorGetSpawnFlag(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorGetUserParam(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_SetActorZoneDeleteOff(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorSetEyeToEye(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorFindByGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerGetActorInFront(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerSetSpecialSequence(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerMoveToYAsync(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerMoveToYAsync_0248(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerTurnByTrigger(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerGetExState(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorJumpToGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_CheckPlayerFormModel(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorMoveLinear(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerGetRailPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorGetRailPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorPlayRailSlipdown(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_PlayerSetRailPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorSetRailPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorPlayTeleportSeq(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldActor_ActorFallDownToXZ(VM *vm, FieldScriptEnv *env);

// The actors the script waits on to stop, among the common symbols at the end of the .bss
extern u8 g_ScrEventActorFlags;

#endif // POKEBW2_FIELD_SCRCMD_FIELD_ACTOR_H
