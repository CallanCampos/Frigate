#ifndef POKEBW2_APP_WF2DMAP_WF2DMAP_MAP_H
#define POKEBW2_APP_WF2DMAP_WF2DMAP_MAP_H

// The grid of the Wi-Fi Club's 2D map (overlay 203), named after the ROM's "wf2dmap_map.c"

#include "types.h"

// What a square without data reads as: a wall
#define WF2DMAP_MAPDATA_NONE 0xffffffff

// A square of the map
typedef union {
    u32 data;
    struct {
        // Whether objects cannot enter it
        u32 hit : 1;
        u32 param : 31;
    } val;
} WF2DMAP_MAP;

typedef struct WF2DMAP_MAPSYS WF2DMAP_MAPSYS;

WF2DMAP_MAPSYS *WF2DMAP_MAPSysInit(u16 gridX, u16 gridY, u32 heapId);
void WF2DMAP_MAPSysExit(WF2DMAP_MAPSYS *sys);
u16 WF2DMAP_MAPSysGridXGet(const WF2DMAP_MAPSYS *sys);
u16 WF2DMAP_MAPSysGridYGet(const WF2DMAP_MAPSYS *sys);
// Copies gridX * gridY squares
void WF2DMAP_MAPSysDataSet(WF2DMAP_MAPSYS *sys, const WF2DMAP_MAP *buff);
WF2DMAP_MAP WF2DMAP_MAPSysDataGet(const WF2DMAP_MAPSYS *sys, u16 x, u16 y);
BOOL WF2DMAP_MAPSysHitGet(const WF2DMAP_MAPSYS *sys, u16 x, u16 y);
u32 WF2DMAP_MAPSysParamGet(const WF2DMAP_MAPSYS *sys, u16 x, u16 y);

#endif // POKEBW2_APP_WF2DMAP_WF2DMAP_MAP_H
