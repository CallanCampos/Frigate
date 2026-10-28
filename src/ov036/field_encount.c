// Wild encounters: the encounter system, the encounter rate while the player walks, and the battles it starts, wild,
// static, fishing and trainers'. The name is the ROM's own, from GFL_HeapAllocate's file argument. Function names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "battle/btl_setup.h"
#include "battle/trainer_data.h"
#include "constants/battle.h"
#include "constants/species.h"
#include "field/enc_pokeset.h"
#include "field/encounter.h"
#include "field/event_battle.h"
#include "field/event_data.h"
#include "field/event_wild_battle.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_environment.h"
#include "field/field_event.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/pair_sys.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/records.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/zone_weather.h"

static int FieldEncount_DecideTypeByTerrain(u32 tileType, u32 mode);
static const EncountRateConfig *EncSys_GetEncountRateConfig(EncountSystem *system, int encType);
static BOOL EncSys_CheckRNGEncounter(EncountSystem *system, EncountState *state, u32 rate, int encType);
static void FieldEncount_SetupNormal(EncountSystem *system, EncountManager *manager, BtlSetup *setup, HeapID heapId,
                                     WildPkmParam *params);
static void FieldEncount_SetupRoaming(EncountSystem *system, EncountManager *manager, BtlSetup *setup, HeapID heapId,
                                      RoamingPkm *roamingPkm);
static void FieldEncount_SetupNPoke(EncountSystem *system, EncountManager *manager, BtlSetup *setup, HeapID heapId,
                                    u8 index);
static void EncountState_SyncToPlayer(EncountState *state, FieldPlayer *player);
static void EncountState_UpdateTerrain(EncountState *state, u32 tileType);
static void EncSys_UpdateEncountRate(EncountSystem *system, EncountState *state, u32 tileType);
static u32 EncountState_GetEncountRateStepCounter(EncountState *state);

// How the encounter rate climbs, for each setting of the zone's encounter data
static const EncountRateConfig ENCOUNT_RATE_CONFIGS[12] = {
    { 3, 1, 13, 2 }, { 3, 1, 19, 2 }, { 2, 3, 13, 3 }, { 2, 2, 19, 2 }, { 5, 1, 31, 2 }, { 5, 1, 31, 2 },
    { 2, 18, 7, 8 }, { 2, 9, 7, 4 },  { 2, 3, 13, 3 }, { 1, 12, 7, 3 }, { 1, 9, 7, 3 },  { 2, 3, 13, 3 },
};

EncountSystem *EncSys_Create(Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    EncountSystem *system = GFL_HeapAllocate(heapId, sizeof(EncountSystem), TRUE, "field_encount.c", 103);

    system->field = field;
    system->gsys = Field_GetGameSystem(field);
    system->gameData = GSYS_GetGameData(system->gsys);
    system->encData = GetEncountData(GameData_GetEventData(system->gameData));
    system->effectEncountState = EffectEncountState_Create(heapId);
    PrepareFieldEncountSystem(system, system->effectEncountState);
    return system;
}

void EncSys_Free(EncountSystem *system) {
    EffectEncount_Exit(system, system->effectEncountState);
    EffectEncountState_Free(system->effectEncountState);
    GFL_HeapFree(system);
}

BOOL EncSys_IsActive(EncountSystem *system, u32 unused) {
    if (system == NULL || system->encData == NULL || !system->encData->fishingEnable) {
        return FALSE;
    }
    if (GetZoneEncID(Field_GetPlayerStateZoneID(system->field)) == 0x1fff) {
        MI_CpuFill8(system->encData, 0, sizeof(EncData));
        return FALSE;
    }
    return TRUE;
}

void EncSys_Update(EncountSystem *system, BOOL moved, BOOL turned) {
    EncountState *state = GameData_GetEncountState(system->gameData);
    FieldPlayer *player = Field_GetPlayer(system->field);
    u32 tileType;

    FieldPlayer_GetActor(player);
    tileType = FieldPlayer_GetTileTypeUnder(player);
    if (EncSys_IsActive(system, 1)) {
        if (moved || turned) {
            EncountState_UpdateTerrain(state, tileType);
        }
        if (state->rateStepIncrement) {
            EncSys_UpdateEncountRate(system, state, tileType);
        } else if (moved) {
            EncSys_UpdateEncountRate(system, state, tileType);
            state->rateStepIncrement = 1;
        }
    }
}

