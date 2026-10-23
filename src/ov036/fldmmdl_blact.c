// The billboard actors of the field actors: which materials they share, and the loading and unloading of their
// resources, which spreads over frames. The name is the ROM's own, from GFL_HeapAllocate's file argument.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/field_actor.h"
#include "field/fldmmdl_drawdata.h"
#include "gfl/arc.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/mi.h"

// A material of an object code, for the actors that share it. A free one has BLACT_NONE as its object code
typedef struct {
    u16 objCode;
    u16 mat;
    u16 flags;
    u16 unk6;
} MatCacheEntry;

typedef struct {
    int count;
    MatCacheEntry *entries;
} MatCache;

// A resource to read, once there is room for it
typedef struct {
    u16 objCode;
    u16 fileId;
    u8 sizeIdx;
    u8 atlasIdx;
    u32 flags;
} AsyncRead;

// A resource that has been read, until its material is loaded
typedef struct {
    u8 state;
    u32 flags;
    u16 objCode;
    u8 sizeIdx;
    u8 atlasIdx;
    u32 unkC;
    G3DResource *resource;
} AsyncResource;

// An actor that waits for its material
typedef struct {
    u16 *index;
    FieldActor *actor;
    u16 objCode;
    u8 sizeIdx;
    u8 atlasIdx;
    u8 ctrlType;
    u8 state;
    G3DResource *resource;
    void (*callback)(u16 index, void *data);
    void *callbackData;
    VecFx32 pos;
} PendingActor;

typedef struct {
    BOOL active;
    u16 readCount;
    u16 resourceCount;
    u16 actorCount;
    AsyncRead *reads;
    AsyncResource *resources;
    PendingActor *actors;
} AsyncLoader;

// An actor whose billboard actor is deleted once it is safe
typedef struct {
    FieldActor *actor;
    u16 index;
    u16 objCode;
    u16 newObjCode;
} UnloadEntry;

struct FieldBlAct {
    u8 callbackFlag;
    u16 count;
    BlActSys *sys;
    u16 firstMat;
    u32 unkC;
    MMSys *system;
    MatCache cache;
    AsyncLoader *loader;
    UnloadEntry *unloader;
};

static u32 BlActSys_LoadSingleUseTexMaterial(BlActSys *sys, G3DResource *resource, u32 sizeIdx, u32 atlasIdx);
static void FieldBlAct_FreeSingleUseMaterial(BlActSys *sys, u32 texMat);
static u16 FldActSys_AddBlActDefault(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat);
static u16 FldActSys_AddBlAct(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat, u32 lights);
static u16 FldAct_InitBlActWithMat(FieldActor *actor, u16 mat);
static u16 FldActSys_AddBlActWithPalette(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat, u16 palMat);
static u16 FldAct_InitBlActWithPalette(FieldActor *actor, u16 mat, u16 palMat);
static u16 FldAct_InitBlActExistMat(FieldActor *actor, u16 objCode);
static void FieldBlAct_UnbindActorObjCode(FieldBlAct *blact, FieldActor *actor, u16 index, u16 objCode);
static void FieldBlAct_ActorCallback(BlActSys *sys, u32 actor, void *data);
static void FldBlAct_CreateMatCache(MatCache *cache, int count, HeapID heapId);
static void FldBlAct_FreeMatCache(MatCache *cache);
static void FldBlAct_RegistCachedMat(MatCache *cache, u16 objCode, u16 mat, u16 flags);
static void FldBlAct_RemoveCachedMat(MatCache *cache, u16 objCode);
static BOOL FldBlAct_FindCachedMat(MatCache *cache, u16 objCode, u16 *mat, u16 *flags);
static void FldBlAct_InitMatCache(FieldBlAct *blact, int count);
static void FldBlAct_FreeCachedMaterials(FieldBlAct *blact);
static void FldBlAct_LoadCachedMaterial(FieldBlAct *blact, G3DResource *resource, u16 objCode, u32 sizeIdx, u8 atlasIdx,
                                        u32 flags);
static void FldBlAct_LoadConfigMaterial(FieldBlAct *blact, const FieldActorConfig *config, u32 flags);
static void FldBlAct_UnloadCachedMaterial(FieldBlAct *blact, u16 objCode);
static BOOL FieldBlAct_FindCachedObjMat(FieldBlAct *blact, u16 objCode, u16 *mat, u16 *flags);
static void FldBlAct_InitLoader(FieldBlAct *blact);
static void FldBlAct_FreeLoader(FieldBlAct *blact);
static BOOL FieldBlAct_IsAsyncLoadPending(FieldBlAct *blact);
static void FldBlAct_ReqAsyncLoad(FieldBlAct *blact, u16 objCode, u8 sizeIdx, u8 atlasIdx, u16 fileId, u32 flags);
static void FldBlAct_UpdateAsyncRead(FieldBlAct *blact);
static BOOL FldBlAct_ExecAsyncRead(FieldBlAct *blact, u16 objCode, u8 sizeIdx, u8 atlasIdx, u16 fileId, u32 flags);
static void FieldBlAct_FinishAsyncMatLoad(FieldBlAct *blact);
static BOOL FldBlAct_CheckAsyncLoading(FieldBlAct *blact, u16 objCode, u16 *flags);
static BOOL FldBlAct_CancelAsyncLoad(FieldBlAct *blact, u16 objCode);
static void FldAct_InitBlActNewMat(FieldBlAct *blact, FieldActor *actor, const FieldActorConfig *config, u16 *index,
                                   void (*callback)(u16 index, void *data), void *callbackData, const VecFx32 *pos);
