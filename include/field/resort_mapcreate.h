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

extern const char data_ov036_021d5744[];

ResortMapCreateWork *func_ov036_021c8954(HeapID heapId);
void func_ov036_021c897c(ResortMapCreateWork *work);
void func_ov036_021c8984(ResortMapCreateWork *work, u16 zoneId, JoinAvenueInfo *info, JoinAvenueOccupants *occupants);
u32 func_ov036_021c898c(ResortMapCreateWork *work, void *map, void *a2, void *a3, u32 count, u32 chunk, HeapID heapId);
u32 func_ov036_021c89cc(ResortMapCreateWork *work, void *map, void *a2, void *a3, u32 count, u32 chunk, HeapID heapId);
void func_ov036_021c8b40(ResortMapCreateWork *work, EventData *eventData);

#endif // POKEBW2_FIELD_RESORT_MAPCREATE_H
