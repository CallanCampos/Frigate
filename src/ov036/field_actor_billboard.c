// The billboard actors of the field actors: which materials they share, and the loading and unloading of their
// resources, which spreads over frames. The ROM names it fldmmdl_blact.c, by GFL_HeapAllocate's file argument.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), in English: swan's BlAct is BillboardActor.
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/field_actor.h"
#include "field/field_actor_drawdata.h"
#include "gfl/arc.h"
#include "gfl/billboard_actor.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/mi.h"

// A material of an object code, for the actors that share it. A free one has BILLBOARD_ACTOR_NONE as its object code
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

struct FieldBillboardActor {
    u8 callbackFlag;
    u16 count;
    BillboardActorSys *sys;
    u16 firstMat;
    u32 unkC;
    MMSys *system;
    MatCache cache;
    AsyncLoader *loader;
    UnloadEntry *unloader;
};

static u32 BillboardActorSys_LoadSingleUseTexMaterial(BillboardActorSys *sys, G3DResource *resource, u32 sizeIdx,
                                                      u32 atlasIdx);
static void FieldBillboardActor_FreeSingleUseMaterial(BillboardActorSys *sys, u32 texMat);
static u16 FldActSys_AddBillboardActorDefault(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat);
static u16 FldActSys_AddBillboardActor(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat, u32 lights);
static u16 FldAct_InitBillboardActorWithMat(FieldActor *actor, u16 mat);
static u16 FldActSys_AddBillboardActorWithPalette(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat,
                                                  u16 palMat);
static u16 FldAct_InitBillboardActorWithPalette(FieldActor *actor, u16 mat, u16 palMat);
static u16 FldAct_InitBillboardActorExistMat(FieldActor *actor, u16 objCode);
static void FieldBillboardActor_UnbindActorObjCode(FieldBillboardActor *billboardActor, FieldActor *actor, u16 index,
                                                   u16 objCode);
static void FieldBillboardActor_ActorCallback(BillboardActorSys *sys, u32 actor, void *data);
static void FldBillboardActor_CreateMatCache(MatCache *cache, int count, HeapID heapId);
static void FldBillboardActor_FreeMatCache(MatCache *cache);
static void FldBillboardActor_RegistCachedMat(MatCache *cache, u16 objCode, u16 mat, u16 flags);
static void FldBillboardActor_RemoveCachedMat(MatCache *cache, u16 objCode);
static BOOL FldBillboardActor_FindCachedMat(MatCache *cache, u16 objCode, u16 *mat, u16 *flags);
static void FldBillboardActor_InitMatCache(FieldBillboardActor *billboardActor, int count);
static void FldBillboardActor_FreeCachedMaterials(FieldBillboardActor *billboardActor);
static void FldBillboardActor_LoadCachedMaterial(FieldBillboardActor *billboardActor, G3DResource *resource,
                                                 u16 objCode, u32 sizeIdx, u8 atlasIdx, u32 flags);
static void FldBillboardActor_LoadConfigMaterial(FieldBillboardActor *billboardActor, const FieldActorConfig *config,
                                                 u32 flags);
static void FldBillboardActor_UnloadCachedMaterial(FieldBillboardActor *billboardActor, u16 objCode);
static BOOL FieldBillboardActor_FindCachedObjMat(FieldBillboardActor *billboardActor, u16 objCode, u16 *mat,
                                                 u16 *flags);
static void FldBillboardActor_InitLoader(FieldBillboardActor *billboardActor);
static void FldBillboardActor_FreeLoader(FieldBillboardActor *billboardActor);
static BOOL FieldBillboardActor_IsAsyncLoadPending(FieldBillboardActor *billboardActor);
static void FldBillboardActor_ReqAsyncLoad(FieldBillboardActor *billboardActor, u16 objCode, u8 sizeIdx, u8 atlasIdx,
                                           u16 fileId, u32 flags);
static void FldBillboardActor_UpdateAsyncRead(FieldBillboardActor *billboardActor);
static BOOL FldBillboardActor_ExecAsyncRead(FieldBillboardActor *billboardActor, u16 objCode, u8 sizeIdx, u8 atlasIdx,
                                            u16 fileId, u32 flags);
static void FieldBillboardActor_FinishAsyncMatLoad(FieldBillboardActor *billboardActor);
static BOOL FldBillboardActor_CheckAsyncLoading(FieldBillboardActor *billboardActor, u16 objCode, u16 *flags);
static BOOL FldBillboardActor_CancelAsyncLoad(FieldBillboardActor *billboardActor, u16 objCode);
static void FldAct_InitBillboardActorNewMat(FieldBillboardActor *billboardActor, FieldActor *actor,
                                            const FieldActorConfig *config, u16 *index,
                                            void (*callback)(u16 index, void *data), void *callbackData,
                                            const VecFx32 *pos);
static void FldBillboardActor_SetupPendingActor(FieldBillboardActor *billboardActor, PendingActor *pending);
static void FldBillboardActor_UpdateAsyncActorSetup(FieldBillboardActor *billboardActor);
static BOOL FldBillboardActor_HasOtherPendingActor(FieldBillboardActor *billboardActor, u16 objCode,
                                                   FieldActor *exclude);
