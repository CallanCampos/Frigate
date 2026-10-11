#ifndef POKEBW2_FIELD_FEST_MISSION_SPARKLE_PLACE_H
#define POKEBW2_FIELD_FEST_MISSION_SPARKLE_PLACE_H

#include "types.h"
#include "struct_decls.h"

// Overlay 24: the Funfest missions of type 4, on entering a zone. It picks which of the zone's places get a sparkle
// and which Pokémon the sparkle gives, for overlay 71's fest_mission_sparkle.c, and remembers the zone. No string names
// the file; the name is a guess from what it does.

// On entering a zone: picks the places of the mission's sparkles and what they give
void FestSparklePlace_OnZoneEnter(RivalEntry *entry, GameSystem *gsys, LinkFestival *festival,
                                  FestMissionConfig *config, u16 zoneId);

#endif // POKEBW2_FIELD_FEST_MISSION_SPARKLE_PLACE_H
