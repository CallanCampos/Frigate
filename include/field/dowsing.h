#ifndef POKEBW2_FIELD_DOWSING_H
#define POKEBW2_FIELD_DOWSING_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// The Dowsing MCHN's touch screen (FLD_SUBSCREEN_ID_DOWSING), overlay 86, named after the ROM's "dowsing.c". It shows
// how near the closest hidden item is. Overlay 36's field subscreen table calls these
#define OVERLAY_DOWSING OVERLAY_ID(86)

typedef struct DowsingWork DowsingWork;

DowsingWork *Dowsing_Create(HeapID heapId, HeapID subscreenHeapId, FieldSubscreen *subscreen, Field *field);
void Dowsing_Free(DowsingWork *work);
// Both take whether the field lets the screen act, which is not while an event runs
void Dowsing_Update(DowsingWork *work, BOOL active);
void Dowsing_UpdateAltFrame(DowsingWork *work, BOOL active);

#endif // POKEBW2_FIELD_DOWSING_H
