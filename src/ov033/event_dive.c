#include "types.h"
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

GameEventReturnCode EventDiveIn_Callback(GameEvent *event, u32 *state, void *eventData) {
    DiveEventData *data = eventData;
    FieldTaskManager *taskManager;
    FieldTask *task;
    FieldPlayer *player;
    VecFx32 offset;
    u16 zoneId;

    switch (*state) {
    case 0:
        taskManager = Field_GetTaskManager(data->field);
        offset.x = 0;
        offset.y = -0x40000;
        offset.z = 0;
        task = FieldActorMoveTask_CreatePlayer(data->field, 0x28, &offset);
        FieldTaskManager_AddTask(taskManager, task, 0);
        data->timer = 8;
        (*state)++;
        break;
    case 1:
        if (--data->timer > 0) {
            break;
        }
        GFL_SndSEPlay(0x63c);
        (*state)++;
        break;
    case 2:
        if (GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(0x63c))) {
            break;
        }
        player = Field_GetPlayer(data->field);
        FieldPlayer_GetActor(player);
        FieldPlayer_SetSpecialState(player, 3);
        GetAbyssalRuinsDiveZoneID(data->field, &zoneId);
        GameEvent_ChainNext(event, EventMapChangeDiveIn_Create(data->gsys, zoneId));
        (*state)++;
        break;
    case 3:
        FldAct_SetAcmd(FieldPlayer_GetActor(Field_GetPlayer(data->field)), 8);
        (*state)++;
        break;
    case 4:
        if (FldAct_CheckAcmdEnd(FieldPlayer_GetActor(Field_GetPlayer(data->field)))) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventDiveOut_Callback(GameEvent *event, u32 *state, void *eventData) {
    DiveEventData *data = eventData;
    FieldTaskManager *taskManager = Field_GetTaskManager(data->field);
    FieldFog *fog;
    FieldTask *accelTask;
    FieldTask *spinTask;

    switch (*state) {
    case 0:
        GFL_SndSEPlay(0x63c);
        data->timer = 0x18;
        if (data->param != 0) {
            fog = Field_GetFog(data->field);
            accelTask = FieldActorSpinTask_CreatePlayerAccel(data->field, 0x14, 3);
            spinTask = FieldActorSpinTask_CreatePlayer(data->field, 0xc, 3);
            FieldTaskManager_AddTask(taskManager, accelTask, 0);
            FieldTaskManager_AddTask(taskManager, spinTask, (u32)accelTask);
            FieldFog_Animate(fog, 0x7f0f, FieldFog_GetDepthShift(fog), 0x1a);
        }
        (*state)++;
        break;
    case 1:
        FieldPlayer_SetSpecialState(Field_GetPlayer(data->field), 2);
        (*state)++;
        break;
    case 2:
        if (--data->timer > 0) {
            break;
        }
        GameEvent_ChainNext(event, EventMapChangeDiveOut_Create(data->gsys));
        (*state)++;
        break;
    case 3:
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

GameEvent *func_ov033_0217a0f4(GameSystem *gsys, Field *field, u32 direction) {
    GameEvent *event;
    void **data;
    FieldPlayer *player;
    FieldActor *actor;

    event = GameEvent_Create(gsys, NULL, func_ov033_0217a184, sizeof(void *));
    data = GameEvent_GetData(event);
    GFL_OvlLoad(OVERLAY_ID(131));
    player = Field_GetPlayer(field);
    actor = FieldPlayer_GetActor(player);
    FieldPlayer_SetDirection(player, direction);
    *data = RailSlipdown_Create(gsys, field, actor, TRUE);
    return event;
}

GameEvent *func_ov033_0217a148(GameSystem *gsys, Field *field, FieldActor *actor) {
    GameEvent *event;
    void **data;

    event = GameEvent_Create(gsys, NULL, func_ov033_0217a184, sizeof(void *));
    data = GameEvent_GetData(event);
    GFL_OvlLoad(OVERLAY_ID(131));
    *data = RailSlipdown_Create(gsys, field, actor, FALSE);
    return event;
}

GameEventReturnCode func_ov033_0217a184(GameEvent *event, u32 *state, void *data) {
    void **work = GameEvent_GetData(event);

    if (RailSlipdown_IsDone(*work)) {
        RailSlipdown_Delete(*work);
        GFL_OvlUnload(OVERLAY_ID(131));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
