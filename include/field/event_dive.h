#ifndef POKEBW2_FIELD_EVENT_DIVE_H
#define POKEBW2_FIELD_EVENT_DIVE_H

#include "system/game_event.h"

struct DiveEventData {
    GameSystem *gsys;
    Field *field;
    u32 param;
    s32 timer;
};

GameEvent *EventDiveIn_Create(GameSystem *gsys, Field *field);
GameEvent *CreateDiveOutEvent(GameSystem *gsys, Field *field, u32 param);
GameEventReturnCode EventDiveIn_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventDiveOut_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventRailSlipdown_CreatePlayer(GameSystem *gsys, Field *field, u16 direction);
GameEvent *EventRailSlipdown_CreateActor(GameSystem *gsys, Field *field, FieldActor *actor);
GameEventReturnCode EventRailSlipdown_Callback(GameEvent *event, u32 *state, void *data);

#endif // POKEBW2_FIELD_EVENT_DIVE_H
