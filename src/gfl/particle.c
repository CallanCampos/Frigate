#include "types.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"
#include "spl/spl.h"

// Particle systems over SPL managers: each allocates from its own work memory through one of 16 allocators, and gives
// back the VRAM its resource took when freed

// Most emitters and particles a system has at once
#define PARTICLE_EMITTER_MAX 24
#define PARTICLE_MAX 200

// The kinds of an emitter's behaviors, as ParticleEmitter_FindBehavior finds them
enum {
    PARTICLE_BEHAVIOR_GRAVITY,
    PARTICLE_BEHAVIOR_RANDOM,
    PARTICLE_BEHAVIOR_MAGNET,
    PARTICLE_BEHAVIOR_SPIN,
    PARTICLE_BEHAVIOR_COLLISION_PLANE,
    PARTICLE_BEHAVIOR_CONVERGENCE,
};

typedef struct {
    ParticleSystem *systems[PARTICLE_SYSTEM_MAX];
    // What Particle_CreateEmitterWithCallback's callback reads through Particle_GetCallbackParam
    void *callbackParam;
    // The system whose resource is being uploaded, which the linked-list VRAM allocators record keys in
    ParticleSystem *uploading;
} ParticleGlobal;

static ParticleGlobal *sParticle;

static void *Particle_AllocForSystem0(u32 size);
static void *Particle_AllocForSystem1(u32 size);
static void *Particle_AllocForSystem2(u32 size);
static void *Particle_AllocForSystem3(u32 size);
static void *Particle_AllocForSystem4(u32 size);
static void *Particle_AllocForSystem5(u32 size);
static void *Particle_AllocForSystem6(u32 size);
static void *Particle_AllocForSystem7(u32 size);
static void *Particle_AllocForSystem8(u32 size);
static void *Particle_AllocForSystem9(u32 size);
static void *Particle_AllocForSystem10(u32 size);
static void *Particle_AllocForSystem11(u32 size);
static void *Particle_AllocForSystem12(u32 size);
static void *Particle_AllocForSystem13(u32 size);
static void *Particle_AllocForSystem14(u32 size);
static void *Particle_AllocForSystem15(u32 size);
static void Particle_UploadResourceTask(TCB *tcb, void *data);
static const void *ParticleEmitter_FindBehavior(SPLEmitter *emitter, int type);
static u32 Particle_AllocTexFrm(u32 size, BOOL is4x4comp);
static u32 Particle_AllocTexLnk(u32 size, BOOL is4x4comp);
static u32 Particle_AllocPlttFrm(u32 size, BOOL is4pltt);
static u32 Particle_AllocPlttLnk(u32 size, BOOL is4pltt);

static const VecFx32 sUnitScale = { FX32_ONE, FX32_ONE, FX32_ONE };
static const VecFx32 sDefaultCameraPos = { 0, 0, 4 * FX32_ONE };
static const VecFx32 sDefaultCameraUp = { 0, FX32_ONE, 0 };
static const VecFx32 sDefaultCameraTarget = { 0, 0, 0 };
static const SPLAllocFunc sAllocFuncs[PARTICLE_SYSTEM_MAX] = {
    Particle_AllocForSystem0,  Particle_AllocForSystem1,  Particle_AllocForSystem2,  Particle_AllocForSystem3,
    Particle_AllocForSystem4,  Particle_AllocForSystem5,  Particle_AllocForSystem6,  Particle_AllocForSystem7,
    Particle_AllocForSystem8,  Particle_AllocForSystem9,  Particle_AllocForSystem10, Particle_AllocForSystem11,
    Particle_AllocForSystem12, Particle_AllocForSystem13, Particle_AllocForSystem14, Particle_AllocForSystem15,
};

void Particle_Init(HeapID heapId) {
    int i;

    sParticle = GFL_HeapAllocate(heapId, sizeof(ParticleGlobal), FALSE, "particle.c", 190);
    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        sParticle->systems[i] = NULL;
    }
    sParticle->callbackParam = NULL;
    sParticle->uploading = NULL;
}

void Particle_DrawAndUpdateAll(void) {
    if (Particle_GetSystemCount() != 0) {
        Particle_DrawAll();
        Particle_UpdateAll();
    }
}

