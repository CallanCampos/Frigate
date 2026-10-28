#include "field/field_nogrid_mapper.h"
#include "types.h"
#include "constants/arc.h"
#include "field/field_rail.h"
#include "field/field_rail_loader.h"
#include "field/field_scene_area.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"

// The mapper of the maps without a grid, where the player walks on rails: the rail system, the loader of its data,
// the rails' tile attributes and the camera areas. The layout is swan's (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0)

struct NoGridMapper {
    FieldRailSystem *railSystem;
    RailLoader *railLoader;
    RailAttr *tilemap;
    FieldSceneArea *sceneArea;
    FieldSceneAreaLoader *sceneAreaLoader;
    BOOL cameraAreaDisabled;
};

// A map's file in ARCID_RAIL_DATA
// A header's railFileId or cameraId when the map has no rails or no camera areas
#define NOGRID_RAIL_NONE 0xffff
#define NOGRID_CAMERA_NONE 0xffff

typedef struct {
    // The rail data in ARCID_RAIL_HEADERS, whose second half holds the rails' tile attributes in the same order, or
    // NOGRID_RAIL_NONE
    u16 railFileId;
    // The camera areas, or NOGRID_CAMERA_NONE
    u16 cameraId;
} NoGridMapperHeader;

static void FieldNoGridMapper_Load(NoGridMapper *mapper, const NoGridMapperHeader *header, u32 heapId);
static void FieldNoGridMapper_Reset(NoGridMapper *mapper);

NoGridMapper *FieldNoGridMapper_Create(u32 heapId, FieldCamera *camera, FieldSceneArea *sceneArea,
                                       FieldSceneAreaLoader *sceneAreaLoader) {
    NoGridMapper *mapper = GFL_HeapAllocate(heapId, sizeof(NoGridMapper), TRUE, "field_nogrid_mapper.c", 71);

    mapper->railSystem = FieldRailSystem_Create(heapId, 48, camera);
    mapper->railLoader = FieldRailLoader_Create(heapId);
    mapper->sceneArea = sceneArea;
    mapper->sceneAreaLoader = sceneAreaLoader;
    mapper->tilemap = AllocateRailAttrBlock(heapId);
    return mapper;
}

void FieldNoGridMapper_Free(NoGridMapper *mapper) {
    FieldRailTilemap_Delete(mapper->tilemap);
    FieldRailSystem_Free(mapper->railSystem);
    FieldRailLoader_Free(mapper->railLoader);
    GFL_HeapFree(mapper);
}

void FieldNoGridMapper_SetCameraAreaEnabled(NoGridMapper *mapper, BOOL enabled) {
    if (enabled) {
        mapper->cameraAreaDisabled = FALSE;
    } else {
        mapper->cameraAreaDisabled = TRUE;
    }
    SetFieldSceneAreaCameraAreaEnable(mapper->sceneArea, enabled);
}

static void FieldNoGridMapper_Load(NoGridMapper *mapper, const NoGridMapperHeader *header, u32 heapId) {
    FieldNoGridMapper_Reset(mapper);
    if (header->railFileId != NOGRID_RAIL_NONE) {
        u32 fileCount = GFL_ArcSysGetDataMax(ARCID_RAIL_HEADERS);

        FieldRailLoader_LoadFile(mapper->railLoader, header->railFileId, heapId);
        FieldRailSystem_Load(mapper->railSystem, FieldRailLoader_GetRailData(mapper->railLoader));
        FieldRailTilemap_Load(mapper->tilemap, header->railFileId + fileCount / 2, heapId);
    }
    if (header->cameraId != NOGRID_CAMERA_NONE) {
        LoadCameraDataToSceneAreaLoader(mapper->sceneAreaLoader, 0, header->cameraId, heapId);
        BuildSceneArea(mapper->sceneArea, GetFldSceneAreaLoaderCameraData(mapper->sceneAreaLoader),
                       GetFldSceneAreaLoaderCamCount(mapper->sceneAreaLoader),
                       GetFldSceneAreaLoaderCamFuncsStaticOffs(mapper->sceneAreaLoader));
    }
}