static void FldBillboardActor_CancelPendingActor(FieldBillboardActor *billboardActor, FieldActor *actor);
static void FldBillboardActor_ClearPendingActor(PendingActor *pending);
static void FldBillboardActor_InitUnloader(FieldBillboardActor *billboardActor);
static void FldBillboardActor_FreeUnloader(FieldBillboardActor *billboardActor);
static void FldBillboardActor_UpdateUnloadEntry(FieldBillboardActor *billboardActor, UnloadEntry *entry);
static void FldBillboardActor_UpdateAsyncDelete(FieldBillboardActor *billboardActor);
static BOOL FldBillboardActor_IsUnloadPending(FieldBillboardActor *billboardActor, FieldActor *actor, u16 index);
static const FieldBillboardAnimSet *FieldBillboardActor_GetBillboardAnimSet(u32 ctrlType);
static u16 FldBillboardActor_LoadMaterial(FieldBillboardActor *billboardActor, u16 objCode, u32 sizeIdx, u32 atlasIdx,
                                          G3DResource *resource, int mode);

// The scale of the billboard actors' scene
static const VecFx32 BILLBOARD_ACTOR_SCENE_SCALE = { 0x7000, 0x7000, 0x7000 };

// The texture size bits of each sprite atlas size
static const u16 ACTOR_TEXTURE_ATLAS_SIZE_BITS[64] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
};

static u32 BillboardActorSys_LoadSingleUseTexMaterial(BillboardActorSys *sys, G3DResource *resource, u32 sizeIdx,
                                                      u32 atlasIdx) {
    u32 material = -1;

    if (GFL_G3DResUploadTexData(resource) == TRUE) {
        BillboardActorScene *scene;
        const BillboardSize *size;
        NNSGfdTexKey texKey;
        NNSGfdPlttKey plttKey;

        scene = BillboardActorSys_GetScene(sys);
        size = &ACTOR_BILLBOARD_SIZES_INT[sizeIdx];
        texKey = GFL_G3DResGetTexVRAMHandle(resource);
        plttKey = GFL_G3DResGetPltVRAMHandle(resource);
        material = BillboardActorSys_AddTexMat(
            sys,
            BillboardActorScene_AddMaterialExistTex(scene, texKey, plttKey, 0, ACTOR_TEXTURE_ATLAS_SIZE_BITS[atlasIdx],
                                                    size->width, size->height),
            NULL);
        GFL_G3DResFree(resource);
    }
    return material;
}

static void FieldBillboardActor_FreeSingleUseMaterial(BillboardActorSys *sys, u32 texMat) {
    u16 material = BillboardActorSys_GetTexMatMaterial(sys, texMat);
    BillboardActorScene *scene = BillboardActorSys_GetScene(sys);

    BillboardActorSys_FreeTexMat(sys, texMat);
    BillboardActorScene_FreeMaterialVram(scene, material);
}

void FldActSys_InitBillboardActor(MMSys *system, BillboardActorSys *sys, u32 count) {
    FieldBillboardActor *billboardActor =
        GFL_HeapAllocate(FldActSys_GetFieldHeapID(system), sizeof(FieldBillboardActor), TRUE, "fldmmdl_blact.c", 364);
    BillboardActorScene *scene;
    VecFx32 scale;
    u8 polyId;

    billboardActor->system = system;
    billboardActor->sys = sys;
    billboardActor->count = count;
    FldActSys_BindFieldBillboardActor(system, billboardActor);
    FldBillboardActor_InitMatCache(billboardActor, count);
    FldBillboardActor_InitLoader(billboardActor);
    FldBillboardActor_InitUnloader(billboardActor);
    scene = BillboardActorSys_GetScene(sys);
    scale = BILLBOARD_ACTOR_SCENE_SCALE;
    BillboardActorScene_SetScale(scene, &scale);
    polyId = 0;
    BillboardActorScene_SetBasePolyID(scene, &polyId);
}

void FldBillboardActor_Exit(MMSys *system) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(system);

    FldBillboardActor_FreeUnloader(billboardActor);
    FldBillboardActor_FreeLoader(billboardActor);
    FldBillboardActor_FreeCachedMaterials(billboardActor);
    GFL_HeapFree(billboardActor);
    FldActSys_BindFieldBillboardActor(system, NULL);
}

void FldActSys_UpdateResourceLoader(MMSys *system) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(system);

    if (billboardActor != NULL) {
        if (billboardActor->loader != NULL) {
            FldBillboardActor_UpdateAsyncActorSetup(billboardActor);
            FldBillboardActor_UpdateAsyncRead(billboardActor);
        }
        FldBillboardActor_UpdateAsyncDelete(billboardActor);
        billboardActor->callbackFlag = 0;
    }
}

void FldActSys_FinishAsyncMatLoad(MMSys *system) {
    FieldBillboardActor_FinishAsyncMatLoad(FldActSys_GetFieldBillboardActor(system));
}

