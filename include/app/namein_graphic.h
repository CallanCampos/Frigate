#ifndef POKEBW2_APP_NAMEIN_GRAPHIC_H
#define POKEBW2_APP_NAMEIN_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// namein_graphic.c (overlay 280): the BGs and cell actors of the name-entry screen

typedef struct NameinGraphic NameinGraphic;

// Sets up the display layout, the BGs and the cell actor system, and adds a VBlank task
NameinGraphic *NameinGraphic_Create(u32 layout, HeapID heapId);
void NameinGraphic_Delete(NameinGraphic *graphic);
// Called every frame
void NameinGraphic_Main(NameinGraphic *graphic);
ClActUnit *NameinGraphic_GetClActUnit(NameinGraphic *graphic);

#endif // POKEBW2_APP_NAMEIN_GRAPHIC_H
