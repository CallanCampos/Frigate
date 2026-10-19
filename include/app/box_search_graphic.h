#ifndef POKEBW2_APP_BOX_SEARCH_GRAPHIC_H
#define POKEBW2_APP_BOX_SEARCH_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// box_search_graphic.c: the BGs and cell actors of the PC box's Pokémon search

typedef struct BoxSearchGraphic BoxSearchGraphic;

BoxSearchGraphic *BoxSearchGraphic_Create(u32 layout, HeapID heapId);
void BoxSearchGraphic_Delete(BoxSearchGraphic *graphic);
// Called every frame
void BoxSearchGraphic_Update(BoxSearchGraphic *graphic);
// Begin and end the frame's 3D, which the search doesn't draw
void BoxSearchGraphic_Begin3D(BoxSearchGraphic *graphic);
void BoxSearchGraphic_End3D(BoxSearchGraphic *graphic);
ClActUnit *BoxSearchGraphic_GetClActUnit(BoxSearchGraphic *graphic);

#endif // POKEBW2_APP_BOX_SEARCH_GRAPHIC_H
