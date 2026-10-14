#include "types.h"
#include "field/field_map.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"

MapMatrix *InitMapMatrix(HeapID heapId) {
    MapMatrix *matrix = GFL_HeapAllocate(heapId, sizeof(MapMatrix), TRUE, "map_matrix.c", 0x4c);

    matrix->heapId = heapId;
    sys_memset32((u32)-1, matrix->chunkIds, sizeof(matrix->chunkIds));
    sys_memset32(0xffff, matrix->zoneIds, sizeof(matrix->zoneIds));
    return matrix;
}

void MapMatrix_Load(MapMatrix *matrix, u16 matrixId, u16 zoneId, HeapID heapId) {
    void *data = GFL_ArcSysReadHeapNew(9, matrixId, heapId);

    ProcessMapMatrix(matrix, data, matrixId, zoneId);
    GFL_HeapFree(data);
}

void FreeMapMatrix(MapMatrix *matrix) {
    GFL_HeapFree(matrix);
}

u16 GetZoneIDAtMatrixXZ(MapMatrix *matrix, s32 x, s32 z) {
    return matrix->zoneIds[z * matrix->width + x];
}

u16 GetZoneIDAtMatrixXZWorld(MapMatrix *matrix, s32 x, s32 z) {
    return GetZoneIDAtMatrixXZ(matrix, GetChunkCoordOfWorld(x), GetChunkCoordOfWorld(z));
}

u16 GetMapMatrixWidth(MapMatrix *matrix) {
    return matrix->width;
}

u16 GetMapMatrixHeight(MapMatrix *matrix) {
    return matrix->height;
}

u32 GetMapMatrixChunkIDCount(MapMatrix *matrix) {
    return matrix->chunkIdCount;
}

u32 *GetMapMatrixChunkIDs(MapMatrix *matrix) {
    return matrix->chunkIds;
}

BOOL RangeCheckChunkCoordinate(MapMatrix *matrix, s32 x, s32 z) {
    return z >= 0 && z < matrix->height && x >= 0 && x < matrix->width;
}

BOOL RangeCheckChunkCoordinateWorld(MapMatrix *matrix, s32 x, s32 z) {
    return RangeCheckChunkCoordinate(matrix, GetChunkCoordOfWorld(x), GetChunkCoordOfWorld(z));
}

void MapReplace_Patch(MapMatrix *matrix, MapReplace *replace, HeapID heapId) {
    u32 oldValue;
    u32 newValue;
    u32 result = MapReplace_ResolvePatch(replace, &oldValue, &newValue);
    u32 i;

    switch (result) {
    case 2:
        MapMatrix_Load(matrix, (u16)newValue, matrix->zoneId, heapId);
        return;
    case 1:
        for (i = 0; i < matrix->chunkIdCount; i++) {
            if (matrix->chunkIds[i] == oldValue) {
                matrix->chunkIds[i] = newValue;
            }
        }
        break;
    case 0:
        break;
    }
}

void MapMatrix_Patch(MapMatrix *matrix, GameSystem *gsys, HeapID heapId) {
    MapReplace *replace = MapReplace_Create(heapId, gsys);
    s32 count = MapReplace_GetEntryCount(replace);
    s32 i;

    for (i = 0; i < count; i++) {
        s32 entryId = MapReplace_LoadEntry(replace, i);

        if (entryId == matrix->matrixId) {
            MapReplace_Patch(matrix, replace, heapId);
        }
    }
    MapReplace_Free(replace);
}

void ProcessMapMatrix(MapMatrix *matrix, const MapMatrixFileHeader *data, u32 matrixId, u16 zoneId) {
    u16 width;
    u16 height;
    const u32 *chunkIds;
    u16 *zoneIds;
    const u32 *zones;
    u32 i;

    matrix->zoneId = zoneId;
    width = data->width;
    height = data->height;
    matrix->matrixId = matrixId;
    matrix->width = width;
    matrix->height = height;
    matrix->chunkIdCount = width * height;
    matrix->format = data->format;
    chunkIds = data->chunkIds;
    sys_memcpy32(chunkIds, matrix->chunkIds, matrix->chunkIdCount * sizeof(u32));
    zoneIds = matrix->zoneIds;
    if (matrix->format == 1) {
        zones = &chunkIds[matrix->chunkIdCount];
        for (i = 0; i < matrix->chunkIdCount; i++) {
            zoneIds[i] = zones[i];
            zoneIds[i] = GetVersionedMapChangeZoneNum2(zoneIds[i]);
        }
    } else {
        sys_memset16(zoneId, zoneIds, sizeof(matrix->zoneIds));
    }
}

u32 GetChunkCoordOfWorld(s32 coordinate) {
    return ((coordinate / 0x1000) / 16) / 32;
}
