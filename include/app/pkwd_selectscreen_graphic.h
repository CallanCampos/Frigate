#ifndef POKEBW2_APP_PKWD_SELECTSCREEN_GRAPHIC_H
#define POKEBW2_APP_PKWD_SELECTSCREEN_GRAPHIC_H

// The BGs and cell actors of Pokéstar Studios' movie select screen (overlay 323), named after the ROM's
// "pkwd_selectscreen_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct PkwdSelectScreenGraphic PkwdSelectScreenGraphic;

PkwdSelectScreenGraphic *PkwdSelectScreenGraphic_Create(u32 layout, HeapID heapId);
void PkwdSelectScreenGraphic_Free(PkwdSelectScreenGraphic *graphic);
void PkwdSelectScreenGraphic_Update(PkwdSelectScreenGraphic *graphic);
ClActUnit *PkwdSelectScreenGraphic_GetClActUnit(PkwdSelectScreenGraphic *graphic);

#endif // POKEBW2_APP_PKWD_SELECTSCREEN_GRAPHIC_H
