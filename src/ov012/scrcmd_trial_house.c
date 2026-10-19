// The Trial House's script commands, 0x1E6 to 0x1F5. Command names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the file's name is descriptive
#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_trial_house.h"
#include "field/trial_house.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL s01E6_TrialHouseWorkInit(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    TrialHouseWork **workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));

    if (*workPtr == NULL) {
        *workPtr = CreateTrialHouseWk(gsys);
    }
    return FALSE;
}

BOOL s01E7_TrialHouseWorkDelete(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    TrialHouseWorkDelete(gsys, GetTrialHouseWkPPtr(GSYS_GetGameData(gsys)));
    return FALSE;
}

BOOL s01E8_TrialHousePrepareParty(VM *vm, FieldScriptEnv *env) {
    TrialHouseWork **workPtr;

    FieldScriptEnv_GetScriptWork(env);
    workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    func_ov033_0217ad78(*workPtr, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s01E9_TrialHouseCallTeamSelect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    TrialHouseWork **workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));
    u16 mode = ScriptReadAny(vm, env);
    u16 battleBox = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    ScriptWork_CallEvent(work, func_ov012_02162c48(gsys, *workPtr, mode, battleBox, result));
    return TRUE;
}

BOOL func_ov012_02164564(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    TrialHouseWork **workPtr;
    u16 mode;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));
    mode = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    *result = func_ov033_0217adc4(gsys, *workPtr, mode);
    return FALSE;
}

BOOL s01EB_TrialHouseMsgDisp(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    TrialHouseWork **workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));
    u16 index = ScriptReadAny(vm, env);
    u16 actorId = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, func_ov033_0217aedc(gsys, *workPtr, index, actorId));
    return TRUE;
}

BOOL s01EC_TrialHouseStartBattle(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, CallTrialHouseBattle(gsys, *GetTrialHouseWkPPtr(GSYS_GetGameData(gsys))));
    return TRUE;
}

BOOL func_ov012_0216462c(VM *vm, FieldScriptEnv *env) {
    TrialHouseWork **workPtr;

    FieldScriptEnv_GetScriptWork(env);
    workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    func_ov033_0217adbc(*workPtr, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov012_0216465c(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    u16 rank;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    rank = ScriptReadAny(vm, env);
    RecordSetTrialHouseRank(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData)), rank);
    return FALSE;
}

BOOL s01EF_TrialHouseGetBattleTestRank(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    result = ScriptReadVar(vm, env);
    *result = RecordGetTrialHouseRank(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData)));
    return FALSE;
}

BOOL func_ov012_021646cc(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    TrialHouseWork **workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));

    ScriptWork_CallEvent(work, func_ov033_0217aee8(gsys, *workPtr, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL s01F1_TrialHouseCalcPointsStars(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    TrialHouseWork **workPtr;
    u16 *rank;
    u16 *points;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));
    rank = ScriptReadVar(vm, env);
    points = ScriptReadVar(vm, env);
    TrialHouseCalcPointScore(gsys, *workPtr, rank, points);
    return FALSE;
}

BOOL func_ov012_0216474c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    TrialHouseWork **workPtr;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));
    result = ScriptReadVar(vm, env);
    *result = func_ov033_0217b2e4(gsys, *workPtr);
    return FALSE;
}

BOOL s01F3_TrialHouseSaveData(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    TrialHouseWork **workPtr = GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));

    ScriptWork_CallEvent(work, func_ov033_0217b2ec(gsys, *workPtr, ScriptReadAny(vm, env)));
    return TRUE;
}

BOOL func_ov012_021647c4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 a1;
    u16 a2;

    GetTrialHouseWkPPtr(GSYS_GetGameData(gsys));
    a1 = ScriptReadAny(vm, env);
    a2 = ScriptReadAny(vm, env);
    ScriptWork_CallEvent(work, func_ov012_02162eb4(gsys, a1, a2));
    return TRUE;
}

BOOL func_ov012_0216480c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    result = ScriptReadVar(vm, env);
    *result = func_ov033_0217b32c(gsys);
    return FALSE;
}
