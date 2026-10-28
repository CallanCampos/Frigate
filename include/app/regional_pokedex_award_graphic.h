#ifndef POKEBW2_APP_REGIONAL_POKEDEX_AWARD_GRAPHIC_H
#define POKEBW2_APP_REGIONAL_POKEDEX_AWARD_GRAPHIC_H

// The BGs, cell actors and 3D system of the regional Pokédex diploma (overlay 314), named after the ROM's
// "chihou_zukan_award_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct RegionalPokedexAwardGraphic RegionalPokedexAwardGraphic;

RegionalPokedexAwardGraphic *RegionalPokedexAwardGraphic_Create(u32 layout, HeapID heapId);
void RegionalPokedexAwardGraphic_Free(RegionalPokedexAwardGraphic *graphic);
void RegionalPokedexAwardGraphic_Update(RegionalPokedexAwardGraphic *graphic);
ClActUnit *RegionalPokedexAwardGraphic_GetClActUnit(RegionalPokedexAwardGraphic *graphic);

#endif // POKEBW2_APP_REGIONAL_POKEDEX_AWARD_GRAPHIC_H
