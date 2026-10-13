#include "types.h"
#include "constants/items.h"
#include "field/event_data.h"
#include "field/fest_mission_people.h"
#include "field/festival.h"
#include "field/field_actor.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "system/game_data.h"
#include "system/game_system.h"

// A kind of person a mission's target places: the object code of their model and a flag
typedef struct {
    u16 objCode;
    // Whether the items of a target 3 mission are on sale
    u16 unk2;
} FesMissionPerson;

// The people of one target, by the mission's variant
typedef struct {
    u32 count;
    const FesMissionPerson *people;
} FesMissionPeopleSet;

// A person's value and the weight it has among the others
typedef struct {
    u16 value;
    u16 weight;
} FesMissionWeightedValue;

// An item for sale and its price
typedef struct {
    u16 item;
    u16 price;
} FesMissionShopItem;

static const FesMissionPerson *FesMissionPeople_GetPerson(FestMission *mission);
static BOOL FesMissionPeople_PickPlaces(GameSystem *gsys, FesMissionWork *work, FestMission *mission, u16 zoneId,
                                        MATHRandContext32 *rand);
static void FesMissionPeople_SetParams(FesMissionWork *work, FestMission *mission, u8 index, MATHRandContext32 *rand,
                                       u32 *picked);
static void FesMissionPeople_SetRandomParam(FesMissionWork *work, FestMission *mission, u8 index,
                                            MATHRandContext32 *rand);
static void FesMissionPeople_SetAnswerParams(FesMissionWork *work, FestMission *mission, u8 index,
                                             MATHRandContext32 *rand);
static void FesMissionPeople_SetShopParams(FesMissionWork *work, FestMission *mission, u8 index,
                                           MATHRandContext32 *rand);
static void FesMissionPeople_SetBattleParam(FesMissionWork *work, FestMission *mission, u8 index,
                                            MATHRandContext32 *rand, u32 *picked);
static void FesMissionPeople_SetWeightedParam(FesMissionWork *work, FestMission *mission, u8 index,
                                              MATHRandContext32 *rand);
static void FesMissionPeople_SetSpeciesParam(FesMissionWork *work, FestMission *mission, u8 index,
                                             MATHRandContext32 *rand, u32 *picked);
static BOOL FesMissionPeople_NeedsNewZone(FesMissionWork *work);
static void FesMissionPeople_AddZone(FesMissionWork *work, u16 zoneId);
static void FesMissionPeople_PlaceActors(GameSystem *gsys, FesMissionWork *work, FestMission *mission, u16 zoneId);

// The people of each target, by the mission's variant
static const FesMissionPerson sTarget8People[1] = { { 73, 0 } };
static const FesMissionPerson sTarget1People[1] = { { 11, 0 } };
static const FesMissionPerson sTarget4People[1] = { { 154, 0 } };
static const FesMissionPerson sTarget0People[1] = { { 8, 0 } };
static const FesMissionPerson sTarget5People[1] = { { 54, 0 } };
static const FesMissionPerson sTarget2People[3] = { { 17, 1 }, { 48, 0 }, { 48, 0 } };
static const FesMissionPerson sTarget6People[3] = { { 46, 0 }, { 31, 1 }, { 32, 2 } };
static const FesMissionPerson sTarget7People[1] = { { 78, 0 } };
static const FesMissionPerson sTarget3People[5] = { { 77, 0 }, { 83, 1 }, { 83, 1 }, { 83, 1 }, { 83, 1 } };

// The weights of the shop's 16 items, out of 100
static const u8 sShopWeights[16] = { 10, 10, 10, 10, 10, 10, 10, 10, 5, 5, 5, 1, 1, 1, 1, 1 };

// What target 6's people give, by the mission's variant
static const u16 sBattleParams[3][6] = {
    { 712, 713, 714, 715, 716, 717 },
    { 718, 719, 720, 721, 722, 723 },
    { 757, 758, 759, 760, 761, 762 },
};

static const FesMissionWeightedValue sWeightedParams[2][6] = {
    { { 158, 40 }, { 14, 15 }, { 63, 15 }, { 29, 15 }, { 92, 10 }, { 50, 5 } },
    { { 504, 20 }, { 42, 20 }, { 54, 20 }, { 591, 20 }, { 51, 15 }, { 50, 5 } },
};

