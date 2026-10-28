// The wild Pokémon of an encounter: the slots of the zone's encounter data and swarms, the lead Pokémon's abilities,
// the roaming Pokémon and N's Pokémon. The name is descriptive; the ROM has no string for this file.
// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/abilities.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/natures.h"
#include "constants/pokemon.h"
#include "constants/types.h"
#include "constants/zones.h"
#include "field/enc_pokeset.h"
#include "field/encounter.h"
#include "field/event_wild_battle.h"
#include "field/field.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/encounter.h"
#include "save/event_work.h"
#include "save/high_link.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/aeabi.h"
#include "system/game_data.h"
#include "system/zone_weather.h"

// The event flag that starts the swarms, which other systems wait for too
#define FLAG_SWARMS EVENT_FLAG_0x0960
// The event flag that lets N's Pokémon appear
#define FLAG_N_POKEMON EVENT_FLAG_0x098a

static void SetupEncountFlagsByPartyPokeAbil(EncountManager *manager, u16 weather);
static BOOL FieldEncount_CheckLevelRepelled(EncountManager *manager, u8 level);
static BOOL createSwarm(EncData *encData, EncountManager *manager, u16 zoneId, WildEncSlot *slots);
static u8 FieldEncount_PrepareActiveSlots(EncData *encData, EncountManager *manager, u16 zoneId, WildEncSlot *slots);
static void FieldEncount_SetupFromData(const WildEncSlot *slot, WildPkmParam *param);
static u8 GetEncountIndexForPreferredType(EncountManager *manager, const WildEncSlot *slots, u8 type);
static void FieldEncount_CheckApplyHighLevelWilds(const EncountManager *manager, const WildEncSlot *slots,
                                                  WildPkmParam *param);
static void FieldEncount_GenWildPokeParam(EncountManager *manager, const WildEncSlot *slots, WildPkmParam *param);
static void FieldEncount_SetRandomHeldItem(PartyPkm *pkm, PersonalData *personal, BOOL compoundEyes, BOOL doubleBattle);
static u32 FieldEncount_GenPID(const EncountManager *manager, PersonalData *personal, WildPkmParam *param);
static int GetEncountSlotByProbability(const u8 *odds, int count);
static int EncountMakeRnd12(int count);
static int EncountMakeRnd5T1(int count);
static int EncountMakeRnd5T2(int count);
static int EncountMakeRndFull(int count);

// The chance of each held item of the species' three, in percent: without and with Compound Eyes, in normal and in dark
// grass
// clang-format off
static const u8 HELD_ITEM_RATES[4][3] = {
    {50, 5, 0},
    {60, 20, 0},
    {50, 5, 1},
    {60, 20, 5},
};

// clang-format on

static int (*const ENCOUNT_SLOT_RANDOMIZE_FUNCS[4])(int count) = {
    EncountMakeRnd12,
    EncountMakeRnd5T1,
    EncountMakeRnd5T2,
    EncountMakeRndFull,
};

// The chance of each slot, in percent, for each row the Lucky Encounter pass powers pick. The two five-slot tables are
// declared the other way round, since MWCC's sort of the data by size swaps them
// clang-format off
static const u8 ENCOUNTRATE_RND5T2[4][5] = {
    {40, 40, 15, 4, 1},
    {40, 35, 15, 5, 5},
    {30, 30, 20, 10, 10},
    {20, 20, 20, 20, 20},
};

// clang-format on

// clang-format off
static const u8 ENCOUNTRATE_RND5T1[4][5] = {
    {60, 30, 5, 4, 1},
    {50, 30, 10, 5, 5},
    {40, 30, 10, 10, 10},
    {30, 20, 10, 20, 20},
};

// clang-format on

// clang-format off
static const u8 ENCOUNTRATE_RND12[4][12] = {
    {20, 20, 10, 10, 10, 10, 5, 5, 4, 4, 1, 1},
    {10, 10, 10, 10, 10, 10, 10, 10, 5, 5, 5, 5},
    {5, 5, 5, 5, 10, 10, 10, 10, 10, 10, 10, 10},
    {1, 1, 4, 4, 5, 5, 10, 10, 10, 10, 20, 20},
};

