#include "types.h"
#include "constants/arc.h"
#include "constants/places.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "constants/zones.h"
#include "field/fest_mission_people.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "system/game_data.h"
#include "system/season.h"

// The zone headers and the tables of the zones that the code treats specially. One header is loaded at a time, from
// the zone data archive, and kept until another zone's is asked for. Black City and White Forest, their gates and
// Route 4 are zones of their own in each version: with a version's zones allowed, the functions here read the other
// version's zone as this one's.
// The file's name is the ROM's string, from GFL_HeapAllocate's call. The names of the functions and tables are swan's

// The archives of the fog and the static light of each zone, which have no name in swan
#define ARCID_ZONE_FOG_TABLE 80
#define ARCID_ZONE_LIGHT_TABLE 81

#define ZONE_FOG_TABLE_FILE 29
#define ZONE_LIGHT_TABLE_FILE 1

#define ZONE_COUNT (ZONE_PLEDGE_GROVE + 1)
#define VERSION_SPECIFIC_COUNT 3

// The area the Desert Resort has in White 2
#define AREA_DESERT_RESORT_WHITE2 250

// The name of the gates of Black City and White Forest in this version
#ifdef BLACK2
#define PLACE_OWN_GATE PLACE_BLACK_GATE
#else
#define PLACE_OWN_GATE PLACE_WHITE_GATE
#endif

#define ZONE_FOG_NONE 0x0fffffff
#define ZONE_LIGHT_NONE 0x0fffffff

typedef struct {
    u8 mapType;
    u8 npcInfoCacheIdx;
    u16 areaId;
    u16 matrixId;
    u16 scriptDatId;
    u16 unk08;
    u16 textDatId;
    u16 bgmSpring;
    u16 bgmSummer;
    u16 bgmAutumn;
    u16 bgmWinter;
    u16 encId : 13;
    u16 encIdHighest3Bits : 3;
    u16 entitiesId;
    u16 parentZone;
    u16 placeNameId : 10;
    u16 locNameDispType : 6;
    u16 weather : 6;
    u16 objectProjectionMatrixType : 3;
    u16 defaultCameraIndex : 7;
    u16 mapTransitionEffectType : 5;
    u16 battleBGId : 5;
    u16 enableCycling : 1;
    u16 unk1E_11 : 1;
    u16 enableEscapeRope : 1;
    u16 enableFlyFrom : 1;
    u16 enableCycleSurfBGM : 1;
    u16 enableEntralinkWarp : 1;
    u16 matrixCamBoundIdx;
    u16 nameIcon : 13;
    u16 easyChallengeModeLevelAdjust : 3;
    s32 flyX;
    s32 flyY;
    s32 flyZ;
} ZoneHeader; // 0x30

// An entry of the fog, light and flash tables
typedef struct {
    u16 zoneId;
    u16 value;
} ZoneValue;

typedef struct {
    ArcTool *arc;
    ZoneHeader header;
    u16 headerZoneId;
    ZoneValue *fogTable;
    ZoneValue *lightTable;
    u16 fogCount;
    u8 lightCount;
    u8 flashCount;
    ZoneValue *flashTable;
    u8 allowVersionSpecificZone[VERSION_SPECIFIC_COUNT];
    u8 allowVersionSpecificArea[VERSION_SPECIFIC_COUNT];
} ZoneDataSystem; // 0x50

// A zone or an area and the one that replaces it while it is allowed
typedef struct {
    u16 from;
    u16 to;
} VersionSpecificID;

// The rail or the camera of a zone
typedef struct {
    u16 zoneId : 11;
    u16 id : 5;
} ZoneAssignment;

static u16 GetVersionedMapChangeZoneNum(u16 zoneId);
static ZoneHeader *ZoneData_GetHeaderCore(u16 zoneId, BOOL versioned);
static ZoneHeader *ZoneData_GetHeader(u16 zoneId);
static BOOL IsZoneIDArrayContainsZoneID(u16 zoneId, const u16 *zones, int count);
static BOOL IsZoneBlackCityOrWhiteForest(u16 zoneId);
static BOOL IsZoneGearStationPlatform(u16 zoneId);

