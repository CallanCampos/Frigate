#include "types.h"
#include "field/fest_mission_data.h"
#include "field/fest_mission_gimmick.h"
#include "field/fest_mission_hollow.h"
#include "field/festival.h"
#include "field/rival_select.h"
#include "gfl/std.h"
#include "nitro/math.h"

// A person of a hollow mission, by the mission's unk0_20: the object code (probably) and an unknown byte
typedef struct {
    u16 unk0;
    u8 unk2;
} FesMissionHollowPerson;

static u16 FesMissionHollow_GetPersonObjCode(FesGimmick *gimmick);
static u8 FesMissionHollow_GetPersonUnk2(FesGimmick *gimmick);
static void FesMissionHollow_Shuffle(u8 *list, int count, MATHRandContext32 *rand);

// clang-format off
static const FesMissionHollowPerson data_ov026_0216fa50[] = {
    { 1, 0 },   { 431, 0 }, { 434, 0 }, { 133, 0 }, { 136, 0 }, { 134, 0 }, { 135, 0 },
    { 196, 0 }, { 197, 0 }, { 471, 0 }, { 470, 0 }, { 276, 0 }, { 285, 0 }, { 290, 0 },
    { 316, 0 }, { 327, 0 }, { 293, 0 }, { 261, 0 }, { 263, 0 }, { 270, 0 }, { 296, 0 },
    { 148, 0 }, { 82, 0 },  { 444, 0 }, { 372, 0 }, { 176, 0 }, { 113, 0 },
};

// clang-format on

void FesMissionHollow_Setup(FesGimmick *gimmick) {
    FestMissionConfig *mission = GetFestMissionCfg(gimmick->festival);
    RivalEntry *entries = LinkFestival_GetMissionWork(gimmick->festival)->hollows;
    MATHRandContext32 rand;
    u8 hollows[20];
    int i;

    MI_CpuFill8(entries, 0, sizeof(RivalEntry) * 10);
    FestMission_InitRand(mission->seed, 0, &rand);
    for (i = 0; i < 20; i++) {
        hollows[i] = i;
    }
    FesMissionHollow_Shuffle(hollows, 20, &rand);
    for (i = 0; i < 10; i++) {
        RivalEntry *entry = &entries[i];

        entries[i].id = hollows[i];
        entry->female = FALSE;
        entry->selected = 1;
        entry->active = 0;
        entry->species = FesMissionHollow_GetPersonObjCode(gimmick);
        entry->form = FesMissionHollow_GetPersonUnk2(gimmick);
    }
    FesMissionHollow_Shuffle(hollows, 20, &rand);
    for (i = 0; i < 10; i++) {
        hollows[i] = i;
    }
    FesMissionHollow_Shuffle(hollows, 10, &rand);
    for (i = 0; i < 2; i++) {
        entries[hollows[i]].female = TRUE;
    }
}

static u16 FesMissionHollow_GetPersonObjCode(FesGimmick *gimmick) {
    FestMissionConfig *mission = GetFestMissionCfg(gimmick->festival);
    return data_ov026_0216fa50[mission->info.unk0_20].unk0;
}

static u8 FesMissionHollow_GetPersonUnk2(FesGimmick *gimmick) {
    FestMissionConfig *mission = GetFestMissionCfg(gimmick->festival);
    return data_ov026_0216fa50[mission->info.unk0_20].unk2;
}

// Shuffles the list
static void FesMissionHollow_Shuffle(u8 *list, int count, MATHRandContext32 *rand) {
    int i;

    for (i = 0; i < count; i++) {
        u32 j = MATH_Rand32(rand, count);
        u8 tmp = list[j];
        list[j] = list[i];
        list[i] = tmp;
    }
}

BOOL FesMissionHollow_IsActive(FesGimmick *gimmick) {
    FestMissionConfig *mission = GetFestMissionCfg(gimmick->festival);
    if (mission->info.target == 5 && mission->info.unk0_20 != 0) {
        return TRUE;
    }
    return FALSE;
}
