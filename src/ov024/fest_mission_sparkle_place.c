#include "types.h"
#include "field/event_data.h"
#include "field/fest_mission_data.h"
#include "field/fest_mission_people.h"
#include "field/fest_mission_sparkle.h"
#include "field/fest_mission_sparkle_place.h"
#include "field/festival.h"
#include "field/zone.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "system/game_data.h"
#include "system/game_system.h"

// A Pokémon a sparkle can give and the weight it has, out of 100
typedef struct {
    u16 species;
    u16 weight;
} FestSparkleSpecies;

static BOOL FestSparklePlace_PickPlaces(GameSystem *gsys, FestSparkleWork *work, FestMission *mission, u16 zoneId,
                                        MATHRandContext32 *rand);
static u16 FestSparklePlace_PickSpecies(FestSparkleWork *work, FestMission *mission, MATHRandContext32 *rand);
static BOOL FestSparklePlace_IsLastZoneUsed(FestSparkleWork *work);
static void FestSparklePlace_AddZone(FestSparkleWork *work, u16 zoneId);

// How many sparkles a zone has, by the mission's setting for the zone's stage and a random roll
static const u8 sSparkleCounts[7][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 1, 2 }, { 0, 0, 0, 1, 1, 1, 2, 2 }, { 1, 2, 2, 3, 3, 3, 4, 4 },
    { 4, 4, 5, 5, 5, 6, 7, 8 }, { 6, 6, 7, 7, 7, 7, 8, 8 }, { 0, 0, 0, 0, 0, 0, 8, 8 },
};

// What the sparkles give, by the mission's target
static const FestSparkleSpecies sSpecies[17][6] = {
    { { 72, 25 }, { 73, 25 }, { 75, 20 }, { 74, 20 }, { 75, 5 }, { 74, 5 } },
    { { 86, 20 }, { 88, 30 }, { 90, 20 }, { 87, 15 }, { 89, 10 }, { 92, 5 } },
    { { 4, 20 }, { 22, 22 }, { 18, 22 }, { 78, 20 }, { 30, 10 }, { 89, 6 } },
    { { 78, 25 }, { 26, 25 }, { 77, 25 }, { 572, 10 }, { 323, 10 }, { 324, 5 } },
    { { 78, 25 }, { 25, 25 }, { 76, 25 }, { 573, 10 }, { 322, 10 }, { 252, 5 } },
    { { 151, 20 }, { 149, 20 }, { 150, 15 }, { 155, 25 }, { 152, 15 }, { 158, 5 } },
    { { 156, 20 }, { 153, 20 }, { 158, 20 }, { 154, 15 }, { 157, 5 }, { 155, 20 } },
    { { 28, 25 }, { 73, 25 }, { 72, 25 }, { 84, 10 }, { 82, 10 }, { 108, 5 } },
    { { 28, 25 }, { 74, 25 }, { 75, 25 }, { 83, 10 }, { 85, 10 }, { 107, 5 } },
    { { 86, 20 }, { 78, 30 }, { 229, 20 }, { 90, 15 }, { 88, 10 }, { 89, 5 } },
    { { 95, 25 }, { 96, 25 }, { 97, 20 }, { 97, 5 }, { 98, 20 }, { 98, 5 } },
    { { 280, 20 }, { 280, 20 }, { 280, 20 }, { 280, 20 }, { 93, 10 }, { 93, 10 } },
    { { 86, 30 }, { 90, 20 }, { 89, 20 }, { 88, 20 }, { 586, 8 }, { 587, 2 } },
    { { 88, 40 }, { 90, 40 }, { 584, 14 }, { 585, 3 }, { 586, 2 }, { 588, 1 } },
    { { 157, 30 }, { 201, 14 }, { 202, 14 }, { 203, 14 }, { 204, 14 }, { 205, 14 } },
    { { 155, 20 }, { 158, 20 }, { 154, 20 }, { 150, 20 }, { 157, 10 }, { 157, 10 } },
    { { 169, 16 }, { 170, 17 }, { 171, 17 }, { 172, 17 }, { 173, 17 }, { 174, 16 } },
};

