#ifndef POKEBW2_FIELD_EVENT_FEST_MISSION_H
#define POKEBW2_FIELD_EVENT_FEST_MISSION_H

#include "types.h"
#include "field/festival.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// Overlay 157: the field's events for a Funfest mission starting, ending and being shown. The file's name is a guess;
// the ROM has no string for it.

#define OVERLAY_FUNFEST_MISSION OVERLAY_ID(157)

// What the start, the message and the mission's window are given, built by ov033's event_funfest_mission.c
struct FestMissionEventArgs {
    FestMissionConfig config;
    // Nonzero when the player joins another's mission, 0 when the player hosts it
    u32 joined;
    // Which message EventFestMissionMessage_Create shows, from message 4
    u32 messageType;
};

// GameEventProviders. args is a FestMissionEventArgs for the start, the message and the mission's window
GameEvent *EventFestMissionStart_Create(GameSystem *gsys, void *args);
GameEvent *EventFestMissionMessage_Create(GameSystem *gsys, void *args);
// args is the field
GameEvent *EventFestMissionMenu_Create(GameSystem *gsys, void *args);
GameEvent *EventFestMissionInfo_Create(GameSystem *gsys, void *args);
GameEvent *EventFestMissionRetire_Create(GameSystem *gsys, void *args);
GameEvent *EventFestMissionDisconnect_Create(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_FEST_MISSION_H