static void FldBlAct_SetupPendingActor(FieldBlAct *blact, PendingActor *pending);
static void FldBlAct_UpdateAsyncActorSetup(FieldBlAct *blact);
static BOOL FldBlAct_HasOtherPendingActor(FieldBlAct *blact, u16 objCode, FieldActor *exclude);
static void FldBlAct_CancelPendingActor(FieldBlAct *blact, FieldActor *actor);
static void FldBlAct_ClearPendingActor(PendingActor *pending);
static void FldBlAct_InitUnloader(FieldBlAct *blact);
static void FldBlAct_FreeUnloader(FieldBlAct *blact);
static void FldBlAct_UpdateUnloadEntry(FieldBlAct *blact, UnloadEntry *entry);
static void FldBlAct_UpdateAsyncDelete(FieldBlAct *blact);
static BOOL FldBlAct_IsUnloadPending(FieldBlAct *blact, FieldActor *actor, u16 index);
static const FieldBillboardAnimSet *FieldBlAct_GetBillboardAnimSet(u32 ctrlType);
static u16 FldBlAct_LoadMaterial(FieldBlAct *blact, u16 objCode, u32 sizeIdx, u32 atlasIdx, G3DResource *resource,
                                 int mode);

// The scale of the billboard actors' scene
static const VecFx32 BLACT_SCENE_SCALE = { 0x7000, 0x7000, 0x7000 };

// The texture size bits of each sprite atlas size
static const u16 MMDL_TEXTURE_ATLAS_SIZE_BITS[64] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
};

static u32 BlActSys_LoadSingleUseTexMaterial(BlActSys *sys, G3DResource *resource, u32 sizeIdx, u32 atlasIdx) {
    u32 material = -1;

    if (GFL_G3DResUploadTexData(resource) == TRUE) {
        BlActScene *scene;
        const BillboardSize *size;
        NNSGfdTexKey texKey;
        NNSGfdPlttKey plttKey;

        scene = BlActSys_GetScene(sys);
        size = &MMDL_BILLBOARD_SIZES_INT[sizeIdx];
        texKey = GFL_G3DResGetTexVRAMHandle(resource);
        plttKey = GFL_G3DResGetPltVRAMHandle(resource);
        material = BlActSys_AddTexMat(sys,
                                      BlActScene_AddMaterialExistTex(scene, texKey, plttKey, 0,
                                                                     MMDL_TEXTURE_ATLAS_SIZE_BITS[atlasIdx],
                                                                     size->width, size->height),
                                      NULL);
        GFL_G3DResFree(resource);
    }
    return material;
}

static void FieldBlAct_FreeSingleUseMaterial(BlActSys *sys, u32 texMat) {
    u16 material = BlActSys_GetTexMatMaterial(sys, texMat);
    BlActScene *scene = BlActSys_GetScene(sys);

    BlActSys_FreeTexMat(sys, texMat);
    BlActScene_FreeMaterialVram(scene, material);
}

void FldActSys_InitBlAct(MMSys *system, BlActSys *sys, u32 count) {
    FieldBlAct *blact =
        GFL_HeapAllocate(FldActSys_GetFieldHeapID(system), sizeof(FieldBlAct), TRUE, "fldmmdl_blact.c", 364);
    BlActScene *scene;
    VecFx32 scale;
    u8 polyId;

    blact->system = system;
    blact->sys = sys;
    blact->count = count;
    FldActSys_BindFieldBlAct(system, blact);
    FldBlAct_InitMatCache(blact, count);
    FldBlAct_InitLoader(blact);
    FldBlAct_InitUnloader(blact);
    scene = BlActSys_GetScene(sys);
    scale = BLACT_SCENE_SCALE;
    BlActScene_SetScale(scene, &scale);
    polyId = 0;
    BlActScene_SetBasePolyID(scene, &polyId);
}

void func_ov036_021937a4(MMSys *system) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(system);

    FldBlAct_FreeUnloader(blact);
    FldBlAct_FreeLoader(blact);
    FldBlAct_FreeCachedMaterials(blact);
    GFL_HeapFree(blact);
    FldActSys_BindFieldBlAct(system, NULL);
}

void FldActSys_UpdateResourceLoader(MMSys *system) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(system);

    if (blact != NULL) {
        if (blact->loader != NULL) {
            FldBlAct_UpdateAsyncActorSetup(blact);
            FldBlAct_UpdateAsyncRead(blact);
        }
        FldBlAct_UpdateAsyncDelete(blact);
        blact->callbackFlag = 0;
    }
}

