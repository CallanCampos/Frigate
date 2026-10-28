#ifndef POKEBW2_APP_NATIONAL_POKEDEX_AWARD_GRAPHIC_H
#define POKEBW2_APP_NATIONAL_POKEDEX_AWARD_GRAPHIC_H

// The BGs, cell actors and 3D system of the national Pokédex diploma (overlay 315), named after the ROM's
// "zenkoku_zukan_award_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct NationalPokedexAwardGraphic NationalPokedexAwardGraphic;

NationalPokedexAwardGraphic *NationalPokedexAwardGraphic_Create(u32 layout, HeapID heapId);
void NationalPokedexAwardGraphic_Free(NationalPokedexAwardGraphic *graphic);
void NationalPokedexAwardGraphic_Update(NationalPokedexAwardGraphic *graphic);
ClActUnit *NationalPokedexAwardGraphic_GetClActUnit(NationalPokedexAwardGraphic *graphic);

#endif // POKEBW2_APP_NATIONAL_POKEDEX_AWARD_GRAPHIC_H
