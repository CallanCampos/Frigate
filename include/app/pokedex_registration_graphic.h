#ifndef POKEBW2_APP_POKEDEX_REGISTRATION_GRAPHIC_H
#define POKEBW2_APP_POKEDEX_REGISTRATION_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The BGs and the cell actor system of the Pokédex registration, overlay 297 (pokedex_registration_graphic.c,
// zukan_toroku_graphic.c by the ROM's string). The same template as the egg demo's and the intro's graphic, without
// their 3D system

typedef struct PokedexRegistrationGraphic PokedexRegistrationGraphic;

PokedexRegistrationGraphic *PokedexRegistrationGraphic_Create(u32 layout, HeapID heapId);
void PokedexRegistrationGraphic_Free(PokedexRegistrationGraphic *graphic);
void PokedexRegistrationGraphic_Update(PokedexRegistrationGraphic *graphic);
void PokedexRegistrationGraphic_Begin3D(PokedexRegistrationGraphic *graphic);
void PokedexRegistrationGraphic_End3D(PokedexRegistrationGraphic *graphic);
ClActUnit *PokedexRegistrationGraphic_GetClActUnit(PokedexRegistrationGraphic *graphic);

#endif // POKEBW2_APP_POKEDEX_REGISTRATION_GRAPHIC_H
