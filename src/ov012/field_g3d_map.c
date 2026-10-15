#include "types.h"
#include "field/field_map.h"
#include "field/field_map_chunk.h"
#include "field/field_prop.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

const VecFx32 FIELD_CHUNK_TRANSLATION_IDENTITY = { 0, 0, 0 };
const VecFx32 FIELD_CHUNK_SCALE_IDENTITY = { FX32_ONE, FX32_ONE, FX32_ONE };
const MtxFx33 FIELD_CHUNK_ROTATION_IDENTITY = { { { FX32_ONE, 0, 0 }, { 0, FX32_ONE, 0 }, { 0, 0, FX32_ONE } } };

FieldChunk *FieldChunk_Create(const FieldChunkConfig *config, HeapID heapId) {
    FieldChunk *chunk = GFL_HeapAllocate(heapId, sizeof(FieldChunk), TRUE, "field_g3d_map.c", 110);
    u32 resourceSize = GFL_G3DResGetAllocSize();

    chunk->propCapacity1 = config->propCapacity1;
    chunk->propCapacity2 = config->propCapacity2;
    chunk->unkA8 = config->unkA8;
    chunk->unkAC = config->unkAC;
    if (chunk->propCapacity1 != 0) {
        chunk->propInstances = GFL_HeapAllocate(heapId, sizeof(FieldPropInstance) * config->propCapacity1, TRUE,
                                                "field_g3d_map.c", 119);
    }
    if (chunk->propCapacity2 != 0) {
        chunk->propInstances2 = GFL_HeapAllocate(heapId, sizeof(FieldPropInstance) * config->propCapacity2, TRUE,
                                                 "field_g3d_map.c", 122);
    }
    FieldChunk_Reset(chunk);
    chunk->vtable = config->vtable;
    chunk->bufferSize = config->bufferSize;
    chunk->container = GFL_HeapAllocate(heapId, config->bufferSize, TRUE, "field_g3d_map.c", 134);
    if (config->streamerBufSize == 0) {
        chunk->streamerBufSize = 0x800;
    } else {
        chunk->streamerBufSize = config->streamerBufSize;
    }
    chunk->context = config->context;
    chunk->model = &chunk->modelObj;
    chunk->model->resMdl = NULL;
    chunk->arc = NULL;
    chunk->modelRsc = GFL_HeapAllocate(heapId, resourceSize, TRUE, "field_g3d_map.c", 151);
    chunk->texRsc = GFL_HeapAllocate(heapId, resourceSize, TRUE, "field_g3d_map.c", 154);
    chunk->mapTextures = NULL;
    if (config->textureVRAMSize != 0) {
        chunk->textureVRAMSize = config->textureVRAMSize;
        chunk->textureVRAM = NNS_GfdDefaultFuncAllocTexVram(config->textureVRAMSize, FALSE, 0);
        chunk->paletteVRAM = NNS_GfdDefaultFuncAllocPlttVram(0x200, FALSE, 0);
    } else {
        chunk->textureVRAMSize = 0;
        chunk->textureVRAM = 0;
        chunk->paletteVRAM = 0;
    }
    return chunk;
}


void FieldChunk_Free(FieldChunk *chunk) {
    FieldChunk_UnbindArc(chunk);
    if (chunk->textureVRAMSize != 0) {
        NNS_GfdDefaultFuncFreePlttVram(chunk->paletteVRAM);
        NNS_GfdDefaultFuncFreeTexVram(chunk->textureVRAM);
    }
    GFL_HeapFree(chunk->texRsc);
    GFL_HeapFree(chunk->modelRsc);
    GFL_HeapFree(chunk->container);
    if (chunk->propCapacity1 != 0) {
        GFL_HeapFree(chunk->propInstances);
    }
    if (chunk->propCapacity2 != 0) {
        GFL_HeapFree(chunk->propInstances2);
    }
    GFL_HeapFree(chunk);
}

void FieldChunk_CallLoaderUpdate(FieldChunk *chunk) {
    if (chunk->loader.state != 0 && chunk->vtable[chunk->vtableIndex].loaderUpdate != NULL) {
        chunk->vtable[chunk->vtableIndex].loaderUpdate(chunk, chunk->context);
    }
}