ParticleSystem *Particle_CreateSystem(void *work, u32 size, BOOL camera, u32 heapId) {
    return Particle_CreateSystemEx(work, size, camera, 5, 6, 63, heapId);
}

ParticleSystem *Particle_CreateSystemEx(void *work, u32 size, BOOL camera, u32 fixPolyID, u32 minPolyID, u32 maxPolyID,
                                        u32 heapId) {
    ParticleSystem *system;
    int id;

    for (id = 0; id < PARTICLE_SYSTEM_MAX; id++) {
        if (sParticle->systems[id] == NULL) {
            break;
        }
    }
    if (id >= PARTICLE_SYSTEM_MAX) {
        return NULL;
    }
    system = GFL_HeapAllocate(heapId, sizeof(ParticleSystem), FALSE, "particle.c", 285);
    MI_CpuFill8(system, 0, sizeof(ParticleSystem));
    system->cameraPos = sDefaultCameraPos;
    system->cameraUp = sDefaultCameraUp;
    system->cameraTarget = sDefaultCameraTarget;
    system->extResource = FALSE;
    MI_CpuFill8(work, 0, size);
    system->workStart = work;
    system->work = work;
    system->workEnd = (u8 *)work + size;
    system->id = id;
    sParticle->systems[id] = system;
    if (camera == TRUE) {
        Particle_SetCamera(system, NULL, 2 * FX32_ONE, NULL, NULL, NULL, heapId);
    }
    system->manager =
        SPLManager_New(sAllocFuncs[id], PARTICLE_EMITTER_MAX, PARTICLE_MAX, fixPolyID, minPolyID, maxPolyID);
    return system;
}

static inline void Particle_Remove(ParticleSystem *system) {
    int i;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] == system) {
            sParticle->systems[i] = NULL;
            break;
        }
    }
}

void Particle_DeleteSystem(ParticleSystem *system) {
    int i;

    Particle_DeleteAllEmitters(system);
    if (system->vramRelease & PARTICLE_VRAM_TEX_FRM) {
        NNS_GfdSetFrmTexVramState(&system->texState);
    } else if (system->vramRelease & PARTICLE_VRAM_TEX_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            if (system->texKeys[i] != 0) {
                NNS_GfdFreeLnkTexVram(system->texKeys[i]);
                system->texKeys[i] = 0;
            }
        }
    }
    if (system->vramRelease & PARTICLE_VRAM_PLTT_FRM) {
        NNS_GfdSetFrmPlttVramState(&system->plttState);
    } else if (system->vramRelease & PARTICLE_VRAM_PLTT_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            if (system->plttKeys[i] != 0) {
                NNS_GfdFreeLnkPlttVram(system->plttKeys[i]);
                system->plttKeys[i] = 0;
            }
        }
    }
    system->vramRelease = 0;
    system->lastEmitter = NULL;
    if (system->resource != NULL && !system->extResource) {
        GFL_HeapFree(system->resource);
        system->resource = NULL;
    }
    Particle_Remove(system);
    if (system->camera != NULL) {
        GFL_G3DCameraFree(system->camera);
    }
    GFL_HeapFree(system);
}

void Particle_Free(void) {
    int i;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            Particle_DeleteSystem(sParticle->systems[i]);
        }
    }
    GFL_HeapFree(sParticle);
    sParticle = NULL;
}

// Takes size bytes of the system's work, rounded up to a word
static inline void *Particle_Alloc(ParticleSystem *system, u32 size) {
    void *ret = system->work;
    u32 next = (u32)system->work + size;

    if (next % 4 != 0) {
        next += 4 - next % 4;
    }
    system->work = (void *)next;
    return ret;
}

static void *Particle_AllocForSystem0(u32 size) {
    return Particle_Alloc(sParticle->systems[0], size);
}

static void *Particle_AllocForSystem1(u32 size) {
    return Particle_Alloc(sParticle->systems[1], size);
}

static void *Particle_AllocForSystem2(u32 size) {
    return Particle_Alloc(sParticle->systems[2], size);
}

static void *Particle_AllocForSystem3(u32 size) {
    return Particle_Alloc(sParticle->systems[3], size);
}

