#ifndef POKEBW2_FIELD_FEST_MISSION_HOLLOW_H
#define POKEBW2_FIELD_FEST_MISSION_HOLLOW_H

#include "types.h"
#include "struct_decls.h"

// The Funfest missions of type 3 about the hidden hollows, overlay 26: ten of the twenty hollows are picked by the
// mission's seed, each with a person from the mission's table, and two of them are marked. Overlay 36's Funfest
// gimmick sets them up when such a mission starts, and it and overlay 73 (rival_select.c) check whether the mission
// is one. No string names the file; the name is a guess.

// Picks the mission's hollows into the festival's entries (LinkFestival_GetMissionWork)
void FesMissionHollow_Setup(FesGimmick *gimmick);
// Whether the current mission is a hollow mission (target 5 with a person set)
BOOL FesMissionHollow_IsActive(FesGimmick *gimmick);

#endif // POKEBW2_FIELD_FEST_MISSION_HOLLOW_H