// How many people a zone gets, by the mission's pattern and a random number of 8
static const u8 sActorCounts[7][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 1, 1 }, { 0, 0, 1, 1, 1, 1, 1, 2 }, { 1, 1, 1, 1, 2, 2, 2, 2 },
    { 1, 1, 2, 2, 2, 3, 3, 3 }, { 1, 2, 2, 3, 3, 3, 4, 4 }, { 0, 0, 0, 0, 0, 0, 4, 4 },
};

static const u16 sRandomParams[30] = {
    17, 19, 28, 4,  137, 145, 26, 21, 79, 3,  138, 146, 25, 30, 76,
    12, 139, 147, 18, 31,  77,  14, 140, 148, 22, 32, 27,  11, 144, 17,
};

static const FesMissionPeopleSet sPeopleSets[9] = {
    { 1, sTarget0People }, { 1, sTarget1People }, { 3, sTarget2People }, { 5, sTarget3People }, { 1, sTarget4People },
    { 1, sTarget5People }, { 3, sTarget6People }, { 1, sTarget7People }, { 1, sTarget8People },
};

const FesMissionAnswers FesMissionPeople_Answers[3] = {
    { { { 78, 79, 4, 40, 319, 233 },
        { 17, 30, 25, 24, 259, 314 },
        { 95, 96, 97, 98, 258, 325 },
        { 72, 73, 74, 75, 110, 109 } } },
    { { { 78, 79, 4, 76, 77, 29 },
        { 17, 155, 30, 26, 33, 54 },
        { 18, 22, 19, 20, 21, 41 },
        { 238, 229, 72, 75, 82, 85 } } },
    { { { 78, 79, 4, 76, 77, 29 },
        { 17, 155, 30, 26, 33, 504 },
        { 18, 22, 19, 20, 21, 41 },
        { 238, 229, 73, 74, 84, 83 } } },
};

// The items for sale by the mission's variant, 16 each; the first variant sells the same Premier Ball over and over
static const FesMissionShopItem sShopItems[5][16] = {
    { { ITEM_POTION, 150 },
      { ITEM_SUPER_POTION, 350 },
      { ITEM_GREET_MAIL, 25 },
      { ITEM_ANTIDOTE, 50 },
      { ITEM_PARLYZ_HEAL, 100 },
      { ITEM_AWAKENING, 125 },
      { ITEM_BURN_HEAL, 125 },
      { ITEM_REPEL, 175 },
      { ITEM_MAX_REPEL, 280 },
      { ITEM_LUXURY_BALL, 400 },
      { ITEM_REVIVE, 600 },
      { ITEM_PREMIER_BALL, 500 },
      { ITEM_PREMIER_BALL, 500 },
      { ITEM_PREMIER_BALL, 500 },
      { ITEM_PREMIER_BALL, 500 },
      { ITEM_PREMIER_BALL, 500 } },
    { { ITEM_ORAN_BERRY, 200 },
      { ITEM_LEPPA_BERRY, 300 },
      { ITEM_SITRUS_BERRY, 300 },
      { ITEM_LUM_BERRY, 500 },
      { ITEM_HEART_SCALE, 1000 },
      { ITEM_NUGGET, 3000 },
      { ITEM_RARE_CANDY, 10000 },
      { ITEM_SWEET_HEART, 10000 },
      { ITEM_PP_UP, 9800 },
      { ITEM_DEEP_SEA_SCALE, 5000 },
      { ITEM_HELIX_FOSSIL, 5000 },
      { ITEM_OLD_AMBER, 5000 },
      { ITEM_CLAW_FOSSIL, 5000 },
      { ITEM_ARMOR_FOSSIL, 5000 },
      { ITEM_PLUME_FOSSIL, 5000 },
      { ITEM_MOON_STONE, 5000 } },
    { { ITEM_ORAN_BERRY, 200 },
      { ITEM_LEPPA_BERRY, 300 },
      { ITEM_SITRUS_BERRY, 300 },
      { ITEM_LUM_BERRY, 500 },
      { ITEM_HEART_SCALE, 1000 },
      { ITEM_NUGGET, 3000 },
      { ITEM_RARE_CANDY, 10000 },
      { ITEM_SWEET_HEART, 10000 },
      { ITEM_PP_UP, 9800 },
      { ITEM_DEEP_SEA_TOOTH, 5000 },
      { ITEM_DOME_FOSSIL, 5000 },
      { ITEM_OLD_AMBER, 5000 },
      { ITEM_ROOT_FOSSIL, 5000 },
      { ITEM_SKULL_FOSSIL, 5000 },
      { ITEM_COVER_FOSSIL, 5000 },
      { ITEM_SUN_STONE, 5000 } },
    { { ITEM_FIRE_STONE, 3000 },
      { ITEM_THUNDERSTONE, 3000 },
      { ITEM_WATER_STONE, 3000 },
      { ITEM_LEAF_STONE, 3000 },
      { ITEM_STAR_PIECE, 2000 },
      { ITEM_HEART_SCALE, 1000 },
      { ITEM_HEART_SCALE, 500 },
      { ITEM_GOOEY_MULCH, 100 },
      { ITEM_SUN_STONE, 5000 },
      { ITEM_MOON_STONE, 5000 },
      { ITEM_RELIC_VASE, 25000 },
      { ITEM_RARE_CANDY, 10000 },
      { ITEM_RAZOR_CLAW, 3000 },
      { ITEM_RAZOR_FANG, 3000 },
      { ITEM_RELIC_VASE, 10000 },
      { ITEM_RELIC_STATUE, 40000 } },
    { { ITEM_ORAN_BERRY, 200 },
      { ITEM_LEPPA_BERRY, 300 },
      { ITEM_SITRUS_BERRY, 300 },
      { ITEM_MAX_POTION, 1000 },
      { ITEM_DUSK_BALL, 500 },
      { ITEM_LUXURY_BALL, 500 },
      { ITEM_QUICK_BALL, 500 },
      { ITEM_HEART_SCALE, 1000 },
      { ITEM_LUM_BERRY, 500 },
      { ITEM_STAR_PIECE, 2000 },
      { ITEM_BIG_MUSHROOM, 1000 },
      { ITEM_FIRE_STONE, 3000 },
      { ITEM_WATER_STONE, 3000 },
      { ITEM_THUNDERSTONE, 3000 },
      { ITEM_LEAF_STONE, 3000 },
      { ITEM_GOOEY_MULCH, 1000 } },
};

