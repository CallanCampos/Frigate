// Script commands of the step counter. Command names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0);
// the file's name is descriptive
#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_pedometer.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL Scrcmd_Pedometer_PedometerStart(VM *vm, FieldScriptEnv *env) {
    PlayerSave_BeginStepCounter(
        SaveControl_GetPlayerSave(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)))));
    return FALSE;
}

BOOL Scrcmd_Pedometer_PedometerEnd(VM *vm, FieldScriptEnv *env) {
    PlayerSave_EndStepCounter(
        SaveControl_GetPlayerSave(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)))));
    return FALSE;
}

BOOL Scrcmd_Pedometer_PedometerGet(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    u16 *result = ScriptReadVar(vm, env);

    *result = PlayerSave_GetStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
    return FALSE;
}
