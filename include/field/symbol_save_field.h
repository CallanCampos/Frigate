#ifndef POKEBW2_FIELD_SYMBOL_SAVE_FIELD_H
#define POKEBW2_FIELD_SYMBOL_SAVE_FIELD_H

// Overlay 12's symbol_save_field.c (a descriptive name): what the field reads of the symbol encounters of the Entree
// Forest, which the save keeps in its area NPC data, encrypted

#include "types.h"
#include "field/entree_forest.h"
#include "struct_decls.h"
#include "save/symbol_save.h"

u32 func_ov012_02161260(AreaNPCSave *npcData);
u32 func_ov012_0216127c(AreaNPCSave *npcData);
// The Pokémon of an area, and how many there are
u32 func_ov012_02161354(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int area, u8 *count);

#endif // POKEBW2_FIELD_SYMBOL_SAVE_FIELD_H