void FldActSys_FinishAsyncMatLoad(MMSys *system) {
    FieldBlAct_FinishAsyncMatLoad(FldActSys_GetFieldBlAct(system));
}

BOOL FldActSys_IsAsyncLoadPending(MMSys *system) {
    if (FieldBlAct_IsAsyncLoadPending(FldActSys_GetFieldBlAct(system)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

u8 func_ov036_0219381c(FieldActor *actor) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(GetActorMModelSystem(actor));

    return blact->callbackFlag;
}

void FldActSys_LoadStaticBlact(MMSys *system, u16 objCode) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(system);
    FieldActorConfig config;

    GetNPCMdlInfoForOBJCODE(system, objCode, &config);
    FldBlAct_LoadConfigMaterial(blact, &config, 0);
}

void FldActSys_LoadCachedBlact(MMSys *system) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(system);
    int count = GetMMSysMdlInfoCacheEntryCount(system);
    int i;
    FieldActorConfig config;

    for (i = 0; i < count; i++) {
        GetMMSysMdlInfoCacheEntry(system, i, &config);
        FldBlAct_LoadConfigMaterial(blact, &config, 0);
    }
}

static u16 FldActSys_AddBlActDefault(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat) {
    return FldActSys_AddBlAct(system, pos, sizeIdx, ctrlType, mat, 1);
}

static u16 FldActSys_AddBlAct(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat, u32 lights) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(system);
    BlActActorRequest request;
    u16 actor;
    const FieldBillboardAnimSet *anims;
    BlActSys *sys;
    const BillboardScale *scale = &BILLBOARD_SCALES[sizeIdx];

    request.texMat = mat;
    request.scaleX = BILLBOARD_SCALES[sizeIdx].x;
    request.scaleY = scale->y;
    request.pos = *pos;
    request.alpha = 31;
    request.visible = TRUE;
    request.lights = lights;
    request.callback = FieldBlAct_ActorCallback;
    request.callbackData = blact;
    actor = BlActSys_ExecActorRequests(blact->sys, blact->firstMat, &request, 1, 0);
    anims = FieldBlAct_GetBillboardAnimSet(ctrlType);
    if (anims->animations != NULL) {
        BlActSys_SetActorAnims(blact->sys, actor, anims->animations, anims->count);
        sys = blact->sys;
        BlActSys_SetActorAnim(sys, actor, 0);
        BlActSys_SetActorAnimating(sys, actor, TRUE);
    }
    return actor;
}

static u16 FldAct_InitBlActWithMat(FieldActor *actor, u16 mat) {
    MMSys *system = GetActorMModelSystem(actor);
    const FieldActorConfig *config = GetActorMdlInfo(actor);
    VecFx32 pos;

    CopyActorPosAllAdd(actor, &pos);
    return FldActSys_AddBlActDefault(system, &pos, config->billboardSize, config->spriteControllerType, mat);
}

static u16 FldActSys_AddBlActWithPalette(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat, u16 palMat) {
    u16 actor = FldActSys_AddBlActDefault(system, pos, sizeIdx, ctrlType, mat);

    if (palMat != BLACT_NONE) {
        BlActSys_ShareActorPltt(FldActSys_GetFieldBlAct(system)->sys, actor, palMat);
    }
    return actor;
}

static u16 FldAct_InitBlActWithPalette(FieldActor *actor, u16 mat, u16 palMat) {
    MMSys *system = GetActorMModelSystem(actor);
    const FieldActorConfig *config = GetActorMdlInfo(actor);
    VecFx32 pos;

    CopyActorPosAllAdd(actor, &pos);
    return FldActSys_AddBlActWithPalette(system, &pos, config->billboardSize, config->spriteControllerType, mat,
                                         palMat);
}

static u16 FldAct_InitBlActExistMat(FieldActor *actor, u16 objCode) {
    u16 info[2];

    FieldBlAct_FindCachedObjMat(FldActSys_GetFieldBlAct(GetActorMModelSystem(actor)), objCode, &info[1], &info[0]);
    return FldAct_InitBlActWithMat(actor, info[1]);
}

BOOL func_ov036_02193a00(FieldActor *actor, u16 *index) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(GetActorMModelSystem(actor));
    const FieldActorConfig *config = GetActorMdlInfo(actor);
    u16 info[2];

    FieldBlAct_FindCachedObjMat(blact, config->uid, &info[1], &info[0]);
    if (info[1] != BLACT_NONE && !(info[0] & 2)) {
        *index = FldAct_InitBlActExistMat(actor, config->uid);
        return TRUE;
    }
    *index = BLACT_NONE;
    FldAct_InitBlActNewMat(blact, actor, config, index, NULL, NULL, NULL);
    return FALSE;
}

static void FieldBlAct_UnbindActorObjCode(FieldBlAct *blact, FieldActor *actor, u16 index, u16 objCode) {
    u16 info[2];

    if (FieldBlAct_FindCachedObjMat(blact, objCode, &info[1], &info[0]) == TRUE) {
        if (info[0] & 2) {
            BlActSys_FreeMaterials(blact->sys, BlActSys_GetMatByActor(blact->sys, index), 1);
        }
        if (info[0] & 1) {
            if (!FldAct_CheckObjCodeShared(actor, objCode)) {
                FldBlAct_UnloadCachedMaterial(blact, objCode);
            }
        }
    }
}

