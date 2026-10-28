// The Funfest's field gimmick: loads the overlay of the festival's current mission when the field starts and hands
// the mission its calls. The name is the ROM's own, from GFL_HeapAllocate's file argument. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/fest_mission_data.h"
#include "field/fest_mission_gimmick.h"
#include "field/fest_mission_hollow.h"
#include "field/fest_mission_people.h"
#include "field/fest_mission_sparkle.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_player.h"
#include "field/rival_select.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "save/event_work.h"
#include "system/fest_mission.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The event flags that the first kind of mission clears when it ends
#define FIRST_MISSION_FLAG 0x3a1
#define MISSION_FLAG_COUNT 12

static void FesGimmick_SetEnabled(FesGimmick *gimmick, BOOL enabled);
static void ClearFesEventFlags(FesGimmick *gimmick);

FesGimmick *FesGimmick_Create(GameSystem *gsys, Field *field, HeapID heapId) {
    FesGimmick *gimmick = GFL_HeapAllocate(heapId, sizeof(FesGimmick), TRUE, "fes_gimmick.c", 74);
    NoGridMapper *mapper;

    gimmick->gsys = gsys;
    gimmick->gameData = GSYS_GetGameData(gsys);
    gimmick->festival = GSYS_GetLinkFestival(gsys);
    gimmick->mission = GetFestMissionCfg(gimmick->festival);
    gimmick->work = LinkFestival_GetMissionWork(gimmick->festival);
    gimmick->field = field;
    gimmick->eventData = GameData_GetEventData(gimmick->gameData);
    gimmick->eventWork = GameData_GetEventWork(gimmick->gameData);
    mapper = Field_GetNoGridMapper(field);
    if (mapper != NULL) {
        gimmick->railSystem = FieldNoGridMapper_GetRailSystem(mapper);
    }
    FesGimmick_SetEnabled(gimmick, TRUE);
    return gimmick;
}

void FesGimmick_Free(FesGimmick *gimmick) {
    FesGimmick_SetEnabled(gimmick, FALSE);
    MI_CpuFill8(gimmick, 0, sizeof(FesGimmick));
    GFL_HeapFree(gimmick);
}

static void FesGimmick_SetEnabled(FesGimmick *gimmick, BOOL enabled) {
    if (enabled) {
        u32 overlayId;

        if (getStatusOfFesMission(gimmick->festival)) {
            switch (gimmick->mission->header.kind) {
            case MISSION_SPARKLE:
                overlayId = OVERLAY_ID(71);
                break;
            case MISSION_PEOPLE:
                overlayId = OVERLAY_ID(72);
                break;
            case MISSION_HOLLOW:
                overlayId = OVERLAY_ID(73);
                break;
            default:
                return;
            }
            gimmick->overlayLoaded = TRUE;
            gimmick->overlayId = overlayId;
            GFL_OvlLoad(overlayId);
        }
    } else if (gimmick->overlayLoaded) {
        GFL_OvlUnload(gimmick->overlayId);
        gimmick->overlayLoaded = FALSE;
    }
}

void FesGimmick_BindActorSystem(FesGimmick *fes, MMSys *actorSystem) {
    fes->actors = actorSystem;
    if (getStatusOfFesMission(fes->festival)) {
        if (fes->mission->header.kind == MISSION_PEOPLE) {
            FesMissionGimmick_LoadActorResource(fes, &fes->work->people);
        }
    }
}

void FesGimmick_BindPlayer(FesGimmick *fes, FieldEffects *effects, FieldPlayer *player) {
    fes->effects = effects;
    fes->player = player;
    fes->playerActor = FieldPlayer_GetActor(player);
    fes->sparkles = FldeffFesSparkle_Start(fes->effects);
}

void FesGimmick_DeleteSparkles(FesGimmick *fes) {
    if (fes->sparkles != NULL) {
        FieldEffects_DeleteObject(fes->sparkles);
        fes->sparkles = NULL;
    }
}

void FesGimmick_StartMission(FesGimmick *fes) {
    if (getStatusOfFesMission(fes->festival)) {
        MI_CpuFill8(&LinkFestival_GetMissionWork(fes->festival)->people, 0, sizeof(FesMissionWork));
        switch (fes->mission->header.kind) {
        case MISSION_FLAGS:
            ClearFesEventFlags(fes);
            break;
        case MISSION_HOLLOW:
            if (FesMissionHollow_IsActive(fes)) {
                FesMissionHollow_Setup(fes);
            }
            break;
        case MISSION_PEOPLE:
            FesMissionPeople_LoadSpecies(fes, &fes->work->people);
            break;
        }
    }
}