// clang-format on

// N's Pokémon: the zones they appear in, the species and level, the nature, and the sex and ability
const NPokeSpec N_POKEMON_INFO[ENCOUNT_SAVE_N_POKE_COUNT] = {
    { ZONE_ROUTE_2, 1, SPECIES_PURRLOIN, 7, NATURE_TIMID, GENDER_MALE, 1 },
    { ZONE_PINWHEEL_FOREST, 1, SPECIES_PIDOVE, 13, NATURE_SASSY, GENDER_MALE, 1 },
    { ZONE_PINWHEEL_FOREST, 1, SPECIES_TYMPOLE, 13, NATURE_MODEST, GENDER_MALE, 1 },
    { ZONE_PINWHEEL_FOREST, 1, SPECIES_TIMBURR, 13, NATURE_RASH, GENDER_MALE, 0 },
    { ZONE_DESERT_RESORT_2, 1, SPECIES_SANDILE, 22, NATURE_DOCILE, GENDER_MALE, 1 },
    { ZONE_DESERT_RESORT_2, 1, SPECIES_DARUMAKA, 22, NATURE_NAIVE, GENDER_MALE, 0 },
    { ZONE_DESERT_RESORT_2, 1, SPECIES_SIGILYPH, 22, NATURE_GENTLE, GENDER_MALE, 1 },
    { ZONE_DESERT_RESORT_2, 1, SPECIES_SCRAGGY, 22, NATURE_LAX, GENDER_MALE, 0 },
    { ZONE_CHARGESTONE_CAVE_2, 3, SPECIES_BOLDORE, 28, NATURE_NAIVE, GENDER_MALE, 0 },
    { ZONE_CHARGESTONE_CAVE_2, 3, SPECIES_FERROSEED, 28, NATURE_BASHFUL, GENDER_MALE, 0 },
    { ZONE_CHARGESTONE_CAVE_2, 3, SPECIES_JOLTIK, 28, NATURE_DOCILE, GENDER_MALE, 1 },
    { ZONE_CHARGESTONE_CAVE_2, 3, SPECIES_KLINK, 28, NATURE_RASH, GENDER_MALE, 0 },
    { ZONE_WELLSPRING_CAVE, 1, SPECIES_WOOBAT, 55, NATURE_TIMID, GENDER_MALE, 0 },
    { ZONE_DESERT_RESORT_2, 1, SPECIES_DARMANITAN, 35, NATURE_CALM, GENDER_MALE, 2 },
};

// The swarms: the zone, the species and form, and the level range. White 2 has Minun, Illumise, Mr. Mime and Ledian
// for Plusle, Volbeat, Sudowoodo and Ariados, with Illumise's Route 3 swarm moved down the list
static const SwarmData SWARM_DATA[ENCOUNT_SAVE_SWARM_LOCATION_COUNT] = {
    { ZONE_ROUTE_1, SPECIES_FARFETCHD, 0, 40, 55 },
    { ZONE_DREAMYARD, SPECIES_HYPNO, 0, 40, 55 },
#ifdef BLACK2
    { ZONE_ROUTE_6, SPECIES_PLUSLE, 0, 40, 55 },
    { ZONE_ROUTE_3, SPECIES_VOLBEAT, 0, 40, 55 },
#else
    { ZONE_ROUTE_6, SPECIES_MINUN, 0, 40, 55 },
#endif
    { ZONE_ROUTE_9, SPECIES_SWALOT, 0, 40, 55 },
    { ZONE_DESERT_RESORT_2, SPECIES_HIPPOWDON, 0, 40, 55 },
    { ZONE_ROUTE_5, SPECIES_NATU, 0, 40, 55 },
    { ZONE_ROUTE_7, SPECIES_FURRET, 0, 40, 55 },
    { ZONE_ROUTE_12, SPECIES_DODUO, 0, 40, 55 },
    { ZONE_ROUTE_8, SPECIES_QUAGSIRE, 0, 40, 55 },
    { ZONE_ROUTE_11, SPECIES_MASQUERAIN, 0, 40, 55 },
    { ZONE_ROUTE_13, SPECIES_SWELLOW, 0, 40, 55 },
    { ZONE_ABUNDANT_SHRINE, SPECIES_SLOWPOKE, 0, 40, 55 },
    { ZONE_ROUTE_15, SPECIES_FEAROW, 0, 40, 55 },
    { ZONE_ROUTE_16, SPECIES_PINECO, 0, 40, 55 },
    { ZONE_ROUTE_18, SPECIES_HOPPIP, 0, 40, 55 },
    { ZONE_REVERSAL_MOUNTAIN, SPECIES_CACTURNE, 0, 40, 55 },
#ifdef BLACK2
    { ZONE_ROUTE_20, SPECIES_SUDOWOODO, 0, 40, 55 },
    { ZONE_ROUTE_22, SPECIES_ARIADOS, 0, 40, 55 },
#else
    { ZONE_ROUTE_3, SPECIES_ILLUMISE, 0, 40, 55 },
    { ZONE_ROUTE_20, SPECIES_MR_MIME, 0, 40, 55 },
    { ZONE_ROUTE_22, SPECIES_LEDIAN, 0, 40, 55 },
#endif
};

