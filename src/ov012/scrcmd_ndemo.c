#include "types.h"
#include "field/field_script.h"
#include "field/fld_faceup.h"
#include "gfl/overlay.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// The scenes with N, which overlay 155 plays (a descriptive name). Command names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

static GameEventReturnCode EventNDemoEnd_Callback(GameEvent *event, u32 *state, void *data);

BOOL Scrcmd_Ndemo_NDemoStart(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 a0 = VM_Read16(vm);
    u8 a1 = VM_Read16(vm);
    u16 a2 = VM_Read16(vm);
    GameEvent *event;

    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_NDEMO);
    GFL_OvlLoad(OVERLAY_ID(155));
    event = EventFaceUpStart_Create(a0, a1, a2, gsys, env);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Ndemo_NDemoReadyTalkMotion(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    FaceUp_StartTalk(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL Scrcmd_Ndemo_NDemoEnd(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GameEvent_Create(FieldScriptEnv_GetGameSystem(env), NULL, EventNDemoEnd_Callback, 0);

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL FieldScriptSubEventFinish_NDemo(FinishScriptSubEventsWork *work, u32 *state) {
    FaceUp_FreeNow(GSYS_GetField(work->gsys));
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_NDEMO);
    GFL_OvlUnload(OVERLAY_ID(155));
    return TRUE;
}

static GameEventReturnCode EventNDemoEnd_Callback(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys = GameEvent_GetGameSystem(event);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, EventFaceUpEnd_Create(gsys));
        (*state)++;
        break;
    case 1:
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_NDEMO);
        GFL_OvlUnload(OVERLAY_ID(155));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
