#include "types.h"
#include "constants/sound.h"
#include "field/event_dive.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_fog.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_task.h"
#include "field/rail_slipdown.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The steps of diving: sink the player, play the dive's sound once it is under, change the map when the sound ends,
// and wait for the player's animation in the new map
enum {
    DIVE_IN_STEP_SINK,
    DIVE_IN_STEP_WAIT_SINK,
    DIVE_IN_STEP_WAIT_SOUND,
    DIVE_IN_STEP_MAP_CHANGE,
    DIVE_IN_STEP_WAIT_ANIME,
};

GameEventReturnCode EventDiveIn_Callback(GameEvent *event, u32 *state, void *eventData) {
    DiveEventData *data = eventData;
    FieldTaskManager *taskManager;
    FieldTask *task;
    FieldPlayer *player;
    VecFx32 offset;
    u16 zoneId;

    switch (*state) {
    case DIVE_IN_STEP_SINK:
        taskManager = Field_GetTaskManager(data->field);
        offset.x = 0;
        offset.y = -0x40000;
        offset.z = 0;
        task = FieldActorMoveTask_CreatePlayer(data->field, 0x28, &offset);
        FieldTaskManager_AddTask(taskManager, task, 0);
        data->timer = 8;
        (*state)++;
        break;
    case DIVE_IN_STEP_WAIT_SINK:
        if (--data->timer > 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_FLD_123);
        (*state)++;
        break;
    case DIVE_IN_STEP_WAIT_SOUND:
        if (GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_123))) {
            break;
        }
        player = Field_GetPlayer(data->field);
        FieldPlayer_GetActor(player);
        FieldPlayer_SetSpecialState(player, 3);
        GetAbyssalRuinsDiveZoneID(data->field, &zoneId);
        GameEvent_ChainNext(event, EventMapChangeDiveIn_Create(data->gsys, zoneId));
        (*state)++;
        break;
    case DIVE_IN_STEP_MAP_CHANGE:
        FldAct_SetAcmd(FieldPlayer_GetActor(Field_GetPlayer(data->field)), 8);
        (*state)++;
        break;
    case DIVE_IN_STEP_WAIT_ANIME:
        if (FldAct_CheckAcmdEnd(FieldPlayer_GetActor(Field_GetPlayer(data->field)))) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

// The steps of surfacing: the sound and, when asked, the spin and the fog, the player's state, and the map change after
// 24 frames
enum {
    DIVE_OUT_STEP_START,
    DIVE_OUT_STEP_SET_STATE,
    DIVE_OUT_STEP_WAIT,
    DIVE_OUT_STEP_END,
};

GameEventReturnCode EventDiveOut_Callback(GameEvent *event, u32 *state, void *eventData) {
    DiveEventData *data = eventData;
    FieldTaskManager *taskManager = Field_GetTaskManager(data->field);
    FieldFog *fog;
    FieldTask *accelTask;
    FieldTask *spinTask;

    switch (*state) {
    case DIVE_OUT_STEP_START:
        GFL_SndSEPlay(SEQ_SE_FLD_123);
        data->timer = 0x18;
        if (data->param != 0) {
            fog = Field_GetFog(data->field);
            accelTask = FieldActorSpinTask_CreatePlayerAccel(data->field, 0x14, 3);
            spinTask = FieldActorSpinTask_CreatePlayer(data->field, 0xc, 3);
            FieldTaskManager_AddTask(taskManager, accelTask, 0);
            FieldTaskManager_AddTask(taskManager, spinTask, accelTask);
            FieldFog_Animate(fog, 0x7f0f, FieldFog_GetDepthShift(fog), 0x1a);
        }
        (*state)++;
        break;
    case DIVE_OUT_STEP_SET_STATE:
        FieldPlayer_SetSpecialState(Field_GetPlayer(data->field), 2);
        (*state)++;
        break;
    case DIVE_OUT_STEP_WAIT:
        if (--data->timer > 0) {
            break;
        }
        GameEvent_ChainNext(event, EventMapChangeDiveOut_Create(data->gsys));
        (*state)++;
        break;
    case DIVE_OUT_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventDiveIn_Create(GameSystem *gsys, Field *field) {
    GameEvent *event;
    DiveEventData *data;

    event = GameEvent_Create(gsys, NULL, EventDiveIn_Callback, sizeof(DiveEventData));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->param = 0;
    return event;
}

GameEvent *CreateDiveOutEvent(GameSystem *gsys, Field *field, u32 param) {
    GameEvent *event;
    DiveEventData *data;

    event = GameEvent_Create(gsys, NULL, EventDiveOut_Callback, sizeof(DiveEventData));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = field;
    data->param = param;
    return event;
}

GameEvent *EventRailSlipdown_CreatePlayer(GameSystem *gsys, Field *field, u16 direction) {
    GameEvent *event;
    void **data;
    FieldPlayer *player;
    FieldActor *actor;

    event = GameEvent_Create(gsys, NULL, EventRailSlipdown_Callback, sizeof(void *));
    data = GameEvent_GetData(event);
    GFL_OvlLoad(OVERLAY_ID(131));
    player = Field_GetPlayer(field);
    actor = FieldPlayer_GetActor(player);
    FieldPlayer_SetDirection(player, direction);
    *data = RailSlipdown_Create(gsys, field, actor, TRUE);
    return event;
}

GameEvent *EventRailSlipdown_CreateActor(GameSystem *gsys, Field *field, FieldActor *actor) {
    GameEvent *event;
    void **data;

    event = GameEvent_Create(gsys, NULL, EventRailSlipdown_Callback, sizeof(void *));
    data = GameEvent_GetData(event);
    GFL_OvlLoad(OVERLAY_ID(131));
    *data = RailSlipdown_Create(gsys, field, actor, FALSE);
    return event;
}

GameEventReturnCode EventRailSlipdown_Callback(GameEvent *event, u32 *state, void *data) {
    void **work = GameEvent_GetData(event);

    if (RailSlipdown_IsDone(*work)) {
        RailSlipdown_Delete(*work);
        GFL_OvlUnload(OVERLAY_ID(131));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