static void *Particle_AllocForSystem4(u32 size) {
    return Particle_Alloc(sParticle->systems[4], size);
}

static void *Particle_AllocForSystem5(u32 size) {
    return Particle_Alloc(sParticle->systems[5], size);
}

static void *Particle_AllocForSystem6(u32 size) {
    return Particle_Alloc(sParticle->systems[6], size);
}

static void *Particle_AllocForSystem7(u32 size) {
    return Particle_Alloc(sParticle->systems[7], size);
}

static void *Particle_AllocForSystem8(u32 size) {
    return Particle_Alloc(sParticle->systems[8], size);
}

static void *Particle_AllocForSystem9(u32 size) {
    return Particle_Alloc(sParticle->systems[9], size);
}

static void *Particle_AllocForSystem10(u32 size) {
    return Particle_Alloc(sParticle->systems[10], size);
}

static void *Particle_AllocForSystem11(u32 size) {
    return Particle_Alloc(sParticle->systems[11], size);
}

static void *Particle_AllocForSystem12(u32 size) {
    return Particle_Alloc(sParticle->systems[12], size);
}

static void *Particle_AllocForSystem13(u32 size) {
    return Particle_Alloc(sParticle->systems[13], size);
}

static void *Particle_AllocForSystem14(u32 size) {
    return Particle_Alloc(sParticle->systems[14], size);
}

static void *Particle_AllocForSystem15(u32 size) {
    return Particle_Alloc(sParticle->systems[15], size);
}

void *Particle_ReadResource(u32 arcId, u32 fileId, u32 heapId) {
    return GFL_ArcSysReadHeapNew(arcId, fileId, heapId);
}

void Particle_SetResource(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr) {
    int release;
    int i;

    release = GFL_G3DSysUsesFrmTexVramMgr() == TRUE ? PARTICLE_VRAM_TEX_FRM : PARTICLE_VRAM_TEX_LNK;
    release |= GFL_G3DSysUsesFrmPlttVramMgr() == TRUE ? PARTICLE_VRAM_PLTT_FRM : PARTICLE_VRAM_PLTT_LNK;
    system->vramRelease = release;
    if (release & PARTICLE_VRAM_TEX_FRM) {
        NNS_GfdGetFrmTexVramState(&system->texState);
        system->texAllocFunc = Particle_AllocTexFrm;
    } else if (release & PARTICLE_VRAM_TEX_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            system->texKeys[i] = 0;
        }
        system->texAllocFunc = Particle_AllocTexLnk;
        system->texKeyCount = 0;
    }
    if (release & PARTICLE_VRAM_PLTT_FRM) {
        NNS_GfdGetFrmPlttVramState(&system->plttState);
        system->palAllocFunc = Particle_AllocPlttFrm;
    } else if (release & PARTICLE_VRAM_PLTT_LNK) {
        for (i = 0; i < PARTICLE_VRAM_KEY_MAX; i++) {
            system->plttKeys[i] = 0;
        }
        system->palAllocFunc = Particle_AllocPlttLnk;
        system->plttKeyCount = 0;
    }
    system->resource = resource;
    SPLManager_LoadResources(system->manager, resource);
    if (immediate == TRUE) {
        Particle_UploadResource(system);
    } else if (tcbMgr != NULL) {
        GFL_TCBMgrAddTask(tcbMgr, Particle_UploadResourceTask, system, 0);
    }
}

void Particle_SetExternalResource(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr) {
    Particle_SetResource(system, resource, immediate, tcbMgr);
    system->extResource = TRUE;
}

void Particle_SetResourceNoUpload(ParticleSystem *system, void *resource) {
    Particle_SetResource(system, resource, FALSE, NULL);
}

void Particle_UploadResource(ParticleSystem *system) {
    sParticle->uploading = system;
    if (system->texAllocFunc == NULL) {
        SPLManager_UploadTextures(system->manager);
    } else {
        SPLManager_UploadTexturesEx(system->manager, system->texAllocFunc);
    }
    if (system->palAllocFunc == NULL) {
        SPLManager_UploadPalettes(system->manager);
    } else {
        SPLManager_UploadPalettesEx(system->manager, system->palAllocFunc);
    }
    sParticle->uploading = NULL;
}

