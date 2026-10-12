#ifndef POKEBW2_APP_COMM_BTL_DEMO_GRAPHIC_H
#define POKEBW2_APP_COMM_BTL_DEMO_GRAPHIC_H

// The BGs, cell actors and 3D system of the link battle's introduction and result screen (overlay 305), named after
// the ROM's "comm_btl_demo_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct CommBtlDemoGraphic CommBtlDemoGraphic;

CommBtlDemoGraphic *CommBtlDemoGraphic_Create(u32 layout, HeapID heapId);
void CommBtlDemoGraphic_Free(CommBtlDemoGraphic *graphic);
void CommBtlDemoGraphic_Update(CommBtlDemoGraphic *graphic);
// Around the frame's 3D drawing
void CommBtlDemoGraphic_Begin3D(CommBtlDemoGraphic *graphic);
void CommBtlDemoGraphic_End3D(CommBtlDemoGraphic *graphic);
ClActUnit *CommBtlDemoGraphic_GetClActUnit(CommBtlDemoGraphic *graphic);

#endif // POKEBW2_APP_COMM_BTL_DEMO_GRAPHIC_H
