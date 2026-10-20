#ifndef POKEBW2_FIELD_SYMBOL_MAP_H
#define POKEBW2_FIELD_SYMBOL_MAP_H

// Overlay 12's symbol_map.c: the maps of the Entree Forest's areas, three wide and ten deep, by how far the forest has
// grown, and the symbol encounters in them

#include "types.h"
#include "field/entree_forest.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The symbol encounters of the forest, which SymbolMap_CreatePokemonList gives
typedef struct {
    EntreeForestPokemon pokemon[20];
    u32 count : 6;
    u32 unk50_6 : 4;
    u32 unk50_10 : 4;
    u32 unk50_14 : 4;
    u32 unk50_18 : 6;
    u32 unk50_24 : 8;
} SymbolMapList;

BOOL SymbolMap_ToggleKeepPokemon(AreaNPCSave *npcData, u32 index);
SymbolMapList *SymbolMap_CreatePokemonList(HeapID heapId, GameSystem *gsys, u32 *count);
// The zone of an area of the forest
u16 SymbolMap_GetZone(GameSystem *gsys, u8 area);
// The area next to an area of the forest, in a direction
u32 SymbolMap_GetAdjacentArea(GameSystem *gsys, u8 area, u32 dir);
BOOL SymbolMap_IsTopArea(u32 area);
BOOL SymbolMap_IsEntranceArea(u32 area);
BOOL SymbolMap_IsUpperRowArea(u32 area);

#endif // POKEBW2_FIELD_SYMBOL_MAP_H
