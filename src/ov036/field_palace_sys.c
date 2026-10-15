#include "types.h"
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

// The zones of the Entralink's worlds, and the flag that each needs
static const struct {
    u16 zoneId;
    u16 flag;
} ENTRALINK_WORLD_ZONES[25] = {
    { 0x121, 0x9ad }, { 0x122, 0x9ae }, { 0x123, 0x9af }, { 0x124, 0x9b0 }, { 0x125, 0x9b1 },
    { 0x126, 0x9aa }, { 0x127, 0x9aa }, { 0x12d, 0x9ae }, { 0x12e, 0x9b1 }, { 0x12f, 0x9b4 },
    { 0x130, 0x9ad }, { 0x131, 0x9ae }, { 0x132, 0x9af }, { 0x133, 0x9b0 }, { 0x134, 0x9b1 },
    { 0x135, 0x9b1 }, { 0x136, 0x9b2 }, { 0x137, 0x9b2 }, { 0x138, 0x9b5 }, { 0x139, 0x9b4 },
    { 0x13a, 0x9ad }, { 0x13b, 0x9b2 }, { 0x13c, 0x9b5 }, { 0x129, 0x9ae }, { 0x12a, 0x9af },
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

void *FieldPalaceSys_GetLuminanceTable(FieldPalaceSys *sys) {
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
        state = func_ov012_0215364c(commSys, gameData);
        fileId = 0xffff;
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
        if (fileId != 0xffff) {
            FieldPalaceSys_LoadLuminanceTable(sys, fileId, zoneId, getSeasonFromPlayerData(commSys), heapId);
        }
    }
}

// An entry of a luminance file: the table for an area and season, where 0xff matches any
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
        if ((entries[i].area == area || entries[i].area == 0xff) &&
            (entries[i].season == season || entries[i].season == 0xff)) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        i = 0;
    }
    sys->luminanceTable = GFL_HeapAllocate(heapId, sizeof(entries[i].table), FALSE, "field_palace_sys.c", 212);
    sys_memcpy(entries[i].table, sys->luminanceTable, sizeof(entries[i].table));
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
