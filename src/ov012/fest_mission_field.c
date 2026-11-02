// The Funfest Missions on the field: the zones the missions use in place of others, the random numbers that pick
// their targets, and what a mission does when the player enters a zone. The name is descriptive
#include "types.h"
#include "constants/zones.h"
#include "field/fest_mission_data.h"
#include "field/fest_mission_people.h"
#include "field/fest_mission_sparkle_place.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/rival_select.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "system/game_system.h"

typedef struct {
    u16 zoneId;
    u16 missionZoneId;
} FestMissionZone;

static const FestMissionZone FEST_MISSION_ZONES[10] = {
    { ZONE_ROUTE_4, ZONE_ROUTE_4_16 },
    { ZONE_REVERSAL_MOUNTAIN_2, ZONE_REVERSAL_MOUNTAIN_8 },
    { ZONE_REVERSAL_MOUNTAIN_3, ZONE_REVERSAL_MOUNTAIN_9 },
    { ZONE_REVERSAL_MOUNTAIN_4, ZONE_REVERSAL_MOUNTAIN_10 },
    { ZONE_REVERSAL_MOUNTAIN_5, ZONE_REVERSAL_MOUNTAIN_11 },
    { ZONE_REVERSAL_MOUNTAIN_6, ZONE_REVERSAL_MOUNTAIN_12 },
    { ZONE_REVERSAL_MOUNTAIN_7, ZONE_REVERSAL_MOUNTAIN_13 },
    { ZONE_VICTORY_ROAD_17, ZONE_VICTORY_ROAD_24 },
    { ZONE_VICTORY_ROAD_19, ZONE_VICTORY_ROAD_25 },
    { ZONE_BLACK_CITY, ZONE_WHITE_FOREST },
};

static BOOL ListContains(u32 value, const u32 *list, int count);

u16 FestMission_GetMissionZone(u16 zoneId) {
    u32 i;

    for (i = 0; i < NELEMS(FEST_MISSION_ZONES); i++) {
        if (zoneId == FEST_MISSION_ZONES[i].zoneId) {
            return FEST_MISSION_ZONES[i].missionZoneId;
        }
    }
    return zoneId;
}

void FestMission_InitRand(u32 seed, int skip, MATHRandContext32 *rand) {
    int i;
    u8 count;

    MATH_InitRand32(rand, ((u64)seed << 32) | seed);
    count = skip % 256;
    for (i = 0; i < count; i++) {
        rand->x = rand->mul * rand->x + rand->add;
    }
}

// A random number below max that the list doesn't have
u32 FestMission_RandExcluding(MATHRandContext32 *rand, u32 max, const u32 *list, int count) {
    u32 value;

    do {
        rand->x = rand->mul * rand->x + rand->add;
        value = ((rand->x >> 32) * max) >> 32;
    } while (ListContains(value, list, count));
    return value;
}

void FestMission_OnZoneEnter(GameSystem *gsys, u16 zoneId) {
    LinkFestival *festival = GSYS_GetLinkFestival(gsys);
    FestMissionWork *work = LinkFestival_GetMissionWork(festival);
    FestMissionConfig *config;

    OS_GetTick();
    if (getStatusOfFesMission(festival) == 0) {
        return;
    }
    config = GetFestMissionCfg(festival);
    switch (config->header.kind) {
    case MISSION_SPARKLE:
        FestSparklePlace_OnZoneEnter(&work->sparkle, gsys, festival, config, zoneId);
        break;
    case MISSION_PEOPLE:
        FesMissionPeople_OnZoneEnter(&work->people, gsys, festival, config, zoneId);
        break;
    }
}

static BOOL ListContains(u32 value, const u32 *list, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (value == list[i]) {
            return TRUE;
        }
    }
    return FALSE;
}
