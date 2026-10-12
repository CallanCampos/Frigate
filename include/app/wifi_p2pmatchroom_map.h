#ifndef POKEBW2_APP_WIFI_P2PMATCHROOM_MAP_H
#define POKEBW2_APP_WIFI_P2PMATCHROOM_MAP_H

// The maps of the Wi-Fi Club match board's room (overlay 173), one per number of blocks of friends. The ROM has no
// name for the file: it follows the older games' wifi_p2pmatchroom_map.c

#include "types.h"
#include "app/wf2dmap/wf2dmap_map.h"

// How many maps there are: mapNo is the number of blocks of friends, minus 1
#define WIFI_MCR_MAP_NUM 4

// A map's size in squares
typedef struct {
    u16 x;
    u16 y;
} WIFI_MCR_MAPSIZE;

WIFI_MCR_MAPSIZE WIFI_MCR_MapSizeGet(u32 mapNo);
const WF2DMAP_MAP *WIFI_MCR_MapDataGet(u32 mapNo);

#endif // POKEBW2_APP_WIFI_P2PMATCHROOM_MAP_H
