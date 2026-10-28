#ifndef POKEBW2_FIELD_SCRCMD_TRIAL_HOUSE_H
#define POKEBW2_FIELD_SCRCMD_TRIAL_HOUSE_H

// Overlay 12's scrcmd_trial_house.c (a descriptive name): the Trial House's script commands, 0x1E6 to 0x1F5. Names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_TrialHouse_TrialHouseWorkInit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseWorkDelete(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHousePrepareParty(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseCallTeamSelect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseSetTrainer(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseMsgDisp(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseStartBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseSetDownloadMode(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseSetBattleTestRank(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseGetBattleTestRank(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseDownloadExamination(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseCalcPointsStars(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseGetExaminationType(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseSaveData(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseCallResults(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_TrialHouse_TrialHouseGetRecordMask(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_TRIAL_HOUSE_H
