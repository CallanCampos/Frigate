// The town map's table of places, archive 85, which townmap.c reads and overlays 298 (the Pokédex's habitat map), 303
// and 308 (a beacon's details) load too. The ROM doesn't name the file; the name is a guess. It is its own file after
// townmap_grh.c: no call crosses 0x0219f718, townmap_grh.c never calls these functions, and they are the only ones
// other overlays call

#include "app/townmap/townmap_data.h"
#include "types.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"

#define ARCID_TOWNMAP_DATA 85

void *TownMapData_Load(HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(ARCID_TOWNMAP_DATA, 0, FALSE, heapId);
}

void TownMapData_Free(void *data) {
    GFL_HeapFree(data);
}

u16 TownMapData_GetParam(void *data, u16 place, u32 param) {
    u16(*places)[TOWNMAP_PARAM_COUNT] = data;

    return places[place][param];
}

u16 TownMapData_GetPlaceByZone(void *data, u16 zone) {
    int i;

    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        if (TownMapData_GetParam(data, i, TOWNMAP_PARAM_ZONE) == zone) {
            return i;
        }
    }
    return TOWNMAP_PLACE_NONE;
}
