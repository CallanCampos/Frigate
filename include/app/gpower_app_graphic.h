#ifndef POKEBW2_APP_GPOWER_APP_GRAPHIC_H
#define POKEBW2_APP_GPOWER_APP_GRAPHIC_H

// The BGs and cell actors of the Pass Power screen (overlay 328), named after the ROM's "gpower_app_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct GPowerAppGraphic GPowerAppGraphic;

GPowerAppGraphic *GPowerAppGraphic_Create(u32 layout, HeapID heapId);
void GPowerAppGraphic_Free(GPowerAppGraphic *graphic);
void GPowerAppGraphic_Update(GPowerAppGraphic *graphic);
// Around the frame's 3D drawing; the screen has none, so both are empty
void GPowerAppGraphic_Begin3D(GPowerAppGraphic *graphic);
void GPowerAppGraphic_End3D(GPowerAppGraphic *graphic);
ClActUnit *GPowerAppGraphic_GetClActUnit(GPowerAppGraphic *graphic);

#endif // POKEBW2_APP_GPOWER_APP_GRAPHIC_H