void CreateEncountManager(EncountManager *manager, GameData *gameData, int encType, u32 mode, u16 weather) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    PokeParty *party = GameData_GetParty(gameData);
    PartyPkm *lead;

    MI_CpuFill8(manager, 0, sizeof(EncountManager));
    manager->encType = encType;
    manager->mode = mode;
    manager->gameData = gameData;
    manager->encountSave = SaveControl_GetEncountSave(save);
    if (encType == ENCTYPE_FISHING || encType == ENCTYPE_FISHING_RARE) {
        manager->isFishing = TRUE;
    }
    manager->playerInfo = GetGameDataPlayerInfo(gameData);
    manager->trainerId = getIDAsUInt(manager->playerInfo);
    lead = PokeParty_GetPkm(party, 0);
    manager->leadIsEgg = PokeParty_GetParam(lead, PKM_PARAM_IS_EGG, NULL);
    if (manager->leadIsEgg == FALSE) {
        manager->leadSpecies = PokeParty_GetParam(lead, PKM_PARAM_SPECIES, NULL);
        manager->leadItem = PokeParty_GetParam(lead, PKM_PARAM_ITEM, NULL);
        manager->leadAbility = PokeParty_GetParam(lead, PKM_PARAM_ABILITY, NULL);
        manager->leadSex = PokeParty_GetParam(lead, PKM_PARAM_SEX, NULL);
        manager->leadNature = PokeParty_GetNature(lead);
        SetupEncountFlagsByPartyPokeAbil(manager, weather);
    }
    manager->swarmsEnabled = EventWork_FlagGet(GameData_GetEventWork(gameData), FLAG_SWARMS);
    if (!EncountSave_IsRepelDepleted(SaveControl_GetEncountSave(save))) {
        manager->repelActive = TRUE;
    }
    if (manager->mode == ENCOUNT_MODE_SWEET_SCENT) {
        manager->forced = TRUE;
        manager->mode = ENCOUNT_MODE_NORMAL;
    }
    if (manager->mode == ENCOUNT_MODE_EFFECT) {
        manager->forced = TRUE;
    }
    manager->pairActive = GameData_CheckPairFlag(gameData);
    manager->doubleBattle = FALSE;
    if (manager->encType == ENCTYPE_GRASS_RARE && howManyPokesAreAbleToFight(party) >= 2 && GFL_RandomLC(100) < 40) {
        manager->doubleBattle = TRUE;
    }
    if (manager->pairActive) {
        manager->doubleBattle = TRUE;
    }
    if (manager->repelsWeaker || manager->repelActive) {
        manager->leadLevel =
            PokeParty_GetParam(PokeParty_GetPkm(party, PokeParty_GetFirstBattleReady(party)), PKM_PARAM_LEVEL, NULL);
    }
    manager->pokeCount = manager->doubleBattle + 1;
}

