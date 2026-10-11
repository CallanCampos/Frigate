#include "types.h"
#include "constants/flags.h"
#include "field/zone.h"
#include "save/event_work.h"
#include "system/game_data.h"

const RespawnZoneInfo RESPAWN_ZONE_INFO[82] = {
    { EVENT_FLAG_ARRIVED_NUVEMA_TOWN, 0, 1, 0, 390, 6, 8, 389, 782, 748, 389, 782, 748 },
    { EVENT_FLAG_ARRIVED_ACCUMULA_TOWN, 1, 1, 0, 398, 7, 12, 397, 791, 658, 397, 791, 658 },
    { EVENT_FLAG_ARRIVED_STRIATON_CITY, 1, 1, 0, 8, 7, 12, 6, 1048, 107, 6, 1048, 107 },
    { EVENT_FLAG_ARRIVED_NACRENE_CITY, 1, 1, 0, 20, 7, 12, 16, 1309, 132, 16, 1309, 132 },
    { EVENT_FLAG_ARRIVED_CASTELIA_CITY, 1, 0, 0, 41, 7, 12, 28, 1418, 235, 28, 1418, 235 },
    { EVENT_FLAG_ARRIVED_NIMBASA_CITY, 1, 1, 0, 65, 7, 12, 62, 1297, 295, 62, 1297, 295 },
    { EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY, 1, 0, 0, 99, 7, 12, 96, 1231, 238, 96, 1231, 238 },
    { EVENT_FLAG_ARRIVED_MISTRALTON_CITY, 1, 1, 0, 109, 7, 12, 107, 1209, 440, 107, 1209, 440 },
    { EVENT_FLAG_ARRIVED_ICIRRUS_CITY, 1, 1, 0, 115, 7, 12, 113, 1209, 440, 113, 1209, 440 },
    { EVENT_FLAG_ARRIVED_OPELUCID_CITY, 1, 0, 0, 122, 7, 12, 120, 1209, 440, 120, 1209, 440 },
    { EVENT_FLAG_ARRIVED_LACUNOSA_TOWN, 1, 0, 0, 407, 7, 12, 406, 1032, 263, 406, 1032, 263 },
    { EVENT_FLAG_ARRIVED_POKEMON_LEAGUE, 1, 1, 0, 146, 7, 12, 136, 17, 17, 136, 17, 17 },
    { EVENT_FLAG_ARRIVED_UNDELLA_TOWN, 1, 1, 0, 413, 7, 12, 412, 100, 100, 412, 100, 100 },
    { EVENT_FLAG_ARRIVED_UNITY_TOWER, 0, 1, 0, 147, 139, 236, 147, 395, 748, 147, 395, 748 },
    { EVENT_FLAG_ARRIVED_BLACK_CITY_WHITE_FOREST, 1, 1, 0, 1, 7, 12, 0, 100, 100, 0, 100, 100 },
    { EVENT_FLAG_ARRIVED_BLACK_CITY_WHITE_FOREST, 1, 1, 0, 425, 7, 12, 424, 100, 100, 424, 100, 100 },
    { 0x9b7, 0, 1, 0, 214, 0, 0, 214, 0, 0, 214, 0, 0 },
    { EVENT_FLAG_ARRIVED_ASPERTIA_CITY, 1, 1, 0, 435, 7, 12, 427, 0, 0, 427, 0, 0 },
    { 0x9c0, 0, 1, 0, 437, 0, 0, 437, 0, 0, 437, 0, 0 },
    { 0x9c1, 0, 1, 0, 446, 0, 0, 446, 0, 0, 446, 0, 0 },
    { 0x9c2, 0, 1, 0, 444, 0, 0, 444, 0, 0, 444, 0, 0 },
    { EVENT_FLAG_ARRIVED_VIRBANK_CITY, 1, 0, 0, 454, 7, 12, 448, 0, 0, 448, 0, 0 },
    { 0x9c3, 0, 1, 0, 456, 0, 0, 456, 0, 0, 456, 0, 0 },
    { 0x9c4, 0, 1, 0, 495, 0, 0, 495, 0, 0, 495, 0, 0 },
    { 0x9c5, 0, 1, 0, 326, 0, 0, 326, 0, 0, 326, 0, 0 },
    { 0x9c6, 0, 1, 0, 157, 0, 0, 157, 0, 0, 157, 0, 0 },
    { 0x9c7, 0, 1, 0, 160, 0, 0, 160, 0, 0, 160, 0, 0 },
    { 0x9c8, 0, 1, 0, 329, 0, 0, 329, 0, 0, 329, 0, 0 },
    { 0x9c9, 0, 1, 0, 383, 0, 0, 383, 0, 0, 383, 0, 0 },
    { 0x9ca, 0, 1, 0, 385, 0, 0, 385, 0, 0, 385, 0, 0 },
    { 0x9cb, 0, 1, 0, 253, 0, 0, 253, 0, 0, 253, 0, 0 },
    { 0x9cc, 0, 1, 0, 331, 0, 0, 331, 0, 0, 331, 0, 0 },
    { 0x9cd, 0, 1, 0, 503, 0, 0, 503, 0, 0, 503, 0, 0 },
    { 0x9ce, 0, 1, 0, 333, 0, 0, 333, 0, 0, 333, 0, 0 },
    { 0x9cf, 0, 1, 0, 194, 0, 0, 194, 0, 0, 194, 0, 0 },
    { 0x9d0, 0, 1, 0, 337, 0, 0, 337, 0, 0, 337, 0, 0 },
    { 0x9d1, 0, 1, 0, 338, 0, 0, 338, 0, 0, 338, 0, 0 },
    { 0x9d2, 0, 1, 0, 461, 0, 0, 461, 0, 0, 461, 0, 0 },
    { 0x9d3, 0, 1, 0, 462, 0, 0, 462, 0, 0, 462, 0, 0 },
    { 0x9d4, 0, 1, 0, 370, 0, 0, 370, 0, 0, 370, 0, 0 },
    { 0x9d5, 0, 1, 0, 240, 0, 0, 240, 0, 0, 240, 0, 0 },
    { 0x9d6, 0, 1, 0, 374, 0, 0, 374, 0, 0, 374, 0, 0 },
    { 0x9d7, 0, 1, 0, 376, 0, 0, 376, 0, 0, 376, 0, 0 },
    { 0x9d8, 0, 1, 0, 368, 0, 0, 368, 0, 0, 368, 0, 0 },
    { 0x9d9, 0, 1, 0, 255, 0, 0, 255, 0, 0, 255, 0, 0 },
    { 0x9da, 0, 1, 0, 365, 0, 0, 365, 0, 0, 365, 0, 0 },
    { 0x9db, 0, 1, 0, 348, 0, 0, 348, 0, 0, 348, 0, 0 },
    { 0x9dc, 0, 1, 0, 515, 0, 0, 515, 0, 0, 515, 0, 0 },
    { 0x9dd, 0, 1, 0, 463, 0, 0, 463, 0, 0, 463, 0, 0 },
    { 0x9de, 0, 1, 0, 378, 0, 0, 378, 0, 0, 378, 0, 0 },
    { 0x9bf, 0, 1, 0, 263, 0, 0, 263, 0, 0, 263, 0, 0 },
    { EVENT_FLAG_ARRIVED_HUMILAU_CITY, 1, 1, 0, 472, 7, 12, 465, 0, 0, 465, 0, 0 },
    { 0x9df, 0, 1, 0, 474, 0, 0, 474, 0, 0, 474, 0, 0 },
    { 0x9e0, 0, 1, 0, 230, 0, 0, 231, 0, 0, 230, 0, 0 },
    { 0x9e1, 0, 1, 0, 475, 0, 0, 475, 0, 0, 475, 0, 0 },
    { EVENT_FLAG_ARRIVED_VICTORY_ROAD, 1, 0, 0, 602, 7, 12, 573, 0, 0, 573, 0, 0 },
    { 0x9e3, 0, 1, 0, 345, 0, 0, 345, 0, 0, 345, 0, 0 },
    { 0x9e4, 0, 1, 0, 346, 0, 0, 346, 0, 0, 346, 0, 0 },
    { 0x9e5, 0, 1, 0, 205, 0, 0, 205, 0, 0, 205, 0, 0 },
    { 0x9e6, 0, 1, 0, 198, 0, 0, 198, 0, 0, 198, 0, 0 },
    { 0x9e7, 0, 1, 0, 154, 0, 0, 155, 0, 0, 154, 0, 0 },
    { 0x9e8, 0, 1, 0, 321, 0, 0, 321, 0, 0, 321, 0, 0 },
    { 0x9e9, 0, 1, 0, 324, 0, 0, 324, 0, 0, 324, 0, 0 },
    { 0x9ea, 0, 1, 0, 152, 0, 0, 152, 0, 0, 152, 0, 0 },
    { 0x9ec, 0, 1, 0, 319, 0, 0, 319, 0, 0, 319, 0, 0 },
    { 0x9eb, 0, 1, 0, 317, 0, 0, 317, 0, 0, 317, 0, 0 },
    { 0x9ed, 0, 1, 0, 423, 0, 0, 423, 0, 0, 423, 0, 0 },
    { 0x9ee, 0, 1, 0, 387, 0, 0, 387, 0, 0, 387, 0, 0 },
    { 0x9ef, 0, 1, 0, 238, 0, 0, 238, 0, 0, 238, 0, 0 },
    { 0x9f0, 0, 1, 0, 506, 0, 0, 506, 0, 0, 506, 0, 0 },
    { EVENT_FLAG_ARRIVED_POKESTAR_STUDIOS, 0, 1, 0, 566, 0, 0, 566, 0, 0, 566, 0, 0 },
    { EVENT_FLAG_ARRIVED_JOIN_AVENUE, 0, 1, 0, 490, 0, 0, 490, 0, 0, 490, 0, 0 },
    { EVENT_FLAG_ARRIVED_PWT, 0, 1, 0, 192, 0, 0, 192, 0, 0, 192, 0, 0 },
    { 0x9a1, 0, 1, 0, 479, 0, 0, 479, 0, 0, 479, 0, 0 },
    { 0x9a2, 0, 1, 0, 478, 0, 0, 478, 0, 0, 478, 0, 0 },
    { EVENT_FLAG_ARRIVED_FLOCCESY_TOWN, 1, 1, 0, 443, 7, 12, 439, 0, 0, 439, 0, 0 },
    { EVENT_FLAG_ARRIVED_LENTIMAS_TOWN, 1, 1, 0, 460, 7, 12, 458, 0, 0, 458, 0, 0 },
    { 0x9bd, 0, 1, 0, 249, 0, 0, 249, 0, 0, 249, 0, 0 },
    { 0x9be, 0, 1, 0, 254, 0, 0, 254, 0, 0, 254, 0, 0 },
    { EVENT_FLAG_TOWN_MAP_NS_CASTLE, 0, 1, 0, 273, 0, 0, 273, 0, 0, 273, 0, 0 },
    { EVENT_FLAG_TOWN_MAP_ABYSSAL_RUINS, 0, 0, 0, 245, 0, 0, 245, 0, 0, 245, 0, 0 },
    { 0x9c5, 0, 1, 0, 551, 0, 0, 551, 0, 0, 551, 0, 0 },
};

