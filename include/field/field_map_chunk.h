#ifndef POKEBW2_FIELD_FIELD_MAP_CHUNK_H
#define POKEBW2_FIELD_FIELD_MAP_CHUNK_H

#include "types.h"
#include "field/field_terrain_animator.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// FieldChunkLoader.state: the steps of a chunk's loaderUpdate
enum {
    FIELD_CHUNK_LOAD_NONE,
    FIELD_CHUNK_LOAD_START,
    FIELD_CHUNK_LOAD_READ,
    FIELD_CHUNK_LOAD_SETUP,
    // 'RD' chunks only
    FIELD_CHUNK_LOAD_END,
};

// Layouts from swan's field_map_chunk.h.
typedef struct {
    u16 state;
    u16 pad02;
    u32 nowLoadedLength;
    u32 bufferLoadedLength;
    u32 totalRawLength;
    BOOL unk10;
    BOOL unk14;
    BOOL unk18;
    BOOL unk1c;
    BOOL terrainLoadDone;
} FieldChunkLoader;

// How to load and sample a kind of chunk, found by the magic at the start of its file. A table of them ends with
// magic 0xffffffff
typedef struct {
    u32 magic;
    // Returns FALSE once the chunk is loaded
    BOOL (*loaderUpdate)(FieldChunk *chunk, FieldChunkContext *context);
    // pos is relative to the chunk's center, size is the chunk's width and baseY its height
    void (*getTerrain)(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos, fx32 size, fx32 baseY);
    void (*getTerrainBaseLayer)(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos, fx32 size,
                                fx32 baseY);
} FieldChunkVTable;

// The props section of a chunk file: the count, then that many FieldPropSourceInfo
typedef struct {
    u32 count;
} FieldChunkProps;

struct FieldChunkContext {
    FieldTerrainSRTAnimatorChunkState *srtAnimatorState;
    FieldPropSystem *propSystem;
    void *wfbc;
    ResortMapCreateWork *resortMap;
    u32 chunkIndex;
    u16 heapId;
};

// What FieldChunk_Create sets up a chunk with
typedef struct {
    u32 bufferSize;
    u32 textureVRAMSize;
    const FieldChunkVTable *vtable;
    FieldChunkContext *context;
    // 0 for 0x800 bytes
    u32 streamerBufSize;
    u16 propCapacity1;
    u16 propCapacity2;
    u32 unkA8;
    u32 unkAC;
} FieldChunkConfig;

struct FieldChunk {
    u16 active;
    u16 disableStreamer;
    VecFx32 worldPos;
    ArcTool *arc;
    u32 reqLoadDatID;
    u32 vtableIndex;
    u32 bufferSize;
    u32 textureVRAMSize;
    u32 streamerBufSize;
    const FieldChunkVTable *vtable;
    NNSG3dRenderObj *model;
    NNSG3dRenderObj modelObj;
    G3DResource *modelRsc;
    G3DResource *texRsc;
    u32 textureVRAM;
    u32 paletteVRAM;
    G3DResource *mapTextures;
    FieldPropResBank *propResBank;
    FieldPropInstance *propInstances;
    FieldPropInstance *propInstances2;
    u16 propCapacity1;
    u16 propCapacity2;
    u32 unkA8;
    u32 unkAC;
    FieldChunkLoader loader;
    void *container;
    void *context;
};

FieldChunk *FieldChunk_Create(const FieldChunkConfig *config, HeapID heapId);
void FieldChunk_Free(FieldChunk *chunk);
void FieldChunk_CallLoaderUpdate(FieldChunk *chunk);
void FieldChunk_LoadFullSync(FieldChunk *chunk);
BOOL FieldChunk_Draw(FieldChunk *chunk, G3DCamera *camera);
void FieldChunk_BindArc(FieldChunk *chunk, u32 arcId, HeapID heapId);
void FieldChunk_UnbindArc(FieldChunk *chunk);
void FieldChunk_BindMapTextures(FieldChunk *chunk, G3DResource *textures);
void FieldChunk_UnbindMapTextures(FieldChunk *chunk);
void FieldChunk_BindPropResInstanceBank(FieldChunk *chunk, FieldPropResBank *bank);
void FieldChunk_UnbindPropResInstanceBank(FieldChunk *chunk);
void FieldChunk_ReleasePropInstance(FieldChunk *chunk, u32 propIndex);
void FieldChunk_ChangeDatID(FieldChunk *chunk, u32 datID);
void FieldChunk_Disable(FieldChunk *chunk);
void FieldChunk_BeginLoad(FieldChunk *chunk, u32 datID);
BOOL FieldChunk_UpdateStreamLoad(FieldChunk *chunk);
BOOL FieldChunk_RenderMap(FieldChunk *chunk);
void FieldChunk_RenderProps(FieldChunk *chunk, G3DCamera *camera);
BOOL FieldChunk_IsTerrainReady(FieldChunk *chunk);
void FieldChunk_GetTerrain(MapTerrainSamplerOutput *out, FieldChunk *chunk, const VecFx32 *pos, fx32 a3);
void FieldChunk_GetTerrainBaseLayer(MapTerrainSamplerOutput *out, FieldChunk *chunk, const VecFx32 *pos, fx32 a3);
void FieldChunk_ResetProps(FieldChunk *chunk);
void FieldChunk_Reset(FieldChunk *chunk);
void FieldChunk_DetachModelResource(NNSG3dRenderObj *model);
BOOL FieldChunk_CheckModelBBox(NNSG3dResMdl *mdl);
s32 FieldChunk_CheckBBox(const GXBoxTestParam *box);
void FieldChunk_LoadAtOnce(FieldChunk *chunk, u32 datID);

