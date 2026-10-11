#ifndef POKEBW2_APP_ZUKAN_TOROKU_GRAPHIC_H
#define POKEBW2_APP_ZUKAN_TOROKU_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The BGs and the cell actor system of the Pokédex registration, overlay 297 (zukan_toroku_graphic.c, by the ROM's
// string). The same template as the egg demo's and the intro's graphic, without their 3D system

typedef struct ZukanTorokuGraphic ZukanTorokuGraphic;

ZukanTorokuGraphic *ZukanTorokuGraphic_Create(u32 layout, HeapID heapId);
void ZukanTorokuGraphic_Free(ZukanTorokuGraphic *graphic);
void ZukanTorokuGraphic_Update(ZukanTorokuGraphic *graphic);
void ZukanTorokuGraphic_Begin3D(ZukanTorokuGraphic *graphic);
void ZukanTorokuGraphic_End3D(ZukanTorokuGraphic *graphic);
ClActUnit *ZukanTorokuGraphic_GetClActUnit(ZukanTorokuGraphic *graphic);

#endif // POKEBW2_APP_ZUKAN_TOROKU_GRAPHIC_H
