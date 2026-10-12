#ifndef POKEBW2_APP_PSEL_GRAPHIC_H
#define POKEBW2_APP_PSEL_GRAPHIC_H

// The BGs, cell actors and 3D system of the starter Pokémon choice (overlay 316), named after the ROM's
// "psel_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"

typedef struct PselGraphic PselGraphic;

PselGraphic *PselGraphic_Create(u32 layout, HeapID heapId);
void PselGraphic_Free(PselGraphic *graphic);
void PselGraphic_Update(PselGraphic *graphic);
// Around the frame's 3D drawing
void PselGraphic_Begin3D(PselGraphic *graphic);
void PselGraphic_End3D(PselGraphic *graphic);
ClActUnit *PselGraphic_GetClActUnit(PselGraphic *graphic);
G3DCamera *PselGraphic_GetCamera(PselGraphic *graphic);

#endif // POKEBW2_APP_PSEL_GRAPHIC_H
