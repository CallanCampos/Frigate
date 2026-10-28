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

ResortMapCreateWork *ResortMapCreate_Create(HeapID heapId);
void ResortMapCreate_Free(ResortMapCreateWork *work);
void ResortMapCreate_Setup(ResortMapCreateWork *work, u16 zoneId, JoinAvenueInfo *info, JoinAvenueOccupants *occupants);
// In Join Avenue's zones, patches the shops that reach the map chunk chunkIndex into its terrain map and adds their
// buildings after the propCount props already there; returns the new prop count
u32 ResortMapCreate_PatchChunk(ResortMapCreateWork *work, void *map, FieldPropSystem *propSystem,
                               FieldChunk *fieldChunk, u32 propCount, u32 chunkIndex, HeapID heapId);
u32 ResortMapCreate_PatchShops(ResortMapCreateWork *work, void *map, FieldPropSystem *propSystem,
                               FieldChunk *fieldChunk, u32 propCount, u32 chunkIndex, HeapID heapId);
void ResortMapCreate_MoveShopEntities(ResortMapCreateWork *work, EventData *eventData);

#endif // POKEBW2_FIELD_RESORT_MAPCREATE_H
