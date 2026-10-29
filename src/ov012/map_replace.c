#include "types.h"
#include "constants/arc.h"
#include "field/field_map.h"
#include "field/intrude_work.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/season.h"

// The events that change the map, which a MapReplaceEntry's condition picks: an event happened when its work has the
// value
// clang-format off
const MapReplaceEvent EVENT_MAP_REPLACE_TABLE[10] = {
    { 3, 0, 0x4031, 0x1220 },
    { 4, 1, 0x4032, 0x224 },
    { 5, 2, 0x4033, 0x206 },
    { 6, 3, 0x4034, 0x1209 },
    { 7, 4, 0x4035, 0x316 },
    { 8, 5, 0x4036, 0x1227 },
    { 9, 6, 0x4037, 0x830 },
    { 10, 7, 0x4038, 0x408 },
    { 11, 8, 0x4039, 0x120 },
    { 12, 9, 0x403a, 0x1028 },
};

// clang-format on

MapReplace *MapReplace_Create(HeapID heapId, GameSystem *gsys) {
    MapReplace *replace = GFL_HeapAllocate(heapId, sizeof(MapReplace), TRUE, "map_replace.c", 0x77);

    replace->heapId = heapId;
    replace->arc = GFL_ArcSysCreateFileHandle(ARCID_MAP_REPLACE, heapId);
    replace->entryCount = (u16)GFL_ArcToolGetDataLength(replace->arc, 0) / sizeof(MapReplaceEntry);
    MapReplace_LoadVariables(&replace->variables, gsys);
    return replace;
}

s32 MapReplace_GetEntryCount(MapReplace *replace) {
    return replace->entryCount;
}

s32 MapReplace_LoadEntry(MapReplace *replace, u32 index) {
    GFL_ArcToolReadRange(replace->arc, 0, index * sizeof(MapReplaceEntry), sizeof(MapReplaceEntry), &replace->entry);
    return replace->entry.id;
}

void MapReplace_Free(MapReplace *replace) {
    GFL_ArcToolFree(replace->arc);
    GFL_HeapFree(replace);
}

u32 MapReplace_ResolvePatch(MapReplace *replace, u32 *oldValue, u32 *newValue) {
    MapReplaceEntry *entry = &replace->entry;
    MapReplaceVariables *variables = &replace->variables;
    u32 originalValue = entry->values[0];
    u32 index;
    u32 replacementValue;

    switch (entry->condition) {
    case 0:
        index = variables->season;
        break;
    case 1:
        index = variables->version;
        break;
    case 2:
        index = variables->versionSeason;
        break;
    default:
        index = variables->events[MapReplace_GetEventByCond(entry->condition)];
        break;
    }
    replacementValue = entry->values[index];
    *oldValue = originalValue;
    *newValue = replacementValue;
    if (originalValue == replacementValue) {
        return 0;
    }
    if (entry->kind == 1) {
        return 2;
    }
    if (entry->kind == 0) {
        return 1;
    }
    return 0;
}

int MapReplace_GetEventByCond(u8 condition) {
    u32 i;

    for (i = 0; i < 10; i++) {
        if (condition == EVENT_MAP_REPLACE_TABLE[i].condition) {
            return i;
        }
    }
    return -1;
}

void MapReplace_LoadVariables(MapReplaceVariables *variables, GameSystem *gsys) {
    EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(gsys));
    GameCommSys *commSys = GSYS_GetGameCommSystem(gsys);
    u8 season = GameSystem_GetSeason(gsys);
    s32 i;

    variables->season = season;
    if (getGameOrigin(commSys) == 0x15 || getGameOrigin(commSys) == 0x17) {
        variables->version = 0;
    } else {
        variables->version = 1;
    }
    if (variables->version == 0) {
        variables->versionSeason = 0;
    } else {
        variables->versionSeason = season + 1;
    }
    for (i = 0; i < 10; i++) {
        u16 value = *EventWork_GetWkPtr(eventWork, EVENT_MAP_REPLACE_TABLE[i].workId);

        if (value == EVENT_MAP_REPLACE_TABLE[i].expectedValue) {
            variables->events[i] = 1;
        } else {
            variables->events[i] = 0;
        }
    }
}

const MapReplaceEvent *MapReplace_GetEventByUID(GameData *gameData, u16 uid) {
    u32 i;

    for (i = 0; i < 10; i++) {
        u8 eventUid = EVENT_MAP_REPLACE_TABLE[i].uid;

        if (eventUid == uid) {
            return &EVENT_MAP_REPLACE_TABLE[i];
        }
    }
    return NULL;
}

void GameData_SetEventMapReplace(GameData *gameData, u16 uid, BOOL set) {
    const MapReplaceEvent *event = MapReplace_GetEventByUID(gameData, uid);
    u16 *work;

    if (event != NULL) {
        work = EventWork_GetWkPtr(GameData_GetEventWork(gameData), event->workId);
        if (set != 0) {
            *work = event->expectedValue;
        } else {
            *work = 0;
        }
    }
}

BOOL GameData_IsMapReplaceEventSet(GameData *gameData, u16 uid) {
    const MapReplaceEvent *event = MapReplace_GetEventByUID(gameData, uid);

    if (event != NULL) {
        if (event->expectedValue == *EventWork_GetWkPtr(GameData_GetEventWork(gameData), event->workId)) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

u32 GetLeaguePokeCenReturnLocationIdx(void) {
    return 1;
}