GameEvent *EventWildBattleCall_CreateRandom(EncountSystem *system, u32 mode) {
    WildPkmParam params[2];
    EncountManager manager;
    int encType;
    BOOL forced = FALSE;
    BtlSetup *setup;
    u8 nIndex = N_POKE_NONE;
    RoamingPkm *roamingPkm = NULL;
    EncountState *state;
    FieldPlayer *player;

    player = Field_GetPlayer(system->field);
    state = GameData_GetEncountState(system->gameData);

    if (mode == ENCOUNT_MODE_EFFECT || mode == ENCOUNT_MODE_SWEET_SCENT) {
        forced = TRUE;
    }
    if (!EncSys_IsActive(system, 1)) {
        return NULL;
    }
    encType = FieldEncount_DecideTypeByTerrain(FieldPlayer_GetTileTypeUnder(player), mode);
    if (encType >= ENCTYPE_MAX || system->encData->userData[encType] == 0) {
        return NULL;
    }
    CreateEncountManager(&manager, system->gameData, encType, mode,
                         FieldWeather_GetWeatherId(Field_GetWeatherSystem(system->field)));
    if (!forced &&
        !EncSys_CheckRNGEncounter(
            system, state, FieldEncount_CalcEncountRate(&manager, system->gameData, state->encountRate), encType)) {
        return NULL;
    }
    if (mode == ENCOUNT_MODE_EFFECT) {
        RecordAddOne(GameData_GetRecords(system->gameData), 0x70);
    } else {
        u16 zoneId = Field_GetPlayerStateZoneID(system->field);

        roamingPkm = FieldEncount_RndCheckRoaming(&manager, zoneId);
        nIndex = FieldEncount_RndCheckNPoke(&manager, zoneId);
    }
    if (roamingPkm != NULL) {
        setup = BtlSetup_Create(4);
        FieldEncount_SetupRoaming(system, &manager, setup, 4, roamingPkm);
    } else if (nIndex != N_POKE_NONE) {
        setup = BtlSetup_Create(4);
        FieldEncount_SetupNPoke(system, &manager, setup, 4, nIndex);
    } else {
        int count;

        MI_CpuFill8(params, 0, sizeof(params));
        count = FieldEncount_GenWilds(system->encData, &manager, Field_GetPlayerStateZoneID(system->field), params);
        if (count == 0) {
            return NULL;
        }
        if (manager.doubleBattle && count == 1) {
            if (manager.pairActive) {
                return NULL;
            }
            manager.doubleBattle = FALSE;
            manager.pokeCount = count;
        }
        setup = BtlSetup_Create(4);
        FieldEncount_SetupNormal(system, &manager, setup, 4, params);
    }
    EncountState_SyncToPlayer(state, player);
    return EventWildBattleCall_CreateCore(system->gsys, system->field, setup, 0, mode, nIndex);
}

