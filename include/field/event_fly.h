#ifndef POKEBW2_FIELD_EVENT_FLY_H
#define POKEBW2_FIELD_EVENT_FLY_H

#include "system/game_event.h"

GameEvent *EventFly_CreateMapChangeEvent(GameSystem *gsys, Field *field, u32 zoneId);
GameEventReturnCode EventFly_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventFly_FadeOutEventMain(GameEvent *event, u32 *state, void *data);

#endif // POKEBW2_FIELD_EVENT_FLY_H
