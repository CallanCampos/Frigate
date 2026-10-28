#ifndef POKEBW2_FIELD_SCRCMD_GYM_H
#define POKEBW2_FIELD_SCRCMD_GYM_H

// Overlay 36's scrcmd_gym.c: the script commands of the gyms' puzzles

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Gym_GymElecReapplyProgress(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_GymElecSetFollower(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_GymElecSetProgress(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_GymElecSetStage(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_GymInsectPlayObject(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_GymInsectShowEffect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_DriftveilGymCallElevator(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_DriftveilGymCallFreight(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_DriftveilLiftCallMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_DriftveilGymPrepareFreight(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_DriftveilLiftPlaceAtBottom(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_MistraltonGymFireCannon(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_Gym0601FanAmbienceStart(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_IcirrusGymCallSwitchAnim(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_IcirrusGymCallFlipSwitch(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_IcirrusGymToggleMovementAtPlace(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_OpelucidGymCallMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_OpelucidGymCallButton(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_OpelucidGymRefreshVars(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_OpelucidGymFreeHeap(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_OpelucidGymCreateHeap(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_VirbankGymStopModelAnimations(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_VirbankGymSetActorFlag(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_HumilauGymPlayLeaderSplash(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_HumilauGymPlayLeaderWaves(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_RollerCoasterCallSwitch(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_RollerCoasterSetCarIntroSeen(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Gym_RollerCoasterAmbienceStart(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_GYM_H
