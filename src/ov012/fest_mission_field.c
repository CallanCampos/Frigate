// The Funfest Missions on the field: the zones the missions use in place of others, the random numbers that pick
// their targets, and what a mission does when the player enters a zone. The name is descriptive
#include "types.h"
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

static const FestMissionZone data_ov012_0216dc68[10] = {
    { 0x146, 0x227 }, { 0x213, 0x219 }, { 0x214, 0x21a }, { 0x215, 0x21b }, { 0x216, 0x21c },
    { 0x217, 0x21d }, { 0x218, 0x21e }, { 0x24b, 0x252 }, { 0x24d, 0x253 }, { 0x0, 0x1a8 },
};

static BOOL ListContains(u32 value, const u32 *list, int count);

u16 FestMission_GetMissionZone(u16 zoneId) {
    u32 i;

    for (i = 0; i < NELEMS(data_ov012_0216dc68); i++) {
        if (zoneId == data_ov012_0216dc68[i].zoneId) {
            return data_ov012_0216dc68[i].missionZoneId;
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
    RivalEntry *entry = LinkFestival_GetEntries(festival);
    FestMissionConfig *config;

    OS_GetTick();
    if (getStatusOfFesMission(festival) == 0) {
        return;
    }
    config = GetFestMissionCfg(festival);
    switch (config->header.kind) {
    case 4:
        FestSparklePlace_OnZoneEnter(entry, gsys, festival, config, zoneId);
        break;
    case 5:
        FesMissionPeople_OnZoneEnter(entry, gsys, festival, config, zoneId);
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