void FieldChunk_LoadFullSync(FieldChunk *chunk) {
    FieldChunkLoader *loader;

    chunk->disableStreamer = TRUE;
    do {
        FieldChunk_CallLoaderUpdate(chunk);
        FieldChunk_GetLoaderHandle(chunk, &loader);
    } while (loader->state != 0);
    chunk->disableStreamer = FALSE;
}

BOOL FieldChunk_Draw(FieldChunk *chunk, void *camera) {
    BOOL drawn = FALSE;

    if (FieldChunk_RenderMap(chunk) == TRUE) {
        if (chunk->propResBank != NULL) {
            FieldChunk_RenderProps(chunk, camera);
        }
        drawn = TRUE;
    }
    return drawn;
}

void FieldChunk_BindArc(FieldChunk *chunk, u32 arcId, HeapID heapId) {
    FieldChunk_Reset(chunk);
    FieldChunk_UnbindArc(chunk);
    chunk->model->resMdl = NULL;
    chunk->arc = GFL_ArcSysCreateFileHandle(arcId, heapId);
}

void FieldChunk_UnbindArc(FieldChunk *chunk) {
    if (chunk->arc != NULL) {
        GFL_ArcToolFree(chunk->arc);
        chunk->arc = NULL;
    }
}

void FieldChunk_BindMapTextures(FieldChunk *chunk, void *textures) {
    chunk->mapTextures = textures;
}

void FieldChunk_UnbindMapTextures(FieldChunk *chunk) {
    chunk->mapTextures = NULL;
}

void FieldChunk_BindPropResInstanceBank(FieldChunk *chunk, FieldPropResBank *bank) {
    chunk->propResBank = bank;
}

void FieldChunk_UnbindPropResInstanceBank(FieldChunk *chunk) {
    chunk->propResBank = NULL;
}

void FieldPropSystem_InstantiateProp(FieldChunk *chunk, FieldPropInstance *instance, u32 propIndex) {
    chunk->propInstances[propIndex].resIndex = instance->resIndex;
    chunk->propInstances[propIndex].pos.position = instance->pos.position;
    chunk->propInstances[propIndex].pos.rotationY = instance->pos.rotationY;
}

void FieldChunk_ReleasePropInstance(FieldChunk *chunk, u32 propIndex) {
    if (propIndex != 0xffffffff && propIndex < chunk->propCapacity1) {
        chunk->propInstances[propIndex].resIndex = 0xffffffff;
        VEC_Set(&chunk->propInstances[propIndex].pos.position, 0, 0, 0);
        chunk->propInstances[propIndex].pos.rotationY = 0;
    }
}

FieldPropInstance *FieldChunk_GetPropInstance(FieldChunk *chunk, u32 propIndex) {
    FieldPropInstance *instance;

    if (propIndex >= chunk->propCapacity1) {
        return NULL;
    }
    instance = &chunk->propInstances[propIndex];
    if (instance->resIndex == 0xffffffff) {
        instance = NULL;
    }
    return instance;
}

void FieldChunk_ChangeDatID(FieldChunk *chunk, u32 datID) {
    u16 magic;
    u32 i;

    FieldChunk_DetachModelResource(chunk->model);
    FieldChunk_UnbindModel(chunk);
    FieldChunk_FreeTexRsc(chunk);
    FieldChunk_ResetProps(chunk);
    chunk->reqLoadDatID = datID;
    GFL_ArcToolReadRange(chunk->arc, datID, 0, sizeof(magic), &magic);
    i = 0;
    while (chunk->vtable[i].magic != 0xffffffff) {
        if (magic == chunk->vtable[i].magic) {
            break;
        }
        i++;
    }
    chunk->vtableIndex = i;
    chunk->loader.state = 1;
}

void FieldChunk_Disable(FieldChunk *chunk) {
    FieldChunk_DetachModelResource(chunk->model);
    FieldChunk_UnbindModel(chunk);
    FieldChunk_FreeTexRsc(chunk);
    FieldChunk_ResetStreamer(chunk);
    FieldChunk_Reset(chunk);
}

void FieldChunk_SetActive(FieldChunk *chunk, u16 active) {
    chunk->active = active;
}