static ZoneDataSystem *g_ZoneDataSystem;

// Declared before the zones, which MWCC's size sort then lays out first, as the ROM has them
static VersionSpecificID VERSION_SPECIFIC_AREAS[VERSION_SPECIFIC_COUNT] = {
    { 50, 266 },
    { 50, 270 },
    { 54, 266 },
};

static VersionSpecificID VERSION_SPECIFIC_ZONES[VERSION_SPECIFIC_COUNT] = {
    { ZONE_BLACK_CITY, ZONE_WHITE_FOREST },
    { ZONE_BLACK_CITY_GATE, ZONE_WHITE_FOREST_GATE },
    { ZONE_ROUTE_4, ZONE_ROUTE_4_16 },
};

static const u16 ZONES_WITH_RAILS[12] = {
    ZONE_CASTELIA_CITY,    ZONE_SKYARROW_BRIDGE,     ZONE_VICTORY_ROAD_2,  ZONE_DRAGONSPIRAL_TOWER_5,
    ZONE_DRAGONSPIRAL_TOWER_7, ZONE_GEAR_STATION,    ZONE_ABYSSAL_RUINS,   ZONE_ABYSSAL_RUINS_2,
    ZONE_ABYSSAL_RUINS_3,  ZONE_ABYSSAL_RUINS_4,     ZONE_CASTELIA_CITY_8, ZONE_MARINE_TUBE,
};

static const ZoneAssignment ZONE_RAIL_ASSIGNMENTS[28] = {
    { ZONE_CASTELIA_CITY, 1 },         { ZONE_SKYARROW_BRIDGE, 17 },      { ZONE_CASTELIA_CITY_2, 2 },
    { ZONE_VICTORY_ROAD_2, 15 },       { ZONE_CELESTIAL_TOWER_2, 20 },    { ZONE_CELESTIAL_TOWER_3, 21 },
    { ZONE_CELESTIAL_TOWER_4, 22 },    { ZONE_CELESTIAL_TOWER_5, 23 },    { ZONE_ICIRRUS_CITY_2, 5 },
    { ZONE_DRAGONSPIRAL_TOWER_5, 13 }, { ZONE_DRAGONSPIRAL_TOWER_7, 14 }, { ZONE_GEAR_STATION, 4 },
    { ZONE_OPELUCID_CITY_GYM, 6 },     { ZONE_ABYSSAL_RUINS, 16 },        { ZONE_ABYSSAL_RUINS_2, 16 },
    { ZONE_ABYSSAL_RUINS_3, 16 },      { ZONE_ABYSSAL_RUINS_4, 16 },      { ZONE_CASTELIA_CITY_8, 3 },
    { ZONE_VILLAGE_BRIDGE, 18 },       { ZONE_POKEMON_LEAGUE_2, 7 },      { ZONE_POKEMON_LEAGUE_5, 8 },
    { ZONE_POKEMON_LEAGUE_6, 9 },      { ZONE_POKEMON_LEAGUE_7, 10 },     { ZONE_POKEMON_LEAGUE_8, 11 },
    { ZONE_POKEMON_LEAGUE_9, 12 },     { ZONE_MARINE_TUBE, 19 },          { ZONE_WHITE_FOREST, 24 },
    { ZONE_BLACK_CITY, 0 },
};

static u16 WT_BT_ZONES[10] = {
    ZONE_WHITE_TREEHOLLOW_2, ZONE_BLACK_TOWER_2,      ZONE_WHITE_TREEHOLLOW_3, ZONE_BLACK_TOWER_3,
    ZONE_BLACK_TOWER_4,      ZONE_WHITE_TREEHOLLOW_4, ZONE_BLACK_TOWER_5,      ZONE_WHITE_TREEHOLLOW_5,
    ZONE_WHITE_TREEHOLLOW_6, ZONE_BLACK_TOWER_6,
};

static const u16 BC_WF_LOBBY_ZONES[4] = {
    ZONE_WHITE_FOREST,
    ZONE_BLACK_CITY,
    ZONE_BLACK_CITY_GATE,
    ZONE_WHITE_FOREST_GATE,
};

