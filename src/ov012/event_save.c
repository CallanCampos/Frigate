#include "types.h"
#include "field/app_call.h"
#include "field/event_action_call.h"
#include "field/event_save.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/inf_dungeon_retire.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

GameEvent *EventSave_Create(GameSystem *gsys, Field *field, u16 heapId, void *msgBgSys, u32 screenId, u32 *result) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventSave_Callback, sizeof(EventSaveWork));
    EventSaveWork *work = GameEvent_GetData(event);
    GameData *gameData;

    sys_memset(work, 0, sizeof(EventSaveWork));
    work->msgBgSys = msgBgSys;
    work->heapId = heapId;
    work->gameSystem = gsys;
    work->field = field;
    gameData = GSYS_GetGameData(gsys);
    work->save = GameData_GetSaveControl(gameData);
    work->screenId = screenId;
    work->result = result;
    return event;
}

GameEventReturnCode EventSave_Callback(GameEvent *event, u32 *state, void *data) {
    EventSaveWork *work = data;
    u32 result = EventSave_Update(work, state);

    switch (result) {
    case 0:
        *work->result = 1;
        return GAMEEVENT_DONE;
    case 1:
        *work->result = 0;
        return GAMEEVENT_DONE;
    default:
        return GAMEEVENT_CONTINUE;
    }
}

void func_ov012_0215c574(FieldAppCallWork *work, s32 appParam) {
    FieldAppCallInput *input = work->input;
    GameEvent *event = InfDungeonRetire_CreateEvent(input->gameSystem, input->field, input->screenId, &work->result);
    GameEvent_ChainNext(work->event, event);
}

void func_ov012_0215c594(void *param) {
    GFL_HeapFree(param);
}

// A third actor that would wait with the player and the actor of move code 0x30. The retail game has none, but the
// test stays in the code, which MWCC only does for a variable of an enum type
typedef enum {
    THIRD_ACTOR_NONE,
    THIRD_ACTOR_PRESENT,
} ThirdActorKind;

GameEventReturnCode func_ov012_0215c59c(GameEvent *event, u32 *state, void *data) {
    PrepareResidentActorsWork *work = data;
    Field *field = work->field;
    MMSys *actorSystem;
    FieldActor *player;
    FieldActor *actor;
    FieldActor *third;
    ThirdActorKind kind;

    switch (*state) {
    case 0:
        actorSystem = Field_GetActorSystem(field);
        player = FindPlayerFieldActor(actorSystem);
        actor = FindActorByMoveCode(actorSystem, 0x30);
        work->movingActors = 0;
        DisableAllActorsMovement(actorSystem);
        if (!func_ov012_02166ecc(player)) {
            work->movingActors |= 1;
            EnableActorMovement(player);
        }
        kind = THIRD_ACTOR_NONE;
        third = NULL;
        if (kind != THIRD_ACTOR_NONE && IsActorFlag16(third) == TRUE) {
            work->movingActors |= 4;
            EnableActorMovement(third);
        }
        if (actor != NULL && IsActorFlag16(actor) == TRUE) {
            work->movingActors |= 2;
            EnableActorMovement(actor);
        }
        (*state)++;
        break;
    case 1:
        actorSystem = Field_GetActorSystem(field);
        player = FindPlayerFieldActor(actorSystem);
        if ((work->movingActors & 1) && func_ov012_02166ecc(player) == TRUE) {
            DisableActorMovement(player);
            work->movingActors &= ~1;
        }
        if (work->movingActors & 4) {
            third = NULL;
            if (!IsActorFlag16(third)) {
                DisableActorMovement(third);
                work->movingActors &= ~4;
            }
        }
        if (work->movingActors & 2) {
            actor = FindActorByMoveCode(actorSystem, 0x30);
            if (!IsActorFlag16(actor)) {
                DisableActorMovement(actor);
                work->movingActors &= ~2;
            }
        }
        if (work->movingActors == 0) {
            (*state)++;
        }
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
