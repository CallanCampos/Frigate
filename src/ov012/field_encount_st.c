// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/zones.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_player.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/encounter.h"
#include "system/game_data.h"
#include "system/zone_weather.h"

// A group of ROAMING_POKEMON_ZONES's indices. The groups hold indices 0 to 15 once each, but nothing reads them, so
// what they are for is unknown
typedef struct {
    u16 count;
    u8 indices[4];
} RoamingZoneGroup;

BOOL EncData_Load(EncData *encData, ArcTool *arc, u16 zoneId, u8 season) {
    u32 fileId;
    u32 count;

    MI_CpuFill8(encData, 0, sizeof(EncData));
    fileId = GetZoneEncID(zoneId);
    if (fileId == 0x1fff) {
        return FALSE;
    }
    count = GFL_ArcToolGetDataLength(arc, fileId) / sizeof(EncData);
    if (count != 1 && count != 4) {
        return FALSE;
    }
    if (count == 1) {
        season = 0;
    }
    GFL_ArcToolReadRange(arc, fileId, season * sizeof(EncData), sizeof(EncData), encData);
    encData->fishingEnable = TRUE;
    return TRUE;
}

EncountState *EncountState_Create(HeapID heapId) {
    EncountState *state = GFL_HeapAllocate(heapId, sizeof(EncountState), TRUE, "field_encount_st.c", 0x2d);
    EncountState_SetTerrain(state, 0xff);
    return state;
}

void EncountState_Free(EncountState *state) {
    GFL_HeapFree(state);
}

void func_ov012_0215917c(GameData *gameData, Field *field) {
    EncountSystem *system = Field_GetEncountSystem(field);
    FieldPlayer *player;
    EncountState *state;
    u32 terrain;

    player = Field_GetPlayer(field);
    func_ov036_021a203c(system, system->effectEncountState);
    state = GameData_GetEncountState(gameData);
    terrain = FieldPlayer_GetTileTypeUnder(player);
    EncountState_SetTerrain(state, terrain);
}

void func_ov012_021591b4(GameData *gameData) {
    EncountState *state = GameData_GetEncountState(gameData);
    state->phenomenonSteps = 0;
}

void GameData_InitEncountTerrain(GameData *gameData, Field *field) {
    FieldPlayer *player = Field_GetPlayer(field);
    EncountState *state = GameData_GetEncountState(gameData);
    u32 terrain = FieldPlayer_GetTileTypeUnder(player);
    EncountState_SetTerrain(state, terrain);
}

void EncountState_SetTerrain(EncountState *state, u32 terrain) {
    state->terrain = terrain;
    state->rateStepCounter = 0;
    state->encountRate = 1;
    state->rateBlockCounter = 0;
    state->rateStepIncrement = 0;
}

void func_ov012_021591f4(GameData *gameData) {
}

// The routes a roaming Pokémon moves between, by EncountSave's zone clock, and none for the last
static const u16 ROAMING_POKEMON_ZONES[17] = {
    ZONE_ROUTE_2,  ZONE_ROUTE_3,  ZONE_ROUTE_4,  ZONE_ROUTE_5,  ZONE_ROUTE_6,  ZONE_ROUTE_7,
    ZONE_ROUTE_8,  ZONE_ROUTE_9,  ZONE_ROUTE_10, ZONE_ROUTE_11, ZONE_ROUTE_12, ZONE_ROUTE_13,
    ZONE_ROUTE_14, ZONE_ROUTE_15, ZONE_ROUTE_16, ZONE_ROUTE_18, 0xffff,
};

u16 EncountSave_GetRoamingPkmZone(EncountSave *save, u8 slot) {
    u32 clock = EncountSave_GetRoamingPkmZoneClock(save, slot);
    if (clock > 16) {
        return ZONE_ROUTE_2;
    }
    return ROAMING_POKEMON_ZONES[clock];
}

u32 func_ov012_02159218(EncountSave *save) {
    return 0;
}

void func_ov012_0215921c(GameData *gameData, u8 slot) {
    // Nothing reads these groups, so which of the empty functions held them is a guess
    static const RoamingZoneGroup sRoamingZoneGroups[5] = {
        { 3, { 0, 1, 15 } }, { 3, { 2, 3, 14 } }, { 3, { 4, 5, 6 } }, { 3, { 7, 8, 9 } }, { 4, { 10, 11, 12, 13 } },
    };
}

void func_ov012_02159220(GameData *gameData) {
}

u32 GetDefaultWeatherValue(GameData *gameData, u16 zoneId) {
    return WEATHER_NONE;
}

BOOL func_ov012_0215922c(GameData *gameData) {
    return FALSE;
}