void FesMissionPeople_OnZoneEnter(RivalEntry *entry, GameSystem *gsys, LinkFestival *festival, FestMissionConfig *config,
                         u16 zoneId) {
    FesMissionWork *work = (FesMissionWork *)entry;
    FestMission *mission = (FestMission *)config;
    MATHRandContext32 rand;

    work->placeCount = 0;
    work->actorCount = 0;
    work->festival = festival;
    work->zoneId = zoneId;
    sys_memset(work->places, 0, sizeof(work->places));
    sys_memset(work->placeIndex, 0, sizeof(work->placeIndex));
    sys_memset(work->params, 0, sizeof(work->params));
    sys_memset(work->actors, 0, sizeof(work->actors));
    func_ov012_02168348(mission->unk1C, func_ov012_02168320(zoneId), &rand);
    FesMissionPeople_PickPlaces(gsys, work, mission, zoneId, &rand);
    FesMissionPeople_AddZone(work, zoneId);
    if (work->actorCount != 0) {
        FesMissionPeople_PlaceActors(gsys, work, mission, zoneId);
    }
}

void FesMissionPeople_LoadSpecies(FesGimmick *gimmick, FesMissionWork *work) {
    FestMission *mission = gimmick->mission;
    u32 fileId;
    int size;
    ArcTool *arc;

    switch (mission->target) {
    case 5:
        fileId = 20;
        break;
    case 8:
        fileId = mission->unk0_20 + 2;
        break;
    default:
        return;
    }
    arc = func_02014590(gimmick->festival);
    size = GFL_ArcToolGetDataLength(arc, fileId);
    work->speciesCount = size;
    if (size > 0x500) {
        work->speciesCount = 0x500;
    }
    GFL_ArcToolReadRange(arc, fileId, 0, work->speciesCount, work->species);
}

static const FesMissionPerson *FesMissionPeople_GetPerson(FestMission *mission) {
    const FesMissionPeopleSet *set = &sPeopleSets[mission->target];
    u32 variant = mission->unk0_20;

    if (set->count == 1) {
        variant = 0;
    } else if (variant >= set->count) {
        variant = 0;
    }
    return &set->people[variant];
}

