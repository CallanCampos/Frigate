#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_GRAPHIC_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_GRAPHIC_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_graphic.c

LocalTvtGraphic *LocalTvtGraphic_Create(u32 layout, HeapID heapId);
void LocalTvtGraphic_Free(LocalTvtGraphic *graphic);
void LocalTvtGraphic_Update(LocalTvtGraphic *graphic);
ClActUnit *LocalTvtGraphic_GetClActUnit(LocalTvtGraphic *graphic);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_GRAPHIC_H