u16 FieldChunk_IsActive(FieldChunk *chunk) {
    return chunk->active;
}

void FieldChunk_SetWorldPos(FieldChunk *chunk, const VecFx32 *position) {
    chunk->worldPos = *position;
}

void FieldChunk_GetWorldPos(FieldChunk *chunk, VecFx32 *position) {
    *position = chunk->worldPos;
}

void FieldChunk_GetLoaderHandle(FieldChunk *chunk, FieldChunkLoader **handle) {
    *handle = &chunk->loader;
}

void GetChunkRawDataContainer(FieldChunk *chunk, void **container) {
    *container = chunk->container;
}

void FieldChunk_GetDatID(FieldChunk *chunk, u32 *datID) {
    *datID = chunk->reqLoadDatID;
}

void FieldChunk_GetRawDataLength(FieldChunk *chunk, u32 *length) {
    *length = chunk->loader.totalRawLength;
}

void FieldChunk_BindModel(FieldChunk *chunk, void *model) {
    GFL_G3DResBindData(chunk->modelRsc, 1, model);
}

void FieldChunk_UnbindModel(FieldChunk *chunk) {
    GFL_G3DResBindData(chunk->modelRsc, 1, NULL);
}

void *FieldChunk_GetModelResource(FieldChunk *chunk) {
    return chunk->modelRsc;
}

void FieldChunk_FreeTexRsc(FieldChunk *chunk) {
    GFL_G3DResBindData(chunk->texRsc, 2, NULL);
}

void *FieldChunk_GetUsedTexRsc(FieldChunk *chunk) {
    return FieldChunk_GetUsedTexRscCore(chunk);
}

void FieldChunk_SetupModel(FieldChunk *chunk) {
    void *texture = FieldChunk_GetUsedTexRscCore(chunk);
    FieldChunk_LinkMdlTex(chunk->model, chunk->modelRsc, texture);
}

NNSG3dRenderObj *FieldChunk_GetModel(FieldChunk *chunk) {
    return chunk->model;
}

void FieldChunk_ResetStreamer(FieldChunk *chunk) {
    chunk->loader.nowLoadedLength = 0;
    chunk->loader.unk10 = 0;
    chunk->loader.unk18 = 0;
    chunk->loader.unk1c = 0;
    chunk->loader.terrainLoadDone = 0;
}

void FieldChunk_BeginLoad(FieldChunk *chunk, u32 datID) {
    u8 *dest;

    if (chunk->disableStreamer) {
        FieldChunk_LoadAtOnce(chunk, datID);
        return;
    }
    dest = (u8 *)chunk->container + chunk->loader.nowLoadedLength;
    chunk->loader.totalRawLength = GFL_ArcToolGetDataLength(chunk->arc, datID);
    chunk->loader.bufferLoadedLength = 0;
    GFL_ArcToolReadRange(chunk->arc, datID, 0, chunk->streamerBufSize, dest);
    chunk->loader.nowLoadedLength += chunk->streamerBufSize;
    chunk->loader.bufferLoadedLength += chunk->streamerBufSize;
}

BOOL FieldChunk_UpdateStreamLoad(FieldChunk *chunk) {
    u8 *container = chunk->container;
    u32 nowLoaded = chunk->loader.nowLoadedLength;
    u32 bufferLoaded = chunk->loader.bufferLoadedLength;
    u32 remaining;

    if (chunk->loader.totalRawLength <= bufferLoaded) {
        return FALSE;
    }
    remaining = chunk->loader.totalRawLength - bufferLoaded;
    if (remaining > chunk->streamerBufSize) {
        GFL_ArcToolReadRaw(chunk->arc, chunk->streamerBufSize, container + nowLoaded);
        chunk->loader.nowLoadedLength += chunk->streamerBufSize;
        chunk->loader.bufferLoadedLength += chunk->streamerBufSize;
        return TRUE;
    }
    GFL_ArcToolReadRaw(chunk->arc, remaining, container + nowLoaded);
    chunk->loader.nowLoadedLength += remaining;
    return FALSE;
}

