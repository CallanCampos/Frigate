#ifndef POKEBW2_SYSTEM_RESORT_LAYOUT_H
#define POKEBW2_SYSTEM_RESORT_LAYOUT_H

#include "types.h"

// The Join Avenue's fixed layout (resort_layout.c, a descriptive name): its zones, and where its people and shops
// stand. Our names

// The avenue's zones
#define RESORT_ZONE_COUNT 3

// The zone of an index, and the index of a zone (0 if it is none of them)
u16 ResortLayout_GetZoneId(u32 index);
u32 ResortLayout_GetZoneIndex(u16 zoneId);
// Positions on the grid and directions, from two tables of {u16 x, u16 z, u32 dir}: one of 48, one of 4
void ResortLayout_GetOccupantPlace(u16 index, u16 *x, u16 *z, u16 *dir);
// The offset of a shop's entity
void ResortLayout_GetShopOffset(u16 index, u16 *x, u16 *y);
void ResortLayout_GetRecordPlace(u16 index, u16 *x, u16 *z, u16 *dir);
u32 ResortLayout_GetSlotRow(u32 index);

#endif // POKEBW2_SYSTEM_RESORT_LAYOUT_H
