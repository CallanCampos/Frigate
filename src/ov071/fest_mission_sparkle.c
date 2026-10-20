// Overlay 71: the Funfest missions of type 4, where the player looks for sparkles hidden around the zones. Overlay
// 36's festival gimmick calls into it while such a mission runs. The file name is a guess
#include "types.h"
#include "field/fest_mission_sparkle.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_rail.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/mi.h"

#define POS_TO_GRID(pos) (((pos) >> 4) / FX32_ONE)
// How far above or below a sparkle the player still finds it
#define SPARKLE_HEIGHT_RANGE FX32_CONST(20)
// The script that gives what a sparkle hides
#define SCRID_FEST_SPARKLE 0x2a21
#define SCRID_NONE 0xffff

static GameEvent *FestSparkle_PickUp(FestSparkleGimmick *gimmick, FestSparkleWork *work, u8 idx);
static BOOL FestSparkle_ShowAll(FestSparkleGimmick *gimmick, FestSparkleWork *work);
static BOOL FestSparkle_IsBlockedGrid(FestSparkleGimmick *gimmick, FestSparkleWork *work, const VecFx32 *position);
static BOOL FestSparkle_IsBlockedRail(FestSparkleGimmick *gimmick, FestSparkleWork *work, const RailPosition *position);
static FestSparkleZone *FestSparkle_GetZone(FestSparkleWork *work, u16 zoneId);

void FestSparkle_Show(FestSparkleGimmick *gimmick, FestSparkleWork *work) {
    func_ov036_021a5c44(gimmick->sparkles);
    if (work->count != 0) {
        FestSparkle_ShowAll(gimmick, work);
    }
}

BOOL FestSparkle_End(FestSparkleGimmick *gimmick, FestSparkleWork *work) {
    BOOL result;

    if (work->unk2 != 0) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    func_ov036_021a5c44(gimmick->sparkles);
    MI_CpuClear32(work, sizeof(FestSparkleWork));
    return result;
}

GameEvent *FestSparkle_CheckPickUpGrid(FestSparkleGimmick *gimmick, const VecFx32 *position) {
    FestSparkleWork *work = gimmick->work;
    int i;
    ZoneBGEntity *place;

    for (i = 0; i < work->count; i++) {
        place = &work->places[work->placeIndex[i]];
        if (func_ov036_021a5c5c(gimmick->sparkles, i) && place->isRail == FALSE &&
            CheckBGPositionMatchGrid(place, position)) {
            return FestSparkle_PickUp(gimmick, work, i);
        }
    }
    return NULL;
}

GameEvent *FestSparkle_CheckPickUpRail(FestSparkleGimmick *gimmick, const RailPosition *position) {
    FestSparkleWork *work = gimmick->work;
    int i;
    ZoneBGEntity *place;

    Field_GetResolvedControllerTypeID(gimmick->field);
    for (i = 0; i < work->count; i++) {
        place = &work->places[work->placeIndex[i]];
        if (func_ov036_021a5c5c(gimmick->sparkles, i) && place->isRail == TRUE &&
            CheckBGPositionMatchRail(place, position)) {
            return FestSparkle_PickUp(gimmick, work, i);
        }
    }
    return NULL;
}

