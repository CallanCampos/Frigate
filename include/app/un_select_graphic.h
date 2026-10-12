#ifndef POKEBW2_APP_UN_SELECT_GRAPHIC_H
#define POKEBW2_APP_UN_SELECT_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// un_select_graphic.c, which the ROM names: the BGs, cell actors and 3D system of the Unity Tower's floor select
// (overlay 311). The names are ours

typedef struct UnSelectGraphic UnSelectGraphic;

UnSelectGraphic *UnSelectGraphic_Create(u32 layout, HeapID heapId);
void UnSelectGraphic_Free(UnSelectGraphic *graphic);
// Called every frame
void UnSelectGraphic_Update(UnSelectGraphic *graphic);
ClActUnit *UnSelectGraphic_GetClActUnit(UnSelectGraphic *graphic);

#endif // POKEBW2_APP_UN_SELECT_GRAPHIC_H