static void Particle_UploadResourceTask(TCB *tcb, void *data) {
    Particle_UploadResource(data);
    GFL_TCBRemove(tcb);
}

void Particle_DrawSystem(ParticleSystem *system) {
    G3DCameraProjection projection;
    FxLookAt lookAt;

    if (system->camera != NULL) {
        VecFx32 scale;
        MtxFx33 rot;

        // Draw with the system's camera, from the origin
        GFL_G3DSysMtxGetProjection(&projection);
        GFL_G3DSysMtxGetViewLookAt(&lookAt);
        scale = sUnitScale;
        {
            VecFx32 trans = { 0, 0, 0 };

            MTX_Identity33_(&rot);
            NNS_G3dGlbSetBaseTrans(&trans);
            NNS_G3dGlbSetBaseRot(&rot);
            NNS_G3dGlbSetBaseScale(&scale);
        }
        GFL_G3DCameraFlush(system->camera);
        GFL_G3DSysMtxViewFlush();
        NNS_G3dGeFlushBuffer();
    }
    SPLManager_Draw(system->manager, &NNS_G3dGlb.cameraMtx);
    if (system->camera != NULL) {
        GFL_G3DSysMtxSetProjection(&projection);
        GFL_G3DSysMtxSetViewLookAt(&lookAt);
        GFL_G3DSysMtxViewFlush();
        NNS_G3dGeFlushBuffer();
    }
}

void Particle_UpdateSystem(ParticleSystem *system) {
    SPLManager_Update(system->manager);
}

int Particle_GetSystemCount(void) {
    int count = 0;
    int i;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            count++;
        }
    }
    return count;
}

int Particle_DrawAll(void) {
    int i;
    int count = 0;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            Particle_DrawSystem(sParticle->systems[i]);
            count++;
        }
    }
    return count;
}

int Particle_UpdateAll(void) {
    int i;
    int count = 0;

    for (i = 0; i < PARTICLE_SYSTEM_MAX; i++) {
        if (sParticle->systems[i] != NULL) {
            Particle_UpdateSystem(sParticle->systems[i]);
            count++;
        }
    }
    return count;
}

SPLEmitter *Particle_CreateEmitter(ParticleSystem *system, int resourceId, const VecFx32 *pos) {
    SPLEmitter *emitter = SPLManager_CreateEmitter(system->manager, resourceId, pos);

    system->lastEmitter = emitter;
    return emitter;
}

SPLEmitter *Particle_CreateEmitterWithCallback(ParticleSystem *system, int resourceId, SPLEmitterCallback callback,
                                               void *param) {
    SPLEmitter *emitter;

    if (system->manager->activeEmitters.count >= PARTICLE_EMITTER_MAX) {
        return (SPLEmitter *)-1;
    }
    sParticle->callbackParam = param;
    emitter = SPLManager_CreateEmitterWithCallback(system->manager, resourceId, callback);
    sParticle->callbackParam = NULL;
    system->lastEmitter = emitter;
    return emitter;
}

int Particle_GetEmitterCount(ParticleSystem *system) {
    return system->manager->activeEmitters.count;
}

void Particle_DeleteAllEmitters(ParticleSystem *system) {
    SPLManager_DeleteAllEmitters(system->manager);
}

void Particle_DeleteEmitter(ParticleSystem *system, SPLEmitter *emitter) {
    SPLManager_DeleteEmitter(system->manager, emitter);
}

void *Particle_GetWork(ParticleSystem *system) {
    return system->workStart;
}