void func_ov036_02193ac0(FieldActor *actor, u32 index) {
    u16 objCode = FldAct_GetObjCode(actor);
    FieldBlAct *blact = FldActSys_GetFieldBlAct(GetActorMModelSystem(actor));

    if (index == BLACT_NONE) {
        FldBlAct_CancelPendingActor(blact, actor);
        return;
    }
    if (FldBlAct_IsUnloadPending(blact, actor, index) != TRUE) {
        FieldBlAct_UnbindActorObjCode(blact, actor, index, objCode);
        BlActSys_DeleteActors(blact->sys, (u16)index, 1);
    }
}

BOOL func_ov036_02193b14(MMSys *system, const FieldActorConfig *config, u16 *out, VecFx32 *pos,
                         void (*callback)(u16 index, void *data), void *callbackData, u32 lights) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(system);
    u16 mat;

    FieldBlAct_FindCachedObjMat(blact, config->uid, &mat, &out[0]);
    if (!(out[0] & 2)) {
        out[1] = FldActSys_AddBlAct(system, pos, config->billboardSize, config->spriteControllerType, mat, lights);
        return TRUE;
    }
    out[1] = BLACT_NONE;
    FldAct_InitBlActNewMat(blact, NULL, config, &out[1], callback, callbackData, pos);
    return FALSE;
}

void func_ov036_02193b7c(MMSys *system, u16 *out) {
    FieldBlAct *blact = FldActSys_GetFieldBlAct(system);

    if (out[1] != BLACT_NONE) {
        if (out[0] & 2) {
            BlActSys_FreeMaterials(blact->sys, BlActSys_GetMatByActor(blact->sys, out[1]), 1);
        }
        BlActSys_DeleteActors(blact->sys, out[1], 1);
    }
    out[0] = 0;
    out[1] = BLACT_NONE;
}

static void FieldBlAct_ActorCallback(BlActSys *sys, u32 actor, void *data) {
    FieldBlAct *blact = data;

    blact->callbackFlag |= 1;
}

BlActSys *FieldBlAct_GetBlActSys(FieldBlAct *blact) {
    return blact->sys;
}

void func_ov036_02193bd0(FieldActor *actor, u16 *scaleX, u16 *scaleY) {
    u32 sizeIdx = GetActorMdlInfo(actor)->billboardSize;
    const BillboardScale *scale = &BILLBOARD_SCALES[sizeIdx];

    *scaleX = BILLBOARD_SCALES[sizeIdx].x;
    *scaleY = scale->y;
}

static void FldBlAct_CreateMatCache(MatCache *cache, int count, HeapID heapId) {
    MatCacheEntry *entry = GFL_HeapAllocate(heapId, count * sizeof(MatCacheEntry), TRUE, "fldmmdl_blact.c", 1177);

    cache->count = count;
    cache->entries = entry;
    do {
        entry->objCode = BLACT_NONE;
        entry->mat = BLACT_NONE;
        entry++;
        count--;
    } while (count != 0);
}

static void FldBlAct_FreeMatCache(MatCache *cache) {
    GFL_HeapFree(cache->entries);
}

static void FldBlAct_RegistCachedMat(MatCache *cache, u16 objCode, u16 mat, u16 flags) {
    int i = 0;
    MatCacheEntry *entry = cache->entries;

    do {
        if (entry->objCode == BLACT_NONE) {
            entry->objCode = objCode;
            entry->mat = mat;
            entry->flags = flags;
            return;
        }
        i++;
        entry++;
    } while (i < cache->count);
}

static void FldBlAct_RemoveCachedMat(MatCache *cache, u16 objCode) {
    int i = 0;
    MatCacheEntry *entry = cache->entries;

    do {
        if (entry->objCode == objCode) {
            entry->objCode = BLACT_NONE;
            entry->mat = BLACT_NONE;
            entry->flags = 0;
            return;
        }
        i++;
        entry++;
    } while (i < cache->count);
}

static BOOL FldBlAct_FindCachedMat(MatCache *cache, u16 objCode, u16 *mat, u16 *flags) {
    int i = 0;
    MatCacheEntry *entry = cache->entries;

    if (mat != NULL) {
        *mat = BLACT_NONE;
    }
    do {
        if (entry->objCode == objCode) {
            if (mat != NULL) {
                *mat = entry->mat;
            }
            if (flags != NULL) {
                *flags = entry->flags;
            }
            return TRUE;
        }
        i++;
        entry++;
    } while (i < cache->count);
    return FALSE;
}

static void FldBlAct_InitMatCache(FieldBlAct *blact, int count) {
    FldBlAct_CreateMatCache(&blact->cache, count, FldActSys_GetFieldHeapID(blact->system));
}

