#ifndef POKEBW2_APP_CHIHOU_ZUKAN_AWARD_GRAPHIC_H
#define POKEBW2_APP_CHIHOU_ZUKAN_AWARD_GRAPHIC_H

// The BGs, cell actors and 3D system of the regional Pokédex diploma (overlay 314), named after the ROM's
// "chihou_zukan_award_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct ChihouZukanAwardGraphic ChihouZukanAwardGraphic;

ChihouZukanAwardGraphic *ChihouZukanAwardGraphic_Create(u32 layout, HeapID heapId);
void ChihouZukanAwardGraphic_Free(ChihouZukanAwardGraphic *graphic);
void ChihouZukanAwardGraphic_Update(ChihouZukanAwardGraphic *graphic);
ClActUnit *ChihouZukanAwardGraphic_GetClActUnit(ChihouZukanAwardGraphic *graphic);

#endif // POKEBW2_APP_CHIHOU_ZUKAN_AWARD_GRAPHIC_H