u32 FieldEncount_CalcEncountRate(EncountManager *manager, GameData *gameData, u32 baseRate) {
    u32 rate = baseRate;

    if (manager->isFishing) {
        if (manager->fishingRateUp) {
            rate *= 2;
        }
        if (rate > 100) {
            rate = 100;
        }
        return rate;
    }
    if (manager->rateUp) {
        rate *= 2;
    } else if (manager->rateDown) {
        rate /= 2;
    }
    if (manager->leadItem == ITEM_CLEANSE_TAG || manager->leadItem == ITEM_PURE_INCENSE) {
        rate = baseRate * 2 / 3;
    }
    if (rate > 100) {
        rate = 100;
    }
    return PassPower_ApplyEncounter(rate);
}

static int FieldEncount_GenWildsCore(EncountManager *manager, const WildEncSlot *slots, WildPkmParam *params) {
    int count = 0;
    int i;

    for (i = 0; i < manager->pokeCount; i++) {
        WildPkmParam *param = &params[count];

        FieldEncount_GenWildPokeParam(manager, slots, param);
        if (!FieldEncount_CheckLevelRepelled(manager, param->level)) {
            count++;
        }
    }
    return count;
}

int FieldEncount_GenWilds(EncData *encData, EncountManager *manager, u16 zoneId, WildPkmParam *params) {
    WildEncSlot slots[12];

    MIi_CpuClear32(0, slots, sizeof(slots));
    FieldEncount_PrepareActiveSlots(encData, manager, zoneId, slots);
    return FieldEncount_GenWildsCore(manager, slots, params);
}

RoamingPkm *FieldEncount_RndCheckRoaming(EncountManager *manager, u32 zoneId) {
    RoamingPkm *roaming[ENCOUNT_SAVE_ROAMING_COUNT];
    u8 count = 0;
    u8 i;

    for (i = 0; i < ENCOUNT_SAVE_ROAMING_COUNT; i++) {
        if (EncountSave_GetRoamingPkmStatus(manager->encountSave, i)) {
            RoamingPkm *roamingPkm = EncountSave_GetRoamingPkm(manager->encountSave, i);

            if (zoneId == EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_ZONE) &&
                !FieldEncount_CheckLevelRepelled(manager,
                                                 EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_LEVEL))) {
                roaming[count++] = roamingPkm;
            }
        }
    }
    if (count == 0 || GFL_RandomLCAlt(1000) < 1000 / 2) {
        return NULL;
    }
    return roaming[GFL_RandomLCAlt(count)];
}

u8 FieldEncount_RndCheckNPoke(EncountManager *manager, u32 zoneId) {
    u8 candidates[ENCOUNT_SAVE_N_POKE_COUNT];
    u8 count;
    u32 chance;
    int caught;
    u8 i;

    if (!EventWork_FlagGet(GameData_GetEventWork(manager->gameData), FLAG_N_POKEMON)) {
        return N_POKE_NONE;
    }
    chance = 5;
    caught = GetAlreadyCaughtNPokeCount(manager->encountSave);
    if (caught == 0) {
        chance = 40;
    } else if (caught < 6) {
        chance = 20;
    } else if (caught < 12) {
        chance = 10;
    }
    if (GFL_RandomLCAlt(100) >= chance) {
        return N_POKE_NONE;
    }
    count = 0;
    for (i = 0; i < ENCOUNT_SAVE_N_POKE_COUNT; i++) {
        const NPokeSpec *spec = &N_POKEMON_INFO[i];

        if (!IsNPokeAlreadyCaught(manager->encountSave, i)) {
            u8 zone;

            for (zone = 0; zone < spec->zoneCount; zone++) {
                if (zoneId == spec->firstZone + zone) {
                    break;
                }
            }
            if (zone != spec->zoneCount && !FieldEncount_CheckLevelRepelled(manager, spec->level)) {
                candidates[count++] = i;
            }
        }
    }
    if (count == 0) {
        return N_POKE_NONE;
    }
    return candidates[GFL_RandomLCAlt(count)];
}