// The two tables that no code of the ROM reads are globals, read by the functions the original link dropped
const u16 BC_WF_ZONES[9] = {
    ZONE_BLACK_CITY,   ZONE_BLACK_CITY_POKEMON_CENTER, ZONE_BLACK_CITY_2,
    ZONE_BLACK_CITY_3, ZONE_BLACK_CITY_4,              ZONE_BLACK_CITY_5,
    ZONE_WHITE_FOREST, ZONE_WHITE_FOREST_POKEMON_CENTER, ZONE_WHITE_FOREST_2,
};

static const ZoneAssignment ZONE_CAMERA_ASSIGNMENTS[9] = {
    { ZONE_ANVILLE_TOWN, 6 },      { ZONE_ROUTE_4, 4 },           { ZONE_VILLAGE_BRIDGE, 3 },
    { ZONE_POKEMON_LEAGUE_3, 0 },  { ZONE_ASPERTIA_CITY, 1 },     { ZONE_ROUTE_4_16, 5 },
    { ZONE_PLASMA_FRIGATE_10, 7 }, { ZONE_PLASMA_FRIGATE_13, 7 }, { ZONE_POKESTAR_STUDIOS, 2 },
};

static const u16 ABYSSAL_RUINS_ZONES[8] = {
    ZONE_ABYSSAL_RUINS,   ZONE_ABYSSAL_RUINS_2, ZONE_ABYSSAL_RUINS_3, ZONE_ABYSSAL_RUINS_4,
    ZONE_ABYSSAL_RUINS_5, ZONE_ABYSSAL_RUINS_6, ZONE_ABYSSAL_RUINS_7, ZONE_ABYSSAL_RUINS_8,
};

static const u16 ABYSSAL_RUINS_ZONES_INSIDE[4] = {
    ZONE_ABYSSAL_RUINS_5,
    ZONE_ABYSSAL_RUINS_6,
    ZONE_ABYSSAL_RUINS_7,
    ZONE_ABYSSAL_RUINS_8,
};

static const u16 NIMBASA_STADIUM_ZONES[5] = {
    ZONE_NIMBASA_CITY_5, ZONE_NIMBASA_CITY_6, ZONE_NIMBASA_CITY_7, ZONE_NIMBASA_CITY_10, ZONE_NIMBASA_CITY_11,
};

const u16 GEAR_STATION_PLATFORM_ZONES[9] = {
    ZONE_GEAR_STATION_2, ZONE_GEAR_STATION_3, ZONE_GEAR_STATION_4, ZONE_GEAR_STATION_5, ZONE_GEAR_STATION_6,
    ZONE_GEAR_STATION_7, ZONE_GEAR_STATION_8, ZONE_BATTLE_SUBWAY,  ZONE_BATTLE_SUBWAY_2,
};

void InitZoneDataSystem(HeapID heapId) {
    u32 size;
    u32 flashSize;

    if (g_ZoneDataSystem != NULL) {
        return;
    }

    g_ZoneDataSystem = GFL_HeapAllocate(heapId, sizeof(ZoneDataSystem), TRUE, "zonedata.c", 143);
    g_ZoneDataSystem->arc = GFL_ArcSysCreateFileHandle(ARCID_ZONEDATA, heapId);
    g_ZoneDataSystem->fogTable =
        GFL_ArcSysReadHeapNewLZGetLen(ARCID_ZONE_FOG_TABLE, ZONE_FOG_TABLE_FILE, FALSE, heapId, &size);
    g_ZoneDataSystem->fogCount = size / sizeof(ZoneValue);
    g_ZoneDataSystem->lightTable =
        GFL_ArcSysReadHeapNewLZGetLen(ARCID_ZONE_LIGHT_TABLE, ZONE_LIGHT_TABLE_FILE, FALSE, heapId, &size);
    g_ZoneDataSystem->lightCount = size / sizeof(ZoneValue);
    g_ZoneDataSystem->flashTable = GFL_ArcSysReadHeapNewLZGetLen(ARCID_MAPEFF_SKILL_TBL, 0, FALSE, heapId, &flashSize);
    g_ZoneDataSystem->flashCount = flashSize / sizeof(ZoneValue);
    g_ZoneDataSystem->headerZoneId = ZONE_COUNT;

#ifdef WHITE2
    SetAllowVersionSpecificZone(0, TRUE);
    SetAllowVersionSpecificZone(1, TRUE);
    SetAllowVersionSpecificZone(2, TRUE);
#endif
}