u8 FesMissionPeople_IsOnSale(FestMission *mission) {
    return FesMissionPeople_GetPerson(mission)->unk2;
}

u16 FesMissionPeople_GetObjCode(FesMissionWork *work, FestMission *mission) {
    return FesMissionPeople_GetPerson(mission)->objCode;
}

FesMissionZone *FesMissionPeople_GetZone(FesMissionWork *work, u16 zoneId) {
    int i;

    for (i = 0; i < work->zoneCount; i++) {
        if (zoneId == work->zones[i].zoneId) {
            return &work->zones[i];
        }
    }
    return &work->zones[0];
}

static void FesMissionPeople_PlaceActors(GameSystem *gsys, FesMissionWork *work, FestMission *mission, u16 zoneId) {
    GameData *gameData = GSYS_GetGameData(gsys);
    FesMissionZone *zone = FesMissionPeople_GetZone(work, zoneId);
    ZoneNPC npc;
    int i;

    sys_memset(&npc, 0, sizeof(ZoneNPC));
    npc.modelId = FesMissionPeople_GetObjCode(work, mission);
    npc.direction = 1;
    npc.scrId = mission->target + 0x2a22;
    for (i = 0; i < work->actorCount; i++) {
        MMSys *mmSys = GameData_GetMMSys(gameData);
        const ZoneBGEntity *place = &work->places[work->placeIndex[i]];

        if (zone->removed[i] == FALSE) {
            if (place->isRail == FALSE) {
                const s32 *coords = (const s32 *)place->pos.grid;

                func_ov012_021682c0(&npc, coords[0], coords[1], FX32_CONST(coords[2]));
                npc.moveCode = 2;
            } else {
                const u16 *values = &place->pos.rail.componentId;

                func_ov012_021682d4(&npc, values[0], values[1], values[2]);
                npc.moveCode = 70;
            }
            npc.uid = 236 + i;
            npc.param0 = work->params[i][0];
            npc.param1 = work->params[i][1];
            npc.param2 = work->params[i][2];
            work->actors[i] = CreateNewActorByEntityNoWKOBJCODE(mmSys, &npc, zoneId);
            SetActorMovementFlag(work->actors[i], 0x100000);
        }
    }
}

static BOOL FesMissionPeople_PickPlaces(GameSystem *gsys, FesMissionWork *work, FestMission *mission, u16 zoneId,
                                        MATHRandContext32 *rand) {
    int count;
    u32 pickedPlaces[4];
    u32 pickedParams[4];
    int i;
    ZoneBGEntity *entity;
    int level;
    u8 pattern;

    entity = GetZoneProxiesAndCount(GameData_GetEventData(GSYS_GetGameData(gsys)), &count);
    if (entity == NULL) {
        return FALSE;
    }
    work->placeCount = 0;
    for (i = 0; i < count; i++, entity++) {
        if (entity->condition != 5 && entity->condition != 4) {
            continue;
        }
        work->places[work->placeCount++] = *entity;
        if (work->placeCount >= 20) {
            break;
        }
    }
    if (work->placeCount == 0) {
        return FALSE;
    }
    level = GetZoneEncIDHighest3Bits(work->zoneId);
    if (level >= 6) {
        work->actorCount = 0;
        return FALSE;
    }
    pattern = (mission->unk8_0 >> (level * 3)) & 7;
    work->actorCount = sActorCounts[pattern][func_ov012_021683a8(rand, 8, NULL, 0)];
    if (work->actorCount > work->placeCount) {
        work->actorCount = work->placeCount;
    }
    if (work->actorCount == 0) {
        return FALSE;
    }
    for (i = 0; i < work->actorCount; i++) {
        work->placeIndex[i] = func_ov012_021683a8(rand, work->placeCount, pickedPlaces, i);
        FesMissionPeople_SetParams(work, mission, i, rand, pickedParams);
        pickedPlaces[i] = work->placeIndex[i];
    }
    return TRUE;
}

static void FesMissionPeople_SetParams(FesMissionWork *work, FestMission *mission, u8 index, MATHRandContext32 *rand,
                                       u32 *picked) {
    switch (mission->target) {
    case 1:
        FesMissionPeople_SetRandomParam(work, mission, index, rand);
        break;
    case 2:
        FesMissionPeople_SetAnswerParams(work, mission, index, rand);
        break;
    case 3:
        FesMissionPeople_SetShopParams(work, mission, index, rand);
        break;
    case 6:
        FesMissionPeople_SetBattleParam(work, mission, index, rand, picked);
        break;
    case 7:
        FesMissionPeople_SetWeightedParam(work, mission, index, rand);
        break;
    case 8:
        FesMissionPeople_SetSpeciesParam(work, mission, index, rand, picked);
        break;
    }
}