extern const VecFx32 FIELD_CHUNK_SCALE_IDENTITY;
extern const VecFx32 FIELD_CHUNK_TRANSLATION_IDENTITY;
extern const MtxFx33 FIELD_CHUNK_ROTATION_IDENTITY;
void FieldChunk_SetActive(FieldChunk *chunk, u16 active);
u16 FieldChunk_IsActive(FieldChunk *chunk);
void FieldChunk_SetWorldPos(FieldChunk *chunk, const VecFx32 *position);
void FieldChunk_GetWorldPos(FieldChunk *chunk, VecFx32 *position);
void FieldChunk_GetLoaderHandle(FieldChunk *chunk, FieldChunkLoader **handle);
void GetChunkRawDataContainer(FieldChunk *chunk, void **container);
void FieldChunk_GetDatID(FieldChunk *chunk, u32 *datID);
void FieldChunk_GetRawDataLength(FieldChunk *chunk, u32 *length);
void FieldChunk_BindModel(FieldChunk *chunk, void *model);
void FieldChunk_UnbindModel(FieldChunk *chunk);
G3DResource *FieldChunk_GetModelResource(FieldChunk *chunk);
void FieldChunk_FreeTexRsc(FieldChunk *chunk);
G3DResource *FieldChunk_GetUsedTexRsc(FieldChunk *chunk);
void FieldChunk_SetupModel(FieldChunk *chunk);
NNSG3dRenderObj *FieldChunk_GetModel(FieldChunk *chunk);
void FieldChunk_ResetStreamer(FieldChunk *chunk);

G3DResource *FieldChunk_GetUsedTexRscCore(FieldChunk *chunk);
void FieldChunk_LinkMdlTex(NNSG3dRenderObj *model, G3DResource *resource, G3DResource *texture);

// The kinds of chunk, by the magic at the start of their files
// field_map_chunk_wb.c
BOOL FieldChunkAccessor_WB_Update(FieldChunk *chunk, FieldChunkContext *context);
void FieldChunkAccessor_WB_GetTerrain(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos,
                                      fx32 size, fx32 baseY);
// field_map_chunk_gc.c
BOOL FieldChunkAccessor_GC_Update(FieldChunk *chunk, FieldChunkContext *context);
void FieldChunkAccessor_GC_GetTerrain(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos,
                                      fx32 size, fx32 baseY);
void FieldChunkAccessor_GC_GetTerrainBaseLayer(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos,
                                               fx32 size, fx32 baseY);
// field_map_chunk_ng.c
BOOL FieldChunkAccessor_NG_Update(FieldChunk *chunk, FieldChunkContext *context);
void FieldChunkAccessor_NG_GetTerrain(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos,
                                      fx32 size, fx32 baseY);
// field_map_chunk_rd.c
BOOL FieldChunkAccessor_RD_Update(FieldChunk *chunk, FieldChunkContext *context);
void FieldChunkAccessor_RD_GetTerrain(MapTerrainSamplerOutput *out, const void *container, const VecFx32 *pos,
                                      fx32 size, fx32 baseY);
// field_map_chunk_height.c
void FieldChunkAccessor_GetTerrainCore(MapTerrainSamplerOutput *out, u32 layer, MapHeightData *terrain,
                                       const VecFx32 *pos, fx32 size, fx32 baseY);

#endif // POKEBW2_FIELD_FIELD_MAP_CHUNK_H
