#include "types.h"
#include "field/entree_forest.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/save_control.h"
#include "save/symbol_save.h"
#include <stddef.h>

// The symbol encounters of the Entree Forest: the Pokémon that came from the Dream World, kept encrypted in ranges
// of slots by kind. The file's name is the ROM's string. Block 0x3c.

#define SYMBOL_SLOT_NONE 0xffff

// The slots of the last kind are the deep areas', 60 each after the first two
#define SYMBOL_DEEP_AREA_FIRST_SLOT 50
#define SYMBOL_DEEP_AREA_SLOTS 60
#define SYMBOL_DEEP_AREA_MAX 6

typedef struct {
    u16 start;
    u16 end;
} SymbolSlotRange;

static SymbolKind getKindOfSlot(u32 slot);
static u32 findFreeSlot(AreaNPCSave *npcData, u32 kind);
static void removeSlot(AreaNPCSave *npcData, u32 slot);

static const SymbolSlotRange sSlotRanges[SYMBOL_KIND_COUNT] = {
    { 0, 10 },
    { 10, 20 },
    { 20, 50 },
    { 50, SYMBOL_POKEMON_MAX },
};

// Encrypts the block with a new seed, despite swan's name
void decryptNpcData(AreaNPCSave *npcData) {
    npcData->seed = (OS_GetVBlankCount() << 8) | OS_GetVBlankCount();
    if (npcData->seed == 0) {
        npcData->seed = 1;
    }
    npcData->checksum = addUpDataStream((const u8 *)npcData, offsetof(AreaNPCSave, checksum));
    decryptData(npcData, offsetof(AreaNPCSave, checksum), npcData->checksum + (npcData->seed << 16));
}

// Decrypts the block
void SymbolSave_Decrypt(AreaNPCSave *npcData) {
    _decryptData(npcData, offsetof(AreaNPCSave, checksum), npcData->checksum + (npcData->seed << 16));
    npcData->seed = 0;
}

u32 getSizeofNpcData(void) {
    return sizeof(AreaNPCSave);
}

void initNpcData(AreaNPCSave *npcData) {
    MI_CpuFill8(npcData, 0, sizeof(AreaNPCSave));
    decryptNpcData(npcData);
}

AreaNPCSave *getAreaNPCData(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_SYMBOL);
}

static SymbolKind getKindOfSlot(u32 slot) {
    SymbolKind i;
    SymbolKind kind = SYMBOL_KIND_FREE_SMALL;

    for (i = SYMBOL_KIND_KEEP_LARGE; i < NELEMS(sSlotRanges); i++) {
        if (slot < sSlotRanges[i].end) {
            kind = i;
            break;
        }
    }
    return kind;
}

static u32 findFreeSlot(AreaNPCSave *npcData, u32 kind) {
    u32 end;
    u32 i;
    EntreeForestPokemon *pokemon;
    u32 slot = SYMBOL_SLOT_NONE;

    i = sSlotRanges[kind].start;
    end = sSlotRanges[kind].end;
    pokemon = &npcData->pokemon[i];
    for (; i < end; i++, pokemon++) {
        if (pokemon->species == 0) {
            slot = i;
            break;
        }
    }
    return slot;
}

// Moves the Pokémon after the slot down, within the slots of its kind, and frees the last
static void removeSlot(AreaNPCSave *npcData, u32 slot) {
    u32 end = sSlotRanges[getKindOfSlot(slot)].end;

    if (slot < end) {
        MI_CpuCopy8(&npcData->pokemon[slot + 1], &npcData->pokemon[slot],
                    (end - (slot + 1)) * sizeof(EntreeForestPokemon));
    }
    npcData->pokemon[end - 1].species = 0;
}

// The first free slot of a kind, or SYMBOL_SLOT_NONE
u32 SymbolSave_FindFreeSlot(AreaNPCSave *npcData, u32 kind) {
    u32 slot;

    SymbolSave_Decrypt(npcData);
    slot = findFreeSlot(npcData, kind);
    decryptNpcData(npcData);
    return slot;
}

u32 SymbolSave_GetKindOfSlot(u32 index) {
    return getKindOfSlot(index);
}

void SymbolSave_RemoveSlot(AreaNPCSave *npcData, u32 index) {
    SymbolSave_Decrypt(npcData);
    removeSlot(npcData, index);
    decryptNpcData(npcData);
}

void SymbolSave_AddPokemon(AreaNPCSave *npcData, u16 species, u16 a2, u8 sex, u8 form, u8 a5, u32 size) {
    u32 start;
    u32 end;
    u32 slot;
    EntreeForestPokemon *pokemon;

    SymbolSave_Decrypt(npcData);
    start = sSlotRanges[size].start;
    end = sSlotRanges[size].end;
    slot = findFreeSlot(npcData, size);
    if (slot == SYMBOL_SLOT_NONE) {
        removeSlot(npcData, start);
        pokemon = &npcData->pokemon[end - 1];
    } else {
        pokemon = &npcData->pokemon[slot];
    }
    pokemon->species = species;
    pokemon->move = a2;
    pokemon->sex = sex;
    pokemon->form = form;
    pokemon->actorType = a5;
    if (size == SYMBOL_KIND_FREE_LARGE) {
        if (npcData->unk848 < 1) {
            npcData->unk848++;
        }
    } else if (size == SYMBOL_KIND_FREE_SMALL) {
        SymbolSave_UpdateDeepestArea(npcData, slot);
    }
    decryptNpcData(npcData);
}

// Records how deep the slot is, once it is past the first two deep areas
void SymbolSave_UpdateDeepestArea(AreaNPCSave *npcData, u16 index) {
    u8 area = (index - SYMBOL_DEEP_AREA_FIRST_SLOT) / SYMBOL_DEEP_AREA_SLOTS - 1;

    if (index != SYMBOL_SLOT_NONE && index >= SYMBOL_DEEP_AREA_FIRST_SLOT + 2 * SYMBOL_DEEP_AREA_SLOTS &&
        index <= SYMBOL_POKEMON_MAX) {
        if (area > SYMBOL_DEEP_AREA_MAX) {
            area = SYMBOL_DEEP_AREA_MAX;
        }
        npcData->deepestArea = area;
    }
}

BOOL SymbolSave_HasPokemon(SaveControl *save, u32 *pokemon, HeapID heapId, BOOL check) {
    BOOL found = FALSE;
    AreaNPCSave *npcData = GFL_HeapAllocate(heapId, sizeof(AreaNPCSave), TRUE, "symbol_save.c", 428);
    const EntreeForestPokemon *wanted = (const EntreeForestPokemon *)pokemon;
    int copy;
    int i;

    for (copy = 0; copy < 2; copy++) {
        if (SaveControl_ReadBlockCopy(save, SAVE_BLOCK_SYMBOL, copy, npcData, sizeof(AreaNPCSave)) == TRUE) {
            SymbolSave_Decrypt(npcData);
            for (i = 0; i < SYMBOL_POKEMON_MAX; i++) {
                if (check == TRUE) {
                    if (wanted->species == npcData->pokemon[i].species && wanted->form == npcData->pokemon[i].form) {
                        found = TRUE;
                        break;
                    }
                } else if (GFL_STD_MemCmp(&npcData->pokemon[i], pokemon, sizeof(EntreeForestPokemon)) == 0) {
                    found = TRUE;
                    break;
                }
            }
        }
        if (found == TRUE) {
            break;
        }
    }
    GFL_HeapFree(npcData);
    return found;
}