BOOL FldActSys_IsAsyncLoadPending(MMSys *system) {
    if (FieldBillboardActor_IsAsyncLoadPending(FldActSys_GetFieldBillboardActor(system)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

u8 FldBillboardActor_GetEntryHookFlag(FieldActor *actor) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(GetActorMModelSystem(actor));

    return billboardActor->callbackFlag;
}

void FldActSys_LoadStaticBillboardActor(MMSys *system, u16 objCode) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(system);
    FieldActorConfig config;

    GetNPCMdlInfoForOBJCODE(system, objCode, &config);
    FldBillboardActor_LoadConfigMaterial(billboardActor, &config, 0);
}

void FldActSys_LoadCachedBillboardActor(MMSys *system) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(system);
    int count = GetMMSysMdlInfoCacheEntryCount(system);
    int i;
    FieldActorConfig config;

    for (i = 0; i < count; i++) {
        GetMMSysMdlInfoCacheEntry(system, i, &config);
        FldBillboardActor_LoadConfigMaterial(billboardActor, &config, 0);
    }
}

static u16 FldActSys_AddBillboardActorDefault(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat) {
    return FldActSys_AddBillboardActor(system, pos, sizeIdx, ctrlType, mat, 1);
}

static u16 FldActSys_AddBillboardActor(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat, u32 lights) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(system);
    BillboardActorRequest request;
    u16 actor;
    const FieldBillboardAnimSet *anims;
    BillboardActorSys *sys;
    const BillboardScale *scale = &BILLBOARD_SCALES[sizeIdx];

    request.texMat = mat;
    request.scaleX = BILLBOARD_SCALES[sizeIdx].x;
    request.scaleY = scale->y;
    request.pos = *pos;
    request.alpha = 31;
    request.visible = TRUE;
    request.lights = lights;
    request.callback = FieldBillboardActor_ActorCallback;
    request.callbackData = billboardActor;
    actor = BillboardActorSys_ExecActorRequests(billboardActor->sys, billboardActor->firstMat, &request, 1, 0);
    anims = FieldBillboardActor_GetBillboardAnimSet(ctrlType);
    if (anims->animations != NULL) {
        BillboardActorSys_SetActorAnims(billboardActor->sys, actor, anims->animations, anims->count);
        sys = billboardActor->sys;
        BillboardActorSys_SetActorAnim(sys, actor, 0);
        BillboardActorSys_SetActorAnimating(sys, actor, TRUE);
    }
    return actor;
}

static u16 FldAct_InitBillboardActorWithMat(FieldActor *actor, u16 mat) {
    MMSys *system = GetActorMModelSystem(actor);
    const FieldActorConfig *config = GetActorMdlInfo(actor);
    VecFx32 pos;

    CopyActorPosAllAdd(actor, &pos);
    return FldActSys_AddBillboardActorDefault(system, &pos, config->billboardSize, config->spriteControllerType, mat);
}

static u16 FldActSys_AddBillboardActorWithPalette(MMSys *system, VecFx32 *pos, u32 sizeIdx, u32 ctrlType, u16 mat,
                                                  u16 palMat) {
    u16 actor = FldActSys_AddBillboardActorDefault(system, pos, sizeIdx, ctrlType, mat);

    if (palMat != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys_ShareActorPltt(FldActSys_GetFieldBillboardActor(system)->sys, actor, palMat);
    }
    return actor;
}

static u16 FldAct_InitBillboardActorWithPalette(FieldActor *actor, u16 mat, u16 palMat) {
    MMSys *system = GetActorMModelSystem(actor);
    const FieldActorConfig *config = GetActorMdlInfo(actor);
    VecFx32 pos;

    CopyActorPosAllAdd(actor, &pos);
    return FldActSys_AddBillboardActorWithPalette(system, &pos, config->billboardSize, config->spriteControllerType,
                                                  mat, palMat);
}

static u16 FldAct_InitBillboardActorExistMat(FieldActor *actor, u16 objCode) {
    u16 info[2];

    FieldBillboardActor_FindCachedObjMat(FldActSys_GetFieldBillboardActor(GetActorMModelSystem(actor)), objCode,
                                         &info[1], &info[0]);
    return FldAct_InitBillboardActorWithMat(actor, info[1]);
}

BOOL FldBillboardActor_AddFieldActorOrReserve(FieldActor *actor, u16 *index) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(GetActorMModelSystem(actor));
    const FieldActorConfig *config = GetActorMdlInfo(actor);
    u16 info[2];

    FieldBillboardActor_FindCachedObjMat(billboardActor, config->uid, &info[1], &info[0]);
    if (info[1] != BILLBOARD_ACTOR_NONE && !(info[0] & 2)) {
        *index = FldAct_InitBillboardActorExistMat(actor, config->uid);
        return TRUE;
    }
    *index = BILLBOARD_ACTOR_NONE;
    FldAct_InitBillboardActorNewMat(billboardActor, actor, config, index, NULL, NULL, NULL);
    return FALSE;
}

