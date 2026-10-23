#ifndef POKEBW2_FIELD_FIELD_G3D_MAPPER_H
#define POKEBW2_FIELD_FIELD_G3D_MAPPER_H

// Overlay 36's field_g3d_mapper.c, which streams the map chunks around the player and draws them. Names and layouts
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_map.h"
#include "field/field_map_chunk.h"
#include "field/field_terrain_animator.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// How the mapper picks the chunks to load (locatorGenType, FieldG3DMapper_GenLocators0 to 3): every chunk of the
// matrix in layers, the chunks around the player within the matrix, the chunks around the player wrapping around it,
// and the player's layer of the matrix with the nearer of the layers above and below it
enum {
    FIELD_G3D_MAPPER_LOCATORS_ALL,
    FIELD_G3D_MAPPER_LOCATORS_AROUND,
    FIELD_G3D_MAPPER_LOCATORS_WRAP,
    FIELD_G3D_MAPPER_LOCATORS_LAYERS,
};

// The passes of FieldG3D_RenderFieldmap: the first draws the chunks that fit in the time left in the frame, up to
// renderPhase1MaxChunkCount, and the second the others and the props
enum {
    FIELD_G3D_RENDER_PHASE_1,
    FIELD_G3D_RENDER_PHASE_2,
};

// The chunk ID of a locator that places no chunk
#define FIELD_CHUNK_ID_NONE 0xffffffff

// The terrain layers that FieldG3DMapperTerrain holds
#define FIELD_G3D_MAPPER_LAYER_MAX 16

// The end of drawnChunkIndices
#define FIELD_G3D_MAPPER_DRAW_END 0xff

// An archive and a file in it
typedef struct {
    u32 arcId;
    u32 fileId;
} FieldG3DMapperTextureFile;

struct FieldG3DMapperConfig {
    fx32 chunkSpan;
    u32 unk04;
    u16 chunkLoadDiameterX;
    u16 chunkLoadDiameterZ;
    u32 locatorGenType;
    u32 chunkArcId;
    u16 matrixWidth;
    u16 matrixHeight;
    u32 chunkIdCount;
    u32 *chunkDatIds;
    BOOL isSharedTexResource;
    // The textures that all the chunks use, if isSharedTexResource is 1
    FieldG3DMapperTextureFile textures;
    FieldTerrainAnmInfo terrainAnmInfo;
    u32 chunkBufferSize;
    u8 renderPhase1MaxChunkCount;
};

// The terrain layers at a position, which the mapper gathers from its chunks
typedef struct {
    MapTerrainBuf layers[FIELD_G3D_MAPPER_LAYER_MAX];
    u16 layerCount;
} FieldG3DMapperTerrain;

// Where a chunk goes, or FIELD_CHUNK_ID_NONE
typedef struct {
    u32 chunkId;
    VecFx32 position;
} FieldChunkLocator;

typedef struct {
    FieldChunk *chunk;
    FieldChunkContext context;
    FieldChunkLocator locator;
} FieldChunkHandle;

struct FieldG3DMapper {
    HeapID heapId;
    fx32 chunkSpan;
    u32 unk08;
    u32 locatorGenType;
    u32 chunkArcId;
    u16 matrixWidth;
    u16 matrixHeight;
    u32 chunkIdCount;
    u32 *chunkDatIds;
    BOOL isSharedTexResource;
    FieldChunkHandle *chunkHandles;
    FieldChunkLocator *chunkLocators;
    u8 chunkLoadDiameterX;
    u8 chunkLoadDiameterZ;
    u16 chunkCapacity;
    u32 chunkIdOfPlayer;
    VecFx32 playerPos;
    u8 phase1MaxChunkCount;
    u8 drawnChunkCount;
    u8 lastDrawnCount;
    u8 nowDrawnCount;
    // The chunk handles in drawing order, FIELD_G3D_MAPPER_DRAW_END after the last
    u8 *drawnChunkIndices;
    s32 maxConcurrentDrawChunkByteSize;
    u8 chunkSizeCapPassingChunkEndIdx;
    u8 nowDrawScanline;
    u16 unk4E;
    u32 unk50;
    VecFx32 chunkBasePos;
    G3DResource *mapTextures;
    FieldPropSystem *propSystem;
    FieldTerrainAnimator *terrainAnimator;
    void *wfbc;
    ResortMapCreateWork *resortMap;
    HeightExList *heightEx;
};

FieldG3DMapper *FieldG3DMapper_Create(HeapID heapId, u16 season);
void FieldG3DMapper_Free(FieldG3DMapper *mapper);
void FieldG3DMapper_UpdateSync(FieldG3DMapper *mapper);
BOOL FieldG3DMapper_Update(FieldG3DMapper *mapper);
void FieldG3DMapper_CallAllLoaderUpdate(FieldG3DMapper *mapper);
// pass is a FIELD_G3D_RENDER_PHASE_
void FieldG3D_RenderFieldmap(FieldG3DMapper *mapper, void *camera, u32 pass);
HeightExList *func_ov036_02184590(FieldG3DMapper *mapper);
void FieldG3DMapper_CreateMap(FieldG3DMapper *mapper, const FieldG3DMapperConfig *config,
                              FieldColorPostFX *colorPostFx);
void FieldG3DMapper_FreeMap(FieldG3DMapper *mapper);
void FieldG3DMapper_LoadMapTextures(FieldG3DMapper *mapper, const FieldG3DMapperConfig *config,
                                    FieldColorPostFX *colorPostFx);