void Particle_SetCamera(ParticleSystem *system, G3DCameraProjection *projection, u16 fov, const VecFx32 *position,
                        const VecFx32 *upVector, const VecFx32 *target, u32 heapId) {
    system->unk24.x = 0;
    system->unk24.y = 0;
    system->unk24.z = 0;
    system->cameraFov = fov;
    if (position == NULL) {
        position = &sDefaultCameraPos;
    }
    if (upVector == NULL) {
        upVector = &sDefaultCameraUp;
    }
    if (target == NULL) {
        target = &sDefaultCameraTarget;
    }
    if (projection != NULL) {
        system->cameraProjection = projection->type;
        system->camera = GFL_G3DCameraCreate(projection->type, projection->param1, projection->param2,
                                             projection->param3, projection->param4, projection->near, projection->far,
                                             projection->ndcRangeOverride, position, upVector, target, heapId);
    } else {
        system->cameraProjection = G3DCAM_PROJECTION_PERSPECTIVE;
        // 45 degrees across a 4:3 view
        system->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, 0xb50, 0xb50, 0x1555, 0, FX32_ONE,
                                             900 * FX32_ONE, 0, position, upVector, target, heapId);
    }
}

void Particle_FreeCamera(ParticleSystem *system) {
    GFL_G3DCameraFree(system->camera);
    system->camera = NULL;
}

void *Particle_GetCallbackParam(void) {
    return sParticle->callbackParam;
}

G3DCamera *Particle_GetCamera(ParticleSystem *system) {
    return system->camera;
}

void Particle_GetCameraPos(ParticleSystem *system, VecFx32 *pos) {
    *pos = system->cameraPos;
}

void Particle_SetCameraPos(ParticleSystem *system, const VecFx32 *pos) {
    system->cameraPos = *pos;
}

void ParticleEmitter_GetAxis(SPLEmitter *emitter, VecFx16 *axis) {
    *axis = emitter->axis;
}

fx32 ParticleEmitter_GetRadius(SPLEmitter *emitter) {
    return emitter->radius;
}

void ParticleEmitter_SetRadius(SPLEmitter *emitter, fx32 radius) {
    emitter->radius = radius;
}

fx32 ParticleEmitter_GetLength(SPLEmitter *emitter) {
    return emitter->length;
}

void ParticleEmitter_SetLength(SPLEmitter *emitter, fx32 length) {
    emitter->length = length;
}

fx16 ParticleEmitter_GetInitVelPositionAmplifier(SPLEmitter *emitter) {
    return emitter->initVelPositionAmplifier;
}

void ParticleEmitter_SetInitVelPositionAmplifier(SPLEmitter *emitter, fx32 amplifier) {
    emitter->initVelPositionAmplifier = amplifier;
}

fx16 ParticleEmitter_GetInitVelAxisAmplifier(SPLEmitter *emitter) {
    return emitter->initVelAxisAmplifier;
}

void ParticleEmitter_SetInitVelAxisAmplifier(SPLEmitter *emitter, fx32 amplifier) {
    emitter->initVelAxisAmplifier = amplifier;
}

fx16 ParticleEmitter_GetBaseScale(SPLEmitter *emitter) {
    return emitter->baseScale;
}

u16 ParticleEmitter_GetParticleLifeTime(SPLEmitter *emitter) {
    return emitter->particleLifeTime;
}

void ParticleEmitter_SetPosition(SPLEmitter *emitter, const VecFx32 *pos) {
    emitter->position.x = pos->x + emitter->resource->header->emitterBasePos.x;
    emitter->position.y = pos->y + emitter->resource->header->emitterBasePos.y;
    emitter->position.z = pos->z + emitter->resource->header->emitterBasePos.z;
}

void ParticleEmitter_SetAxis(SPLEmitter *emitter, const VecFx16 *axis) {
    emitter->axis = *axis;
}

void ParticleEmitter_SetBaseScale(SPLEmitter *emitter, fx32 scale) {
    emitter->baseScale = scale;
}

void ParticleEmitter_SetParticleLifeTime(SPLEmitter *emitter, u16 lifeTime) {
    emitter->particleLifeTime = lifeTime;
}

