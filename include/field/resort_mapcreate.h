#ifndef POKEBW2_FIELD_RESORT_MAPCREATE_H
#define POKEBW2_FIELD_RESORT_MAPCREATE_H

// Overlay 36's resort_mapcreate.c, which builds Join Avenue's shops into its map and moves their entities

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

struct ResortMapCreateWork {
    u16 heapId;
    u16 padding;
    u32 zoneId;
    JoinAvenueInfo *info;
    JoinAvenueOccupants *occupants;
};

ResortMapCreateWork *func_ov036_021c8954(HeapID heapId);
void func_ov036_021c897c(ResortMapCreateWork *work);
void func_ov036_021c8984(ResortMapCreateWork *work, u16 zoneId, JoinAvenueInfo *info, JoinAvenueOccupants *occupants);
// In Join Avenue's zones, patches the shops that reach the map chunk chunkIndex into its terrain map and adds their
// buildings after the propCount props already there; returns the new prop count
u32 func_ov036_021c898c(ResortMapCreateWork *work, void *map, FieldPropSystem *propSystem, FieldChunk *fieldChunk,
                        u32 propCount, u32 chunkIndex, HeapID heapId);
u32 func_ov036_021c89cc(ResortMapCreateWork *work, void *map, FieldPropSystem *propSystem, FieldChunk *fieldChunk,
                        u32 propCount, u32 chunkIndex, HeapID heapId);
void func_ov036_021c8b40(ResortMapCreateWork *work, EventData *eventData);

#endif // POKEBW2_FIELD_RESORT_MAPCREATE_H