GameEvent *EventWildBattleCall_CreateStatic(EncountSystem *system, u16 species, u8 level, u8 form, u16 flags) {
    BtlSetup *setup;
    WildPkmParam params[2];
    EncountManager manager;
    WildPkmParam *param;

    Field_GetPlayer(system->field);
    CreateEncountManager(&manager, system->gameData, ENCTYPE_NONE, ENCOUNT_MODE_NORMAL,
                         FieldWeather_GetWeatherId(Field_GetWeatherSystem(system->field)));
    param = &params[0];
    MI_CpuFill8(params, 0, sizeof(params));
    param->species = species;
    param->level = level;
    param->form = form;
    if (flags & STATIC_ENC_FLAG_NO_ITEM) {
        param->item = 0xffff;
    }
    if (flags & STATIC_ENC_FLAG_SHINY) {
        param->shinyLock = WILD_SHINY_FORCED;
    } else if (flags & STATIC_ENC_FLAG_NOT_SHINY) {
        param->shinyLock = WILD_SHINY_NEVER;
    } else {
        param->shinyLock = WILD_SHINY_RANDOM;
    }
    if (flags & STATIC_ENC_FLAG_HIDDEN_ABILITY) {
        param->hiddenAbility = TRUE;
    }
    if (flags & STATIC_ENC_FLAG_MALE) {
        param->sex = WILD_SEX_MALE;
    } else if (flags & STATIC_ENC_FLAG_FEMALE) {
        param->sex = WILD_SEX_FEMALE;
    }
    setup = BtlSetup_Create(4);
    FieldEncount_SetupNormal(system, &manager, setup, 4, params);
    if (flags & 0x200) {
        setup->fieldSituation.env.terrain = 5;
        setup->fieldSituation.env.bgType = 6;
    }
    if (flags & 1) {
        BtlSetup_SetFlag(setup, 4);
    } else if (flags & 0x80) {
        BtlSetup_SetFlag(setup, 0x80);
    }
    if (flags & 0x100) {
        BtlSetup_SetFlag(setup, 0x200);
    }
    if (flags & 0x400) {
        BtlSetup_SetFlag(setup, 0x8000);
    }
    if (flags & 0x800) {
        setup->fieldSituation.env.bgType = 0x14;
        BtlSetup_SetFlag(setup, 0x10000);
    }
    if (species == SPECIES_KYUREM) {
        setup->fieldSituation.env.bgType = 0x14;
    }
    return EventWildBattleCall_Create(system->gsys, system->field, setup, 1, ENCOUNT_MODE_NORMAL);
}

BtlSetup *BtlSetup_CreateFishing(EncountSystem *system, BOOL rare) {
    WildPkmParam params[2];
    EncountManager manager;
    int encType;
    u8 rate;
    BtlSetup *setup;

    if (system->encData == NULL || !system->encData->fishingEnable) {
        return NULL;
    }
    encType = ENCTYPE_FISHING_RARE;
    if (rare != TRUE) {
        encType = ENCTYPE_FISHING;
    }
    rate = system->encData->userData[encType];
    if (rate == 0) {
        return NULL;
    }
    CreateEncountManager(&manager, system->gameData, encType, rare,
                         FieldWeather_GetWeatherId(Field_GetWeatherSystem(system->field)));
    if (rare != TRUE) {
        u32 encountRate = FieldEncount_CalcEncountRate(&manager, system->gameData, rate);

        if (GFL_RandomLC(100) > encountRate) {
            return NULL;
        }
    }
    MI_CpuFill8(params, 0, sizeof(params));
    if ((u32)FieldEncount_GenWilds(system->encData, &manager, Field_GetPlayerStateZoneID(system->field), params) <
        manager.pokeCount) {
        return NULL;
    }
    setup = BtlSetup_Create(4);
    FieldEncount_SetupNormal(system, &manager, setup, 4, params);
    BtlSetup_SetFlag(setup, 1);
    setup->fieldSituation.env.terrain = 6;
    return setup;
}

void SaveBtlFieldStatus(BtlFieldStatus *status, GameData *gameData, Field *field) {
    u16 zoneId = Field_GetPlayerStateZoneID(field);
    FieldPlayer *player = Field_GetPlayer(field);
    RTCTime time;

    status->bgId = GetZoneBattleBGID(zoneId);
    status->terrain = GetTileEncountType(GetTileClass(FieldPlayer_GetTileTypeUnder(player)));
    status->zoneId = zoneId;
    RTC_GetCachedTime(&time);
    status->hour = time.hour;
    status->minute = time.minute;
    status->weather = ConvFieldWeatherToBtl(field);
    status->season = GameData_GetSeason(gameData);
}

static int FieldEncount_DecideTypeByTerrain(u32 tileType, u32 mode) {
    u32 flags;
    u32 tileClass;

    if (!MapTile_IsValid(tileType)) {
        return ENCTYPE_NONE;
    }
    flags = GetTileFlags(tileType);
    if (mode != ENCOUNT_MODE_EFFECT && !(flags & TILE_FLAG_ENCOUNT)) {
        return ENCTYPE_NONE;
    }
    tileClass = GetTileClass(tileType);
    if (flags & TILE_FLAG_WATER) {
        if (mode == ENCOUNT_MODE_EFFECT) {
            return ENCTYPE_SURF_RARE;
        }
        return ENCTYPE_SURF;
    }
    if (mode == ENCOUNT_MODE_EFFECT) {
        return ENCTYPE_GRASS_SHAKING;
    }
    if (MapTile_IsTallGrassDoubleBtl(tileClass)) {
        return ENCTYPE_GRASS_RARE;
    }
    return ENCTYPE_GRASS;
}

