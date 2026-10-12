#include "types.h"
#include "constants/directions.h"
#include "constants/zones.h"
#include "field/zone.h"
#include "nitro/fx.h"

// The spawn info of a zone change: where the player arrives, by warp or by position, on the grid or on a rail.
// The file's name is a guess, from what it does: the ROM has no string for it. The functions' names are swan's

static u16 ConvWarpDirToDir(s16 warpDir);

void InitZoneSpawnInfo(ZoneSpawnInfo *spawn) {
    spawn->changeType = 0;
    spawn->zoneId = 0;
    spawn->warpId = 0;
    spawn->warpDir = 0;
    spawn->posWeightBits = 0;
    spawn->isRail = FALSE;
    spawn->pos.vec.x = 0;
    spawn->pos.vec.y = 0;
    spawn->pos.vec.z = 0;
}

void SetupZoneWarpArrivalGrid(ZoneSpawnInfo *spawn, u16 zoneId, s16 warpId, s16 warpDir, u16 posWeightBits, s32 x,
                              s32 y, s32 z) {
    spawn->changeType = 0;
    spawn->zoneId = zoneId;
    spawn->warpId = warpId;
    spawn->warpDir = warpDir;
    spawn->posWeightBits = posWeightBits;
    spawn->isRail = FALSE;
    VEC_Set(&spawn->pos.vec, x, y, z);
}

void SetupZoneWarpArrivalRail(ZoneSpawnInfo *spawn, u16 zoneId, s16 warpId, s16 warpDir, u16 posWeightBits,
                              u16 componentId, u16 posFront, s16 posSide) {
    spawn->changeType = 0;
    spawn->zoneId = zoneId;
    spawn->warpId = warpId;
    spawn->warpDir = warpDir;
    spawn->posWeightBits = posWeightBits;
    spawn->isRail = TRUE;
    spawn->pos.rail.componentIsLine = TRUE;
    spawn->pos.rail.componentId = componentId;
    spawn->pos.rail.posFront = posFront;
    spawn->pos.rail.posSide = posSide;
    spawn->pos.rail.railDirection = ConvDirToRailDir(ConvWarpDirToDir(warpDir));
}

void SetupZoneSpawnInfoWarp(ZoneSpawnInfo *spawn, u16 zoneId, u16 warpId, u32 posWeightBits) {
    InitZoneSpawnInfo(spawn);
    spawn->zoneId = zoneId;
    spawn->warpId = warpId;
    spawn->posWeightBits = posWeightBits;
}

void CreateZoneChangeData(ZoneSpawnInfo *spawn, u32 zoneId, s16 warpDir, s32 x, s32 y, s32 z) {
    spawn->changeType = ZONE_SPAWN_CHANGE_TYPE_POSITION;
    spawn->zoneId = zoneId;
    spawn->warpId = -1;
    spawn->warpDir = warpDir;
    spawn->posWeightBits = 0;
    spawn->isRail = FALSE;
    VEC_Set(&spawn->pos.vec, x, y, z);
}

void CreateZoneChangeDataRail(ZoneSpawnInfo *spawn, u16 zoneId, s16 warpDir, u16 componentId, u16 posFront,
                              s16 posSide) {
    spawn->changeType = ZONE_SPAWN_CHANGE_TYPE_POSITION;
    spawn->zoneId = zoneId;
    spawn->warpId = -1;
    spawn->warpDir = warpDir;
    spawn->posWeightBits = 0;
    spawn->isRail = TRUE;
    spawn->pos.rail.componentIsLine = TRUE;
    spawn->pos.rail.componentId = componentId;
    spawn->pos.rail.posFront = posFront;
    spawn->pos.rail.posSide = posSide;
    spawn->pos.rail.railDirection = ConvDirToRailDir(ConvWarpDirToDir(warpDir));
}

BOOL GetZoneSpawnInfoIsRail(ZoneSpawnInfo *spawn) {
    return spawn->isRail;
}

void GetPlayerZoneStateWPos(const ZoneSpawnInfo *spawn, VecFx32 *pos) {
    VEC_Set(pos, spawn->pos.vec.x, spawn->pos.vec.y, spawn->pos.vec.z);
}

void SetPlayerZoneStateWPos(ZoneSpawnInfo *spawn, const VecFx32 *pos) {
    spawn->isRail = FALSE;
    spawn->pos.vec = *pos;
}

// The spawn info of flying to a zone: its Fly position, facing down
void LoadZoneSpawnInfoCheckRail(ZoneSpawnInfo *spawn, u16 zoneId) {
    VecFx32 position;

    InitZoneSpawnInfo(spawn);
    spawn->changeType = ZONE_SPAWN_CHANGE_TYPE_POSITION;
    spawn->zoneId = zoneId;
    spawn->warpId = -1;
    spawn->warpDir = WARP_DIR_DOWN;
    spawn->posWeightBits = 0;
    if (IsZonesWithRailsArrayHasZoneID(zoneId)) {
        LoadZoneFlyPosRailCoords(zoneId, &position);
        spawn->isRail = TRUE;
        spawn->pos.rail.componentIsLine = TRUE;
        spawn->pos.rail.componentId = position.x;
        spawn->pos.rail.posFront = position.y;
        spawn->pos.rail.posSide = position.z;
    } else {
        LoadZoneFlyPosToVecFx32(zoneId, &position);
        spawn->isRail = FALSE;
        spawn->pos.vec = position;
    }
}

// Where a new game starts
void LoadAspertiaCitySpawnInfo(ZoneSpawnInfo *spawn) {
    LoadZoneSpawnInfoCheckRail(spawn, ZONE_ASPERTIA_CITY);
}

static u16 ConvWarpDirToDir(s16 warpDir) {
    switch (warpDir) {
    case WARP_DIR_UP:
        return DIR_UP;
    case WARP_DIR_DOWN:
        return DIR_DOWN;
    case WARP_DIR_LEFT:
        return DIR_LEFT;
    case WARP_DIR_RIGHT:
        return DIR_RIGHT;
    default:
        return DIR_DOWN;
    }
}

u16 ConvDirToWarpDir(u16 dir) {
    switch (dir) {
    case DIR_UP:
        return WARP_DIR_UP;
    case DIR_DOWN:
        return WARP_DIR_DOWN;
    case DIR_LEFT:
        return WARP_DIR_LEFT;
    case DIR_RIGHT:
        return WARP_DIR_RIGHT;
    default:
        return WARP_DIR_DOWN;
    }
}