static void FldBlAct_FreeCachedMaterials(FieldBlAct *blact) {
    MatCacheEntry *entry = blact->cache.entries;
    int i = 0;

    do {
        if (entry->objCode != BLACT_NONE) {
            FldBlAct_UnloadCachedMaterial(blact, entry->objCode);
        }
        i++;
        entry++;
    } while (i < blact->cache.count);
    FldBlAct_FreeMatCache(&blact->cache);
}

static void FldBlAct_LoadCachedMaterial(FieldBlAct *blact, G3DResource *resource, u16 objCode, u32 sizeIdx, u8 atlasIdx,
                                        u32 flags) {
    int mode = -1;
    u16 mat;

    if (flags & 2) {
        mode = 2;
    }
    mat = FldBlAct_LoadMaterial(blact, objCode, sizeIdx, atlasIdx, resource, mode);
    FldBlAct_RegistCachedMat(&blact->cache, objCode, mat, flags);
}

static void FldBlAct_LoadConfigMaterial(FieldBlAct *blact, const FieldActorConfig *config, u32 flags) {
    const FieldActorResGroup *group = GetNPCMdlInfoG2DRscGroup(config);
    G3DResource *resource = GFL_G3DSysReadArcToolResource(GetMMSysDataArcHandle(blact->system), group->res1);

    FldBlAct_LoadCachedMaterial(blact, resource, config->uid, config->billboardSize, config->spriteAtlasSize, flags);
}

static void FldBlAct_UnloadCachedMaterial(FieldBlAct *blact, u16 objCode) {
    u16 info[2];

    FldBlAct_FindCachedMat(&blact->cache, objCode, &info[0], &info[1]);
    if (info[1] & 2) {
        BlActSys_FreeMaterials(blact->sys, info[0], 1);
    } else {
        FieldBlAct_FreeSingleUseMaterial(blact->sys, info[0]);
    }
    FldBlAct_RemoveCachedMat(&blact->cache, objCode);
}

static BOOL FieldBlAct_FindCachedObjMat(FieldBlAct *blact, u16 objCode, u16 *mat, u16 *flags) {
    return FldBlAct_FindCachedMat(&blact->cache, objCode, mat, flags);
}

static void FldBlAct_InitLoader(FieldBlAct *blact) {
    HeapID heapId = FldActSys_GetFieldHeapID(blact->system);
    AsyncLoader *loader = GFL_HeapAllocate(heapId, sizeof(AsyncLoader), TRUE, "fldmmdl_blact.c", 1515);
    AsyncRead *read;
    int i;

    loader->readCount = blact->count;
    loader->actorCount = GetActorLimit(blact->system);
    loader->resourceCount = 1;
    read = loader->reads =
        GFL_HeapAllocate(heapId, sizeof(AsyncRead) * loader->readCount, TRUE, "fldmmdl_blact.c", 1522);
    for (i = 0; i < loader->readCount; i++, read++) {
        read->objCode = BLACT_NONE;
    }
    loader->resources =
        GFL_HeapAllocate(heapId, sizeof(AsyncResource) * loader->resourceCount, TRUE, "fldmmdl_blact.c", 1535);
    loader->actors = GFL_HeapAllocate(heapId, sizeof(PendingActor) * loader->actorCount, TRUE, "fldmmdl_blact.c", 1537);
    loader->active = TRUE;
    blact->loader = loader;
}

static void FldBlAct_FreeLoader(FieldBlAct *blact) {
    AsyncLoader *loader = blact->loader;
    AsyncResource *resource;
    PendingActor *actor;
    u32 i;

    if (loader != NULL) {
        loader->active = FALSE;
        GFL_HeapFree(loader->reads);
        resource = loader->resources;
        for (i = 0; i < loader->resourceCount; i++, resource++) {
            if (resource->resource != NULL) {
                GFL_G3DResFree(resource->resource);
            }
        }
        GFL_HeapFree(loader->resources);
        actor = loader->actors;
        for (i = 0; i < loader->actorCount; i++, actor++) {
            if (actor->index != NULL && actor->resource != NULL) {
                GFL_G3DResFree(actor->resource);
            }
        }
        GFL_HeapFree(loader->actors);
        GFL_HeapFree(loader);
        blact->loader = NULL;
    }
}

static BOOL FieldBlAct_IsAsyncLoadPending(FieldBlAct *blact) {
    const AsyncLoader *loader = blact->loader;
    u32 i;
    AsyncRead *read = loader->reads;
    AsyncResource *resource;
    PendingActor *actor;

    for (i = 0; i < loader->readCount; i++, read++) {
        if (read->objCode != BLACT_NONE) {
            return TRUE;
        }
    }
    resource = loader->resources;
    for (i = 0; i < loader->resourceCount; i++, resource++) {
        if (resource->resource != NULL) {
            return TRUE;
        }
    }
    actor = loader->actors;
    for (i = 0; i < loader->actorCount; i++, actor++) {
        if (actor->index != NULL && actor->resource != NULL) {
            return TRUE;
        }
    }
    return FALSE;
}