void FestSparklePlace_OnZoneEnter(RivalEntry *entry, GameSystem *gsys, LinkFestival *festival,
                                  FestMissionConfig *config, u16 zoneId) {
    FestSparkleWork *work = (FestSparkleWork *)entry;
    FestMission *mission = (FestMission *)config;
    MATHRandContext32 rand;

    work->zoneId = zoneId;
    work->unk2 = 0;
    work->count = 0;
    sys_memset(work->places, 0, sizeof(work->places));
    sys_memset(work->placeIndex, 0, sizeof(work->placeIndex));
    sys_memset(work->values, 0, sizeof(work->values));
    func_ov012_02168348(mission->unk1C, func_ov012_02168320(zoneId), &rand);
    FestSparklePlace_PickPlaces(gsys, work, mission, zoneId, &rand);
    FestSparklePlace_AddZone(work, zoneId);
}

static BOOL FestSparklePlace_PickPlaces(GameSystem *gsys, FestSparkleWork *work, FestMission *mission, u16 zoneId,
                                        MATHRandContext32 *rand) {
    int count;
    u32 picked[8];
    int i;
    ZoneBGEntity *entity;
    int level;
    u8 pattern;

    entity = GetZoneProxiesAndCount(GameData_GetEventData(GSYS_GetGameData(gsys)), &count);
    if (entity == NULL) {
        return FALSE;
    }
    work->unk2 = 0;
    for (i = 0; i < count; i++, entity++) {
        if (entity->condition == 4) {
            work->places[work->unk2++] = *entity;
            if (work->unk2 >= 20) {
                break;
            }
        }
    }
    if (work->unk2 == 0) {
        return FALSE;
    }
    level = GetZoneEncIDHighest3Bits(work->zoneId);
    if (level >= 6) {
        work->count = 0;
        return FALSE;
    }
    pattern = (mission->unk8_0 >> (level * 3)) & 7;
    work->count = sSparkleCounts[pattern][func_ov012_021683a8(rand, 8, NULL, 0)];
    if (work->count > work->unk2) {
        work->count = work->unk2;
    }
    if (work->count == 0) {
        return FALSE;
    }
    for (i = 0; i < work->count; i++) {
        work->placeIndex[i] = func_ov012_021683a8(rand, work->unk2, picked, i);
        work->values[i] = FestSparklePlace_PickSpecies(work, mission, rand);
        picked[i] = work->placeIndex[i];
    }
    return TRUE;
}

static u16 FestSparklePlace_PickSpecies(FestSparkleWork *work, FestMission *mission, MATHRandContext32 *rand) {
    const FestSparkleSpecies *set;
    int i;
    u32 total = 0;
    u32 roll;

    if (mission->target >= 17) {
        set = sSpecies[0];
    } else {
        set = sSpecies[mission->target];
    }
    roll = func_ov012_021683a8(rand, 100000, NULL, 0);
    for (i = 0; i < 6; i++) {
        total += set[i].weight * 1000;
        if (roll < total) {
            return set[i].species;
        }
    }
    return set[0].species;
}

static BOOL FestSparklePlace_IsLastZoneUsed(FestSparkleWork *work) {
    int i;

    if (work->zoneCount == 0) {
        return TRUE;
    }
    for (i = 0; i < FEST_SPARKLE_MAX; i++) {
        if (work->zones[work->zoneCount - 1].found[i] != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static void FestSparklePlace_AddZone(FestSparkleWork *work, u16 zoneId) {
    int index;
    int i;

    if (work->unk2 == 0) {
        return;
    }
    for (i = 0; i < work->zoneCount; i++) {
        if (zoneId == work->zones[i].zoneId) {
            return;
        }
    }
    if (FestSparklePlace_IsLastZoneUsed(work)) {
        index = work->zoneCount;
        if (index < 6) {
            work->zoneCount = index + 1;
        } else {
            index = 5;
            sys_memcpy(&work->zones[1], &work->zones[0], sizeof(work->zones) - sizeof(work->zones[0]));
        }
    } else {
        index = 0;
        if (work->zoneCount != 0) {
            index = work->zoneCount - 1;
        }
    }
    sys_memset(&work->zones[index], 0, sizeof(FestSparkleZone));
    work->zones[index].zoneId = zoneId;
    work->zones[index].unk2 = work->count;
}
