#ifndef POKEBW2_APP_PDC_MAIN_H
#define POKEBW2_APP_PDC_MAIN_H

// Overlay 172's pdc_main.c, the Entree Forest capture's scene: the battle view's graphics, the Pokémon that
// appears and the throw of a Poké Ball from the bag. pdc.c drives it

#include "types.h"
#include "app/pdc.h"
#include "gfl/heap.h"
#include "struct_decls.h"

typedef struct PdcMain PdcMain;

// Overlay 172
PdcMain *PdcMain_Create(PdcWork *pdc, HeapID heapId);
// Runs a frame: 0 while it goes on, then nonzero (1 once the Pokémon is caught, 2 when the player leaves)
u32 PdcMain_Update(PdcMain *work);
void PdcMain_Free(PdcMain *work);

#endif // POKEBW2_APP_PDC_MAIN_H
