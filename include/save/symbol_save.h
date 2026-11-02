#ifndef POKEBW2_SAVE_SYMBOL_SAVE_H
#define POKEBW2_SAVE_SYMBOL_SAVE_H

#include "types.h"
#include "field/entree_forest.h"
#include "struct_decls.h"

// Block 0x3c, the symbol encounters of the Entree Forest (symbol_save.c, the ROM's name). The other functions of the
// file are in save/save_control.h.

// The forest's Pokémon are kept in four ranges of slots, by kind: large and small ones, of the areas the player
// keeps them in and of the areas they arrive in
typedef enum {
    SYMBOL_KIND_KEEP_LARGE,
    SYMBOL_KIND_KEEP_SMALL,
    SYMBOL_KIND_FREE_LARGE,
    SYMBOL_KIND_FREE_SMALL,
    SYMBOL_KIND_COUNT,
} SymbolKind;

#define SYMBOL_POKEMON_MAX 530
// No slot, which SymbolSave_FindFreeSlot returns when its kind's slots are full
#define SYMBOL_SLOT_NONE 0xffff

struct AreaNPCSave {
    EntreeForestPokemon pokemon[SYMBOL_POKEMON_MAX];
    // 1 once a Pokémon of SYMBOL_KIND_FREE_LARGE has arrived, which picks the forest's map with deepestArea
    u8 group;
    // The deepest area that got a Pokémon of the last kind, up to 6
    u8 deepestArea;
    u8 padding84A[2];
    // The block is encrypted but for these two
    u16 checksum;
    u16 seed;
};

// The block's size and its initial state
u32 getSizeofNpcData(void);
void initNpcData(AreaNPCSave *npcData);

#endif // POKEBW2_SAVE_SYMBOL_SAVE_H
