#ifndef POKEBW2_APP_FMISSION_APP_GRA_H
#define POKEBW2_APP_FMISSION_APP_GRA_H

// The BGs, cell actors and 3D system of the Funfest mission screen (overlay 309), named after the ROM's
// "fmission_app_gra.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct FMissionAppGra FMissionAppGra;

FMissionAppGra *FMissionAppGra_Create(u32 layout, HeapID heapId);
void FMissionAppGra_Free(FMissionAppGra *graphic);
void FMissionAppGra_Update(FMissionAppGra *graphic);
// Around the frame's 3D drawing
void FMissionAppGra_Begin3D(FMissionAppGra *graphic);
void FMissionAppGra_End3D(FMissionAppGra *graphic);
ClActUnit *FMissionAppGra_GetClActUnit(FMissionAppGra *graphic);

#endif // POKEBW2_APP_FMISSION_APP_GRA_H