static const EncountRateConfig *EncSys_GetEncountRateConfig(EncountSystem *system, int encType) {
    u8 index;

    if (encType >= ENCTYPE_FISHING) {
        return NULL;
    }
    index = system->encData->userData[encType] - 1;
    if (index >= 12) {
        index = 0;
    }
    return &ENCOUNT_RATE_CONFIGS[index];
}

static BOOL EncSys_CheckRNGEncounter(EncountSystem *system, EncountState *state, u32 rate, int encType) {
    const EncountRateConfig *config = EncSys_GetEncountRateConfig(system, encType);

    if (config == NULL) {
        return FALSE;
    }
    if (rate > 100) {
        rate = 100;
    }
    if (EncountState_GetEncountRateStepCounter(state) <= config->steps) {
        return FALSE;
    }
    if (GFL_RandomLC(100) <= rate) {
        return TRUE;
    }
    return FALSE;
}

static void FieldEncount_SetupNormal(EncountSystem *system, EncountManager *manager, BtlSetup *setup, HeapID heapId,
                                     WildPkmParam *params) {
    PokeParty *party = PokeParty_Create(heapId);
    PartyPkm *pkm = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), TRUE, "field_encount.c", 648);
    BtlFieldStatus status;
    int i;

    for (i = 0; i < (u8)manager->doubleBattle + 1; i++) {
        FieldEncount_CreateWildPkm(pkm, manager, &params[i]);
        PokeParty_AddPkm(party, pkm);
    }
    SaveBtlFieldStatus(&status, system->gameData, system->field);
    if (manager->pairActive) {
        BtlSetup_SetWildMulti(setup, system->gameData, party, GetNowFollowerAllyTrID(system->gameData), &status,
                              heapId);
    } else {
        BtlSetup_SetWildNormal(setup, system->gameData, party, &status, manager->doubleBattle, heapId);
    }
    if (manager->encType == ENCTYPE_GRASS_RARE) {
        BtlSetup_SetFlag(setup, 0x20);
    }
    GFL_HeapFree(pkm);
    GFL_HeapFree(party);
}

static void FieldEncount_SetupRoaming(EncountSystem *system, EncountManager *manager, BtlSetup *setup, HeapID heapId,
                                      RoamingPkm *roamingPkm) {
    PokeParty *party = PokeParty_Create(heapId);
    PartyPkm *pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), TRUE, "field_encount.c", 684);
    BtlFieldStatus status;

    FieldEncount_GenRoamingPkm(pkm, manager, roamingPkm);
    PokeParty_AddPkm(party, pkm);
    SaveBtlFieldStatus(&status, system->gameData, system->field);
    BtlSetup_SetWildNormal(setup, system->gameData, party, &status, FALSE, heapId);
    BtlSetup_SetFlag(setup, 8);
    GFL_HeapFree(pkm);
    GFL_HeapFree(party);
}

static void FieldEncount_SetupNPoke(EncountSystem *system, EncountManager *manager, BtlSetup *setup, HeapID heapId,
                                    u8 index) {
    PokeParty *party = PokeParty_Create(heapId);
    PartyPkm *pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), TRUE, "field_encount.c", 712);
    BtlFieldStatus status;

    makeNPokeFromData(pkm, index);
    PokeParty_AddPkm(party, pkm);
    SaveBtlFieldStatus(&status, system->gameData, system->field);
    BtlSetup_SetWildNormal(setup, system->gameData, party, &status, FALSE, heapId);
    BtlSetup_SetFlag(setup, 1 << 13);
    GFL_HeapFree(pkm);
    GFL_HeapFree(party);
}