void FieldNoGridMapper_LoadByHeader(NoGridMapper *mapper, u32 railId, u32 heapId) {
    NoGridMapperHeader *header =
        GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), GFL_ArcSysGetDataLength(ARCID_RAIL_DATA, railId), TRUE,
                         "field_nogrid_mapper.c", 221);

    GFL_ArcSysRead(header, ARCID_RAIL_DATA, railId);
    FieldNoGridMapper_Load(mapper, header, heapId);
    GFL_HeapFree(header);
}

static void FieldNoGridMapper_Reset(NoGridMapper *mapper) {
    ResetSceneArea(mapper->sceneArea);
    FieldRailSystem_Reset(mapper->railSystem);
    FieldRailLoader_Reset(mapper->railLoader);
    ResetSceneAreaLoader(mapper->sceneAreaLoader);
    FieldRailTilemap_Release(mapper->tilemap);
}

BOOL FieldNoGridMapper_HasRailData(NoGridMapper *mapper) {
    if (FieldRailSystem_HasData(mapper->railSystem)) {
        return TRUE;
    }
    return FALSE;
}

void FieldNoGridMapper_UpdateCamera(NoGridMapper *mapper) {
    FieldRailSystem_Update(mapper->railSystem);
    if (!mapper->cameraAreaDisabled) {
        FieldRailSystem_UpdateCamera(mapper->railSystem, FALSE);
    }
}

void FieldNoGridMapper_ForceUpdateCamera(NoGridMapper *mapper) {
    FieldRailSystem_UpdateCamera(mapper->railSystem, TRUE);
}

RailUnit *FieldNoGridMapper_AcquireUnit(NoGridMapper *mapper) {
    return FieldRailSystem_AcquireUnit(mapper->railSystem);
}

void FieldNoGridMapper_ReleaseUnit(NoGridMapper *mapper, RailUnit *unit) {
    FieldRailSystem_ReleaseUnit(mapper->railSystem, unit);
}

void FieldNoGridMapper_SetCameraParent(NoGridMapper *mapper, RailUnit *unit) {
    FieldRailSystem_SetCameraParent(mapper->railSystem, unit);
}

void FieldNoGridMapper_ClearCameraParent(NoGridMapper *mapper) {
    FieldRailSystem_ClearCameraParent(mapper->railSystem);
}

u32 FieldNoGridMapper_GetTileAtPos(NoGridMapper *mapper, const RailPosition *pos) {
    return FieldRailTilemap_GetTileAtPos(mapper->tilemap, pos);
}

void FieldNoGridMapper_SetPlayerPos(NoGridMapper *mapper, const RailPosition *pos) {
    FieldRailSystem_SetPlayerPos(mapper->railSystem, pos);
}

void FieldNoGridMapper_SetLineEnabled(NoGridMapper *mapper, u32 lineId, BOOL enabled) {
    FieldRailSystem_SetLineEnabled(mapper->railSystem, lineId, enabled);
}

void FieldNoGridMapper_CreatePosExternal(NoGridMapper *mapper, u32 zoneId, u16 componentId, u16 front, u16 side,
                                         RailPosition *pos, HeapID heapId) {
    HeapID tailHeapId = HEAPID_TAIL(heapId);
    NoGridMapperHeader *header =
        GFL_HeapAllocate(tailHeapId, GFL_ArcSysGetDataLength(ARCID_RAIL_DATA, GetRailIDForZone(zoneId)), TRUE,
                         "field_nogrid_mapper.c", 457);
    RailLoader *loader;

    GFL_ArcSysRead(header, ARCID_RAIL_DATA, GetRailIDForZone(zoneId));
    loader = FieldRailLoader_Create(tailHeapId);
    FieldRailLoader_LoadFile(loader, header->railFileId, tailHeapId);
    FieldRailSystem_NormalizePos(mapper->railSystem, FieldRailLoader_GetRailData(loader), componentId, front, side,
                                 pos);
    FieldRailLoader_Free(loader);
    GFL_HeapFree(header);
}

FieldRailSystem *FieldNoGridMapper_GetRailSystem(NoGridMapper *mapper) {
    return mapper->railSystem;
}
