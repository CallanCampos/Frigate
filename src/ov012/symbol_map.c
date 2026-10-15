#include "types.h"
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

static u8 func_ov012_021606e8(const u8 *map, u8 area);
static u16 func_ov012_02160700(const u8 *map, u8 area);
static u32 func_ov012_021607d0(const u8 *map, u8 area, u8 dir);
static BOOL func_ov012_02160810(GameSystem *gsys, u32 *a, u32 *b);
static const u8 *func_ov012_02160840(GameSystem *gsys);

BOOL func_ov012_02160668(AreaNPCSave *npcData, u32 index) {
    u32 kind;
    u32 slot;

    switch (func_0200ea1c(index)) {
    case 0:
        kind = 2;
        break;
    case 1:
        kind = 3;
        break;
    case 2:
        kind = 0;
        break;
    case 3:
        kind = 1;
        break;
    default:
        return FALSE;
    }
    slot = func_0200e9fc(npcData, kind);
    if (slot == 0xffff) {
        return FALSE;
    }
    func_0200e904(npcData);
    npcData->pokemon[slot] = npcData->pokemon[index];
    if (kind == 3) {
        func_0200eb14(npcData, slot);
    }
    decryptNpcData(npcData);
    func_0200ea24(npcData, index);
    return TRUE;
}

static u8 func_ov012_021606e8(const u8 *map, u8 area) {
    int i;

    for (i = 0; i < SYMBOL_MAP_SIZE; i++) {
        if (area == map[i]) {
            return i;
        }
    }
    return 1;
}

static u16 func_ov012_02160700(const u8 *map, u8 area) {
    int pos = func_ov012_021606e8(map, area);
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
        return 0x119;
    }
    if (area == 5) {
        return 0x118;
    }
    if (left && right) {
        return 0x11d;
    }
    if (left && !right) {
        if (up && down) {
            return 0x11e;
        }
        if (up) {
            return 0x120;
        }
        if (down) {
            return 0x11b;
        }
    }
    if (!left && right) {
        if (up && down) {
            return 0x11c;
        }
        if (up) {
            return 0x11f;
        }
        if (down) {
            return 0x11a;
        }
    }
    return 0x119;
}

static u32 func_ov012_021607d0(const u8 *map, u8 area, u8 dir) {
    u8 pos = func_ov012_021606e8(map, area);

    switch (dir) {
    case 0:
        return map[pos - SYMBOL_MAP_WIDTH];
    case 1:
        return map[pos + SYMBOL_MAP_WIDTH];
    case 2:
        return map[pos - 1];
    case 3:
        return map[pos + 1];
    }
    return area;
}

static BOOL func_ov012_02160810(GameSystem *gsys, u32 *a, u32 *b) {
    AreaNPCSave *npcData = getAreaNPCData(GameData_GetSaveControl(GSYS_GetGameData(gsys)));

    GSYS_GetGameCommSystem(gsys);
    *a = func_ov012_0216127c(npcData);
    *b = func_ov012_02161260(npcData);
    return TRUE;
}

static const u8 *func_ov012_02160840(GameSystem *gsys) {
    u32 a;
    u32 b;

    if (func_ov012_02160810(gsys, &a, &b) == FALSE) {
        return NULL;
    }
    return SYMBOL_MAP_TABLE[b * 7 + a];
}

SymbolMapList *func_ov012_02160870(HeapID heapId, GameSystem *gsys, u32 *count) {
    GameData *gameData = GSYS_GetGameData(gsys);
    AreaNPCSave *npcData = getAreaNPCData(GameData_GetSaveControl(gameData));
    u8 season;
    SymbolMapList *list;
    u8 found;

    GSYS_GetGameCommSystem(gsys);
    season = func_02017a24(gameData);
    list = GFL_HeapAllocate(heapId, sizeof(SymbolMapList), TRUE, "symbol_map.c", 466);
    *count = func_ov012_02161354(npcData, list->pokemon, 20, season, &found);
    list->count = found;
    list->unk50_6 = func_ov012_0216127c(npcData);
    list->unk50_10 = func_ov012_02161260(npcData);
    list->unk50_14 = 4;
    list->unk50_18 = season;
    return list;
}

u16 func_ov012_0216092c(GameSystem *gsys, u8 area) {
    const u8 *map = func_ov012_02160840(gsys);

    if (map != NULL) {
        return func_ov012_02160700(map, area + 1);
    }
    return 0x118;
}

u32 func_ov012_0216094c(GameSystem *gsys, u8 area, u32 dir) {
    const u8 *map = func_ov012_02160840(gsys);
    u8 next;

    if (map != NULL) {
        next = func_ov012_021607d0(map, area + 1, dir);
        return next - 1;
    }
    return 4;
}

BOOL func_ov012_02160974(u32 area) {
    u8 id = area + 1;

    if (id == 1) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_02160988(u32 area) {
    u8 id = area + 1;

    if (id == 5) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_0216099c(u32 area) {
    u8 id = area + 1;

    if (id == 2 || id == 3 || id == 4) {
        return TRUE;
    }
    return FALSE;
}
