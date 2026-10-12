#include "types.h"
#include "field/entree_forest.h"
#include "field/symbol_save_field.h"
#include "gfl/std.h"
#include "save/save_control.h"

static u8 func_ov012_02161298(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int start, int count);
static u32 func_ov012_021612e4(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, u8 *count);
static u32 func_ov012_021612fc(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, u8 *count);
static u16 func_ov012_02161314(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int area, u8 *count);
static u16 func_ov012_02161334(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int area, u8 *count);

u32 func_ov012_02161260(AreaNPCSave *npcData) {
    u8 value;

    func_0200e904(npcData);
    value = npcData->unk848;
    decryptNpcData(npcData);
    return value;
}

u32 func_ov012_0216127c(AreaNPCSave *npcData) {
    u8 value;

    func_0200e904(npcData);
    value = npcData->deepestArea;
    decryptNpcData(npcData);
    return value;
}

// Copies count Pokémon from start, and returns how many there are before the first empty one
static u8 func_ov012_02161298(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int start, int count) {
    int i = 0;

    sys_memset(dest, 0, max * sizeof(EntreeForestPokemon));
    func_0200e904(npcData);
    sys_memcpy(&npcData->pokemon[start], dest, count * sizeof(EntreeForestPokemon));
    decryptNpcData(npcData);
    for (; i < count; i++) {
        if (dest[i].species == 0) {
            break;
        }
    }
    return i;
}

static u32 func_ov012_021612e4(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, u8 *count) {
    *count = func_ov012_02161298(npcData, dest, max, 0, 10);
    return 0;
}

static u32 func_ov012_021612fc(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, u8 *count) {
    *count = func_ov012_02161298(npcData, dest, max, 10, 10);
    return 10;
}

static u16 func_ov012_02161314(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int area, u8 *count) {
    u16 start = area * 10 + 20;

    *count = func_ov012_02161298(npcData, dest, max, start, 10);
    return start;
}

static u16 func_ov012_02161334(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int area, u8 *count) {
    u16 start = area * 20 + 50;

    *count = func_ov012_02161298(npcData, dest, max, start, 20);
    return start;
}

u32 func_ov012_02161354(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int area, u8 *count) {
    u8 extra = 0;
    u32 start;

    sys_memset(dest, 0, max * sizeof(EntreeForestPokemon));
    if (area == 0) {
        start = func_ov012_021612e4(npcData, dest, max, count);
        func_ov012_021612fc(npcData, dest + 10, max - 10, &extra);
        *count += extra;
    } else if (area >= 1 && area < 4) {
        start = func_ov012_02161314(npcData, dest, max, area - 1, count);
    } else {
        start = func_ov012_02161334(npcData, dest, max, area - 4, count);
    }
    return start;
}
