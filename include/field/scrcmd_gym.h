#ifndef POKEBW2_FIELD_SCRCMD_GYM_H
#define POKEBW2_FIELD_SCRCMD_GYM_H

// Overlay 36's scrcmd_gym.c: the script commands of the gyms' puzzles

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s025B_GymElecReapplyProgress(VM *vm, FieldScriptEnv *env);
BOOL s025C_GymElecSetFollower(VM *vm, FieldScriptEnv *env);
BOOL s025D_GymElecSetProgress(VM *vm, FieldScriptEnv *env);
BOOL s025F_GymElecSetStage(VM *vm, FieldScriptEnv *env);
BOOL s0268_GymInsectPlayObject(VM *vm, FieldScriptEnv *env);
BOOL s0269_GymInsectShowEffect(VM *vm, FieldScriptEnv *env);
BOOL ScrcmdFieldGimmickPlateDoors_StartDoorEvent(VM *vm, FieldScriptEnv *env);
BOOL s018D_DriftveilGymCallFreight(VM *vm, FieldScriptEnv *env);
BOOL s018E_DriftveilLiftCallMove(VM *vm, FieldScriptEnv *env);
BOOL s0186_DriftveilGymPrepareFreight(VM *vm, FieldScriptEnv *env);
BOOL s0187_DriftveilLiftPlaceAtBottom(VM *vm, FieldScriptEnv *env);
BOOL s028E_MistraltonGymFireCannon(VM *vm, FieldScriptEnv *env);
BOOL s02BB_Gym0601FanAmbienceStart(VM *vm, FieldScriptEnv *env);
BOOL s018F_IcirrusGymCallSwitchAnim(VM *vm, FieldScriptEnv *env);
BOOL s0190_IcirrusGymCallFlipSwitch(VM *vm, FieldScriptEnv *env);
BOOL s0191_IcirrusGymToggleMovementAtPlace(VM *vm, FieldScriptEnv *env);
BOOL s02A5_OpelucidGymCallMove(VM *vm, FieldScriptEnv *env);
BOOL s02A7_OpelucidGymCallButton(VM *vm, FieldScriptEnv *env);
BOOL s02A6_OpelucidGymRefreshVars(VM *vm, FieldScriptEnv *env);
BOOL s02A8_OpelucidGymFreeHeap(VM *vm, FieldScriptEnv *env);
BOOL s02A9_OpelucidGymCreateHeap(VM *vm, FieldScriptEnv *env);
BOOL s0291_VirbankGymStopModelAnimations(VM *vm, FieldScriptEnv *env);
BOOL s0292_VirbankGymSetActorFlag(VM *vm, FieldScriptEnv *env);
BOOL s02A2_HumilauGymPlayLeaderSplash(VM *vm, FieldScriptEnv *env);
BOOL s02A3_HumilauGymPlayLeaderWaves(VM *vm, FieldScriptEnv *env);
BOOL s017C_RollerCoasterCallSwitch(VM *vm, FieldScriptEnv *env);
BOOL ScrcmdFieldGimmickCartLanes_UnlockLaneFromScript(VM *vm, FieldScriptEnv *env);
BOOL s024D_RollerCoasterAmbienceStart(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_GYM_H
