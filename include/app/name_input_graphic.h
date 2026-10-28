#ifndef POKEBW2_APP_NAME_INPUT_GRAPHIC_H
#define POKEBW2_APP_NAME_INPUT_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// name_input_graphic.c (overlay 280): the BGs and cell actors of the name-entry screen

typedef struct NameInputGraphic NameInputGraphic;

// Sets up the display layout, the BGs and the cell actor system, and adds a VBlank task
NameInputGraphic *NameInputGraphic_Create(u32 layout, HeapID heapId);
void NameInputGraphic_Delete(NameInputGraphic *graphic);
// Called every frame
void NameInputGraphic_Main(NameInputGraphic *graphic);
ClActUnit *NameInputGraphic_GetClActUnit(NameInputGraphic *graphic);

#endif // POKEBW2_APP_NAME_INPUT_GRAPHIC_H