void FieldEncount_CreateWildPkm(PartyPkm *pkm, const EncountManager *manager, WildPkmParam *param) {
    PersonalData *personal = PML_PersonalLoad(param->species, param->form, 4);
    u32 rolls = 1;
    u32 pid;
    u32 i;

    if (BagSave_CheckAmount(GameData_GetBag(manager->gameData), ITEM_SHINY_CHARM, 1, 4)) {
        rolls += 2;
    }
    rolls = PassPower_ApplyLuckyShiny(rolls);
    for (i = 0; i < rolls; i++) {
        pid = FieldEncount_GenPID(manager, personal, param);
        if (PML_UtilPIDIsRare(manager->trainerId, pid)) {
            break;
        }
    }
    PokeParty_CreatePkm(pkm, param->species, param->level, manager->trainerId, PKM_IVS_RANDOM, pid);
    PokeParty_ChangeForme(pkm, param->form);
    PokeParty_SetDefaultMoves(pkm);
    if (param->hiddenAbility) {
        PokeParty_SetHiddenAbil(pkm, param->species, param->form);
    }
    if (param->item <= ITEM_LAST) {
        if (param->item != 0) {
            PokeParty_SetParam(pkm, PKM_PARAM_ITEM, param->item);
        } else {
            FieldEncount_SetRandomHeldItem(pkm, personal, manager->compoundEyes,
                                           manager->encType == ENCTYPE_GRASS_RARE);
        }
    }
    PokeParty_SetParam(pkm, PKM_PARAM_OT_GENDER, getTrainerGender(manager->playerInfo));
    PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME_RAW, (u32)GetPlayerName(manager->playerInfo));
    if (manager->synchronize) {
        PokeParty_SetParam(pkm, PKM_PARAM_NATURE, manager->leadNature);
    }
    PokeParty_RecalcStats(pkm);
    PML_PersonalFree(personal);
}

void FieldEncount_GenRoamingPkm(PartyPkm *pkm, const EncountManager *manager, const RoamingPkm *roamingPkm) {
    u16 species = EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_SPECIES);
    u8 level = EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_LEVEL);
    u32 ivs = EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_IVS);

    PokeParty_CreatePkm(pkm, species, level, manager->trainerId, ivs,
                        EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_PERSONALITY));
    PokeParty_SetParam(pkm, PKM_PARAM_NATURE, EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_NATURE));
    PokeParty_SetParam(pkm, PKM_PARAM_STATUS, EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_STATUS));
    PokeParty_SetParam(pkm, PKM_PARAM_HP, EncountSave_GetRoamingPkmParam(roamingPkm, ROAMING_PKM_PARAM_HP));
    PokeParty_RecalcStats(pkm);
}

void makeNPokeFromData(PartyPkm *pkm, u8 index) {
    createNPkm(pkm, &N_POKEMON_INFO[index]);
}

void createNPkm(PartyPkm *pkm, const NPokeSpec *spec) {
    u32 pid = makeSpecialPID(2, spec->species, 0, spec->sex, spec->ability % 2, FALSE);

    PokeParty_CreatePkm(pkm, spec->species, spec->level, 2, 0x3def7bde, pid);
    if (spec->ability > 1) {
        PokeParty_SetHiddenAbil(pkm, spec->species, 0);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_NATURE, spec->nature);
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, 0xff);
    PokeParty_SetParam(pkm, PKM_PARAM_N_POKEMON, TRUE);
    PokeParty_RecalcStats(pkm);
}

u16 getSwarmLevelRangeFromData(GameData *gameData) {
    const SwarmData *swarm = getSwarmDataPtr(gameData);

    if (swarm == NULL) {
        return 0xffff;
    }
    return swarm->zoneId;
}

