#ifndef POKEBW2_APP_WBT_DOWNLOAD_NET_GRAPHIC_H
#define POKEBW2_APP_WBT_DOWNLOAD_NET_GRAPHIC_H

#include "types.h"
#include "gfl/heap.h"

// wbt_download_net_graphic.c, overlay 327: the BGs and the cell actor system of the tournament download's screens. The
// name is the ROM's string, from GFL_HeapAllocate's call. The function names are ours

typedef struct WbtDownloadGraphic WbtDownloadGraphic;

WbtDownloadGraphic *WbtDownloadGraphic_Create(u32 layout, HeapID heapId);
void WbtDownloadGraphic_Free(WbtDownloadGraphic *graphic);
void WbtDownloadGraphic_Update(WbtDownloadGraphic *graphic);

#endif // POKEBW2_APP_WBT_DOWNLOAD_NET_GRAPHIC_H
