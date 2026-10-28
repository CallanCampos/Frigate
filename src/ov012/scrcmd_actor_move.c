// Two script commands that move an actor outside its movement codes: one raises or lowers it to a height over frames,
// and one holds it in place for a few frames. The name is descriptive
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/scrcmd_actor_move.h"
#include "nitro/fx.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

typedef struct {
    fx32 distance;
    u16 frames;
    u16 frame;
    FieldActor *actor;
    VecFx32 pos;
} ActorMoveHeightData;

typedef struct {
    u32 frame;
    FieldActor *actor;
    VecFx32 pos;
} ActorHoldData;

static GameEventReturnCode EventActorMoveHeight_Callback(GameEvent *event, u32 *state, void *work);
static GameEventReturnCode EventActorHold_Callback(GameEvent *event, u32 *state, void *work);

BOOL Scrcmd_ActorMove_ActorMoveToHeight(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    u16 actorId;
    u16 height;
    u16 frames;
    GameEvent *event;
    ActorMoveHeightData *data;
    FieldActor *actor;

    FieldScriptEnv_GetGameData(env);
    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    actorId = ScriptReadAny(vm, env);
    height = ScriptReadAny(vm, env);
    frames = ScriptReadAny(vm, env);
    event = GameEvent_Create(gsys, NULL, EventActorMoveHeight_Callback, sizeof(ActorMoveHeightData));
    data = GameEvent_GetData(event);
    data->frames = frames;
    data->frame = 0;
    actor = FindFieldActor(Field_GetActorSystem(GSYS_GetField(gsys)), actorId);
    CopyActorWPos(actor, &data->pos);
    data->actor = actor;
    SetActorMovementFlag(actor, ACTOR_MOVEMENT_FLAG_HIDE_SHADOW);
    // The height is in half tiles
    data->distance = height * (8 * FX32_ONE) - data->pos.y;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode EventActorMoveHeight_Callback(GameEvent *event, u32 *state, void *work) {
    ActorMoveHeightData *data = work;
    VecFx32 pos;

    data->frame++;
    pos = data->pos;
    pos.y += data->distance * data->frame / data->frames;
    SetActorWPosValue(data->actor, &pos);
    if (data->frame >= data->frames) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL Scrcmd_ActorMove_ActorHoldPos(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    u16 actorId;
    GameEvent *event;
    ActorHoldData *data;
    FieldActor *actor;

    FieldScriptEnv_GetGameData(env);
    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    actorId = ScriptReadAny(vm, env);
    event = GameEvent_Create(gsys, NULL, EventActorHold_Callback, sizeof(ActorHoldData));
    data = GameEvent_GetData(event);
    data->frame = 0;
    actor = FindFieldActor(Field_GetActorSystem(GSYS_GetField(gsys)), actorId);
    CopyActorWPos(actor, &data->pos);
    data->actor = actor;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode EventActorHold_Callback(GameEvent *event, u32 *state, void *work) {
    ActorHoldData *data = work;
    VecFx32 pos = data->pos;

    data->frame++;
    switch (*state) {
    case 0:
        SetActorWPosValue(data->actor, &pos);
        data->frame = 0;
        *state = 1;
        break;
    case 1:
        SetActorWPosValue(data->actor, &pos);
        data->frame = 0;
        *state = 2;
        break;
    case 2:
        SetActorWPosValue(data->actor, &pos);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
