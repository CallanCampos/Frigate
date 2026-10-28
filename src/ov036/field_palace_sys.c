#include "types.h"
#include "constants/flags.h"
#include "constants/zones.h"
#include "field/field_palace.h"
#include "field/intrude_work.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"

// No luminance file, for an area owner of a version class other than 1 and 2
#define LUMINANCE_FILE_NONE 0xffff

// A luminance entry's area or season that matches any
#define PALACE_LUMINANCE_ANY 0xff

// The zones of the Entralink's worlds, and the flag that each needs
static const struct {
    u16 zoneId;
    u16 flag;
} ENTRALINK_WORLD_ZONES[25] = {
    { ZONE_NIMBASA_CITY_GATE, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { ZONE_DRIFTVEIL_CITY_GATE, EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY },
    { ZONE_MISTRALTON_CITY_GATE, EVENT_FLAG_ARRIVED_MISTRALTON_CITY },
    { ZONE_ICIRRUS_CITY_GATE, EVENT_FLAG_ARRIVED_ICIRRUS_CITY },
    { ZONE_OPELUCID_CITY_GATE, EVENT_FLAG_ARRIVED_OPELUCID_CITY },
    { ZONE_BLACK_CITY_GATE, EVENT_FLAG_ARRIVED_STRIATON_CITY },
    { ZONE_WHITE_FOREST_GATE, EVENT_FLAG_ARRIVED_STRIATON_CITY },
    { ZONE_DRIFTVEIL_DRAWBRIDGE_GATE, EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY },
    { ZONE_TUBELINE_BRIDGE_GATE, EVENT_FLAG_ARRIVED_OPELUCID_CITY },
    { ZONE_MARVELOUS_BRIDGE_GATE, EVENT_FLAG_ARRIVED_BLACK_CITY_WHITE_FOREST },
    { ZONE_ROUTE_5_GATE, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { ZONE_ROUTE_6_GATE, EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY },
    { ZONE_ROUTE_7_GATE, EVENT_FLAG_ARRIVED_MISTRALTON_CITY },
    { ZONE_ROUTE_8_GATE, EVENT_FLAG_ARRIVED_ICIRRUS_CITY },
    { ZONE_ROUTE_9_GATE, EVENT_FLAG_ARRIVED_OPELUCID_CITY },
    { ZONE_ROUTE_11_GATE, EVENT_FLAG_ARRIVED_OPELUCID_CITY },
    { ZONE_ROUTE_12_GATE, EVENT_FLAG_ARRIVED_LACUNOSA_TOWN },
    { ZONE_ROUTE_13_GATE, EVENT_FLAG_ARRIVED_LACUNOSA_TOWN },
    { ZONE_ROUTE_14_GATE, EVENT_FLAG_ARRIVED_UNDELLA_TOWN },
    { ZONE_ROUTE_15_GATE, EVENT_FLAG_ARRIVED_BLACK_CITY_WHITE_FOREST },
    { ZONE_ROUTE_16_GATE, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { ZONE_LACUNOSA_TOWN_GATE, EVENT_FLAG_ARRIVED_LACUNOSA_TOWN },
    { ZONE_UNDELLA_TOWN_GATE, EVENT_FLAG_ARRIVED_UNDELLA_TOWN },
    { ZONE_CHARGESTONE_CAVE_GATE, EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY },
    { ZONE_TWIST_MOUNTAIN_GATE, EVENT_FLAG_ARRIVED_MISTRALTON_CITY },
};

FieldPalaceSys *FieldPalaceSys_Create(HeapID heapId, GameSystem *gsys, Field *field, u16 zoneId) {
    FieldPalaceSys *sys;

    sys = GFL_HeapAllocate(heapId, sizeof(FieldPalaceSys), TRUE, "field_palace_sys.c", 67);
    sys->gsys = gsys;
    sys->field = field;
    sys->luminanceTable = NULL;
    FieldPalaceSys_InitPostFX(sys, zoneId, heapId);
    return sys;
}

void FieldPalaceSys_Free(FieldPalaceSys *sys) {
    if (sys->luminanceTable != NULL) {
        GFL_HeapFree(sys->luminanceTable);
    }
    GFL_HeapFree(sys);
}

u8 *FieldPalaceSys_GetLuminanceTable(FieldPalaceSys *sys) {
    return sys->luminanceTable;
}

void FieldPalaceSys_InitPostFX(FieldPalaceSys *sys, u16 zoneId, HeapID heapId) {
    GameCommSys *commSys;
    GameData *gameData;
    BOOL flag;
    u16 fileId;
    u32 state;

    commSys = GSYS_GetGameCommSystem(sys->gsys);
    if (GetZoneIsEntralinkAny(zoneId) == TRUE && commSys != NULL) {
        gameData = GSYS_GetGameData(sys->gsys);
        flag = FieldPalaceSys_CheckEventFlag(gameData, zoneId);
        state = Intrude_GetAreaOwnerVersionClass(commSys, gameData);
        fileId = LUMINANCE_FILE_NONE;
        if (state == 2) {
            if (flag == TRUE) {
                fileId = 1;
            } else {
                fileId = 0;
            }
        } else if (state == 1) {
            if (flag == TRUE) {
                fileId = 3;
            } else {
                fileId = 2;
            }
        }
        if (fileId != LUMINANCE_FILE_NONE) {
            FieldPalaceSys_LoadLuminanceTable(sys, fileId, zoneId, getSeasonFromPlayerData(commSys), heapId);
        }
    }
}

// An entry of a luminance file: the table for an area and season, where PALACE_LUMINANCE_ANY matches any
typedef struct {
    u8 area;
    u8 season;
    u16 unk2;
    u8 table[0x20];
} PalaceLuminanceEntry;

void FieldPalaceSys_LoadLuminanceTable(FieldPalaceSys *sys, u32 fileId, u16 zoneId, u32 season, HeapID heapId) {
    u32 i;
    BOOL found = FALSE;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0xdb, HEAPID_TAIL(heapId));
    u16 area = (ZoneData_GetAreaID(zoneId) - 2) / 4 + 1;
    u32 size;
    PalaceLuminanceEntry *entries = GFL_ArcToolReadHeapNewLZGetLen(arc, fileId, FALSE, HEAPID_TAIL(heapId), &size);

    for (i = 1; i < size / sizeof(PalaceLuminanceEntry); i++) {
        if ((entries[i].area == area || entries[i].area == PALACE_LUMINANCE_ANY) &&
            (entries[i].season == season || entries[i].season == PALACE_LUMINANCE_ANY)) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        i = 0;
    }
    sys->luminanceTable = GFL_HeapAllocate(heapId, sizeof(entries[i].table), FALSE, "field_palace_sys.c", 212);
    MI_CpuCopy8(entries[i].table, sys->luminanceTable, sizeof(entries[i].table));
    GFL_HeapFree(entries);
    GFL_ArcToolFree(arc);
}

BOOL FieldPalaceSys_CheckEventFlag(GameData *gameData, u16 zoneId) {
    EventWork *eventWork;
    u32 index;

    eventWork = GameData_GetEventWork(gameData);
    if (GetZoneIsEntralinkAny(zoneId) == TRUE) {
        for (index = 0; index < NELEMS(ENTRALINK_WORLD_ZONES); index++) {
            if (zoneId == ENTRALINK_WORLD_ZONES[index].zoneId) {
                if (EventWork_FlagGet(eventWork, ENTRALINK_WORLD_ZONES[index].flag) == TRUE) {
                    return TRUE;
                }
                return FALSE;
            }
        }
    }
    return TRUE;
}
