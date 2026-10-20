#include "types.h"
#include "constants/items.h"
#include "gfl/std.h"
#include "save/dream_world.h"
#include "save/save_control.h"

// The save block of the Dream World: the Pokémon sent there to sleep, the items and the result it sent back,
// and the state of the Game Sync. The ROM has no string for the file, so the name is a guess (swan's
// DreamWorldSave and getDreamWorldStuffAddress). Block 0x23.

#define SAVE_BLOCK_DREAM_WORLD 0x23

// Year 0xda, January 1st, Sunday, which no real sync date can be
#define DREAM_WORLD_SYNC_DATE_NONE 0xda010100

u32 DreamWorldSave_GetSize(void) {
    return sizeof(DreamWorldSave);
}

void DreamWorldSave_Init(DreamWorldSave *dreamWorld) {
    u8 i = 0;

    MI_CpuFill8(dreamWorld, 0, DreamWorldSave_GetSize());
    DreamWorldSave_SetSyncDate(dreamWorld, DREAM_WORLD_SYNC_DATE_NONE);
    dreamWorld->lastResultId = -1;
    dreamWorld->unk1A7 = 0;
    dreamWorld->unk1A8 = 0;
    dreamWorld->unk1A9 = 0;
    dreamWorld->resultIndex = DREAM_WORLD_RESULT_INDEX_NONE;
    for (i = 0; i < DREAM_WORLD_RESULT_MAX; i++) {
        dreamWorld->results[i][0] = DREAM_WORLD_RESULT_NONE;
    }
}

PartyPkm *DreamWorldSave_GetPkm(DreamWorldSave *dreamWorld) {
    return &dreamWorld->pkm;
}

void DreamWorldSave_SetPkm(DreamWorldSave *dreamWorld, PartyPkm *pkm) {
    MI_CpuCopy8(pkm, &dreamWorld->pkm, PokeParty_GetPkmRawSize());
}

BOOL DreamWorldSave_TestSpeciesFlag(const u8 *flags, int species) {
    // The byte index is unsigned and the bit signed, as the original divides
    if (flags[(u32)species >> 3] & (1 << (species % 8))) {
        return TRUE;
    }
    return FALSE;
}

void func_020099d8(DreamWorldSave *dreamWorld, u8 value) {
    dreamWorld->unk1A4 = value;
}

u8 func_020099e0(DreamWorldSave *dreamWorld) {
    return dreamWorld->unk1A4;
}

void DreamWorldSave_SetIDShown(DreamWorldSave *dreamWorld, u8 value) {
    dreamWorld->idShown = value;
}

u8 DreamWorldSave_GetIDShown(DreamWorldSave *dreamWorld) {
    return dreamWorld->idShown;
}

void DreamWorldSave_SetPokemonAsleep(DreamWorldSave *dreamWorld, u8 asleep) {
    dreamWorld->asleep = asleep;
}

BOOL DreamWorldSave_IsPokemonAsleep(DreamWorldSave *dreamWorld) {
    return dreamWorld->asleep;
}

// BUG: the three item functions accept index DREAM_WORLD_ITEM_MAX, one past the arrays, which reads and writes
// the first item counts and the first result
u16 DreamWorldSave_GetItem(DreamWorldSave *dreamWorld, int index) {
#ifdef BUGFIX
    if (index >= DREAM_WORLD_ITEM_MAX) {
#else
    if (index > DREAM_WORLD_ITEM_MAX) {
#endif
        return 0;
    }
    if (dreamWorld->items[index] > ITEM_LAST) {
        return 0;
    }
    return dreamWorld->items[index];
}

u8 DreamWorldSave_GetItemCount(DreamWorldSave *dreamWorld, int index) {
#ifdef BUGFIX
    if (index >= DREAM_WORLD_ITEM_MAX) {
#else
    if (index > DREAM_WORLD_ITEM_MAX) {
#endif
        return 0;
    }
    if (dreamWorld->itemCounts[index] > DREAM_WORLD_ITEM_COUNT_MAX) {
        return 0;
    }
    return dreamWorld->itemCounts[index];
}

void DreamWorldSave_SetItem(DreamWorldSave *dreamWorld, int index, u16 item, u8 count) {
#ifdef BUGFIX
    if (index < DREAM_WORLD_ITEM_MAX) {
#else
    if (index <= DREAM_WORLD_ITEM_MAX) {
#endif
        dreamWorld->items[index] = item;
        dreamWorld->itemCounts[index] = count;
    }
}

void DreamWorldSave_ClearItem(DreamWorldSave *dreamWorld, int index) {
    DreamWorldSave_SetItem(dreamWorld, index, 0, 0);
}

s32 DreamWorldSave_CountItems(DreamWorldSave *dreamWorld) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < DREAM_WORLD_ITEM_MAX; i++) {
        if (DreamWorldSave_GetItemCount(dreamWorld, i) != 0) {
            count++;
        }
    }
    return count;
}

u16 *DreamWorldSave_GetResult(DreamWorldSave *dreamWorld, int index) {
    if (index < DREAM_WORLD_RESULT_MAX) {
        return dreamWorld->results[index];
    }
    return NULL;
}

void DreamWorldSave_SetResult(DreamWorldSave *dreamWorld, int index, const void *entry) {
    if (index < DREAM_WORLD_RESULT_MAX) {
        MI_CpuCopy8(entry, dreamWorld->results[index], sizeof(dreamWorld->results[index]));
    }
}

u32 DreamWorldSave_GetSyncDate(DreamWorldSave *dreamWorld) {
    return dreamWorld->syncDate;
}

void DreamWorldSave_SetSyncDate(DreamWorldSave *dreamWorld, u32 date) {
    dreamWorld->syncDate = date;
}

u32 DreamWorldSave_GetLastResultID(DreamWorldSave *dreamWorld) {
    return dreamWorld->lastResultId;
}

void DreamWorldSave_SetLastResultID(DreamWorldSave *dreamWorld, u32 id) {
    dreamWorld->lastResultId = id;
}

u32 DreamWorldSave_GetResultIndex(DreamWorldSave *dreamWorld) {
    u32 value = dreamWorld->resultIndex;
    if (value >= DREAM_WORLD_RESULT_MAX && value != DREAM_WORLD_RESULT_INDEX_NONE) {
        return DREAM_WORLD_RESULT_INDEX_NONE;
    }
    return value;
}

void DreamWorldSave_SetResultIndex(DreamWorldSave *dreamWorld, int value) {
    if (value < DREAM_WORLD_RESULT_MAX || value == DREAM_WORLD_RESULT_INDEX_NONE) {
        dreamWorld->resultIndex = (u8)value;
    }
}

u32 func_02009b20(DreamWorldSave *dreamWorld) {
    return dreamWorld->unk1A6_7;
}

void func_02009b30(DreamWorldSave *dreamWorld, u32 value) {
    dreamWorld->unk1A6_7 = (u8)value;
}

DreamWorldSave *getDreamWorldStuffAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_DREAM_WORLD);
}