// The parameters of the emitter's first behavior of a kind, or NULL
static const void *ParticleEmitter_FindBehavior(SPLEmitter *emitter, int type) {
    int i;
    int count = emitter->resource->behaviorCount;

    if (count == 0) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        SPLBehavior *behavior = &emitter->resource->behaviors[i];

        if (behavior == NULL) {
            continue;
        }
        switch (type) {
        case PARTICLE_BEHAVIOR_GRAVITY:
            if (behavior->applyFunc == SPLBehavior_ApplyGravity) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_RANDOM:
            if (behavior->applyFunc == SPLBehavior_ApplyRandom) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_MAGNET:
            if (behavior->applyFunc == SPLBehavior_ApplyMagnet) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_SPIN:
            if (behavior->applyFunc == SPLBehavior_ApplySpin) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_COLLISION_PLANE:
            if (behavior->applyFunc == SPLBehavior_ApplyCollisionPlane) {
                return behavior->object;
            }
            continue;
        case PARTICLE_BEHAVIOR_CONVERGENCE:
            if (behavior->applyFunc == SPLBehavior_ApplyConvergence) {
                return behavior->object;
            }
            continue;
        default:
            return NULL;
        }
    }
    return NULL;
}

void ParticleEmitter_SetMagnetTarget(SPLEmitter *emitter, const VecFx32 *target) {
    SPLMagnetBehavior *behavior = (SPLMagnetBehavior *)ParticleEmitter_FindBehavior(emitter, PARTICLE_BEHAVIOR_MAGNET);

    if (behavior != NULL) {
        behavior->target = *target;
    }
}

void ParticleEmitter_SetSpinAxis(SPLEmitter *emitter, const u16 *axis) {
    SPLSpinBehavior *behavior = (SPLSpinBehavior *)ParticleEmitter_FindBehavior(emitter, PARTICLE_BEHAVIOR_SPIN);

    if (behavior != NULL) {
        behavior->axis = *axis;
    }
}

void ParticleEmitter_SetConvergenceTarget(SPLEmitter *emitter, const VecFx32 *target) {
    SPLConvergenceBehavior *behavior =
        (SPLConvergenceBehavior *)ParticleEmitter_FindBehavior(emitter, PARTICLE_BEHAVIOR_CONVERGENCE);

    if (behavior != NULL) {
        behavior->target = *target;
    }
}

void ParticleEmitter_SetUpdateCallback(SPLEmitter *emitter, SPLEmitterUpdateCallback callback) {
    emitter->updateCallback = callback;
}

void ParticleEmitter_SetUserData(SPLEmitter *emitter, void *data) {
    emitter->userDataPtr = data;
}

void *ParticleEmitter_GetUserData(SPLEmitter *emitter) {
    return emitter->userDataPtr;
}

// The VRAM allocators of an uploading resource, which keep the keys the linked-list managers give
static u32 Particle_AllocTexFrm(u32 size, BOOL is4x4comp) {
    return NNS_GfdGetTexKeyAddr(NNS_GfdAllocFrmTexVram(size, is4x4comp, 0));
}

static u32 Particle_AllocTexLnk(u32 size, BOOL is4x4comp) {
    NNSGfdTexKey key = NNS_GfdAllocTexVram(size, is4x4comp, 0);
    ParticleSystem *system = sParticle->uploading;

    system->texKeys[system->texKeyCount++] = key;
    return NNS_GfdGetTexKeyAddr(key);
}

static u32 Particle_AllocPlttFrm(u32 size, BOOL is4pltt) {
    return NNS_GfdGetPlttKeyAddr(NNS_GfdAllocFrmPlttVram(size, is4pltt, TRUE));
}

static u32 Particle_AllocPlttLnk(u32 size, BOOL is4pltt) {
    NNSGfdPlttKey key = NNS_GfdAllocPlttVram(size, is4pltt, 0);
    ParticleSystem *system = sParticle->uploading;

    system->plttKeys[system->plttKeyCount++] = key;
    return NNS_GfdGetPlttKeyAddr(key);
}

u16 Particle_GetResourceEmitterCount(const void *resource) {
    return ((const SPLFileHeader *)resource)->resourceCount;
}

void *Particle_GetBGCharPtr(u32 layer) {
    switch (layer) {
    case 0:
        return G2_GetBG0CharPtr();
    case 1:
        return G2_GetBG1CharPtr();
    case 2:
        return G2_GetBG2CharPtr();
    case 3:
        return G2_GetBG3CharPtr();
    case 4:
        return G2S_GetBG0CharPtr();
    case 5:
        return G2S_GetBG1CharPtr();
    case 6:
        return G2S_GetBG2CharPtr();
    case 7:
        return G2S_GetBG3CharPtr();
    }
    return NULL;
}