void FreeZoneDataSystem(void) {
    if (g_ZoneDataSystem == NULL) {
        return;
    }

    GFL_HeapFree(g_ZoneDataSystem->flashTable);
    GFL_HeapFree(g_ZoneDataSystem->lightTable);
    GFL_HeapFree(g_ZoneDataSystem->fogTable);
    GFL_ArcToolFree(g_ZoneDataSystem->arc);
    GFL_HeapFree(g_ZoneDataSystem);
    g_ZoneDataSystem = NULL;
}

void SetAllowVersionSpecificZone(u32 side, u32 allow) {
    if (g_ZoneDataSystem != NULL) {
        g_ZoneDataSystem->allowVersionSpecificZone[side] = allow;
    }
}

void SetAllowVersionSpecificArea(u32 area, BOOL allow) {
    g_ZoneDataSystem->allowVersionSpecificArea[area] = allow;
}

static u16 GetVersionedMapChangeZoneNum(u16 zoneId) {
    int i;

    for (i = 0; i < VERSION_SPECIFIC_COUNT; i++) {
        if (g_ZoneDataSystem->allowVersionSpecificZone[i] && zoneId == VERSION_SPECIFIC_ZONES[i].from) {
            return VERSION_SPECIFIC_ZONES[i].to;
        }
    }
    return zoneId;
}

u16 GetVersionedMapChangeZoneNum2(u16 zoneId) {
    return GetVersionedMapChangeZoneNum(zoneId);
}

static ZoneHeader *ZoneData_GetHeaderCore(u16 zoneId, BOOL versioned) {
    if (g_ZoneDataSystem == NULL) {
        return NULL;
    }
    if (versioned) {
        zoneId = GetVersionedMapChangeZoneNum(zoneId);
    }
    if (zoneId == g_ZoneDataSystem->headerZoneId) {
        return &g_ZoneDataSystem->header;
    }

    GFL_ArcToolReadRange(g_ZoneDataSystem->arc, 0, zoneId * sizeof(ZoneHeader), sizeof(ZoneHeader),
                         &g_ZoneDataSystem->header);
    g_ZoneDataSystem->headerZoneId = zoneId;
    return &g_ZoneDataSystem->header;
}

static ZoneHeader *ZoneData_GetHeader(u16 zoneId) {
    return ZoneData_GetHeaderCore(zoneId, TRUE);
}

