#include "types.h"
#include "field/field_script.h"
#include "field/gimmick_humilau.h"
#include "system/vm.h"

// Script plugin 15 (overlay 67), commands from 1000, of zones 463, 465 and 474, whose gimmick is overlay 113

static BOOL Scrcmd_Plugin15_ShowSplash(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);

    GimmickHumilau_ShowSplash(gsys, x, y, z);
    return FALSE;
}

static BOOL Scrcmd_Plugin15_JumpActorOut(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 actorId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 a4 = ScriptReadAny(vm, env);
    GameEvent *event = GimmickHumilau_CreateJumpEvent(gsys, actorId, x, z, a4);

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

const FieldScriptCommand PLUGIN_15_SCRIPT_COMMANDS[] = {
    Scrcmd_Plugin15_ShowSplash,
    Scrcmd_Plugin15_JumpActorOut,
    (FieldScriptCommand)0xFFFFFFFF,
};
