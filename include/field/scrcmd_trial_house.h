#ifndef POKEBW2_FIELD_SCRCMD_TRIAL_HOUSE_H
#define POKEBW2_FIELD_SCRCMD_TRIAL_HOUSE_H

// Overlay 12's scrcmd_trial_house.c (a descriptive name): the Trial House's script commands, 0x1E6 to 0x1F5. Names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s01E6_TrialHouseWorkInit(VM *vm, FieldScriptEnv *env);
BOOL s01E7_TrialHouseWorkDelete(VM *vm, FieldScriptEnv *env);
BOOL s01E8_TrialHousePrepareParty(VM *vm, FieldScriptEnv *env);
BOOL s01E9_TrialHouseCallTeamSelect(VM *vm, FieldScriptEnv *env);
BOOL s01EA_TrialHouseSetTrainer(VM *vm, FieldScriptEnv *env);
BOOL s01EB_TrialHouseMsgDisp(VM *vm, FieldScriptEnv *env);
BOOL s01EC_TrialHouseStartBattle(VM *vm, FieldScriptEnv *env);
BOOL s01ED_TrialHouseSetDownloadMode(VM *vm, FieldScriptEnv *env);
BOOL s01EE_TrialHouseSetBattleTestRank(VM *vm, FieldScriptEnv *env);
BOOL s01EF_TrialHouseGetBattleTestRank(VM *vm, FieldScriptEnv *env);
BOOL s01F0_TrialHouseDownloadExamination(VM *vm, FieldScriptEnv *env);
BOOL s01F1_TrialHouseCalcPointsStars(VM *vm, FieldScriptEnv *env);
BOOL s01F2_TrialHouseGetExaminationType(VM *vm, FieldScriptEnv *env);
BOOL s01F3_TrialHouseSaveData(VM *vm, FieldScriptEnv *env);
BOOL s01F4_TrialHouseCallResults(VM *vm, FieldScriptEnv *env);
BOOL s01F5_TrialHouseGetRecordMask(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_TRIAL_HOUSE_H
