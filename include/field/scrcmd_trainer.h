#ifndef POKEBW2_FIELD_SCRCMD_TRAINER_H
#define POKEBW2_FIELD_SCRCMD_TRAINER_H

// Overlay 36's scrcmd_trainer.c: the script commands of Trainers, their sighting, battles and music. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Trainer_TrainerEyeEventInit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerEyeEventStart(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerEyeGetBattleType(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerEyeGetTrainerID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerGetActorID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_ActorGetTrainerID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_CallTrainerBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_CallTrainerMultiBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerGetMessageTypes(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerGetBattleType(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerBGMPlayPush(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerClassBGMPlayPush(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerBGMPlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_CallTrainerBattleEnd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerGetFieldAction(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_TrainerGetPrizeItem(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Trainer_CallTradedPokemonBattle(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_TRAINER_H