BOOL RangeCheckTeleportZone(s32 index) {
    if (index <= 0 || (u32)index > 0x52) {
        return FALSE;
    }
    return TRUE;
}

u16 GetRespawnZoneMainZone(u16 index) {
    return RESPAWN_ZONE_INFO[GetActualRespawnZoneIdx(index)].mainZoneId;
}

void SetupTeleportZoneChange(u16 index, ZoneSpawnInfo *spawn) {
    u32 actualIndex = GetActualRespawnZoneIdx(index);
    const RespawnZoneInfo *info = &RESPAWN_ZONE_INFO[actualIndex];

    CreateRespawnZoneChangeData(spawn, RESPAWN_ZONE_INFO[actualIndex].zoneId, 0, info->x, info->z);
}

u32 GetRespawnLocationIndexForRespawnZone(s32 zoneId) {
    u32 i;

    for (i = 0; i < 0x52; i++) {
        const RespawnZoneInfo *info = &RESPAWN_ZONE_INFO[i];

        if (zoneId == info->zoneId && info->canReturnHere) {
            return i + 1;
        }
    }
    return 0;
}

void SetTeleportZoneDiscover(GameData *gameData, s32 respawnZoneId) {
    u32 i;

    for (i = 0; i < 0x52; i++) {
        const RespawnZoneInfo *info = &RESPAWN_ZONE_INFO[i];

        if (respawnZoneId == info->mainZoneId && info->discoverOnVisit) {
            EventWork_FlagSet(GameData_GetEventWork(gameData), info->discoveryFlagId);
            return;
        }
    }
}

void CreateRespawnZoneChangeData(ZoneSpawnInfo *spawn, u16 zoneId, u32 unused, u16 x, u16 z) {
    CreateZoneChangeData(spawn, zoneId, 1, x << 16, 0, z << 16);
}

u32 GetActualRespawnZoneIdx(u32 index) {
    if (!RangeCheckTeleportZone(index)) {
        index = GetLeaguePokeCenReturnLocationIdx();
    }
    return index - 1;
}
