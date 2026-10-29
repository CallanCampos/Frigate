#include "types.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/gimmick_underground_ruins.h"
#include "field/sp_poke_gimmick.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// Script plugin 16 (overlay 68), commands from 1000. The first three drive zone 565's gimmick (overlay 114), the
// others the gimmick of zones 53 and 614 (overlay 129)

typedef struct {
    Field *field;
} Plugin16WaitData;

static BOOL Scrcmd_Plugin16_ShowRuinsModel(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    UndergroundRuins_ShowModel(gsys, ScriptReadAny(vm, env));
    return FALSE;
}

static BOOL Scrcmd_Plugin16_SetRuinsAnimFrame(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    UndergroundRuins_SetAnmEnd(gsys, ScriptReadAny(vm, env));
    return FALSE;
}

static BOOL Scrcmd_Plugin16_PlayRuinsAnim(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    UndergroundRuins_PlayAnm(gsys, ScriptReadAny(vm, env));
    return FALSE;
}

static BOOL Scrcmd_Plugin16_CallFormEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = SpPokeGimmick_CallFormEvent(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Plugin16_HideActor2(VM *vm, FieldScriptEnv *env) {
    SpPokeGimmick_HideActor2(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Plugin16_StartChase(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    FieldPlayer *player = Field_GetPlayer(field);

    SpPokeGimmick_StartChase(field, ScriptReadAny(vm, env));
    return FALSE;
}

// Waits for SpPokeGimmick_IsChaseDone to return TRUE
static GameEventReturnCode Plugin16_WaitChaseEvent(GameEvent *event, u32 *state, void *data) {
    Plugin16WaitData *wait = data;

    if (SpPokeGimmick_IsChaseDone(wait->field) == TRUE) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static BOOL Scrcmd_Plugin16_WaitChase(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, Plugin16_WaitChaseEvent, sizeof(Plugin16WaitData));
    Plugin16WaitData *wait = GameEvent_GetData(event);

    wait->field = field;
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Plugin16_DropActor(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    SpPokeGimmick_DropActor(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

const FieldScriptCommand PLUGIN_16_SCRIPT_COMMANDS[] = {
    Scrcmd_Plugin16_ShowRuinsModel, Scrcmd_Plugin16_SetRuinsAnimFrame, Scrcmd_Plugin16_PlayRuinsAnim,
    Scrcmd_Plugin16_CallFormEvent,  Scrcmd_Plugin16_HideActor2,        Scrcmd_Plugin16_StartChase,
    Scrcmd_Plugin16_WaitChase,      Scrcmd_Plugin16_DropActor,         (FieldScriptCommand)0xFFFFFFFF,
};
