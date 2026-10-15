#include "types.h"
#include "app/wf2dmap/wf2dmap_map.h"
#include "app/wifi_p2pmatchroom.h"
#include "app/wifi_p2pmatchroom_map.h"

// The maps of the Wi-Fi Club match board's room: one more block of eight seats and a PC for every eight friends of the
// friend list. The ROM has no name for this file: it is named after the older games' wifi_p2pmatchroom_map.c

#define WIFI_MCR_MAP_GRID_Y 11

#define WIFI_MCR_MAPDATA(hit, param) { ((param) << 1) | (hit) }

// Floor, wall, a PC, the squares that name a block, a block's seats and the exit
#define FL WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_NONE)
#define WL WIFI_MCR_MAPDATA(TRUE, WIFI_MCR_MAPPM_WALL)
#define PC WIFI_MCR_MAPDATA(TRUE, WIFI_MCR_MAPPM_PC)
#define B0 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_BLOCK0)
#define B1 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_BLOCK1)
#define B2 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_BLOCK2)
#define B3 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_BLOCK3)
#define F0 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND0)
#define F1 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND1)
#define F2 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND2)
#define F3 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND3)
#define F4 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND4)
#define F5 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND5)
#define F6 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND6)
#define F7 WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_FRIEND7)
#define EX WIFI_MCR_MAPDATA(FALSE, WIFI_MCR_MAPPM_EXIT)

static const WIFI_MCR_MAPSIZE sWifiMcrMapSizes[WIFI_MCR_MAP_NUM] = {
    { 16, WIFI_MCR_MAP_GRID_Y },
    { 16, WIFI_MCR_MAP_GRID_Y },
    { 22, WIFI_MCR_MAP_GRID_Y },
    { 28, WIFI_MCR_MAP_GRID_Y },
};

// The second map is defined ahead of the first, which lays the two out in the order of the ROM
// clang-format off
static const WF2DMAP_MAP sWifiMcrMap1[16 * WIFI_MCR_MAP_GRID_Y] = {
    WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL,
    WL, WL, WL, WL, PC, WL, WL, WL, WL, WL, PC, WL, WL, WL, WL, WL,
    WL, FL, F0, FL, FL, FL, F4, FL, F0, FL, FL, FL, F4, FL, WL, WL,
    FL, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, FL, WL,
    WL, FL, F1, FL, FL, FL, F5, FL, F1, FL, FL, FL, F5, FL, WL, WL,
    WL, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, WL, WL,
    WL, FL, F2, FL, FL, FL, F6, FL, F2, FL, FL, FL, F6, FL, WL, WL,
    WL, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, WL, WL,
    WL, FL, F3, FL, FL, FL, F7, FL, F3, FL, FL, FL, F7, FL, WL, WL,
    FL, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, FL, WL,
    WL, FL, FL, FL, FL, FL, FL, EX, FL, FL, FL, FL, FL, FL, WL, WL,
};

// clang-format on

// clang-format off
static const WF2DMAP_MAP sWifiMcrMap0[16 * WIFI_MCR_MAP_GRID_Y] = {
    WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL,
    WL, WL, WL, WL, WL, WL, WL, PC, WL, WL, WL, WL, WL, WL, WL, WL,
    WL, FL, FL, FL, FL, F0, FL, FL, FL, F4, FL, FL, FL, FL, WL, WL,
    FL, FL, FL, FL, FL, B0, B0, B0, B0, B0, FL, FL, FL, FL, FL, WL,
    WL, FL, FL, FL, FL, F1, FL, FL, FL, F5, FL, FL, FL, FL, WL, WL,
    WL, FL, FL, FL, FL, B0, B0, B0, B0, B0, FL, FL, FL, FL, WL, WL,
    WL, FL, FL, FL, FL, F2, FL, FL, FL, F6, FL, FL, FL, FL, WL, WL,
    WL, FL, FL, FL, FL, B0, B0, B0, B0, B0, FL, FL, FL, FL, WL, WL,
    WL, FL, FL, FL, FL, F3, FL, FL, FL, F7, FL, FL, FL, FL, WL, WL,
    FL, FL, FL, FL, FL, B0, B0, B0, B0, B0, FL, FL, FL, FL, FL, WL,
    WL, FL, FL, FL, FL, FL, FL, EX, FL, FL, FL, FL, FL, FL, WL, WL,
};

