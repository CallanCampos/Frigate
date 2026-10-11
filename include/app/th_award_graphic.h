#ifndef POKEBW2_APP_TH_AWARD_GRAPHIC_H
#define POKEBW2_APP_TH_AWARD_GRAPHIC_H

// The BGs and cell actors of the Trial House's results screen (overlay 313), named after the ROM's "th_award_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct ThAwardGraphic ThAwardGraphic;

ThAwardGraphic *ThAwardGraphic_Create(u32 layout, HeapID heapId);
void ThAwardGraphic_Free(ThAwardGraphic *graphic);
void ThAwardGraphic_Update(ThAwardGraphic *graphic);
ClActUnit *ThAwardGraphic_GetClActUnit(ThAwardGraphic *graphic);

#endif // POKEBW2_APP_TH_AWARD_GRAPHIC_H