static void SetupEncountFlagsByPartyPokeAbil(EncountManager *manager, u16 weather) {
    switch (manager->leadAbility) {
    case ABILITY_ILLUMINATE:
    case ABILITY_ARENA_TRAP:
    case ABILITY_NO_GUARD:
        manager->rateUp = TRUE;
        return;
    case ABILITY_STENCH:
    case ABILITY_WHITE_SMOKE:
    case ABILITY_QUICK_FEET:
        manager->rateDown = TRUE;
        return;
    case ABILITY_SAND_VEIL:
        if (weather == WEATHER_SANDSTORM || weather == WEATHER_SANDSTORM_2) {
            manager->rateDown = TRUE;
        }
        return;
    case ABILITY_SNOW_CLOAK:
        if (weather == WEATHER_FLAKE || weather == WEATHER_BLIZZARD || weather == WEATHER_FLUTTER) {
            manager->rateDown = TRUE;
        }
        return;
    case ABILITY_STICKY_HOLD:
    case ABILITY_SUCTION_CUPS:
        if (manager->isFishing) {
            manager->fishingRateUp = TRUE;
        }
        return;
    case ABILITY_COMPOUNDEYES:
        manager->compoundEyes = TRUE;
        return;
    }
    if (manager->leadAbility == ABILITY_CUTE_CHARM && GFL_RandomLC(100) < 67) {
        manager->cuteCharm = TRUE;
        return;
    }
    if (GFL_RandomLC(100) < 50) {
        return;
    }
    switch (manager->leadAbility) {
    case ABILITY_MAGNET_PULL:
        manager->magnetPull = TRUE;
        break;
    case ABILITY_STATIC:
        manager->staticAbility = TRUE;
        break;
    case ABILITY_PRESSURE:
    case ABILITY_HUSTLE:
    case ABILITY_VITAL_SPIRIT:
        manager->higherLevel = TRUE;
        break;
    case ABILITY_INTIMIDATE:
    case ABILITY_KEEN_EYE:
        manager->repelsWeaker = TRUE;
        break;
    case ABILITY_SYNCHRONIZE:
        manager->synchronize = TRUE;
        break;
    }
}

static BOOL FieldEncount_CheckLevelRepelled(EncountManager *manager, u8 level) {
    if (manager->forced) {
        return FALSE;
    }
    if (manager->repelsWeaker && manager->leadLevel >= level + 5) {
        return TRUE;
    }
    if (!manager->isFishing && manager->repelActive && level < manager->leadLevel) {
        return TRUE;
    }
    return FALSE;
}

const SwarmData *getSwarmDataPtr(GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    u8 location;

    if (!EventWork_FlagGet(GameData_GetEventWork(gameData), FLAG_SWARMS)) {
        return NULL;
    }
    location = EncountSave_GetSwarmLocation(SaveControl_GetEncountSave(save));
    if (location >= ENCOUNT_SAVE_SWARM_LOCATION_COUNT) {
        location = 0;
    }
    return &SWARM_DATA[location];
}

static BOOL createSwarm(EncData *encData, EncountManager *manager, u16 zoneId, WildEncSlot *slots) {
    const SwarmData *swarm = getSwarmDataPtr(manager->gameData);

    if (swarm == NULL || swarm->zoneId != zoneId) {
        return FALSE;
    }
    if (manager->encType != ENCTYPE_GRASS) {
        return FALSE;
    }
    if (GFL_RandomLCAlt(100) > 40) {
        return FALSE;
    }
    slots[0].species = swarm->species;
    slots[0].minLevel = swarm->minLevel;
    slots[0].maxLevel = swarm->maxLevel;
    slots[0].form = swarm->form;
    manager->slotFuncIndex = 3;
    manager->slotCount = 1;
    return TRUE;
}