BOOL FestSparkle_IsOnTile(FestSparkleGimmick *gimmick, u16 x, u16 z, fx32 height) {
    int i;
    FestSparkleWork *work = gimmick->work;
    FestSparkleZone *zone;
    ZoneBGEntity *place;
    RailPosition railPos;
    VecFx32 pos;

    Field_GetResolvedControllerTypeID(gimmick->field);
    if (work->count == 0) {
        return FALSE;
    }
    zone = FestSparkle_GetZone(work, work->zoneId);
    for (i = 0; i < work->count; i++) {
        place = &work->places[work->placeIndex[i]];
        if (zone->found[i] == FALSE) {
            if (place->isRail == FALSE) {
                GetBGEntityCenterPosGrid(place, &pos);
            } else {
                GetBGEntityPosRail(place, &railPos);
                func_ov036_021b06ec(gimmick->rail, &railPos, &pos);
            }
            if (POS_TO_GRID(pos.x) == x && POS_TO_GRID(pos.z) == z && pos.y - SPARKLE_HEIGHT_RANGE <= height &&
                pos.y + SPARKLE_HEIGHT_RANGE >= height) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static GameEvent *FestSparkle_PickUp(FestSparkleGimmick *gimmick, FestSparkleWork *work, u8 idx) {
    FestSparkleZone *zone = FestSparkle_GetZone(work, work->zoneId);
    u16 value = func_ov036_021a5c74(gimmick->sparkles, idx);
    GameEvent *event;

    func_ov036_021a5c2c(gimmick->sparkles, idx);
    zone->found[idx] = TRUE;
    if (value == 0) {
        return NULL;
    }
    event = EventScriptCall_Create(gimmick->gsys, SCRID_FEST_SPARKLE, NULL, HEAPID_FIELDMAP);
    ScriptWork_SetParams(EventScriptCall_GetWork(event), value, 0, 0, 0);
    return event;
}

static BOOL FestSparkle_ShowAll(FestSparkleGimmick *gimmick, FestSparkleWork *work) {
    FestSparkleZone *zone;
    ZoneBGEntity *place;
    RailPosition railPos;
    VecFx32 pos;
    int i;

    Field_GetResolvedControllerTypeID(gimmick->field);
    zone = FestSparkle_GetZone(work, work->zoneId);
    for (i = 0; i < work->count; i++) {
        place = &work->places[work->placeIndex[i]];
        if (zone->found[i] != FALSE) {
            continue;
        }
        if (place->isRail == FALSE) {
            GetBGEntityCenterPosGrid(place, &pos);
            if (FestSparkle_IsBlockedGrid(gimmick, work, &pos)) {
                zone->found[i] = TRUE;
                continue;
            }
        } else {
            GetBGEntityPosRail(place, &railPos);
            if (FestSparkle_IsBlockedRail(gimmick, work, &railPos)) {
                zone->found[i] = TRUE;
                continue;
            }
            func_ov036_021b06ec(gimmick->rail, &railPos, &pos);
        }
        func_ov036_021a5c04(gimmick->sparkles, i, work->values[i], &pos);
    }
    return TRUE;
}

// Whether something else is on the tile: an actor other than the player, or a trigger
static BOOL FestSparkle_IsBlockedGrid(FestSparkleGimmick *gimmick, FestSparkleWork *work, const VecFx32 *position) {
    FieldActor *actor = FindActorByGPos(gimmick->actors, POS_TO_GRID(position->x), POS_TO_GRID(position->z),
                                        position->y, SPARKLE_HEIGHT_RANGE, FALSE);

    if (actor != NULL && actor != gimmick->player) {
        return TRUE;
    }
    if (GetTriggerSCRIDAtPosGrid(gimmick->eventData, gimmick->eventWork, position, 8) != SCRID_NONE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FestSparkle_IsBlockedRail(FestSparkleGimmick *gimmick, FestSparkleWork *work,
                                      const RailPosition *position) {
    FieldActor *actor = FindActorByRailPos(gimmick->actors, position, FALSE);

    if (actor != NULL && actor != gimmick->player) {
        return TRUE;
    }
    if (GetTriggerSCRIDAtPosRail(gimmick->eventData, gimmick->eventWork, position) != SCRID_NONE) {
        return TRUE;
    }
    return FALSE;
}

// The zone's record, or the first one when the zone has none
static FestSparkleZone *FestSparkle_GetZone(FestSparkleWork *work, u16 zoneId) {
    int i;

    for (i = 0; i < work->zoneCount; i++) {
        if (work->zones[i].zoneId == zoneId) {
            return &work->zones[i];
        }
    }
    return &work->zones[0];
}
