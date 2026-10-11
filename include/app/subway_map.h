#ifndef POKEBW2_APP_SUBWAY_MAP_H
#define POKEBW2_APP_SUBWAY_MAP_H

// The subway map. The ROM names subway_map_graphic.c; subway_map.c, the proc file before it, is a guess

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The subway map, overlay 317 (subway_map.c and subway_map_graphic.c): a map of the Unova subway on the screens, in the
// C-Gear's colors for the player's gender, that A, B or a touch closes
#define OVERLAY_SUBWAY_MAP OVERLAY_ID(317)

typedef struct {
    PlayerInfo *playerInfo;
} SubwayMapParam;

typedef struct SubwayMapGraphic SubwayMapGraphic;

extern const GameProcFunctions SUBWAY_MAP_PROC_FUNCTIONS;

// subway_map_graphic.c
SubwayMapGraphic *SubwayMapGraphic_Create(u32 layout, HeapID heapId);
void SubwayMapGraphic_Free(SubwayMapGraphic *graphic);
void SubwayMapGraphic_Update(SubwayMapGraphic *graphic);

#endif // POKEBW2_APP_SUBWAY_MAP_H