static u8 FieldEncount_PrepareActiveSlots(EncData *encData, EncountManager *manager, u16 zoneId, WildEncSlot *slots) {
    u8 count = 0;
    const WildEncSlot *src;
    u32 funcIndex = 0;

    if (createSwarm(encData, manager, zoneId, slots)) {
        return manager->slotCount;
    }
    if (manager->encType >= ENCTYPE_MAX) {
        manager->encType = ENCTYPE_GRASS;
    }
    switch (manager->encType) {
    case ENCTYPE_GRASS:
        count = 12;
        src = encData->grass;
        break;
    case ENCTYPE_GRASS_RARE:
        count = 12;
        src = encData->grassRare;
        break;
    case ENCTYPE_GRASS_SHAKING:
        count = 12;
        src = encData->grassShaking;
        break;
    case ENCTYPE_SURF:
        funcIndex = 1;
        count = 5;
        src = encData->surf;
        break;
    case ENCTYPE_SURF_RARE:
        funcIndex = 1;
        count = 5;
        src = encData->surfRare;
        break;
    case ENCTYPE_FISHING:
        funcIndex = 2;
        count = 5;
        src = encData->fishing;
        break;
    case ENCTYPE_FISHING_RARE:
        funcIndex = 2;
        count = 5;
        src = encData->fishingRare;
        break;
    }
    manager->slotFuncIndex = funcIndex;
    manager->slotCount = count;
    MI_CpuCopy8(src, slots, count * sizeof(WildEncSlot));
    return count;
}

static void FieldEncount_SetupFromData(const WildEncSlot *slot, WildPkmParam *param) {
    s16 range = slot->maxLevel - slot->minLevel;
    u8 formCount;

    MI_CpuFill8(param, 0, sizeof(WildPkmParam));
    if (range < 0) {
        range = 0;
    }
    param->level = slot->minLevel + GFL_RandomLC(100) % (range + 1);
    param->species = slot->species;
    formCount = PML_PersonalGetParamSingle(slot->species, 0, PERSONAL_FORM_COUNT);
    if (slot->form == WILD_ENC_FORM_RANDOM) {
        param->form = GFL_RandomLCAlt(formCount);
    } else if (slot->form >= formCount) {
        param->form = 0;
    } else {
        param->form = slot->form;
    }
}

// What GetEncountIndexForPreferredType returns when no slot has the type
#define ENCOUNT_SLOT_NONE 0xff

static u8 GetEncountIndexForPreferredType(EncountManager *manager, const WildEncSlot *slots, u8 type) {
    u8 candidates[12];
    int count = 0;
    int i;

    for (i = 0; i < manager->slotCount; i++) {
        if (slots[i].form != WILD_ENC_FORM_RANDOM) {
            u8 type1 = PML_PersonalGetParamSingle(slots[i].species, slots[i].form, PERSONAL_TYPE_1);
            u8 type2 = PML_PersonalGetParamSingle(slots[i].species, slots[i].form, PERSONAL_TYPE_2);

            if (type1 == type || type2 == type) {
                candidates[count++] = i;
            }
        }
    }
    if (count == 0) {
        return ENCOUNT_SLOT_NONE;
    }
    return candidates[GFL_RandomLC(count)];
}

static void FieldEncount_CheckApplyHighLevelWilds(const EncountManager *manager, const WildEncSlot *slots,
                                                  WildPkmParam *param) {
    u8 level = param->level;
    int i;
    u8 maxLevel = level;
    BOOL hasRange = FALSE;

    if (!manager->higherLevel) {
        return;
    }
    for (i = 0; i < manager->slotCount; i++) {
        if (!hasRange && slots[i].minLevel != slots[i].maxLevel) {
            hasRange = TRUE;
        }
        if (param->species == slots[i].species && maxLevel < slots[i].maxLevel) {
            maxLevel = slots[i].maxLevel;
        }
    }
    if (level + 5 <= maxLevel && hasRange == TRUE) {
        param->level += 5;
    } else {
        param->level = maxLevel;
    }
}

