#include "types.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_map_chunk.h"
#include "field/field_prop.h"
#include "field/field_terrain_animator.h"
#include "gfl/std.h"

// A 'GC' chunk file, with two layers of terrain: a model, the terrain of each layer and props. The ROM doesn't name the
// file; the name is a guess. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0);
// GCChunkFile is ours

// The start of the file. Each offset is from the start of the file
typedef struct {
    // 'GC'
    u16 magic;
    u16 sectionCount;
    u32 modelOffset;
    u32 terrainOffsets[2];
    u32 propsOffset;
    u32 endOffset;
} GCChunkFile;

// Loads the chunk a step at a time: reads the file, then binds its model, places its props and starts its texture
// animation. Returns FALSE once the chunk is loaded
BOOL FieldChunkAccessor_GC_Update(FieldChunk *chunk, FieldChunkContext *context) {
    FieldChunkLoader *loader;
    u32 datId;
    void *container;

    FieldChunk_GetLoaderHandle(chunk, &loader);
    switch (loader->state) {
    case FIELD_CHUNK_LOAD_START:
        FieldChunk_ResetStreamer(chunk);
        FieldChunk_GetDatID(chunk, &datId);
        FieldChunk_BeginLoad(chunk, datId);
        loader->state = FIELD_CHUNK_LOAD_READ;
        break;
    case FIELD_CHUNK_LOAD_READ:
        if (FieldChunk_UpdateStreamLoad(chunk)) {
            break;
        }
        loader->unk18 = TRUE;
        loader->unk1c = TRUE;
        loader->terrainLoadDone = TRUE;
        loader->state = FIELD_CHUNK_LOAD_SETUP;
        // fallthrough
    case FIELD_CHUNK_LOAD_SETUP: {
        GCChunkFile *file;

        GetChunkRawDataContainer(chunk, &container);
        file = container;
        FieldChunk_BindModel(chunk, (u8 *)file + file->modelOffset);
        // The props section is not optional: sectionCount is not checked, only whether the section is empty
        if (file->propsOffset != file->endOffset) {
            FieldChunkProps *props = (FieldChunkProps *)((u8 *)container + file->propsOffset);

            FieldPropSystem_InstantiateProps(FieldChunkContext_GetPropSystem(context), chunk,
                                             (const FieldPropSourceInfo *)(props + 1), props->count);
        }
        FieldChunk_SetupModel(chunk);
        if (FieldChunkContext_HasAnimator(context)) {
            FieldTerrainSRTAnimatorChunkState_Bind(FieldChunkContext_GetSRTAnimatorState(context),
                                                   FieldChunk_GetModelResource(chunk), FieldChunk_GetUsedTexRsc(chunk),
                                                   FieldChunk_GetModel(chunk));
        }
        loader->state = FIELD_CHUNK_LOAD_NONE;
        return FALSE;
    }
    }
    return TRUE;
}

// Both layers
void FieldChunkAccessor_GC_GetTerrain(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos,
                                      fx32 size, fx32 baseY) {
    const GCChunkFile *file = container;
    MapTerrainSamplerOutput terrain;

    MI_CpuFill8(&terrain, 0, sizeof(terrain));
    FieldChunkAccessor_GetTerrainCore(&terrain, 0, (u8 *)file + file->terrainOffsets[0], pos, size, baseY);
    FieldChunkAccessor_GetTerrainCore(&terrain, 1, (u8 *)file + file->terrainOffsets[1], pos, size, baseY);
    *out = terrain;
    out->layerCount = 2;
}

void FieldChunkAccessor_GC_GetTerrainBaseLayer(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos,
                                               fx32 size, fx32 baseY) {
    const GCChunkFile *file = container;
    MapTerrainSamplerOutput terrain;

    MI_CpuFill8(&terrain, 0, sizeof(terrain));
    FieldChunkAccessor_GetTerrainCore(&terrain, 0, (u8 *)file + file->terrainOffsets[0], pos, size, baseY);
    *out = terrain;
    out->layerCount = 1;
}
