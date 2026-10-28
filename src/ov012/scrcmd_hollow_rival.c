// Script commands of the rival's hollows (a descriptive name)
#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_hollow_rival.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

#define HOLLOW_COUNT 15

BOOL Scrcmd_HollowRival_HollowRivalSetValue(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    u16 index = VM_Read16(vm);
    u16 value = VM_Read16(vm);

    HollowRivalSave_SetValue(getHollow_RivalBlk(GameData_GetSaveControl(gameData)), index, value);
    return FALSE;
}

BOOL Scrcmd_HollowRival_HollowRivalGetValue(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    u16 index = VM_Read16(vm);
    u16 *result = ScriptReadVar(vm, env);

    *result = HollowRivalSave_GetValue(getHollow_RivalBlk(GameData_GetSaveControl(gameData)), index);
    return FALSE;
}

BOOL Scrcmd_HollowRival_HollowRivalSetFlag(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    u16 index = VM_Read16(vm);

    HollowRivalSave_SetFlag(getHollow_RivalBlk(GameData_GetSaveControl(gameData)), index, TRUE);
    return FALSE;
}

BOOL func_ov012_0216a748(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    u16 *result = ScriptReadVar(vm, env);
    HollowRivalSave *block;
    int i;
    int count;

    VM_Read16(vm);
    block = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
    count = 0;
    for (i = 0; i < HOLLOW_COUNT; i++) {
        if (func_0200ffd4(block, i)) {
            count++;
        }
    }
    *result = count >= 10 ? TRUE : FALSE;
    return FALSE;
}

BOOL Scrcmd_HollowRival_HollowRivalGetCounter(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    u16 *result = ScriptReadVar(vm, env);

    *result = HollowRivalSave_GetMeeting(getHollow_RivalBlk(GameData_GetSaveControl(gameData)));
    return FALSE;
}

BOOL func_ov012_0216a7cc(VM *vm, FieldScriptEnv *env) {
    HollowRivalSave *block =
        getHollow_RivalBlk(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));

    HollowRivalSave_SetMet(block, TRUE);
    HollowRivalSave_ClearMeeting(block);
    return FALSE;
}

BOOL func_ov012_0216a7f4(VM *vm, FieldScriptEnv *env) {
    HollowRivalSave *block =
        getHollow_RivalBlk(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 value = ScriptReadAny(vm, env);

    HollowRivalSave_SetHasSpecies(block, TRUE);
    HollowRivalSave_SetSpecies(block, value);
    return FALSE;
}
