#include "types.h"
#include "constants/species.h"
#include "field/fest_mission_data.h"
#include "field/fest_mission_gimmick.h"
#include "field/fest_mission_hollow.h"
#include "field/festival.h"
#include "field/rival_select.h"
#include "gfl/std.h"
#include "nitro/math.h"

// The Pokémon of a hollow mission, by the mission's targetVariant
typedef struct {
    u16 species;
    u8 form;
} FesMissionHollowPerson;

static u16 FesMissionHollow_GetPersonSpecies(FesGimmick *gimmick);
static u8 FesMissionHollow_GetPersonForm(FesGimmick *gimmick);
static void FesMissionHollow_Shuffle(u8 *list, int count, MATHRandContext32 *rand);

// clang-format off
static const FesMissionHollowPerson sFesMissionHollowPeople[] = {
    { SPECIES_BULBASAUR, 0 }, { SPECIES_GLAMEOW, 0 },   { SPECIES_STUNKY, 0 },    { SPECIES_EEVEE, 0 },
    { SPECIES_FLAREON, 0 },   { SPECIES_VAPOREON, 0 },  { SPECIES_JOLTEON, 0 },   { SPECIES_ESPEON, 0 },
    { SPECIES_UMBREON, 0 },   { SPECIES_GLACEON, 0 },   { SPECIES_LEAFEON, 0 },   { SPECIES_TAILLOW, 0 },
    { SPECIES_SHROOMISH, 0 }, { SPECIES_NINCADA, 0 },   { SPECIES_GULPIN, 0 },    { SPECIES_SPINDA, 0 },
    { SPECIES_WHISMUR, 0 },   { SPECIES_POOCHYENA, 0 }, { SPECIES_ZIGZAGOON, 0 }, { SPECIES_LOTAD, 0 },
    { SPECIES_MAKUHITA, 0 },  { SPECIES_DRAGONAIR, 0 }, { SPECIES_MAGNETON, 0 },  { SPECIES_GABITE, 0 },
    { SPECIES_SHELGON, 0 },   { SPECIES_TOGETIC, 0 },   { SPECIES_CHANSEY, 0 },
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
        entry->species = FesMissionHollow_GetPersonSpecies(gimmick);
        entry->form = FesMissionHollow_GetPersonForm(gimmick);
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

static u16 FesMissionHollow_GetPersonSpecies(FesGimmick *gimmick) {
    FestMissionConfig *mission = GetFestMissionCfg(gimmick->festival);
    return sFesMissionHollowPeople[mission->info.targetVariant].species;
}

static u8 FesMissionHollow_GetPersonForm(FesGimmick *gimmick) {
    FestMissionConfig *mission = GetFestMissionCfg(gimmick->festival);
    return sFesMissionHollowPeople[mission->info.targetVariant].form;
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
    if (mission->info.target == 5 && mission->info.targetVariant != 0) {
        return TRUE;
    }
    return FALSE;
}