void FieldG3DMapper_FreeMapTextures(FieldG3DMapper *mapper);
void FieldG3DMapper_SetPlayerPos(FieldG3DMapper *mapper, const VecFx32 *position);
u32 FieldG3DMapper_GetBasePosChunkHandleIdx(FieldG3DMapper *mapper);
void FieldG3DMapper_GenLocators0(FieldG3DMapper *mapper, const VecFx32 *playerPos, FieldChunkLocator *locators);
void FieldG3DMapper_GenLocators1(FieldG3DMapper *mapper, const VecFx32 *playerPos, FieldChunkLocator *locators);
void FieldG3DMapper_GenLocators2(FieldG3DMapper *mapper, const VecFx32 *playerPos, FieldChunkLocator *locators);
void FieldG3DMapper_GenLocators3(FieldG3DMapper *mapper, const VecFx32 *playerPos, FieldChunkLocator *locators);
BOOL FieldG3DMapper_UpdateChunkLoad(FieldG3DMapper *mapper, FieldChunkLocator *locators);
s32 func_ov036_02184fb8(u32 diameter, fx32 chunkSpan, fx32 pos);
s32 func_ov036_02184ff4(u32 diameter, fx32 chunkSpan, fx32 pos);
s32 FieldG3DMapper_CalcChunkLoadAreaStart(u32 diameter, fx32 chunkSpan, fx32 pos);
void ClearMapTerrainBuf(MapTerrainBuf *terrain);
void ClearMapTerrainSamplerOutput(FieldG3DMapperTerrain *output);
BOOL FieldG3DMapper_GetTerrainAll(FieldG3DMapper *mapper, const VecFx32 *position, FieldG3DMapperTerrain *output);
BOOL FieldG3DMapper_GetTerrain(FieldG3DMapper *mapper, const VecFx32 *position, MapTerrainBuf *terrain);
BOOL func_ov036_02185230(FieldG3DMapper *mapper, u32 index, const VecFx32 *position, MapTerrainBuf *terrain);
BOOL FieldG3DMapper_CheckChunkHandleTerrainReady(FieldG3DMapper *mapper, u32 index);
BOOL FieldG3DMapper_IsPosOutOfBounds(FieldG3DMapper *mapper, const VecFx32 *position);
void func_ov036_021852d0(FieldG3DMapper *mapper, const VecFx32 *position);
void func_ov036_021852e0(FieldG3DMapper *mapper, VecFx32 *out);
FieldPropSystem *FieldG3DMapper_GetBMSystem(FieldG3DMapper *mapper);
void func_ov036_021852f4(FieldG3DMapper *mapper, CityState *city, BOOL isOther);
ResortMapCreateWork *func_ov036_02185304(FieldG3DMapper *mapper);
FieldTerrainAnimator *FieldG3DMapper_CreateTerrainAnimator(u16 chunkCapacity, void *mapTextures,
                                                           const FieldTerrainAnmInfo *anmInfo, u32 heapId);
BOOL FieldChunkContext_HasAnimator(FieldChunkContext *context);
FieldTerrainSRTAnimatorChunkState *FieldChunkContext_GetSRTAnimatorState(FieldChunkContext *context);
FieldPropSystem *FieldChunkContext_GetPropSystem(FieldChunkContext *context);
ResortMapCreateWork *FieldChunkContext_GetResortMap(FieldChunkContext *context);
u32 FieldChunkContext_GetChunkIndex(FieldChunkContext *context);
HeapID FieldChunkContext_GetHeapID(FieldChunkContext *context);
void FieldChunkContext_Init(FieldChunkContext *context, FieldG3DMapper *mapper, u32 chunkIndex, HeapID heapId);
void FieldChunkContext_FreeAnimator(FieldChunkContext *context);
void func_ov036_021853d4(FieldChunkContext *context);
BOOL FieldChunkContext_HasAnimatorCore(FieldChunkContext *context);
FieldTerrainSRTAnimatorChunkState *FieldChunkContext_GetSRTAnimatorStateCore(FieldChunkContext *context);
void FieldChunkLocator_Disable(FieldChunkLocator *locator);
void FieldChunkLocator_SetChunkID(FieldChunkLocator *locator, u32 chunkId);
BOOL FieldChunkLocator_IsActive(FieldChunkLocator *locator);
u32 FieldChunkLocator_GetChunkID(FieldChunkLocator *locator);
void FieldChunkLocator_SetPos(FieldChunkLocator *locator, fx32 x, fx32 y, fx32 z);
void FieldChunkLocator_GetPos(FieldChunkLocator *locator, VecFx32 *position);
void func_ov036_0218543c(const FieldG3DMapper *mapper, const VecFx32 *position, FieldG3DMapperTerrain *output);
void FieldG3DMapper_ResetDrawCalcState(FieldG3DMapper *mapper);
void func_ov036_02185578(FieldG3DMapper *mapper);
void FieldG3DMapper_AddDrawableChunk(FieldG3DMapper *mapper, FieldChunk *chunk, u32 index);
void FieldG3DMapper_CalcRenderPassSplit(FieldG3DMapper *mapper);
BOOL FieldG3DMapper_CheckChunkDraw(FieldG3DMapper *mapper, u32 order, u32 pass, u32 *index);

// The kinds of chunk, ending with magic 0xffffffff
extern const FieldChunkVTable FIELD_CHUNK_VTABLES[5];

#endif // POKEBW2_FIELD_FIELD_G3D_MAPPER_H
