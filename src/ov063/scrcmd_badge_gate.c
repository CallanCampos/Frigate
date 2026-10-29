#include "types.h"
#include "field/badge_gate.h"
#include "field/field_script.h"
#include "system/vm.h"

// The script plugin of the badge gates on the way to Victory Road (plugin 11), commands from 1000

// Plays the gate of a badge (0 to 7, from the script) checking it
static BOOL Scrcmd_BadgeGate_PlayCheck(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 badge = ScriptReadAny(vm, env);
    GameEvent *event = BadgeGate_CreateCheckEvent(gsys, badge);

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Plays the last gate
static BOOL Scrcmd_BadgeGate_PlayLastGate(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = BadgeGate_CreateLastGateEvent(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

const FieldScriptCommand BADGE_GATE_SCRIPT_COMMANDS[] = {
    Scrcmd_BadgeGate_PlayCheck,
    Scrcmd_BadgeGate_PlayLastGate,
    (FieldScriptCommand)0xFFFFFFFF,
};
