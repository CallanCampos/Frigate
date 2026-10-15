#include "types.h"
#include "field/field_script.h"
#include "field/field_script_plugin.h"
#include "gfl/overlay.h"
#include "system/game_data.h"

// The zones of each plugin. Declared in the order that MWCC lays out as the original does
static const u16 sWbtStadiumZones[] = { 193 };
static const u16 sBadgeGateZones[] = { 573 };
static const u16 sPokemonCenterZones[] = { 1,   8,   20,  41,  65,  99,  109, 115, 122, 398,
                                           407, 413, 425, 435, 443, 454, 460, 472, 602, 146 };
static const u16 sResortZones[] = { 490, 491 };
static const u16 sPlugin14Zones[] = { 604, 427, 139, 213 };
static const u16 sWbtEntranceZones[] = { 192 };
static const u16 sPokewoodZones[] = { 566, 567, 568, 574 };
static const u16 sPleasureBoatZones[] = { 52 };
static const u16 sBlackTowerZones[] = { 478, 479, 480, 481, 482, 483, 484, 485, 486, 487, 492, 493 };
static const u16 sPalParkZones[] = { 381 };
static const u16 sAbyssalRuinsZones[] = { 241, 242, 243, 244, 245 };
static const u16 sPlasmaFrigateZones[] = { 561, 564, 553, 563, 558, 579, 580, 581, 582, 583 };
static const u16 sPlugin15Zones[] = { 465, 463, 474 };
static const u16 sPlugin16Zones[] = { 565, 614, 53 };
static const u16 sPokemonLeagueZones[] = { 140, 141, 142, 143, 144 };
static const u16 sBSubwayZones[] = { 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76 };

const ScriptPluginEntry SCRIPT_PLUGIN_TABLE[] = {
    { NULL, NULL, 0, OVERLAY_NONE, 0 },
    { BSUBWAY_SCRIPT_COMMANDS, sBSubwayZones, NELEMS(sBSubwayZones), OVERLAY_ID(50), OVERLAY_NONE },
    { PLEASURE_BOAT_SCRIPT_COMMANDS, sPleasureBoatZones, NELEMS(sPleasureBoatZones), OVERLAY_ID(51), OVERLAY_NONE },
    { POKEMON_LEAGUE_SCRIPT_COMMANDS, sPokemonLeagueZones, NELEMS(sPokemonLeagueZones), OVERLAY_ID(52), OVERLAY_NONE },
    { PAL_PARK_SCRIPT_COMMANDS, sPalParkZones, NELEMS(sPalParkZones), OVERLAY_ID(53), OVERLAY_NONE },
    { ABYSSAL_RUINS_SCRIPT_COMMANDS, sAbyssalRuinsZones, NELEMS(sAbyssalRuinsZones), OVERLAY_ID(54), OVERLAY_NONE },
    { WBT_SCRIPT_COMMANDS, sWbtEntranceZones, NELEMS(sWbtEntranceZones), OVERLAY_ID(55), OVERLAY_ID(56) },
    { WBT_SCRIPT_COMMANDS, sWbtStadiumZones, NELEMS(sWbtStadiumZones), OVERLAY_ID(55), OVERLAY_ID(57) },
    { RESORT_SCRIPT_COMMANDS, sResortZones, NELEMS(sResortZones), OVERLAY_ID(58), OVERLAY_ID(59) },
    { BLACK_TOWER_SCRIPT_COMMANDS, sBlackTowerZones, NELEMS(sBlackTowerZones), OVERLAY_ID(61), OVERLAY_NONE },
    { POKEWOOD_SCRIPT_COMMANDS, sPokewoodZones, NELEMS(sPokewoodZones), OVERLAY_ID(62), OVERLAY_NONE },
    { BADGE_GATE_SCRIPT_COMMANDS, sBadgeGateZones, NELEMS(sBadgeGateZones), OVERLAY_ID(63), OVERLAY_NONE },
    { PLASMA_FRIGATE_SCRIPT_COMMANDS, sPlasmaFrigateZones, NELEMS(sPlasmaFrigateZones), OVERLAY_ID(64), OVERLAY_NONE },
    { POKEMON_CENTER_SCRIPT_COMMANDS, sPokemonCenterZones, NELEMS(sPokemonCenterZones), OVERLAY_ID(65), OVERLAY_NONE },
    { PLUGIN_14_SCRIPT_COMMANDS, sPlugin14Zones, NELEMS(sPlugin14Zones), OVERLAY_ID(66), OVERLAY_NONE },
    { PLUGIN_15_SCRIPT_COMMANDS, sPlugin15Zones, NELEMS(sPlugin15Zones), OVERLAY_ID(67), OVERLAY_NONE },
    { PLUGIN_16_SCRIPT_COMMANDS, sPlugin16Zones, NELEMS(sPlugin16Zones), OVERLAY_ID(68), OVERLAY_NONE },
};

void SetScrPluginByZone(GameData *gameData, u16 zoneId) {
    u32 i;
    u32 j;

    for (i = 0; i < 17; i++) {
        if (i != 0) {
            for (j = 0; j < SCRIPT_PLUGIN_TABLE[i].zoneCount; j++) {
                if (zoneId == SCRIPT_PLUGIN_TABLE[i].zones[j]) {
                    SetScrPluginNo(gameData, i);
                    return;
                }
            }
        }
    }
}

void LoadScrPluginOverlays(GameData *gameData) {
    u32 pluginNo = GetScrPluginNo(gameData);

    if (pluginNo != 0) {
        if (SCRIPT_PLUGIN_TABLE[pluginNo].overlay0 != -1) {
            GFL_OvlLoad(SCRIPT_PLUGIN_TABLE[pluginNo].overlay0);
        }
        if (SCRIPT_PLUGIN_TABLE[pluginNo].overlay1 != -1) {
            GFL_OvlLoad(SCRIPT_PLUGIN_TABLE[pluginNo].overlay1);
        }
    }
}

void UnloadScrPluginOverlays(GameData *gameData) {
    u32 pluginNo = GetScrPluginNo(gameData);

    if (pluginNo != 0) {
        if (SCRIPT_PLUGIN_TABLE[pluginNo].overlay1 != -1) {
            GFL_OvlUnload(SCRIPT_PLUGIN_TABLE[pluginNo].overlay1);
        }
        if (SCRIPT_PLUGIN_TABLE[pluginNo].overlay0 != -1) {
            GFL_OvlUnload(SCRIPT_PLUGIN_TABLE[pluginNo].overlay0);
        }
        SetScrPluginNo(gameData, 0);
    }
}

const FieldScriptCommand *GetCurrentScrPluginTable(GameData *gameData) {
    u32 pluginNo = GetScrPluginNo(gameData);

    if (pluginNo == 0) {
        return NULL;
    }
    return SCRIPT_PLUGIN_TABLE[pluginNo].commands;
}

u32 GetCurrentScrPluginCmdCount(GameData *gameData) {
    u32 count;

    if (GetScrPluginNo(gameData) == 0 || GetCurrentScrPluginTable(gameData) == NULL) {
        return 0;
    }
    count = 0;
    while ((u32)GetCurrentScrPluginTable(gameData)[count] != (u32)-1) {
        count++;
    }
    return count;
}
