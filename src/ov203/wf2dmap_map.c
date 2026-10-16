#include "types.h"
#include "app/wf2dmap/wf2dmap_map.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// The grid of the Wi-Fi Club's 2D map, named after the ROM's "wf2dmap_map.c"

struct WF2DMAP_MAPSYS {
    u16 gridX;
    u16 gridY;
    WF2DMAP_MAP *buff;
};

WF2DMAP_MAPSYS *WF2DMAP_MAPSysInit(u16 gridX, u16 gridY, u32 heapId) {
    WF2DMAP_MAPSYS *sys;

    sys = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_MAPSYS), TRUE, "wf2dmap_map.c", 76);
    sys->gridX = gridX;
    sys->gridY = gridY;
    sys->buff = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_MAP) * (sys->gridX * sys->gridY), TRUE, "wf2dmap_map.c", 83);
    return sys;
}

void WF2DMAP_MAPSysExit(WF2DMAP_MAPSYS *sys) {
    GFL_HeapFree(sys->buff);
    GFL_HeapFree(sys);
}

u16 WF2DMAP_MAPSysGridXGet(const WF2DMAP_MAPSYS *sys) {
    return sys->gridX;
}

u16 WF2DMAP_MAPSysGridYGet(const WF2DMAP_MAPSYS *sys) {
    return sys->gridY;
}

void WF2DMAP_MAPSysDataSet(WF2DMAP_MAPSYS *sys, const WF2DMAP_MAP *buff) {
    MI_CpuCopy8(buff, sys->buff, sizeof(WF2DMAP_MAP) * (sys->gridX * sys->gridY));
}

WF2DMAP_MAP WF2DMAP_MAPSysDataGet(const WF2DMAP_MAPSYS *sys, u16 x, u16 y) {
    WF2DMAP_MAP map = { WF2DMAP_MAPDATA_NONE };

    if (sys->gridX > x && sys->gridY > y) {
        map = sys->buff[sys->gridX * y + x];
    }
    return map;
}

BOOL WF2DMAP_MAPSysHitGet(const WF2DMAP_MAPSYS *sys, u16 x, u16 y) {
    WF2DMAP_MAP map;

    map = WF2DMAP_MAPSysDataGet(sys, x, y);
    if (map.data == WF2DMAP_MAPDATA_NONE) {
        return TRUE;
    }
    return map.val.hit;
}

u32 WF2DMAP_MAPSysParamGet(const WF2DMAP_MAPSYS *sys, u16 x, u16 y) {
    WF2DMAP_MAP map;

    map = WF2DMAP_MAPSysDataGet(sys, x, y);
    if (map.data == WF2DMAP_MAPDATA_NONE) {
        return 0;
    }
    return map.val.param;
}