void BtlSetup_SetTrainerLocal(EncountSystem *encountSystem, BtlSetup *setup, u32 style, u32 partnerId, u32 trainerId,
                              u32 trainerId2, HeapID heapId) {
    GameData *gameData = encountSystem->gameData;
    u32 terrain;
    BtlFieldStatus status;

    BtlSetup_Reset(setup);
    SaveBtlFieldStatus(&status, encountSystem->gameData, encountSystem->field);
    terrain = GetTrainerClassBattlePedestal(TrainerData_GetParam(trainerId, TRAINER_PARAM_CLASS));
    if (terrain != TRAINER_PEDESTAL_NONE) {
        status.terrain = terrain;
    }
    status.bgId = CheckOverridenTrainerBattleBG(TrainerData_GetParam(trainerId, TRAINER_PARAM_CLASS), status.bgId);
    switch (style) {
    case BTL_STYLE_DOUBLE:
        if (partnerId) {
            BtlSetup_SetTrainer2v2(setup, gameData, &status, partnerId, trainerId, trainerId2, heapId);
            return;
        }
        BtlSetup_SetTrainer1v1Double(setup, gameData, &status, trainerId, heapId);
        return;
    case BTL_STYLE_SINGLE:
        if (trainerId2) {
            BtlSetup_SetTrainer1v2(setup, gameData, &status, trainerId, trainerId2, heapId);
            return;
        }
        BtlSetup_SetTrainer1v1Single(setup, gameData, &status, trainerId, heapId);
        return;
    case BTL_STYLE_TRIPLE:
        BtlSetup_SetTrainer3v3(setup, gameData, &status, trainerId, heapId);
        return;
    case BTL_STYLE_ROTATION:
        BtlSetup_SetTrainerRotation(setup, gameData, &status, trainerId, heapId);
        break;
    }
}

u32 ConvFieldWeatherToBtl(Field *field) {
    switch (FieldWeather_GetWeatherId(Field_GetWeatherSystem(field))) {
    case WEATHER_RAIN:
    case WEATHER_THUNDERSTORM:
    case WEATHER_GUSTY_RAIN:
        return BTL_WEATHER_RAIN;
    case WEATHER_SANDSTORM:
    case WEATHER_SANDSTORM_2:
        return BTL_WEATHER_SANDSTORM;
    case WEATHER_BLIZZARD:
    case WEATHER_FLUTTER:
        return BTL_WEATHER_HAIL;
    default:
        return BTL_WEATHER_NONE;
    }
}

static void EncountState_SyncToPlayer(EncountState *state, FieldPlayer *player) {
    FieldActor *actor = FieldPlayer_GetActor(player);

    MI_CpuFill8(state, 0, 0x14);
    state->x = GetGPosX(actor);
    state->y = FldAct_GetGPosY(actor);
    state->z = GetGPosZ(actor);
    EncountState_SetTerrain(state, FieldPlayer_GetTileTypeUnder(player));
}

static void EncountState_UpdateTerrain(EncountState *state, u32 tileType) {
    u32 grass = GetTileFlags(tileType) & TILE_FLAG_ENCOUNT;
    u32 tileClass;
    u32 oldClass;

    if (grass != (GetTileFlags(state->terrain) & TILE_FLAG_ENCOUNT)) {
        EncountState_SetTerrain(state, tileType);
        return;
    }
    tileClass = GetTileClass(tileType);
    oldClass = GetTileClass(state->terrain);
    if (tileClass != oldClass) {
        if (grass == 0) {
            state->terrain = tileType;
            return;
        }
        if (GetTileEncountType(tileClass) == GetTileEncountType(oldClass)) {
            state->terrain = tileType;
            return;
        }
        EncountState_SetTerrain(state, tileType);
    }
}

static void EncSys_UpdateEncountRate(EncountSystem *system, EncountState *state, u32 tileType) {
    int encType = FieldEncount_DecideTypeByTerrain(tileType, ENCOUNT_MODE_NORMAL);
    const EncountRateConfig *config = EncSys_GetEncountRateConfig(system, encType);

    if (config == NULL || state->rateStepCounter >= 10 * FX32_ONE || encType == ENCTYPE_NONE) {
        return;
    }
    state->rateStepCounter++;
    if (state->rateStepCounter <= config->steps) {
        return;
    }
    state->rateBlockCounter += state->rateStepIncrement;
    if (state->rateBlockCounter < config->threshold) {
        return;
    }
    state->rateBlockCounter = 0;
    if (state->encountRate + config->increase > config->max) {
        state->encountRate = config->max;
    } else {
        state->encountRate += config->increase;
    }
}

static u32 EncountState_GetEncountRateStepCounter(EncountState *state) {
    return state->rateStepCounter;
}
