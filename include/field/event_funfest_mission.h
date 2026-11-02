#ifndef POKEBW2_FIELD_EVENT_FUNFEST_MISSION_H
#define POKEBW2_FIELD_EVENT_FUNFEST_MISSION_H

#include "system/game_event.h"

GameEvent *EventFunfestMission_Create(GameSystem *gsys);
GameEventReturnCode EventFunfestMission_Callback(GameEvent *event, u32 *state, void *data);

#endif // POKEBW2_FIELD_EVENT_FUNFEST_MISSION_H