static void FldBlAct_ReqAsyncLoad(FieldBlAct *blact, u16 objCode, u8 sizeIdx, u8 atlasIdx, u16 fileId, u32 flags) {
    u32 i;
    AsyncRead *read;

    for (i = 0, read = blact->loader->reads; i < blact->loader->readCount; i++, read++) {
        if (read->objCode == BLACT_NONE) {
            read->objCode = objCode;
            read->sizeIdx = sizeIdx;
            read->atlasIdx = atlasIdx;
            read->fileId = fileId;
            read->flags = flags;
            return;
        }
    }
}

static void FldBlAct_UpdateAsyncRead(FieldBlAct *blact) {
    AsyncLoader *loader = blact->loader;
    u32 i = 0;
    u32 done;
    u32 j;
    AsyncRead *read = loader->reads;

    done = 0;
    for (; i < loader->readCount; i++, read++) {
        if (read->objCode != BLACT_NONE) {
            j = FldBlAct_ExecAsyncRead(blact, read->objCode, read->sizeIdx, read->atlasIdx, read->fileId, read->flags);
            if (!j) {
                break;
            }
            done++;
            read->objCode = BLACT_NONE;
            if (done >= loader->resourceCount) {
                break;
            }
        }
    }
    if (done != 0) {
        read = loader->reads;
        for (i = 0; i < loader->readCount - 1; i++) {
            if (read[i].objCode == BLACT_NONE) {
                for (j = i + 1; j < loader->readCount; j++) {
                    if (read[j].objCode != BLACT_NONE) {
                        read[i] = read[j];
                        read[j].objCode = BLACT_NONE;
                        break;
                    }
                }
            }
        }
    }
}

static BOOL FldBlAct_ExecAsyncRead(FieldBlAct *blact, u16 objCode, u8 sizeIdx, u8 atlasIdx, u16 fileId, u32 flags) {
    AsyncLoader *loader = blact->loader;
    AsyncResource *resource = loader->resources;
    u32 i;

    for (i = 0; i < loader->resourceCount; i++, resource++) {
        if (resource->resource == NULL) {
            resource->state = 0;
            resource->objCode = objCode;
            resource->sizeIdx = sizeIdx;
            resource->atlasIdx = atlasIdx;
            resource->flags = flags;
            resource->resource = GFL_G3DSysReadArcToolResource(GetMMSysDataArcHandle(blact->system), fileId);
            resource->state = 1;
            return TRUE;
        }
    }
    return FALSE;
}

static void FieldBlAct_FinishAsyncMatLoad(FieldBlAct *blact) {
    AsyncLoader *loader = blact->loader;
    u32 i;
    AsyncResource *resource;

    if (loader != NULL && loader->active == TRUE) {
        i = 0;
        resource = loader->resources;
        for (; i < loader->resourceCount; i++, resource++) {
            if (resource->resource != NULL && resource->state == 1) {
                resource->state = 0;
                FldBlAct_LoadCachedMaterial(blact, resource->resource, resource->objCode, resource->sizeIdx,
                                            resource->atlasIdx, resource->flags);
                resource->resource = NULL;
            }
        }
    }
}

