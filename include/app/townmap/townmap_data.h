#ifndef POKEBW2_APP_TOWNMAP_TOWNMAP_DATA_H
#define POKEBW2_APP_TOWNMAP_TOWNMAP_DATA_H

#include "types.h"
#include "gfl/heap.h"

// townmap_data.c: the town map's table of places, archive 85 (data/town_map/places.json), which townmap.c reads and
// overlays 298 (the Pokédex's habitat map), 303 and 308 (a beacon's details) load too. The ROM doesn't name the file;
// the name is a guess. The function names are ours

#define TOWNMAP_PLACE_COUNT 85
// What TownMapData_GetPlaceByZone returns for a zone that is no place on the map
#define TOWNMAP_PLACE_NONE 0xffff

// A place's parameters, as TownMapData_GetParam reads them
enum {
    TOWNMAP_PARAM_ZONE = 0,
    // How near the cursor must come to point at it, scaled by the zoom
    TOWNMAP_PARAM_RADIUS = 1,
    // Where it is
    TOWNMAP_PARAM_X = 2,
    TOWNMAP_PARAM_Y = 3,
    // Where the cursor points at it
    TOWNMAP_PARAM_CURSOR_X = 4,
    TOWNMAP_PARAM_CURSOR_Y = 5,
    // The capsule that a touch hits it in: a segment and its radius
    TOWNMAP_PARAM_HIT_START_X = 6,
    TOWNMAP_PARAM_HIT_START_Y = 7,
    TOWNMAP_PARAM_HIT_END_X = 8,
    TOWNMAP_PARAM_HIT_END_Y = 9,
    TOWNMAP_PARAM_HIT_RADIUS = 10,
    // TOWNMAP_PLACE_TYPE_*
    TOWNMAP_PARAM_TYPE = 11,
    // Whether the player can fly there once TOWNMAP_PARAM_ARRIVAL_FLAG is set
    TOWNMAP_PARAM_FLY = 12,
    // The event flag set when the player first arrives there, or TOWNMAP_NO_FLAG
    TOWNMAP_PARAM_ARRIVAL_FLAG = 15,
    // The event flag or TOWNMAP_FLAG_* that shows it, or TOWNMAP_NO_FLAG
    TOWNMAP_PARAM_FLAG = 16,
    // Its description and the landmarks it lists, messages of TEXT_BANK_TOWN_MAP; the landmarks end at 0xffff
    TOWNMAP_PARAM_DESCRIPTION = 17,
    TOWNMAP_PARAM_LANDMARK_FIRST = 18,
    TOWNMAP_PARAM_LANDMARK_LAST = 23,
    // Its area's animation and position on the Pokédex's map
    TOWNMAP_PARAM_AREA_ANIM = 24,
    TOWNMAP_PARAM_AREA_X = 25,
    TOWNMAP_PARAM_AREA_Y = 26,
    // Each place is this many u16s
    TOWNMAP_PARAM_COUNT
};

// The Pokédex's map gives this type its own marker; in the data it is N's Castle's, the Plasma Frigate's and the
// Abyssal Ruins', not the towns' (0)
#define TOWNMAP_PLACE_TYPE_TOWN 4
#define TOWNMAP_NO_FLAG 0xffff

// Reads the table of places, and frees it
void *TownMapData_Load(HeapID heapId);
void TownMapData_Free(void *data);
// A TOWNMAP_PARAM_* of a place
u16 TownMapData_GetParam(void *data, u16 place, u16 param);
// The place whose TOWNMAP_PARAM_ZONE is zone, or TOWNMAP_PLACE_NONE
u16 TownMapData_GetPlaceByZone(void *data, u16 zone);

#endif // POKEBW2_APP_TOWNMAP_TOWNMAP_DATA_H
