#ifndef POKEBW2_SYSTEM_AREA_DATA_H
#define POKEBW2_SYSTEM_AREA_DATA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// area_data.c: an area's record, which names the graphics its zones share. The ROM doesn't name the file; the name is
// a guess. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the fields are named after
// their getters

// No animation of that kind
#define AREA_ANM_NONE 0xff

// One record of ARCID_AREADATA
struct AreaData {
    u16 propBundleId;
    u16 texSetId;
    u8 srtAnmId;
    u8 patAnmId;
    u8 isExterior;
    u8 lightsId;
    u8 edgeColorTableId;
    u8 actorMatColorId;
};

// Reads the record of areaId + variant, or record 0 if there is none. An area with seasons has one record per season
AreaData *AreaData_Create(HeapID heapId, u16 areaId, u32 variant);
void AreaData_Free(AreaData *areaData);
// The area's props, a file of ARCID_AREA_BMDATA_EXT or ARCID_AREA_BMDATA_INT
u16 AreaData_GetPropBundleID(AreaData *areaData);
u32 AreaData_GetTexSetID(AreaData *areaData);
// -1 if the area has none
u32 AreaData_GetSRTAnmID(AreaData *areaData);
u32 AreaData_GetPatAnmID(AreaData *areaData);
BOOL AreaData_IsExterior(AreaData *areaData);
u8 AreaData_GetLightsID(AreaData *areaData);
u8 AreaData_GetEdgeColorTableID(AreaData *areaData);
// The edge color table of an area without edge marking
#define AREA_EDGE_COLOR_TABLE_NONE 0xff
u8 AreaData_GetActorMatColorID(AreaData *areaData);
BOOL AreaData_HasSeasons(u16 areaId);
// Areas whose record is picked by something other than the season
BOOL AreaData_IsInfinityDungeon(u16 areaId);
BOOL AreaData_IsJoinAvenue(u16 areaId);
BOOL AreaData_IsWbt(u16 areaId);
BOOL func_02018fa8(u16 areaId);

#endif // POKEBW2_SYSTEM_AREA_DATA_H
