#ifndef POKEBW2_APP_BTL_REC_SEL_GRAPHIC_H
#define POKEBW2_APP_BTL_REC_SEL_GRAPHIC_H

// The BGs, cell actors and 3D system of the "save this battle video?" screen (overlay 306), named after the ROM's
// "btl_rec_sel_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct BtlRecSelGraphic BtlRecSelGraphic;

BtlRecSelGraphic *BtlRecSelGraphic_Create(u32 layout, HeapID heapId);
void BtlRecSelGraphic_Free(BtlRecSelGraphic *graphic);
void BtlRecSelGraphic_Update(BtlRecSelGraphic *graphic);
// Around the frame's 3D drawing
void BtlRecSelGraphic_Begin3D(BtlRecSelGraphic *graphic);
void BtlRecSelGraphic_End3D(BtlRecSelGraphic *graphic);
ClActUnit *BtlRecSelGraphic_GetClActUnit(BtlRecSelGraphic *graphic);

#endif // POKEBW2_APP_BTL_REC_SEL_GRAPHIC_H