static void FieldEncount_GenWildPokeParam(EncountManager *manager, const WildEncSlot *slots, WildPkmParam *param) {
    u8 index = ENCOUNT_SLOT_NONE;

    if (manager->magnetPull) {
        index = GetEncountIndexForPreferredType(manager, slots, TYPE_STEEL);
    } else if (manager->staticAbility) {
        index = GetEncountIndexForPreferredType(manager, slots, TYPE_ELECTRIC);
    }
    if (index == ENCOUNT_SLOT_NONE) {
        index = ENCOUNT_SLOT_RANDOMIZE_FUNCS[manager->slotFuncIndex](manager->slotCount);
    }
    FieldEncount_SetupFromData(&slots[index], param);
    FieldEncount_CheckApplyHighLevelWilds(manager, slots, param);
}

static void FieldEncount_SetRandomHeldItem(PartyPkm *pkm, PersonalData *personal, BOOL compoundEyes,
                                           BOOL doubleBattle) {
    u16 items[3];
    const u8 *odds;
    u8 roll;
    u8 sum;
    int i;

    for (i = 0; i < 3; i++) {
        items[i] = PML_PersonalGetParam(personal, PERSONAL_ITEM_1 + i);
    }
    if (items[0] == items[1]) {
        PokeParty_SetParam(pkm, PKM_PARAM_ITEM, items[0]);
        return;
    }
    odds = HELD_ITEM_RATES[doubleBattle * 2 + compoundEyes];
    roll = GFL_RandomLC(100);
    sum = 0;
    for (i = 0; i < 3; i++) {
        sum += odds[i];
        if (roll < sum) {
            PokeParty_SetParam(pkm, PKM_PARAM_ITEM, items[i]);
            return;
        }
    }
}

static u32 FieldEncount_GenPID(const EncountManager *manager, PersonalData *personal, WildPkmParam *param) {
    u32 sex = GENDER_UNKNOWN;
    u32 shiny;
    u32 pid;
    u8 sexRatio = PML_PersonalGetParam(personal, PERSONAL_SEX_RATIO);

    if (isGenderlessOrSetGender(sexRatio) != TRUE) {
        if (manager->cuteCharm) {
            if (manager->leadSex == GENDER_MALE) {
                sex = GENDER_FEMALE;
            } else if (manager->leadSex == GENDER_FEMALE) {
                sex = GENDER_MALE;
            }
        }
        if (param->sex == WILD_SEX_MALE) {
            sex = GENDER_MALE;
        } else if (param->sex == WILD_SEX_FEMALE) {
            sex = GENDER_FEMALE;
        }
    }
    if (param->shinyLock == WILD_SHINY_NEVER) {
        shiny = PID_RARE_NEVER;
    } else if (param->shinyLock == WILD_SHINY_FORCED) {
        shiny = PID_RARE_ALWAYS;
    } else {
        shiny = PID_RARE_RANDOM;
    }
    pid = PML_GenPID(manager->trainerId, param->species, param->form, sex, 2, shiny);
    if (param->shinyLock != WILD_SHINY_NEVER && param->shinyLock != WILD_SHINY_FORCED) {
        if ((((manager->trainerId >> 16) ^ (manager->trainerId & 0xffff)) ^ (pid & 0xffff)) & 1) {
            return pid | 0x80000000;
        }
        return pid & 0x7fffffff;
    }
    return pid;
}

static int GetEncountSlotByProbability(const u8 *odds, int count) {
    u32 sum = 0;
    u32 roll = GFL_RandomLCAlt(100);
    int i;

    for (i = 0; i < count; i++) {
        sum += odds[i];
        if (roll < sum) {
            return i;
        }
    }
    return count - 1;
}

static int EncountMakeRnd12(int count) {
    return GetEncountSlotByProbability(ENCOUNTRATE_RND12[PassPower_ApplyLuckyEncProb(0)], count);
}

static int EncountMakeRnd5T1(int count) {
    return GetEncountSlotByProbability(ENCOUNTRATE_RND5T1[PassPower_ApplyLuckyEncProb(0)], count);
}

static int EncountMakeRnd5T2(int count) {
    return GetEncountSlotByProbability(ENCOUNTRATE_RND5T2[PassPower_ApplyLuckyEncProb(0)], count);
}

static int EncountMakeRndFull(int count) {
    return GFL_RandomLCAlt(count);
}