static void FieldBillboardActor_UnbindActorObjCode(FieldBillboardActor *billboardActor, FieldActor *actor, u16 index,
                                                   u16 objCode) {
    u16 info[2];

    if (FieldBillboardActor_FindCachedObjMat(billboardActor, objCode, &info[1], &info[0]) == TRUE) {
        if (info[0] & 2) {
            BillboardActorSys_FreeMaterials(billboardActor->sys,
                                            BillboardActorSys_GetMatByActor(billboardActor->sys, index), 1);
        }
        if (info[0] & 1) {
            if (!FldAct_CheckObjCodeShared(actor, objCode)) {
                FldBillboardActor_UnloadCachedMaterial(billboardActor, objCode);
            }
        }
    }
}

void FldBillboardActor_DeleteFieldActor(FieldActor *actor, u32 index) {
    u16 objCode = FldAct_GetObjCode(actor);
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(GetActorMModelSystem(actor));

    if (index == BILLBOARD_ACTOR_NONE) {
        FldBillboardActor_CancelPendingActor(billboardActor, actor);
        return;
    }
    if (FldBillboardActor_IsUnloadPending(billboardActor, actor, index) != TRUE) {
        FieldBillboardActor_UnbindActorObjCode(billboardActor, actor, index, objCode);
        BillboardActorSys_DeleteActors(billboardActor->sys, (u16)index, 1);
    }
}

BOOL FldBillboardActor_AddActorPosOrReserve(MMSys *system, const FieldActorConfig *config, u16 *out, VecFx32 *pos,
                                            void (*callback)(u16 index, void *data), void *callbackData, u32 lights) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(system);
    u16 mat;

    FieldBillboardActor_FindCachedObjMat(billboardActor, config->uid, &mat, &out[0]);
    if (!(out[0] & 2)) {
        out[1] =
            FldActSys_AddBillboardActor(system, pos, config->billboardSize, config->spriteControllerType, mat, lights);
        return TRUE;
    }
    out[1] = BILLBOARD_ACTOR_NONE;
    FldAct_InitBillboardActorNewMat(billboardActor, NULL, config, &out[1], callback, callbackData, pos);
    return FALSE;
}

void FldBillboardActor_DeleteActorPos(MMSys *system, u16 *out) {
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(system);

    if (out[1] != BILLBOARD_ACTOR_NONE) {
        if (out[0] & 2) {
            BillboardActorSys_FreeMaterials(billboardActor->sys,
                                            BillboardActorSys_GetMatByActor(billboardActor->sys, out[1]), 1);
        }
        BillboardActorSys_DeleteActors(billboardActor->sys, out[1], 1);
    }
    out[0] = 0;
    out[1] = BILLBOARD_ACTOR_NONE;
}

static void FieldBillboardActor_ActorCallback(BillboardActorSys *sys, u32 actor, void *data) {
    FieldBillboardActor *billboardActor = data;

    billboardActor->callbackFlag |= 1;
}

BillboardActorSys *FieldBillboardActor_GetBillboardActorSys(FieldBillboardActor *billboardActor) {
    return billboardActor->sys;
}

void FldBillboardActor_GetActorSize(FieldActor *actor, u16 *scaleX, u16 *scaleY) {
    u32 sizeIdx = GetActorMdlInfo(actor)->billboardSize;
    const BillboardScale *scale = &BILLBOARD_SCALES[sizeIdx];

    *scaleX = BILLBOARD_SCALES[sizeIdx].x;
    *scaleY = scale->y;
}

static void FldBillboardActor_CreateMatCache(MatCache *cache, int count, HeapID heapId) {
    MatCacheEntry *entry = GFL_HeapAllocate(heapId, count * sizeof(MatCacheEntry), TRUE, "fldmmdl_blact.c", 1177);

    cache->count = count;
    cache->entries = entry;
    do {
        entry->objCode = BILLBOARD_ACTOR_NONE;
        entry->mat = BILLBOARD_ACTOR_NONE;
        entry++;
        count--;
    } while (count != 0);
}

static void FldBillboardActor_FreeMatCache(MatCache *cache) {
    GFL_HeapFree(cache->entries);
}

static void FldBillboardActor_RegistCachedMat(MatCache *cache, u16 objCode, u16 mat, u16 flags) {
    int i = 0;
    MatCacheEntry *entry = cache->entries;

    do {
        if (entry->objCode == BILLBOARD_ACTOR_NONE) {
            entry->objCode = objCode;
            entry->mat = mat;
            entry->flags = flags;
            return;
        }
        i++;
        entry++;
    } while (i < cache->count);
}

static void FldBillboardActor_RemoveCachedMat(MatCache *cache, u16 objCode) {
    int i = 0;
    MatCacheEntry *entry = cache->entries;

    do {
        if (entry->objCode == objCode) {
            entry->objCode = BILLBOARD_ACTOR_NONE;
            entry->mat = BILLBOARD_ACTOR_NONE;
            entry->flags = 0;
            return;
        }
        i++;
        entry++;
    } while (i < cache->count);
}

