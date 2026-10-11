#ifndef POKEBW2_APP_ZENKOKU_ZUKAN_AWARD_GRAPHIC_H
#define POKEBW2_APP_ZENKOKU_ZUKAN_AWARD_GRAPHIC_H

// The BGs, cell actors and 3D system of the national Pokédex diploma (overlay 315), named after the ROM's
// "zenkoku_zukan_award_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct ZenkokuZukanAwardGraphic ZenkokuZukanAwardGraphic;

ZenkokuZukanAwardGraphic *ZenkokuZukanAwardGraphic_Create(u32 layout, HeapID heapId);
void ZenkokuZukanAwardGraphic_Free(ZenkokuZukanAwardGraphic *graphic);
void ZenkokuZukanAwardGraphic_Update(ZenkokuZukanAwardGraphic *graphic);
ClActUnit *ZenkokuZukanAwardGraphic_GetClActUnit(ZenkokuZukanAwardGraphic *graphic);

#endif // POKEBW2_APP_ZENKOKU_ZUKAN_AWARD_GRAPHIC_H
