#ifndef POKEBW2_APP_BEACON_DETAIL_GRA_H
#define POKEBW2_APP_BEACON_DETAIL_GRA_H

// The BGs and cell actors of the screen of a beacon's details (overlay 308), named after the ROM's
// "beacon_detail_gra.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct BeaconDetailGraphic BeaconDetailGraphic;

BeaconDetailGraphic *BeaconDetailGraphic_Create(u32 layout, HeapID heapId);
void BeaconDetailGraphic_Free(BeaconDetailGraphic *graphic);
void BeaconDetailGraphic_Update(BeaconDetailGraphic *graphic);
ClActUnit *BeaconDetailGraphic_GetClActUnit(BeaconDetailGraphic *graphic);

#endif // POKEBW2_APP_BEACON_DETAIL_GRA_H