static BOOL IsZoneIDArrayContainsZoneID(u16 zoneId, const u16 *zones, int count) {
    int i;

    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    for (i = 0; i < count; i++) {
        if (zoneId == zones[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u16 ZoneData_GetAreaID(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);
    u16 areaId = header->areaId;
    int i;

    for (i = 0; i < VERSION_SPECIFIC_COUNT; i++) {
        if (g_ZoneDataSystem->allowVersionSpecificArea[i] && areaId == VERSION_SPECIFIC_AREAS[i].from) {
            areaId = VERSION_SPECIFIC_AREAS[i].to;
            break;
        }
    }

#ifdef WHITE2
    if (zoneId == ZONE_DESERT_RESORT) {
        areaId = AREA_DESERT_RESORT_WHITE2;
    }
#endif
    return areaId;
}

u16 GetZoneMatrixId(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->matrixId;
}

u32 GetZoneMapType(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->mapType;
}

u16 ZoneData_GetScriptDatID(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->scriptDatId;
}

u16 ZoneData_GetTextDatID(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->textDatId;
}

// The position Fly lands at, which the header holds in units of 16
void LoadZoneFlyPosToVecFx32(u16 zoneId, VecFx32 *position) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    position->x = header->flyX << (FX32_SHIFT + 4);
    position->y = header->flyY << (FX32_SHIFT + 4);
    position->z = header->flyZ << (FX32_SHIFT + 4);
}

void LoadZoneFlyPosRailCoords(u16 zoneId, VecFx32 *position) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    position->x = header->flyX;
    position->y = header->flyY;
    position->z = header->flyZ;
}

u32 ZoneData_GetObjectProjectionMatrixType(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return (u8)header->objectProjectionMatrixType;
}

u32 GetZoneDefaultCameraIndex(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return (u8)header->defaultCameraIndex;
}

u16 GetZoneMatrixCamBoundIdx(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->matrixCamBoundIdx;
}

u16 DecideZoneHeaderBGMID(u16 zoneId, u8 season) {
    u16 bgmId = 0;
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    switch (season) {
    case SEASON_SPRING:
        bgmId = header->bgmSpring;
        break;
    case SEASON_SUMMER:
        bgmId = header->bgmSummer;
        break;
    case SEASON_AUTUMN:
        bgmId = header->bgmAutumn;
        break;
    case SEASON_WINTER:
        bgmId = header->bgmWinter;
        break;
    }

#ifdef WHITE2
    if (bgmId == SEQ_BGM_C_08_B) {
        bgmId = SEQ_BGM_C_08_W;
    }
#endif
    return bgmId;
}

u16 GetZoneEnvFlagsWeather(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->weather;
}

BOOL GetZoneFlagsEnableCycling(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->enableCycling;
}

BOOL GetZoneFlagsEnableCycleSurfBGM(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->enableCycleSurfBGM;
}

BOOL GetZoneFlagsEnableFlyFrom(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->enableFlyFrom;
}

BOOL GetZoneFlagsEnableEscapeRope(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->enableEscapeRope;
}

BOOL GetZoneFlagsEnableEntralinkWarp(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->enableEntralinkWarp;
}

BOOL IsZonesWithRailsArrayHasZoneID(u16 zoneId) {
    return IsZoneIDArrayContainsZoneID(zoneId, ZONES_WITH_RAILS, NELEMS(ZONES_WITH_RAILS));
}

BOOL GetZoneHasRailSystem(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (GetRailIDForZone(zoneId) != ZONE_RAIL_NONE) {
        return TRUE;
    }
    return FALSE;
}

u32 GetRailIDForZone(u16 zoneId) {
    u32 i;

    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    for (i = 0; i < NELEMS(ZONE_RAIL_ASSIGNMENTS); i++) {
        if (zoneId == ZONE_RAIL_ASSIGNMENTS[i].zoneId) {
            return ZONE_RAIL_ASSIGNMENTS[i].id;
        }
    }
    return ZONE_RAIL_NONE;
}

BOOL GetZoneIsUnionRoom(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_UNION_ROOM) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZone150Or151(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_150 || zoneId == ZONE_151) {
        return TRUE;
    }
    return FALSE;
}

// Whether the zone is one of the Battle Subway's two
BOOL IsZoneLEQ76(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_BATTLE_SUBWAY || zoneId == ZONE_BATTLE_SUBWAY_2) {
        return TRUE;
    }
    return FALSE;
}

