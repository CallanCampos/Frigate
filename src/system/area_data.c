#include "system/area_data.h"
#include "types.h"
#include "constants/arc.h"
#include "gfl/heap.h"
#include "system/file_util.h"

// An area's record, which names the graphics its zones share. The ROM doesn't name the file; the name is a guess.
// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#define AREA_COUNT 0x199

AreaData *AreaData_Create(HeapID heapId, u16 areaId, u32 variant) {
    u16 id = areaId + (u16)variant;

    if (id >= AREA_COUNT) {
        id = 0;
    }
    return GFL_ArcSysReadRawResource(ARCID_AREADATA, heapId, id * sizeof(AreaData), sizeof(AreaData));
}

void AreaData_Free(AreaData *areaData) {
    GFL_HeapFree(areaData);
}

u16 AreaData_GetPropBundleID(AreaData *areaData) {
    return areaData->propBundleId;
}

u32 AreaData_GetTexSetID(AreaData *areaData) {
    return areaData->texSetId;
}

u32 AreaData_GetSRTAnmID(AreaData *areaData) {
    u32 id = areaData->srtAnmId;

    if (id == AREA_ANM_NONE) {
        return 0xffffffff;
    }
    return id;
}

u32 AreaData_GetPatAnmID(AreaData *areaData) {
    u32 id = areaData->patAnmId;

    if (id == AREA_ANM_NONE) {
        return 0xffffffff;
    }
    return id;
}

BOOL AreaData_IsExterior(AreaData *areaData) {
    return areaData->isExterior;
}

u8 AreaData_GetLightsID(AreaData *areaData) {
    return areaData->lightsId;
}

u8 AreaData_GetEdgeColorTableID(AreaData *areaData) {
    return areaData->edgeColorTableId;
}

u8 AreaData_GetActorMatColorID(AreaData *areaData) {
    return areaData->actorMatColorId;
}

BOOL AreaData_HasSeasons(u16 areaId) {
    if (areaId >= 2 && areaId < 0x11a) {
        return TRUE;
    }
    return FALSE;
}

BOOL AreaData_IsInfinityDungeon(u16 areaId) {
    if (areaId >= 0x166 && areaId < 0x17a) {
        return TRUE;
    }
    return FALSE;
}

BOOL AreaData_IsJoinAvenue(u16 areaId) {
    if (areaId >= 0x161 && areaId < 0x165) {
        return TRUE;
    }
    return FALSE;
}

BOOL AreaData_IsWbt(u16 areaId) {
    if (areaId >= 0x17d && areaId < 0x183) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02018fa8(u16 areaId) {
    if (areaId == 0x106) {
        return TRUE;
    }
    return FALSE;
}
