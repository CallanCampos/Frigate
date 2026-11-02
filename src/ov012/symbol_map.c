#include "types.h"
#include "constants/directions.h"
#include "constants/zones.h"
#include "field/symbol_map.h"
#include "field/symbol_save_field.h"
#include "gfl/heap.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"

#define SYMBOL_MAP_WIDTH 3
#define SYMBOL_MAP_SIZE 30

// The areas of the forest as it grows, by their IDs from 1, three to a row; 0 is no area. By how far the forest has
// grown and by the season of its trees
// clang-format off
static const u8 SYMBOL_MAP_0[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_1[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_2[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_3[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_4[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    21, 20, 22,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_5[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    24, 23, 25,
    21, 20, 22,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_6[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    27, 26, 28,
    24, 23, 25,
    21, 20, 22,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_7[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    3, 2, 4,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_8[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    3, 2, 4,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_9[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    3, 2, 4,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_10[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    3, 2, 4,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_11[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    3, 2, 4,
    21, 20, 22,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_12[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    3, 2, 4,
    24, 23, 25,
    21, 20, 22,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
    0, 0, 0,
};

// clang-format on

// clang-format off
static const u8 SYMBOL_MAP_13[SYMBOL_MAP_SIZE] = {
    0, 1, 0,
    3, 2, 4,
    27, 26, 28,
    24, 23, 25,
    21, 20, 22,
    18, 17, 19,
    15, 14, 16,
    12, 11, 13,
    9, 8, 10,
    6, 5, 7,
};

// clang-format on

static const u8 *const SYMBOL_MAP_TABLE[14] = {
    SYMBOL_MAP_0, SYMBOL_MAP_1, SYMBOL_MAP_2, SYMBOL_MAP_3,  SYMBOL_MAP_4,  SYMBOL_MAP_5,  SYMBOL_MAP_6,
    SYMBOL_MAP_7, SYMBOL_MAP_8, SYMBOL_MAP_9, SYMBOL_MAP_10, SYMBOL_MAP_11, SYMBOL_MAP_12, SYMBOL_MAP_13,
};

static u8 SymbolMap_FindAreaPos(const u8 *map, u8 area);
static u16 SymbolMap_GetAreaZone(const u8 *map, u8 area);
static u32 SymbolMap_GetNextArea(const u8 *map, u8 area, u8 dir);
static BOOL SymbolMap_GetMapLevels(GameSystem *gsys, u32 *deepestArea, u32 *group);
static const u8 *SymbolMap_GetMap(GameSystem *gsys);

BOOL SymbolMap_ToggleKeepPokemon(AreaNPCSave *npcData, u32 index) {
    u32 kind;
    u32 slot;

    switch (SymbolSave_GetKindOfSlot(index)) {
    case SYMBOL_KIND_KEEP_LARGE:
        kind = SYMBOL_KIND_FREE_LARGE;
        break;
    case SYMBOL_KIND_KEEP_SMALL:
        kind = SYMBOL_KIND_FREE_SMALL;
        break;
    case SYMBOL_KIND_FREE_LARGE:
        kind = SYMBOL_KIND_KEEP_LARGE;
        break;
    case SYMBOL_KIND_FREE_SMALL:
        kind = SYMBOL_KIND_KEEP_SMALL;
        break;
    default:
        return FALSE;
    }
    slot = SymbolSave_FindFreeSlot(npcData, kind);
    if (slot == SYMBOL_SLOT_NONE) {
        return FALSE;
    }
    SymbolSave_Decrypt(npcData);
    npcData->pokemon[slot] = npcData->pokemon[index];
    if (kind == SYMBOL_KIND_FREE_SMALL) {
        SymbolSave_UpdateDeepestArea(npcData, slot);
    }
    decryptNpcData(npcData);
    SymbolSave_RemoveSlot(npcData, index);
    return TRUE;
}

static u8 SymbolMap_FindAreaPos(const u8 *map, u8 area) {
    int i;

    for (i = 0; i < SYMBOL_MAP_SIZE; i++) {
        if (area == map[i]) {
            return i;
        }
    }
    return 1;
}

static u16 SymbolMap_GetAreaZone(const u8 *map, u8 area) {
    int pos = SymbolMap_FindAreaPos(map, area);
    BOOL left = FALSE;
    BOOL right;
    BOOL up;
    BOOL down;

    if (pos % SYMBOL_MAP_WIDTH != 0 && pos != 1) {
        left = TRUE;
    }
    right = FALSE;
    if (pos % SYMBOL_MAP_WIDTH != 2 && pos != 1) {
        right = TRUE;
    }
    up = FALSE;
    if (pos - SYMBOL_MAP_WIDTH >= 0 && map[pos - SYMBOL_MAP_WIDTH] != 0) {
        up = TRUE;
    }
    down = FALSE;
    if (pos + SYMBOL_MAP_WIDTH < SYMBOL_MAP_SIZE && map[pos + SYMBOL_MAP_WIDTH] != 0) {
        down = TRUE;
    }
    if (area == 1) {
        return ZONE_ENTREE_FOREST_2;
    }
    if (area == 5) {
        return ZONE_ENTREE_FOREST;
    }
    if (left && right) {
        return ZONE_ENTREE_FOREST_6;
    }
    if (left && !right) {
        if (up && down) {
            return ZONE_ENTREE_FOREST_7;
        }
        if (up) {
            return ZONE_ENTREE_FOREST_9;
        }
        if (down) {
            return ZONE_ENTREE_FOREST_4;
        }
    }
    if (!left && right) {
        if (up && down) {
            return ZONE_ENTREE_FOREST_5;
        }
        if (up) {
            return ZONE_ENTREE_FOREST_8;
        }
        if (down) {
            return ZONE_ENTREE_FOREST_3;
        }
    }
    return ZONE_ENTREE_FOREST_2;
}

static u32 SymbolMap_GetNextArea(const u8 *map, u8 area, u8 dir) {
    u8 pos = SymbolMap_FindAreaPos(map, area);

    switch (dir) {
    case DIR_UP:
        return map[pos - SYMBOL_MAP_WIDTH];
    case DIR_DOWN:
        return map[pos + SYMBOL_MAP_WIDTH];
    case DIR_LEFT:
        return map[pos - 1];
    case DIR_RIGHT:
        return map[pos + 1];
    }
    return area;
}

static BOOL SymbolMap_GetMapLevels(GameSystem *gsys, u32 *deepestArea, u32 *group) {
    AreaNPCSave *npcData = getAreaNPCData(GameData_GetSaveControl(GSYS_GetGameData(gsys)));

    GSYS_GetGameCommSystem(gsys);
    *deepestArea = SymbolSaveField_GetDeepestArea(npcData);
    *group = SymbolSaveField_GetGroup(npcData);
    return TRUE;
}

static const u8 *SymbolMap_GetMap(GameSystem *gsys) {
    u32 deepestArea;
    u32 group;

    if (SymbolMap_GetMapLevels(gsys, &deepestArea, &group) == FALSE) {
        return NULL;
    }
    return SYMBOL_MAP_TABLE[group * 7 + deepestArea];
}

SymbolMapList *SymbolMap_CreatePokemonList(HeapID heapId, GameSystem *gsys, u32 *count) {
    GameData *gameData = GSYS_GetGameData(gsys);
    AreaNPCSave *npcData = getAreaNPCData(GameData_GetSaveControl(gameData));
    u8 area;
    SymbolMapList *list;
    u8 found;

    GSYS_GetGameCommSystem(gsys);
    area = GameData_GetEntreeForestArea(gameData);
    list = GFL_HeapAllocate(heapId, sizeof(SymbolMapList), TRUE, "symbol_map.c", 466);
    *count = SymbolSaveField_GetAreaPokemon(npcData, list->pokemon, 20, area, &found);
    list->count = found;
    list->deepestArea = SymbolSaveField_GetDeepestArea(npcData);
    list->group = SymbolSaveField_GetGroup(npcData);
    list->unk50_14 = 4;
    list->area = area;
    return list;
}

u16 SymbolMap_GetZone(GameSystem *gsys, u8 area) {
    const u8 *map = SymbolMap_GetMap(gsys);

    if (map != NULL) {
        return SymbolMap_GetAreaZone(map, area + 1);
    }
    return ZONE_ENTREE_FOREST;
}

u32 SymbolMap_GetAdjacentArea(GameSystem *gsys, u8 area, u32 dir) {
    const u8 *map = SymbolMap_GetMap(gsys);
    u8 next;

    if (map != NULL) {
        next = SymbolMap_GetNextArea(map, area + 1, dir);
        return next - 1;
    }
    return 4;
}

BOOL SymbolMap_IsTopArea(u32 area) {
    u8 id = area + 1;

    if (id == 1) {
        return TRUE;
    }
    return FALSE;
}

BOOL SymbolMap_IsEntranceArea(u32 area) {
    u8 id = area + 1;

    if (id == 5) {
        return TRUE;
    }
    return FALSE;
}

BOOL SymbolMap_IsUpperRowArea(u32 area) {
    u8 id = area + 1;

    if (id == 2 || id == 3 || id == 4) {
        return TRUE;
    }
    return FALSE;
}