static BOOL FldBlAct_CheckAsyncLoading(FieldBlAct *blact, u16 objCode, u16 *flags) {
    u32 i;
    AsyncRead *read;
    u32 j;
    AsyncResource *resource;

    for (i = 0, read = blact->loader->reads; i < blact->loader->readCount; i++, read++) {
        if (read->objCode == objCode) {
            *flags = read->flags;
            return TRUE;
        }
    }
    for (j = 0, resource = blact->loader->resources; j < blact->loader->resourceCount; j++, resource++) {
        if (resource->objCode == objCode && resource->resource != NULL) {
            *flags = resource->flags;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FldBlAct_CancelAsyncLoad(FieldBlAct *blact, u16 objCode) {
    u32 i;
    AsyncRead *read;
    u32 j;
    AsyncResource *resource;

    for (i = 0, read = blact->loader->reads; i < blact->loader->readCount; i++, read++) {
        if (read->objCode == objCode) {
            read->objCode = BLACT_NONE;
            return TRUE;
        }
    }
    for (j = 0, resource = blact->loader->resources; j < blact->loader->resourceCount; j++, resource++) {
        if (resource->resource != NULL && resource->objCode == objCode) {
            resource->state = 0;
            GFL_G3DResFree(resource->resource);
            resource->resource = NULL;
            return TRUE;
        }
    }
    return FALSE;
}

static void FldAct_InitBlActNewMat(FieldBlAct *blact, FieldActor *actor, const FieldActorConfig *config, u16 *index,
                                   void (*callback)(u16 index, void *data), void *callbackData, const VecFx32 *pos) {
    u32 i = 0;
    u16 limit = GetActorLimit(blact->system);
    PendingActor *pending = blact->loader->actors;
    u16 info[2];

    for (; i < limit; i++, pending++) {
        if (pending->index == NULL) {
            pending->state = 0;
            pending->actor = actor;
            pending->index = index;
            pending->objCode = config->uid;
            pending->sizeIdx = config->billboardSize;
            pending->atlasIdx = config->spriteAtlasSize;
            pending->ctrlType = config->spriteControllerType;
            FieldBlAct_FindCachedObjMat(blact, pending->objCode, &info[1], &info[0]);
            if (info[1] == BLACT_NONE && !FldBlAct_CheckAsyncLoading(blact, pending->objCode, &info[0])) {
                const FieldActorResGroup *group = GetNPCMdlInfoG2DRscGroup(config);

                info[0] = 1;
                FldBlAct_ReqAsyncLoad(blact, pending->objCode, pending->sizeIdx, pending->atlasIdx, group->res1, 1);
            }
            pending->callback = callback;
            pending->callbackData = callbackData;
            if (pos != NULL) {
                pending->pos = *pos;
            }
            pending->state = 1;
            return;
        }
    }
}

static void FldBlAct_SetupPendingActor(FieldBlAct *blact, PendingActor *pending) {
    u16 index;
    u16 mat;
    u16 flags;
    u16 palMat = BLACT_NONE;

    pending->state = 0;
    if (pending->resource != NULL) {
        mat = FldBlAct_LoadMaterial(blact, pending->objCode, pending->sizeIdx, pending->atlasIdx, pending->resource, 1);
        pending->resource = NULL;
        FieldBlAct_FindCachedObjMat(blact, pending->objCode, &palMat, &flags);
    } else {
        FieldBlAct_FindCachedObjMat(blact, pending->objCode, &mat, &flags);
    }
    if (pending->actor != NULL) {
        index = FldAct_InitBlActWithPalette(pending->actor, mat, palMat);
    } else {
        index = FldActSys_AddBlActWithPalette(blact->system, &pending->pos, pending->sizeIdx, pending->ctrlType, mat,
                                              palMat);
    }
    *pending->index = index;
    if (pending->callback != NULL) {
        pending->callback(*pending->index, pending->callbackData);
    }
    if (pending->actor != NULL) {
        FldAct_InvokeUpdateCallback(pending->actor);
    }
}

static void FldBlAct_UpdateAsyncActorSetup(FieldBlAct *blact) {
    u32 i = 0;
    u16 limit = GetActorLimit(blact->system);
    PendingActor *pending = blact->loader->actors;
    u16 info[2];

    for (; i < limit; i++, pending++) {
        if (pending->index != NULL && pending->state == 1) {
            if (FieldBlAct_FindCachedObjMat(blact, pending->objCode, &info[0], &info[1]) == TRUE) {
                FldBlAct_SetupPendingActor(blact, pending);
                FldBlAct_ClearPendingActor(pending);
            }
        }
    }
}

static BOOL FldBlAct_HasOtherPendingActor(FieldBlAct *blact, u16 objCode, FieldActor *exclude) {
    u32 i = 0;
    u16 limit = GetActorLimit(blact->system);
    PendingActor *pending = blact->loader->actors;

    for (; i < limit; i++, pending++) {
        if (pending->index != NULL && pending->actor != NULL && pending->actor != exclude &&
            pending->objCode == objCode) {
            return TRUE;
        }
    }
    return FALSE;
}

static void FldBlAct_CancelPendingActor(FieldBlAct *blact, FieldActor *actor) {
    u32 i = 0;
    u16 limit = GetActorLimit(blact->system);
    PendingActor *pending = blact->loader->actors;
    u16 flags;

    for (; i < limit; i++, pending++) {
        if (pending->index != NULL && pending->actor == actor) {
            pending->state = 0;
            if (FldBlAct_CheckAsyncLoading(blact, pending->objCode, &flags) == TRUE && (flags & 1)) {
                if (!FldBlAct_HasOtherPendingActor(blact, pending->objCode, pending->actor)) {
                    FldBlAct_CancelAsyncLoad(blact, pending->objCode);
                }
            }
            if (pending->resource != NULL) {
                GFL_G3DResFree(pending->resource);
                pending->resource = NULL;
            }
            FldBlAct_ClearPendingActor(pending);
            return;
        }
    }
}

static void FldBlAct_ClearPendingActor(PendingActor *pending) {
    pending->state = 0;
    MI_CpuFill8(pending, 0, sizeof(PendingActor));
}

void FldAct_SwapObjCode(FieldActor *actor, u16 newObjCode) {
    int i;
    FieldBlAct *blact = FldActSys_GetFieldBlAct(GetActorMModelSystem(actor));
    UnloadEntry *entry = blact->unloader;
    u16 info[2];
    u16 index;

    FieldBlAct_FindCachedObjMat(blact, newObjCode, &info[1], &info[0]);
    if (info[1] != BLACT_NONE && !(info[0] & 2)) {
        FldAct_UpdateBlInfoForNewObjCode(actor, newObjCode);
        return;
    }
    index = FldAct_GetBlActIdx(actor, 0);
    if (index == BLACT_NONE) {
        FldAct_UpdateBlInfoForNewObjCode(actor, newObjCode);
        return;
    }
    // BUG: The search repeatedly checks the first slot instead of advancing to the next entry.
#ifdef BUGFIX
    for (i = 0; i < 4; i++, entry++) {
#else
    for (i = 0; i < 4; i++) {
#endif
        if (entry->actor == NULL) {
            break;
        }
    }
    if (i >= 4) {
        FldAct_UpdateBlInfoForNewObjCode(actor, newObjCode);
        return;
    }
    entry->actor = actor;
    entry->index = index;
    entry->objCode = FldAct_GetObjCode(actor);
    entry->newObjCode = newObjCode;
    FldAct_UpdateBlInfoForNewObjCode(actor, newObjCode);
    if (FldAct_GetBlActIdx(actor, 0) != BLACT_NONE) {
        FieldBlAct_UnbindActorObjCode(blact, actor, entry->index, entry->objCode);
        BlActSys_DeleteActors(blact->sys, entry->index, 1);
        entry->actor = NULL;
    }
}

static void FldBlAct_InitUnloader(FieldBlAct *blact) {
    blact->unloader = GFL_HeapAllocate(FldActSys_GetFieldHeapID(blact->system), 4 * sizeof(UnloadEntry), TRUE,
                                       "fldmmdl_blact.c", 2449);
}

static void FldBlAct_FreeUnloader(FieldBlAct *blact) {
    UnloadEntry *entry = blact->unloader;
    int i;

    if (entry != NULL) {
        for (i = 0; i < 4; i++, entry++) {
            if (entry->actor != NULL) {
                BlActSys_DeleteActors(blact->sys, entry->index, 1);
                entry->actor = NULL;
            }
        }
        GFL_HeapFree(blact->unloader);
    }
}

static void FldBlAct_UpdateUnloadEntry(FieldBlAct *blact, UnloadEntry *entry) {
    FieldActor *actor = entry->actor;
    u16 index = FldAct_GetBlActIdx(actor, 0);
    u16 objCode;

    if (index != BLACT_NONE) {
        if (FldAct_GetShadowGroup(actor) != 0) {
            BlActSys_SetActorLight(blact->sys, index, TRUE, 3);
            BlActSys_SetActorLight(blact->sys, index, FALSE, 0);
            BlActSys_SetActorLight(blact->sys, index, FALSE, 1);
            BlActSys_SetActorLight(blact->sys, index, FALSE, 2);
        } else {
            BlActSys_SetActorLight(blact->sys, index, TRUE, 0);
            BlActSys_SetActorLight(blact->sys, index, FALSE, 1);
            BlActSys_SetActorLight(blact->sys, index, FALSE, 2);
            BlActSys_SetActorLight(blact->sys, index, FALSE, 3);
        }
    }
    if (!FldAct_IsInUse(actor)) {
        FieldBlAct_UnbindActorObjCode(blact, actor, entry->index, entry->objCode);
        BlActSys_DeleteActors(blact->sys, entry->index, 1);
        entry->actor = NULL;
        return;
    }
    index = FldAct_GetBlActIdx(actor, 0);
    if (index != BLACT_NONE) {
        objCode = entry->objCode;
        if (objCode != FldAct_GetObjCode(actor)) {
            FieldBlAct_UnbindActorObjCode(blact, actor, entry->index, objCode);
        }
        BlActSys_DeleteActors(blact->sys, entry->index, 1);
        entry->actor = NULL;
    }
}

static void FldBlAct_UpdateAsyncDelete(FieldBlAct *blact) {
    UnloadEntry *entry = blact->unloader;
    int i;

    if (entry != NULL) {
        for (i = 0; i < 4; i++, entry++) {
            if (entry->actor != NULL) {
                FldBlAct_UpdateUnloadEntry(blact, entry);
            }
        }
    }
}

static BOOL FldBlAct_IsUnloadPending(FieldBlAct *blact, FieldActor *actor, u16 index) {
    UnloadEntry *entry = blact->unloader;
    int i;

    if (entry != NULL) {
        for (i = 0; i < 4; i++, entry++) {
            if (entry->actor != NULL && entry->actor == actor && entry->index == index) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static const FieldBillboardAnimSet *FieldBlAct_GetBillboardAnimSet(u32 ctrlType) {
    return &FIELD_BILLBOARD_ANIM_SETS[ctrlType];
}

static u16 FldBlAct_LoadMaterial(FieldBlAct *blact, u16 objCode, u32 sizeIdx, u32 atlasIdx, G3DResource *resource,
                                 int mode) {
    BlActMatRequest request;
    const BillboardSize *size;

    if (mode == -1) {
        return BlActSys_LoadSingleUseTexMaterial(blact->sys, resource, sizeIdx, atlasIdx);
    }
    request.texResource = resource;
    request.format = 0;
    request.size = MMDL_TEXTURE_ATLAS_SIZE_BITS[atlasIdx];
    size = &MMDL_BILLBOARD_SIZES_INT[sizeIdx];
    request.faceWidth = MMDL_BILLBOARD_SIZES_INT[sizeIdx].width;
    request.faceHeight = size->height;
    request.mode = mode;
    return BlActSys_ExecMatLoadRequests(blact->sys, &request, 1);
}