static void ClearFesEventFlags(FesGimmick *gimmick) {
    int flag;

    for (flag = FIRST_MISSION_FLAG; flag <= FIRST_MISSION_FLAG + MISSION_FLAG_COUNT - 1; flag++) {
        EventWork_FlagReset(gimmick->eventWork, flag);
    }
}

void FesGimmick_OnZoneEnter(FesGimmick *fes) {
    if (getStatusOfFesMission(fes->festival)) {
        switch (fes->mission->header.kind) {
        case MISSION_SPARKLE:
            FesMissionGimmick_LoadActorResource(fes, &fes->work->people);
            break;
        case MISSION_PEOPLE:
            FesMissionGimmick_RemoveBlockedPeople(fes, &fes->work->people);
            break;
        }
    }
}

BOOL FesGimmick_EndMission(FesGimmick *fes) {
    FestMissionWork *work = LinkFestival_GetMissionWork(fes->festival);

    if (!getStatusOfFesMission(fes->festival)) {
        return FALSE;
    }
    switch (fes->mission->header.kind) {
    case MISSION_SPARKLE:
        return FestSparkle_End(fes, &work->sparkle);
    case MISSION_PEOPLE:
        return FesMissionGimmick_End(fes, &work->people);
    case MISSION_HOLLOW:
        return RivalSelect_RemoveHollowActorIfMission(fes);
    default:
        return FALSE;
    }
}

BOOL FesGimmick_IsCurrent(FesGimmick *fes, u32 type) {
    if (getStatusOfFesMission(fes->festival)) {
        if (fes->mission->header.kind == type) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

GameEvent *FesGimmick_CheckEventGrid(FesGimmick *fes, const VecFx32 *position) {
    if (!getStatusOfFesMission(fes->festival)) {
        return NULL;
    }
    if (fes->mission->header.kind == MISSION_SPARKLE) {
        return FestSparkle_CheckPickUpGrid(fes, position);
    }
    return NULL;
}

GameEvent *FesGimmick_CheckEventRail(FesGimmick *fes, const RailPosition *position) {
    if (!getStatusOfFesMission(fes->festival)) {
        return NULL;
    }
    if (fes->mission->header.kind == MISSION_SPARKLE) {
        return FestSparkle_CheckPickUpRail(fes, position);
    }
    return NULL;
}

BOOL FesGimmick_IsSparkleOnTile(FesGimmick *fes, u16 x, u16 z, fx32 height) {
    if (!getStatusOfFesMission(fes->festival)) {
        return FALSE;
    }
    if (fes->mission->header.kind == MISSION_SPARKLE) {
        return FestSparkle_IsOnTile(fes, x, z, height);
    }
    return FALSE;
}

void DeleteFunfestActor(FesGimmick *fes, u16 zoneId, u8 actorIndex) {
    FestMissionWork *work = LinkFestival_GetMissionWork(fes->festival);

    if (getStatusOfFesMission(fes->festival)) {
        if (fes->mission->header.kind == MISSION_PEOPLE) {
            FesMissionGimmick_RemovePerson(fes, &work->people, zoneId, actorIndex);
        }
    }
}

u32 FesGimmick_IsOnSale(FesGimmick *fes) {
    if (FesGimmick_IsCurrent(fes, MISSION_PEOPLE)) {
        return FesMissionPeople_IsOnSale(fes->mission);
    }
    return 0;
}

void FesGimmick_BoostPartyLevels(FesGimmick *gimmick, PokeParty *party) {
    if (FesGimmick_IsCurrent(gimmick, MISSION_PEOPLE)) {
        FesMissionGimmick_BoostPartyLevels(gimmick, party);
    }
}

BOOL hollowFesMissions(FesGimmick *gimmick, u32 id) {
    if (FesGimmick_IsCurrent(gimmick, MISSION_HOLLOW) && FesMissionHollow_IsActive(gimmick) &&
        RivalSelect_FindEntryIndex(gimmick, id) != RIVAL_ENTRY_NONE) {
        return TRUE;
    }
    return FALSE;
}
