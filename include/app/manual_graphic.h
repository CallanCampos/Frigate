#ifndef POKEBW2_APP_MANUAL_GRAPHIC_H
#define POKEBW2_APP_MANUAL_GRAPHIC_H

// The BGs and cell actors of the manual (overlay 319), named after the ROM's "manual_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct ManualGraphic ManualGraphic;

ManualGraphic *ManualGraphic_Create(u32 layout, HeapID heapId);
void ManualGraphic_Free(ManualGraphic *graphic);
void ManualGraphic_Update(ManualGraphic *graphic);
ClActUnit *ManualGraphic_GetClActUnit(ManualGraphic *graphic);

#endif // POKEBW2_APP_MANUAL_GRAPHIC_H
