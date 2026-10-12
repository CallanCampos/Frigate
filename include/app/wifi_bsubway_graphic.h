#ifndef POKEBW2_APP_WIFI_BSUBWAY_GRAPHIC_H
#define POKEBW2_APP_WIFI_BSUBWAY_GRAPHIC_H

// The BGs of the Wi-Fi Battle Subway's screen (overlay 191), named after the ROM's "wifi_bsubway_graphic.c"

#include "types.h"
#include "gfl/heap.h"

typedef struct WifiBSubwayGraphic WifiBSubwayGraphic;

WifiBSubwayGraphic *WifiBSubwayGraphic_Create(u32 layout, HeapID heapId);
void WifiBSubwayGraphic_Free(WifiBSubwayGraphic *graphic);

#endif // POKEBW2_APP_WIFI_BSUBWAY_GRAPHIC_H
