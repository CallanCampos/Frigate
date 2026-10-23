#ifndef POKEBW2_FIELD_FIELD_TERRAIN_ANIMATOR_H
#define POKEBW2_FIELD_FIELD_TERRAIN_ANIMATOR_H

// Overlay 36's animations of the map chunks' textures. Names and layouts from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "nnsys/g3d.h"
#include "struct_decls.h"

typedef struct FieldTerrainAnimator FieldTerrainAnimator;
typedef struct FieldTerrainSRTAnimatorChunkState FieldTerrainSRTAnimatorChunkState;

// The animation files of an area, 0xffffffff for none
typedef struct {
    u32 srtAnimeId;
    u32 patAnimeId;
} FieldTerrainAnmInfo;

typedef struct {
    u16 hasSRT : 8;
    u16 hasPat : 8;
    u16 chunkCapacity;
    u16 arcIdSRT;
    u16 idSRT;
    u16 arcIdPat;
    u16 idPat;
} FieldTerrainAnimatorSetup;

FieldTerrainAnimator *FieldTerrainAnimator_Create(const FieldTerrainAnimatorSetup *setup, G3DResource *mapTextures,
                                                  HeapID heapId);
void FieldTerrainAnimator_Free(FieldTerrainAnimator *animator);
void FieldTerrainAnimator_Update(FieldTerrainAnimator *animator);
FieldTerrainSRTAnimatorChunkState *FieldTerrainAnimator_GetChunkState(FieldTerrainAnimator *animator, u32 chunkIndex);
// Binds the chunk's SRT animation to its model and textures, in the render object
void FieldTerrainSRTAnimatorChunkState_Bind(FieldTerrainSRTAnimatorChunkState *state, G3DResource *model,
                                            G3DResource *textures, NNSG3dRenderObj *renderObj);
void FieldTerrainSRTAnimatorChunkState_ClearUsedFlag(FieldTerrainSRTAnimatorChunkState *state);

#endif // POKEBW2_FIELD_FIELD_TERRAIN_ANIMATOR_H