static BOOL FldBillboardActor_FindCachedMat(MatCache *cache, u16 objCode, u16 *mat, u16 *flags) {
    int i = 0;
    MatCacheEntry *entry = cache->entries;

    if (mat != NULL) {
        *mat = BILLBOARD_ACTOR_NONE;
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

static void FldBillboardActor_InitMatCache(FieldBillboardActor *billboardActor, int count) {
    FldBillboardActor_CreateMatCache(&billboardActor->cache, count, FldActSys_GetFieldHeapID(billboardActor->system));
}

static void FldBillboardActor_FreeCachedMaterials(FieldBillboardActor *billboardActor) {
    MatCacheEntry *entry = billboardActor->cache.entries;
    int i = 0;

    do {
        if (entry->objCode != BILLBOARD_ACTOR_NONE) {
            FldBillboardActor_UnloadCachedMaterial(billboardActor, entry->objCode);
        }
        i++;
        entry++;
    } while (i < billboardActor->cache.count);
    FldBillboardActor_FreeMatCache(&billboardActor->cache);
}

static void FldBillboardActor_LoadCachedMaterial(FieldBillboardActor *billboardActor, G3DResource *resource,
                                                 u16 objCode, u32 sizeIdx, u8 atlasIdx, u32 flags) {
    int mode = -1;
    u16 mat;

    if (flags & 2) {
        mode = 2;
    }
    mat = FldBillboardActor_LoadMaterial(billboardActor, objCode, sizeIdx, atlasIdx, resource, mode);
    FldBillboardActor_RegistCachedMat(&billboardActor->cache, objCode, mat, flags);
}

static void FldBillboardActor_LoadConfigMaterial(FieldBillboardActor *billboardActor, const FieldActorConfig *config,
                                                 u32 flags) {
    const FieldActorResGroup *group = GetNPCMdlInfoG2DRscGroup(config);
    G3DResource *resource = GFL_G3DSysReadArcToolResource(GetMMSysDataArcHandle(billboardActor->system), group->res1);

    FldBillboardActor_LoadCachedMaterial(billboardActor, resource, config->uid, config->billboardSize,
                                         config->spriteAtlasSize, flags);
}

static void FldBillboardActor_UnloadCachedMaterial(FieldBillboardActor *billboardActor, u16 objCode) {
    u16 info[2];

    FldBillboardActor_FindCachedMat(&billboardActor->cache, objCode, &info[0], &info[1]);
    if (info[1] & 2) {
        BillboardActorSys_FreeMaterials(billboardActor->sys, info[0], 1);
    } else {
        FieldBillboardActor_FreeSingleUseMaterial(billboardActor->sys, info[0]);
    }
    FldBillboardActor_RemoveCachedMat(&billboardActor->cache, objCode);
}

static BOOL FieldBillboardActor_FindCachedObjMat(FieldBillboardActor *billboardActor, u16 objCode, u16 *mat,
                                                 u16 *flags) {
    return FldBillboardActor_FindCachedMat(&billboardActor->cache, objCode, mat, flags);
}

static void FldBillboardActor_InitLoader(FieldBillboardActor *billboardActor) {
    HeapID heapId = FldActSys_GetFieldHeapID(billboardActor->system);
    AsyncLoader *loader = GFL_HeapAllocate(heapId, sizeof(AsyncLoader), TRUE, "fldmmdl_blact.c", 1515);
    AsyncRead *read;
    int i;

    loader->readCount = billboardActor->count;
    loader->actorCount = GetActorLimit(billboardActor->system);
    loader->resourceCount = 1;
    read = loader->reads =
        GFL_HeapAllocate(heapId, sizeof(AsyncRead) * loader->readCount, TRUE, "fldmmdl_blact.c", 1522);
    for (i = 0; i < loader->readCount; i++, read++) {
        read->objCode = BILLBOARD_ACTOR_NONE;
    }
    loader->resources =
        GFL_HeapAllocate(heapId, sizeof(AsyncResource) * loader->resourceCount, TRUE, "fldmmdl_blact.c", 1535);
    loader->actors = GFL_HeapAllocate(heapId, sizeof(PendingActor) * loader->actorCount, TRUE, "fldmmdl_blact.c", 1537);
    loader->active = TRUE;
    billboardActor->loader = loader;
}

static void FldBillboardActor_FreeLoader(FieldBillboardActor *billboardActor) {
    AsyncLoader *loader = billboardActor->loader;
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
        billboardActor->loader = NULL;
    }
}

static BOOL FieldBillboardActor_IsAsyncLoadPending(FieldBillboardActor *billboardActor) {
    const AsyncLoader *loader = billboardActor->loader;
    u32 i;
    AsyncRead *read = loader->reads;
    AsyncResource *resource;
    PendingActor *actor;

    for (i = 0; i < loader->readCount; i++, read++) {
        if (read->objCode != BILLBOARD_ACTOR_NONE) {
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

static void FldBillboardActor_ReqAsyncLoad(FieldBillboardActor *billboardActor, u16 objCode, u8 sizeIdx, u8 atlasIdx,
                                           u16 fileId, u32 flags) {
    u32 i;
    AsyncRead *read;

    for (i = 0, read = billboardActor->loader->reads; i < billboardActor->loader->readCount; i++, read++) {
        if (read->objCode == BILLBOARD_ACTOR_NONE) {
            read->objCode = objCode;
            read->sizeIdx = sizeIdx;
            read->atlasIdx = atlasIdx;
            read->fileId = fileId;
            read->flags = flags;
            return;
        }
    }
}

static void FldBillboardActor_UpdateAsyncRead(FieldBillboardActor *billboardActor) {
    AsyncLoader *loader = billboardActor->loader;
    u32 i = 0;
    u32 done;
    u32 j;
    AsyncRead *read = loader->reads;

    done = 0;
    for (; i < loader->readCount; i++, read++) {
        if (read->objCode != BILLBOARD_ACTOR_NONE) {
            j = FldBillboardActor_ExecAsyncRead(billboardActor, read->objCode, read->sizeIdx, read->atlasIdx,
                                                read->fileId, read->flags);
            if (!j) {
                break;
            }
            done++;
            read->objCode = BILLBOARD_ACTOR_NONE;
            if (done >= loader->resourceCount) {
                break;
            }
        }
    }
    if (done != 0) {
        read = loader->reads;
        for (i = 0; i < loader->readCount - 1; i++) {
            if (read[i].objCode == BILLBOARD_ACTOR_NONE) {
                for (j = i + 1; j < loader->readCount; j++) {
                    if (read[j].objCode != BILLBOARD_ACTOR_NONE) {
                        read[i] = read[j];
                        read[j].objCode = BILLBOARD_ACTOR_NONE;
                        break;
                    }
                }
            }
        }
    }
}

static BOOL FldBillboardActor_ExecAsyncRead(FieldBillboardActor *billboardActor, u16 objCode, u8 sizeIdx, u8 atlasIdx,
                                            u16 fileId, u32 flags) {
    AsyncLoader *loader = billboardActor->loader;
    AsyncResource *resource = loader->resources;
    u32 i;

    for (i = 0; i < loader->resourceCount; i++, resource++) {
        if (resource->resource == NULL) {
            resource->state = 0;
            resource->objCode = objCode;
            resource->sizeIdx = sizeIdx;
            resource->atlasIdx = atlasIdx;
            resource->flags = flags;
            resource->resource = GFL_G3DSysReadArcToolResource(GetMMSysDataArcHandle(billboardActor->system), fileId);
            resource->state = 1;
            return TRUE;
        }
    }
    return FALSE;
}

static void FieldBillboardActor_FinishAsyncMatLoad(FieldBillboardActor *billboardActor) {
    AsyncLoader *loader = billboardActor->loader;
    u32 i;
    AsyncResource *resource;

    if (loader != NULL && loader->active == TRUE) {
        i = 0;
        resource = loader->resources;
        for (; i < loader->resourceCount; i++, resource++) {
            if (resource->resource != NULL && resource->state == 1) {
                resource->state = 0;
                FldBillboardActor_LoadCachedMaterial(billboardActor, resource->resource, resource->objCode,
                                                     resource->sizeIdx, resource->atlasIdx, resource->flags);
                resource->resource = NULL;
            }
        }
    }
}

static BOOL FldBillboardActor_CheckAsyncLoading(FieldBillboardActor *billboardActor, u16 objCode, u16 *flags) {
    u32 i;
    AsyncRead *read;
    u32 j;
    AsyncResource *resource;

    for (i = 0, read = billboardActor->loader->reads; i < billboardActor->loader->readCount; i++, read++) {
        if (read->objCode == objCode) {
            *flags = read->flags;
            return TRUE;
        }
    }
    for (j = 0, resource = billboardActor->loader->resources; j < billboardActor->loader->resourceCount;
         j++, resource++) {
        if (resource->objCode == objCode && resource->resource != NULL) {
            *flags = resource->flags;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FldBillboardActor_CancelAsyncLoad(FieldBillboardActor *billboardActor, u16 objCode) {
    u32 i;
    AsyncRead *read;
    u32 j;
    AsyncResource *resource;

    for (i = 0, read = billboardActor->loader->reads; i < billboardActor->loader->readCount; i++, read++) {
        if (read->objCode == objCode) {
            read->objCode = BILLBOARD_ACTOR_NONE;
            return TRUE;
        }
    }
    for (j = 0, resource = billboardActor->loader->resources; j < billboardActor->loader->resourceCount;
         j++, resource++) {
        if (resource->resource != NULL && resource->objCode == objCode) {
            resource->state = 0;
            GFL_G3DResFree(resource->resource);
            resource->resource = NULL;
            return TRUE;
        }
    }
    return FALSE;
}

static void FldAct_InitBillboardActorNewMat(FieldBillboardActor *billboardActor, FieldActor *actor,
                                            const FieldActorConfig *config, u16 *index,
                                            void (*callback)(u16 index, void *data), void *callbackData,
                                            const VecFx32 *pos) {
    u32 i = 0;
    u16 limit = GetActorLimit(billboardActor->system);
    PendingActor *pending = billboardActor->loader->actors;
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
            FieldBillboardActor_FindCachedObjMat(billboardActor, pending->objCode, &info[1], &info[0]);
            if (info[1] == BILLBOARD_ACTOR_NONE &&
                !FldBillboardActor_CheckAsyncLoading(billboardActor, pending->objCode, &info[0])) {
                const FieldActorResGroup *group = GetNPCMdlInfoG2DRscGroup(config);

                info[0] = 1;
                FldBillboardActor_ReqAsyncLoad(billboardActor, pending->objCode, pending->sizeIdx, pending->atlasIdx,
                                               group->res1, 1);
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

static void FldBillboardActor_SetupPendingActor(FieldBillboardActor *billboardActor, PendingActor *pending) {
    u16 index;
    u16 mat;
    u16 flags;
    u16 palMat = BILLBOARD_ACTOR_NONE;

    pending->state = 0;
    if (pending->resource != NULL) {
        mat = FldBillboardActor_LoadMaterial(billboardActor, pending->objCode, pending->sizeIdx, pending->atlasIdx,
                                             pending->resource, 1);
        pending->resource = NULL;
        FieldBillboardActor_FindCachedObjMat(billboardActor, pending->objCode, &palMat, &flags);
    } else {
        FieldBillboardActor_FindCachedObjMat(billboardActor, pending->objCode, &mat, &flags);
    }
    if (pending->actor != NULL) {
        index = FldAct_InitBillboardActorWithPalette(pending->actor, mat, palMat);
    } else {
        index = FldActSys_AddBillboardActorWithPalette(billboardActor->system, &pending->pos, pending->sizeIdx,
                                                       pending->ctrlType, mat, palMat);
    }
    *pending->index = index;
    if (pending->callback != NULL) {
        pending->callback(*pending->index, pending->callbackData);
    }
    if (pending->actor != NULL) {
        FldAct_InvokeUpdateCallback(pending->actor);
    }
}

static void FldBillboardActor_UpdateAsyncActorSetup(FieldBillboardActor *billboardActor) {
    u32 i = 0;
    u16 limit = GetActorLimit(billboardActor->system);
    PendingActor *pending = billboardActor->loader->actors;
    u16 info[2];

    for (; i < limit; i++, pending++) {
        if (pending->index != NULL && pending->state == 1) {
            if (FieldBillboardActor_FindCachedObjMat(billboardActor, pending->objCode, &info[0], &info[1]) == TRUE) {
                FldBillboardActor_SetupPendingActor(billboardActor, pending);
                FldBillboardActor_ClearPendingActor(pending);
            }
        }
    }
}

static BOOL FldBillboardActor_HasOtherPendingActor(FieldBillboardActor *billboardActor, u16 objCode,
                                                   FieldActor *exclude) {
    u32 i = 0;
    u16 limit = GetActorLimit(billboardActor->system);
    PendingActor *pending = billboardActor->loader->actors;

    for (; i < limit; i++, pending++) {
        if (pending->index != NULL && pending->actor != NULL && pending->actor != exclude &&
            pending->objCode == objCode) {
            return TRUE;
        }
    }
    return FALSE;
}

static void FldBillboardActor_CancelPendingActor(FieldBillboardActor *billboardActor, FieldActor *actor) {
    u32 i = 0;
    u16 limit = GetActorLimit(billboardActor->system);
    PendingActor *pending = billboardActor->loader->actors;
    u16 flags;

    for (; i < limit; i++, pending++) {
        if (pending->index != NULL && pending->actor == actor) {
            pending->state = 0;
            if (FldBillboardActor_CheckAsyncLoading(billboardActor, pending->objCode, &flags) == TRUE && (flags & 1)) {
                if (!FldBillboardActor_HasOtherPendingActor(billboardActor, pending->objCode, pending->actor)) {
                    FldBillboardActor_CancelAsyncLoad(billboardActor, pending->objCode);
                }
            }
            if (pending->resource != NULL) {
                GFL_G3DResFree(pending->resource);
                pending->resource = NULL;
            }
            FldBillboardActor_ClearPendingActor(pending);
            return;
        }
    }
}

static void FldBillboardActor_ClearPendingActor(PendingActor *pending) {
    pending->state = 0;
    MI_CpuFill8(pending, 0, sizeof(PendingActor));
}

void FldAct_SwapObjCode(FieldActor *actor, u16 newObjCode) {
    int i;
    FieldBillboardActor *billboardActor = FldActSys_GetFieldBillboardActor(GetActorMModelSystem(actor));
    UnloadEntry *entry = billboardActor->unloader;
    u16 info[2];
    u16 index;

    FieldBillboardActor_FindCachedObjMat(billboardActor, newObjCode, &info[1], &info[0]);
    if (info[1] != BILLBOARD_ACTOR_NONE && !(info[0] & 2)) {
        FldAct_UpdateBlInfoForNewObjCode(actor, newObjCode);
        return;
    }
    index = FldAct_GetBillboardActorIdx(actor, 0);
    if (index == BILLBOARD_ACTOR_NONE) {
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
    if (FldAct_GetBillboardActorIdx(actor, 0) != BILLBOARD_ACTOR_NONE) {
        FieldBillboardActor_UnbindActorObjCode(billboardActor, actor, entry->index, entry->objCode);
        BillboardActorSys_DeleteActors(billboardActor->sys, entry->index, 1);
        entry->actor = NULL;
    }
}

static void FldBillboardActor_InitUnloader(FieldBillboardActor *billboardActor) {
    billboardActor->unloader = GFL_HeapAllocate(FldActSys_GetFieldHeapID(billboardActor->system),
                                                4 * sizeof(UnloadEntry), TRUE, "fldmmdl_blact.c", 2449);
}

static void FldBillboardActor_FreeUnloader(FieldBillboardActor *billboardActor) {
    UnloadEntry *entry = billboardActor->unloader;
    int i;

    if (entry != NULL) {
        for (i = 0; i < 4; i++, entry++) {
            if (entry->actor != NULL) {
                BillboardActorSys_DeleteActors(billboardActor->sys, entry->index, 1);
                entry->actor = NULL;
            }
        }
        GFL_HeapFree(billboardActor->unloader);
    }
}

static void FldBillboardActor_UpdateUnloadEntry(FieldBillboardActor *billboardActor, UnloadEntry *entry) {
    FieldActor *actor = entry->actor;
    u16 index = FldAct_GetBillboardActorIdx(actor, 0);
    u16 objCode;

    if (index != BILLBOARD_ACTOR_NONE) {
        if (FldAct_GetShadowGroup(actor) != 0) {
            BillboardActorSys_SetActorLight(billboardActor->sys, index, TRUE, 3);
            BillboardActorSys_SetActorLight(billboardActor->sys, index, FALSE, 0);
            BillboardActorSys_SetActorLight(billboardActor->sys, index, FALSE, 1);
            BillboardActorSys_SetActorLight(billboardActor->sys, index, FALSE, 2);
        } else {
            BillboardActorSys_SetActorLight(billboardActor->sys, index, TRUE, 0);
            BillboardActorSys_SetActorLight(billboardActor->sys, index, FALSE, 1);
            BillboardActorSys_SetActorLight(billboardActor->sys, index, FALSE, 2);
            BillboardActorSys_SetActorLight(billboardActor->sys, index, FALSE, 3);
        }
    }
    if (!FldAct_IsInUse(actor)) {
        FieldBillboardActor_UnbindActorObjCode(billboardActor, actor, entry->index, entry->objCode);
        BillboardActorSys_DeleteActors(billboardActor->sys, entry->index, 1);
        entry->actor = NULL;
        return;
    }
    index = FldAct_GetBillboardActorIdx(actor, 0);
    if (index != BILLBOARD_ACTOR_NONE) {
        objCode = entry->objCode;
        if (objCode != FldAct_GetObjCode(actor)) {
            FieldBillboardActor_UnbindActorObjCode(billboardActor, actor, entry->index, objCode);
        }
        BillboardActorSys_DeleteActors(billboardActor->sys, entry->index, 1);
        entry->actor = NULL;
    }
}

static void FldBillboardActor_UpdateAsyncDelete(FieldBillboardActor *billboardActor) {
    UnloadEntry *entry = billboardActor->unloader;
    int i;

    if (entry != NULL) {
        for (i = 0; i < 4; i++, entry++) {
            if (entry->actor != NULL) {
                FldBillboardActor_UpdateUnloadEntry(billboardActor, entry);
            }
        }
    }
}

static BOOL FldBillboardActor_IsUnloadPending(FieldBillboardActor *billboardActor, FieldActor *actor, u16 index) {
    UnloadEntry *entry = billboardActor->unloader;
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

static const FieldBillboardAnimSet *FieldBillboardActor_GetBillboardAnimSet(u32 ctrlType) {
    return &FIELD_BILLBOARD_ANIM_SETS[ctrlType];
}

static u16 FldBillboardActor_LoadMaterial(FieldBillboardActor *billboardActor, u16 objCode, u32 sizeIdx, u32 atlasIdx,
                                          G3DResource *resource, int mode) {
    BillboardActorMatRequest request;
    const BillboardSize *size;

    if (mode == -1) {
        return BillboardActorSys_LoadSingleUseTexMaterial(billboardActor->sys, resource, sizeIdx, atlasIdx);
    }
    request.texResource = resource;
    request.format = 0;
    request.size = ACTOR_TEXTURE_ATLAS_SIZE_BITS[atlasIdx];
    size = &ACTOR_BILLBOARD_SIZES_INT[sizeIdx];
    request.faceWidth = ACTOR_BILLBOARD_SIZES_INT[sizeIdx].width;
    request.faceHeight = size->height;
    request.mode = mode;
    return BillboardActorSys_ExecMatLoadRequests(billboardActor->sys, &request, 1);
}
