#include "types.h"
#include "app/demo3d.h"
#include "field/event_3d_demo.h"
#include "field/field_script.h"
#include "field/pleasure_boat.h"
#include "gfl/heap.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// The Royal Unova's view of the city at night, a 3D demo (a descriptive name). Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#define PLEASURE_BOAT_CLOCK_PERIOD 1350

typedef struct {
    Demo3DParam param;
    PleasureBoat *boat;
    u16 *result;
} EventRoyalUnovaWork;

static u32 PleasureBoat_GetRoomObjCode(PleasureBoat *boat, u32 room);
static GameEvent *EventRoyalUnova_Create(GameSystem *gsys, PleasureBoat *boat, u16 *result);
static GameEventReturnCode EventRoyalUnova_Callback(GameEvent *event, u32 *state, void *data);
static s16 PleasureBoat_GetClock(PleasureBoat *boat);
static void PleasureBoat_SetClock(PleasureBoat *boat, u32 clock);

static u32 PleasureBoat_GetRoomObjCode(PleasureBoat *boat, u32 room) {
    if (boat == NULL) {
        return 10;
    }
    return boat->rooms[room].objCode;
}

static GameEvent *EventRoyalUnova_Create(GameSystem *gsys, PleasureBoat *boat, u16 *result) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventRoyalUnova_Callback, sizeof(EventRoyalUnovaWork));
    EventRoyalUnovaWork *work = GameEvent_GetData(event);

    work->boat = boat;
    work->result = result;
    SetupPlaySequenceEventRealTime(&work->param, gsys, 1, 0);
    work->param.unk08 = PleasureBoat_GetClock(boat);
    return event;
}

void PleasureBoat_Free(PleasureBoat **boat) {
    if (*boat != NULL) {
        GFL_HeapFree(*boat);
        *boat = NULL;
    }
}

static GameEventReturnCode EventRoyalUnova_Callback(GameEvent *event, u32 *state, void *data) {
    EventRoyalUnovaWork *work = GameEvent_GetData(event);

    switch (*state) {
    case 0:
        GSYS_QueueProcAsEvent(event, OVERLAY_DEMO3D, &gDemo3DProcFunctions, work);
        (*state)++;
        break;
    case 1:
        if (work->param.result == 2) {
            *work->result = TRUE;
        } else {
            *work->result = FALSE;
        }
        PleasureBoat_SetClock(work->boat, work->param.unk10);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static s16 PleasureBoat_GetClock(PleasureBoat *boat) {
    return boat->clock;
}

static void PleasureBoat_SetClock(PleasureBoat *boat, u32 clock) {
    s32 period;

    boat->clock = clock;
    period = boat->clock / PLEASURE_BOAT_CLOCK_PERIOD;
    if (boat->period <= period) {
        boat->period = period;
    }
}

BOOL Scrcmd_RoyalUnova_PleasureBoatGetRoomInfo(VM *vm, FieldScriptEnv *env) {
    PleasureBoat **boat;
    u16 room;
    u16 kind;
    u16 index;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    boat = GameData_GetPleasureBoatPtr(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    room = ScriptReadAny(vm, env);
    kind = VM_Read16(vm);
    index = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    switch (kind) {
    case 4:
        *result = PleasureBoat_GetRoomObjCode(*boat, room);
        break;
    case 5:
        *result = PleasureBoat_GetPassengerParam(*boat, room);
        break;
    case 6:
        *result = PleasureBoat_GetPassengerType(*boat, room);
        break;
    case 7:
        *result = PleasureBoat_GetPassengerExtra(*boat, room, index);
        break;
    case 8:
        if (PleasureBoat_GetPassengerOccupied(*boat, room)) {
            *result = TRUE;
        } else {
            *result = FALSE;
        }
        break;
    default:
        *result = 0;
        break;
    }
    return FALSE;
}

BOOL Scrcmd_RoyalUnova_CallRoyalUnovaView(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    PleasureBoat **boat = GameData_GetPleasureBoatPtr(GSYS_GetGameData(gsys));
    GameEvent *event = EventRoyalUnova_Create(gsys, *boat, ScriptReadVar(vm, env));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}