static void FesMissionPeople_SetRandomParam(FesMissionWork *work, FestMission *mission, u8 index,
                                            MATHRandContext32 *rand) {
    work->params[index][0] = sRandomParams[func_ov012_021683a8(rand, 30, NULL, 0)];
}

static void FesMissionPeople_SetAnswerParams(FesMissionWork *work, FestMission *mission, u8 index,
                                             MATHRandContext32 *rand) {
    u8 variant = mission->unk0_20;
    u32 first;
    int second;
    int i;

    if (variant >= 3) {
        variant = 0;
    }
    work->params[index][2] = variant;
    first = func_ov012_021683a8(rand, 3, NULL, 0) + 1;
    second = func_ov012_021683a8(rand, 2, NULL, 0);
    for (i = 0; i < 2; i++) {
        work->params[index][(i + second) % 2] = (first + i) % 3;
    }
    if (func_ov012_021683a8(rand, 100, NULL, 0) < 5) {
        work->params[index][func_ov012_021683a8(rand, 2, NULL, 0)] = 3;
    }
}

static void FesMissionPeople_SetShopParams(FesMissionWork *work, FestMission *mission, u8 index,
                                           MATHRandContext32 *rand) {
    u8 variant = mission->unk0_20;
    u8 pick = 0;
    u32 total = 0;
    int i;
    u32 roll;

    roll = func_ov012_021683a8(rand, 100, NULL, 0);
    for (i = 0; i < 16; i++) {
        total += sShopWeights[i];
        if (roll < total) {
            pick = i;
            break;
        }
    }
    work->params[index][0] = sShopItems[variant][pick].item;
    work->params[index][2] = sShopItems[variant][pick].price;
}

static void FesMissionPeople_SetBattleParam(FesMissionWork *work, FestMission *mission, u8 index,
                                            MATHRandContext32 *rand, u32 *picked) {
    u8 variant = mission->unk0_20;
    u32 pick;

    if (variant >= 3) {
        variant = 0;
    }
    pick = func_ov012_021683a8(rand, 6, picked, index);
    work->params[index][0] = sBattleParams[variant][pick];
    picked[index] = pick;
}

static void FesMissionPeople_SetWeightedParam(FesMissionWork *work, FestMission *mission, u8 index,
                                              MATHRandContext32 *rand) {
    int i;
    int pick = 0;
    u32 total = 0;
    u32 roll;
    const FesMissionWeightedValue *values = sWeightedParams[mission->unk0_20];

    roll = func_ov012_021683a8(rand, 100, NULL, 0);
    for (i = 0; i < 16; i++) {
        total += values[i].weight;
        if (roll < total) {
            pick = i;
            break;
        }
    }
    work->params[index][0] = values[pick].value;
}

static void FesMissionPeople_SetSpeciesParam(FesMissionWork *work, FestMission *mission, u8 index,
                                             MATHRandContext32 *rand, u32 *picked) {
    u32 pick = func_ov012_021683a8(rand, work->speciesCount / 2, picked, index);

    work->params[index][0] = work->species[pick];
    picked[index] = pick;
}

static BOOL FesMissionPeople_NeedsNewZone(FesMissionWork *work) {
    int i;

    if (work->zoneCount == 0) {
        return TRUE;
    }
    for (i = 0; i < 4; i++) {
        if (work->zones[work->zoneCount - 1].removed[i] != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static void FesMissionPeople_AddZone(FesMissionWork *work, u16 zoneId) {
    int index;
    int i;

    if (work->placeCount == 0) {
        return;
    }
    for (i = 0; i < work->zoneCount; i++) {
        if (zoneId == work->zones[i].zoneId) {
            return;
        }
    }
    if (FesMissionPeople_NeedsNewZone(work)) {
        if (work->zoneCount < 6) {
            index = work->zoneCount++;
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
    sys_memset(&work->zones[index], 0, sizeof(FesMissionZone));
    work->zones[index].zoneId = zoneId;
    work->zones[index].count = work->actorCount;
}