BOOL GetZoneIsPWTBattleStage(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_PWT_3) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneRoyalUnova(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_ROYAL_UNOVA) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneBlackTowerOrWhiteTreehollow(u16 zoneId) {
    u32 i;

    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    for (i = 0; i < NELEMS(WT_BT_ZONES); i++) {
        if (zoneId == WT_BT_ZONES[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL GetZoneIsHiddenGroto(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_HIDDEN_GROTTO) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneGameCommDisabled(u16 zoneId) {
    if (GetZoneIsUnionRoom(zoneId) || IsZone150Or151(zoneId) || IsZoneRoyalUnova(zoneId)) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneEntralinkHub(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_ENTRALINK) {
        return TRUE;
    }
    return FALSE;
}

BOOL GetZoneIsEntralinkAny(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if ((zoneId >= ZONE_NIMBASA_CITY_GATE && zoneId <= ZONE_UNDELLA_TOWN_GATE) || GetZoneIsEntreeForest(zoneId) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL GetZoneIsEntreeForest(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId >= ZONE_ENTREE_FOREST && zoneId <= ZONE_ENTREE_FOREST_9) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneEntralinkEdgeColorTable(u16 zoneId) {
    if (zoneId >= ZONE_NIMBASA_CITY_GATE && zoneId <= ZONE_UNDELLA_TOWN_GATE) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneFlashbackMemoryPostFX(u16 zoneId) {
    if (zoneId == ZONE_STRIATON_CITY_10 || zoneId == ZONE_UNDELLA_TOWN_5 || zoneId == ZONE_FLOCCESY_TOWN_5 ||
        zoneId == ZONE_OPELUCID_CITY_12 || zoneId == ZONE_NACRENE_CITY_11 || zoneId == ZONE_DRIFTVEIL_CITY_17 ||
        zoneId == ZONE_MUSICAL_THEATER_3 || zoneId == ZONE_CHARGESTONE_CAVE_5) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IsZoneBlackCityOrWhiteForest(u16 zoneId) {
    return IsZoneIDArrayContainsZoneID(zoneId, BC_WF_ZONES, NELEMS(BC_WF_ZONES));
}

BOOL IsZoneBlackCityOrWhiteForestLobby(u16 zoneId) {
    return IsZoneIDArrayContainsZoneID(zoneId, BC_WF_LOBBY_ZONES, NELEMS(BC_WF_LOBBY_ZONES));
}

BOOL IsZoneJoinAvenue(u16 zoneId) {
    if (zoneId == ZONE_JOIN_AVENUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneJoinAvenueSubZone(u16 zoneId) {
    if (zoneId == ZONE_JOIN_AVENUE_2) {
        return TRUE;
    }
    return FALSE;
}

BOOL GetZoneIsMusicalTheater(u16 zoneId) {
    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    if (zoneId == ZONE_MUSICAL_THEATER_2) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02018c38(u16 zoneId) {
    if (GetZoneIsUnionRoom(zoneId) || IsZone150Or151(zoneId) || IsZoneLEQ76(zoneId) || IsZoneEntralinkHub(zoneId) ||
        GetZoneIsEntralinkAny(zoneId)) {
        return FALSE;
    }
    return TRUE;
}

u16 GetZoneParentZone(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->parentZone;
}

// The gates of Black City and White Forest have this version's name
u16 ZoneData_GetPlaceNameID(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);
    u16 placeNameId = header->placeNameId;

    if (placeNameId == PLACE_BLACK_GATE || placeNameId == PLACE_WHITE_GATE) {
        placeNameId = PLACE_OWN_GATE;
    }
    return placeNameId;
}

// The zone's place name for a game of the given version, without the version's zones replaced
u16 GetZoneLocationNameIdx(u16 zoneId, u32 version) {
    ZoneHeader *header = ZoneData_GetHeaderCore(zoneId, FALSE);
    u16 placeNameId = header->placeNameId;

    if (placeNameId == PLACE_BLACK_GATE || placeNameId == PLACE_WHITE_GATE) {
        if (version == VERSION_WHITE2) {
            placeNameId = PLACE_WHITE_GATE;
        } else {
            placeNameId = PLACE_BLACK_GATE;
        }
    }
    return placeNameId;
}

u32 GetZoneLocNameDispType(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->locNameDispType;
}

u32 GetZoneEasyChallengeModeLevelAdjust(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->easyChallengeModeLevelAdjust;
}

u16 GetZoneNameIcon(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->nameIcon;
}

BOOL GetIsZoneMatrix0(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    if (header->matrixId == 0) {
        return TRUE;
    }
    return FALSE;
}

u16 GetZoneEncID(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->encId;
}

int GetZoneEncIDHighest3Bits(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->encIdHighest3Bits;
}

u16 GetZoneEntitiesID(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->entitiesId;
}

u32 GetZoneBattleBGID(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return (u8)header->battleBGId;
}

u32 GetZoneFogIndex(u16 zoneId) {
    u32 index = ZONE_FOG_NONE;
    int i;

    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    for (i = 0; i < g_ZoneDataSystem->fogCount; i++) {
        if (zoneId == g_ZoneDataSystem->fogTable[i].zoneId) {
            index = g_ZoneDataSystem->fogTable[i].value;
            break;
        }
    }
    return index;
}

u32 GetZoneStaticLightDataIndex(u16 zoneId) {
    u32 index = ZONE_LIGHT_NONE;
    int i;

    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    for (i = 0; i < g_ZoneDataSystem->lightCount; i++) {
        if (zoneId == g_ZoneDataSystem->lightTable[i].zoneId) {
            index = g_ZoneDataSystem->lightTable[i].value;
            break;
        }
    }
    return index;
}

u32 GetZoneFlashFlags(u16 zoneId) {
    u32 flags = 0;
    int i;

    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    for (i = 0; i < g_ZoneDataSystem->flashCount; i++) {
        if (zoneId == g_ZoneDataSystem->flashTable[i].zoneId) {
            flags = g_ZoneDataSystem->flashTable[i].value;
            break;
        }
    }
    return flags;
}

u16 GetCameraIDForZone(u16 zoneId) {
    u32 i;

    zoneId = GetVersionedMapChangeZoneNum(zoneId);
    for (i = 0; i < NELEMS(ZONE_CAMERA_ASSIGNMENTS); i++) {
        if (zoneId == ZONE_CAMERA_ASSIGNMENTS[i].zoneId) {
            return ZONE_CAMERA_ASSIGNMENTS[i].id;
        }
    }
    return ZONE_CAMERA_NONE;
}

u32 GetMapTransitionEffectType(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return (u8)header->mapTransitionEffectType;
}

BOOL IsZoneAbyssalRuinsOutside(u16 zoneId) {
    return IsZoneIDArrayContainsZoneID(zoneId, ABYSSAL_RUINS_ZONES, NELEMS(ABYSSAL_RUINS_ZONES));
}

BOOL IsZoneAbyssalRuinsInside(u16 zoneId) {
    return IsZoneIDArrayContainsZoneID(zoneId, ABYSSAL_RUINS_ZONES_INSIDE, NELEMS(ABYSSAL_RUINS_ZONES_INSIDE));
}

BOOL IsZoneAbyssalRuinsFlashRock(u16 zoneId) {
    if (zoneId == ZONE_ABYSSAL_RUINS_6) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneAbyssalRuinsStrengthRock(u16 zoneId) {
    if (zoneId == ZONE_ABYSSAL_RUINS_7) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsZoneNimbasaStadium(u16 zoneId) {
    return IsZoneIDArrayContainsZoneID(zoneId, NIMBASA_STADIUM_ZONES, NELEMS(NIMBASA_STADIUM_ZONES));
}

static BOOL IsZoneGearStationPlatform(u16 zoneId) {
    return IsZoneIDArrayContainsZoneID(zoneId, GEAR_STATION_PLATFORM_ZONES, NELEMS(GEAR_STATION_PLATFORM_ZONES));
}

BOOL IsMetLocationEntralink(u16 location) {
    if (location >= PLACE_NIMBASA_CITY_2 && location <= PLACE_UNDELLA_TOWN_2) {
        return TRUE;
    }
    return FALSE;
}

BOOL GetIsZoneRoute14Or15(u16 zoneId) {
    if (zoneId == ZONE_ROUTE_15 || zoneId == ZONE_ROUTE_14) {
        return TRUE;
    }
    return FALSE;
}

s32 GetZoneNPCInfoCacheIdx(u16 zoneId) {
    ZoneHeader *header = ZoneData_GetHeader(zoneId);

    return header->npcInfoCacheIdx;
}

BOOL IsZoneInVictoryRoad(u16 zoneId) {
    if (GetZoneParentZone(zoneId) == ZONE_VICTORY_ROAD) {
        return TRUE;
    }
    return FALSE;
}
