#include "types.h"
#include "field/field_async_proc.h"
#include "field/field_script.h"
#include "field/underwater_effect.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The script plugin of the Abyssal Ruins (plugin 5), commands from 1000

// Starts the underwater effect
static BOOL Scrcmd_AbyssalRuins_StartUnderwaterEffect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    FieldAsyncProcManager *mgr = Field_GetAsyncProcMgr(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));

    FieldAsyncProcManager_AddProc(OVERLAY_UNDERWATER_EFFECT, mgr, &UNDERWATER_EFFECT_PROC);
    return FALSE;
}

// Sets a variable to the steps counted in the Abyssal Ruins
static BOOL Scrcmd_AbyssalRuins_GetStepCounter(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    PlayerSave *playerSave =
        SaveControl_GetPlayerSave(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    u16 *var = ScriptReadVar(vm, env);

    *var = PlayerSave_GetAbyssalRuinsStepCounter(playerSave);
    return FALSE;
}

const FieldScriptCommand ABYSSAL_RUINS_SCRIPT_COMMANDS[] = {
    Scrcmd_AbyssalRuins_StartUnderwaterEffect,
    Scrcmd_AbyssalRuins_GetStepCounter,
    (FieldScriptCommand)0xFFFFFFFF,
};