BOOL FieldChunk_RenderMap(FieldChunk *chunk) {
    BOOL drawn = FALSE;

    if (chunk->active && chunk->model->resMdl != NULL) {
        NNS_G3dGlbSetBaseTrans(&FIELD_CHUNK_TRANSLATION_IDENTITY);
        NNS_G3dGlbSetBaseRot(&FIELD_CHUNK_ROTATION_IDENTITY);
        NNS_G3dGlbSetBaseScale(&FIELD_CHUNK_SCALE_IDENTITY);
        NNS_G3DFlushRenderState();
        NNS_G3dGeIdentity();
        NNS_G3dGeLoadMtx43(&NNS_G3dGlb.cameraMtx);
        NNS_G3dGeTranslateVec(&chunk->worldPos);
        if (FieldChunk_CheckModelBBox(chunk->model->resMdl) == TRUE) {
            GFL_G3DSysDispatchDraw(chunk->model);
            drawn = TRUE;
        }
    }
    return drawn;
}

void FieldChunk_RenderProps(FieldChunk *chunk, void *camera) {
    FieldPropResInstanceHandle *handles = chunk->propResBank->handles;
    u32 count = chunk->propResBank->count;
    s32 i;
    u32 resIndex;
    NNSG3dRenderObj *obj;
    NNSG3dResMdl *mdl;
    fx16 sin;
    fx16 cos;
    MtxFx43 mtx;

    if (count == 0 || handles == NULL) {
        return;
    }
    for (i = 0; i < chunk->propCapacity1; i++) {
        resIndex = chunk->propInstances[i].resIndex;
        if (resIndex == 0xffffffff || resIndex >= count) {
            continue;
        }
        obj = GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(handles[resIndex].actor));
        mdl = obj->resMdl;
        if (mdl == NULL) {
            continue;
        }
        sin = FX_SinIdx(chunk->propInstances[i].pos.rotationY);
        cos = FX_CosIdx(chunk->propInstances[i].pos.rotationY);
        NNS_G3dGeLoadMtx43(&NNS_G3dGlb.cameraMtx);
        VEC_Add(&chunk->worldPos, &chunk->propInstances[i].pos.position, &mtx.rt.trans);
        MTX_RotY33_(&mtx.rt.rot, sin, cos);
        NNS_G3dGeMultMtx43(&mtx);
        if (FieldChunk_CheckModelBBox(mdl) == TRUE) {
            GFL_G3DSysDispatchDraw(obj);
        }
    }
}

BOOL FieldChunk_IsTerrainReady(FieldChunk *chunk) {
    if (chunk->loader.terrainLoadDone == FALSE || chunk->vtable[chunk->vtableIndex].getTerrain == NULL) {
        return FALSE;
    }
    return TRUE;
}

void FieldChunk_GetTerrain(MapTerrainSamplerOutput *out, FieldChunk *chunk, const VecFx32 *pos, fx32 a3) {
    VecFx32 local;
    fx32 y;
    void (*getTerrain)(MapTerrainSamplerOutput *, const void *, const VecFx32 *, fx32, fx32);

    out->layerCount = 0;
    if (chunk->loader.terrainLoadDone) {
        y = chunk->worldPos.y;
        VEC_Subtract(pos, &chunk->worldPos, &local);
        getTerrain = chunk->vtable[chunk->vtableIndex].getTerrain;
        if (getTerrain != NULL) {
            getTerrain(out, chunk->container, &local, a3, y);
        }
    }
}

void FieldChunk_GetTerrainBaseLayer(MapTerrainSamplerOutput *out, FieldChunk *chunk, const VecFx32 *pos, fx32 a3) {
    VecFx32 local;

    VEC_Subtract(pos, &chunk->worldPos, &local);
    chunk->vtable[chunk->vtableIndex].getTerrainBaseLayer(out, chunk->container, &local, a3, chunk->worldPos.y);
}

void FieldChunk_ResetProps(FieldChunk *chunk) {
    s32 i;

    for (i = 0; i < chunk->propCapacity1; i++) {
        chunk->propInstances[i].resIndex = 0xffffffff;
        VEC_Set(&chunk->propInstances[i].pos.position, 0, 0, 0);
        chunk->propInstances[i].pos.rotationY = 0;
    }
    for (i = 0; i < chunk->propCapacity2; i++) {
        chunk->propInstances2[i].resIndex = 0xffffffff;
        VEC_Set(&chunk->propInstances2[i].pos.position, 0, 0, 0);
        chunk->propInstances[i].pos.rotationY = 0;
    }
}

