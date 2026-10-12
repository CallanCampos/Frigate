#ifndef POKEBW2_APP_WBT_LIST_GRAPHIC_H
#define POKEBW2_APP_WBT_LIST_GRAPHIC_H

// The BGs and cell actors of the Pokémon World Tournament's list of Trainers (overlay 320), named after the ROM's
// "wbt_list_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct WbtListGraphic WbtListGraphic;

WbtListGraphic *WbtListGraphic_Create(u32 layout, HeapID heapId);
void WbtListGraphic_Free(WbtListGraphic *graphic);
void WbtListGraphic_Update(WbtListGraphic *graphic);
// Around the frame's 3D drawing, of which the screen has none
void WbtListGraphic_Begin3D(WbtListGraphic *graphic);
void WbtListGraphic_End3D(WbtListGraphic *graphic);
ClActUnit *WbtListGraphic_GetClActUnit(WbtListGraphic *graphic);

#endif // POKEBW2_APP_WBT_LIST_GRAPHIC_H