// clang-format on

// clang-format off
static const WF2DMAP_MAP sWifiMcrMap2[22 * WIFI_MCR_MAP_GRID_Y] = {
    WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL,
    WL, WL, WL, WL, PC, WL, WL, WL, WL, WL, PC, WL, WL, WL, WL, WL, PC, WL, WL, WL, WL, WL,
    WL, FL, F0, FL, FL, FL, F4, FL, F0, FL, FL, FL, F4, FL, F0, FL, FL, FL, F4, FL, WL, WL,
    FL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, FL, WL,
    WL, FL, F1, FL, FL, FL, F5, FL, F1, FL, FL, FL, F5, FL, F1, FL, FL, FL, F5, FL, WL, WL,
    WL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, WL, WL,
    WL, FL, F2, FL, FL, FL, F6, FL, F2, FL, FL, FL, F6, FL, F2, FL, FL, FL, F6, FL, WL, WL,
    WL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, WL, WL,
    WL, FL, F3, FL, FL, FL, F7, FL, F3, FL, FL, FL, F7, FL, F3, FL, FL, FL, F7, FL, WL, WL,
    FL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, FL, WL,
    WL, FL, FL, FL, FL, FL, FL, FL, FL, FL, EX, FL, FL, FL, FL, FL, FL, FL, FL, FL, WL, WL,
};

// clang-format on

static const WF2DMAP_MAP sWifiMcrMap3[28 * WIFI_MCR_MAP_GRID_Y] = {
    WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL, WL,
    WL, WL, WL, WL, PC, WL, WL, WL, WL, WL, PC, WL, WL, WL, WL, WL, PC, WL, WL, WL, WL, WL, PC, WL, WL, WL, WL, WL,
    WL, FL, F0, FL, FL, FL, F4, FL, F0, FL, FL, FL, F4, FL, F0, FL, FL, FL, F4, FL, F0, FL, FL, FL, F4, FL, WL, WL,
    FL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, B3, B3, B3, B3, B3, FL, FL, WL,
    WL, FL, F1, FL, FL, FL, F5, FL, F1, FL, FL, FL, F5, FL, F1, FL, FL, FL, F5, FL, F1, FL, FL, FL, F5, FL, WL, WL,
    WL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, B3, B3, B3, B3, B3, FL, WL, WL,
    WL, FL, F2, FL, FL, FL, F6, FL, F2, FL, FL, FL, F6, FL, F2, FL, FL, FL, F6, FL, F2, FL, FL, FL, F6, FL, WL, WL,
    WL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, B3, B3, B3, B3, B3, FL, WL, WL,
    WL, FL, F3, FL, FL, FL, F7, FL, F3, FL, FL, FL, F7, FL, F3, FL, FL, FL, F7, FL, F3, FL, FL, FL, F7, FL, WL, WL,
    FL, FL, B2, B2, B2, B2, B2, FL, B0, B0, B0, B0, B0, FL, B1, B1, B1, B1, B1, FL, B3, B3, B3, B3, B3, FL, FL, WL,
    WL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, EX, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, FL, WL, WL,
};

static const WF2DMAP_MAP *sWifiMcrMaps[WIFI_MCR_MAP_NUM] = {
    sWifiMcrMap0,
    sWifiMcrMap1,
    sWifiMcrMap2,
    sWifiMcrMap3,
};

WIFI_MCR_MAPSIZE WIFI_MCR_MapSizeGet(u32 mapNo) {
    return sWifiMcrMapSizes[mapNo];
}

const WF2DMAP_MAP *WIFI_MCR_MapDataGet(u32 mapNo) {
    return sWifiMcrMaps[mapNo];
}