void FieldChunk_Reset(FieldChunk *chunk) {
    chunk->active = FALSE;
    chunk->worldPos.x = 0;
    chunk->worldPos.y = 0;
    chunk->worldPos.z = 0;
    chunk->loader.state = 0;
    chunk->reqLoadDatID = 0;
    FieldChunk_ResetProps(chunk);
}

// The first model of a model set that may be missing. The missing set and the missing entry share one NULL, so this
// isn't a NULL check around NNS_G3dGetMdlByIdx
static inline NNSG3dResMdl *FieldChunk_GetFirstMdl(const NNSG3dResMdlSet *mdlSet) {
    const u32 *ofs;

    if (mdlSet != NULL) {
        ofs = NNS_G3dGetResDataByIdx(&mdlSet->dict, 0);
        if (ofs != NULL) {
            return (NNSG3dResMdl *)((u8 *)mdlSet + *ofs);
        }
    }
    return NULL;
}

void FieldChunk_LinkMdlTex(NNSG3dRenderObj *model, void *resource, void *texture) {
    void *mdlFile = GFL_G3DResGetResData(resource);
    void *texFile = GFL_G3DResGetResData(texture);
    NNSG3dResTex *tex = NULL;
    NNSG3dResMdl *mdl = FieldChunk_GetFirstMdl(NNS_G3DResGetMdlBlock(mdlFile));

    if (texFile != NULL) {
        tex = NNS_G3DResGetTexBlock(texFile);
    }
    if (tex != NULL) {
        NNS_G3dBindMdlTex(mdl, tex);
        NNS_G3dBindMdlPltt(mdl, tex);
    }
    NNS_G3DModelAttachResource(model, mdl);
}

void FieldChunk_DetachModelResource(NNSG3dRenderObj *model) {
    model->resMdl = NULL;
}

BOOL FieldChunk_CheckModelBBox(NNSG3dResMdl *mdl) {
    BOOL result = TRUE;
    NNSG3dResMdlInfo *info = mdl != NULL ? &mdl->info : NULL;
    GXBoxTestParam box;
    VecFx32 scale;

    box.x = info->boxX;
    box.y = info->boxY;
    box.z = info->boxZ;
    box.width = info->boxW;
    box.height = info->boxH;
    box.depth = info->boxD;
    NNS_G3dGePushMtx();
    VEC_Set(&scale, info->boxPosScale, info->boxPosScale, info->boxPosScale);
    NNS_G3dGeScaleVec(&scale);
    if (FieldChunk_CheckBBox(&box) == 0) {
        result = FALSE;
    }
    NNS_G3dGePopMtx(1);
    return result;
}

s32 FieldChunk_CheckBBox(const GXBoxTestParam *box) {
    s32 in = TRUE;

    NNS_G3dGePolygonAttr(GX_LIGHTMASK_0, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 0,
                         GX_POLYGON_ATTR_MISC_FAR_CLIPPING | GX_POLYGON_ATTR_MISC_DISP_1DOT);
    NNS_G3dGeBegin(GX_BEGIN_TRIANGLES);
    NNS_G3dGeEnd();
    NNS_G3dGeBoxTest(box);
    NNS_G3DWaitFIFO();
    while (gfxGetBoxTestResult(&in) != 0) {
    }
    return in;
}

void *FieldChunk_GetUsedTexRscCore(FieldChunk *chunk) {
    if (GFL_G3DResGetResData(chunk->texRsc) == NULL && chunk->mapTextures != NULL) {
        return chunk->mapTextures;
    }
    return chunk->texRsc;
}

void FieldChunk_LoadAtOnce(FieldChunk *chunk, u32 datID) {
    void *container = chunk->container;
    u32 length = GFL_ArcToolGetDataLength(chunk->arc, datID);

    GFL_ArcToolRead(chunk->arc, datID, container);
    chunk->loader.totalRawLength = length;
    chunk->loader.nowLoadedLength = length;
    chunk->loader.bufferLoadedLength = length;
}
