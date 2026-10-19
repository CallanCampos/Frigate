#ifndef POKEBW2_GFL_PARTICLE_H
#define POKEBW2_GFL_PARTICLE_H

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/gfd.h"
#include "spl/spl.h"

// Particle systems: up to 16 SPL managers, each with its own work memory, resource file and camera. None of these
// functions has a name yet

#define PARTICLE_SYSTEM_MAX 16

// How a system's VRAM is given back when it is freed: the frame managers' state from before its resource was loaded,
// or each key the linked-list managers gave it
enum {
    PARTICLE_VRAM_TEX_FRM = 1 << 0,
    PARTICLE_VRAM_TEX_LNK = 1 << 1,
    PARTICLE_VRAM_PLTT_FRM = 1 << 2,
    PARTICLE_VRAM_PLTT_LNK = 1 << 3,
};

#define PARTICLE_VRAM_KEY_MAX 64

typedef struct {
    SPLManager *manager;
    void *resource;
    SPLEmitter *lastEmitter;
    void *workStart;
    void *work;
    void *workEnd;
    SPLTexVRAMAllocFunc texAllocFunc;
    SPLPalVRAMAllocFunc palAllocFunc;
    G3DCamera *camera;
    VecFx32 unk24;
    u16 cameraFov;
    // Whether the resource is someone else's, which freeing the system leaves
    u16 extResource : 1;
    VecFx32 cameraPos;
    VecFx32 cameraUp;
    VecFx32 cameraTarget;
    union {
        NNSGfdFrmTexVramState texState;
        NNSGfdTexKey texKeys[PARTICLE_VRAM_KEY_MAX];
    };
    union {
        NNSGfdFrmPlttVramState plttState;
        NNSGfdPlttKey plttKeys[PARTICLE_VRAM_KEY_MAX];
    };
    u8 vramRelease;
    u8 unk259;
    u8 id;
    u8 cameraProjection;
    u16 texKeyCount;
    u16 plttKeyCount;
} ParticleSystem;

// Sets up and frees the table of systems
void Particle_Init(HeapID heapId);
void Particle_Free(void);
// Updates and draws every system
void Particle_DrawAndUpdateAll(void);
// Particle_CreateSystemEx with polygon IDs 5, 6 and 0x3f
ParticleSystem *Particle_CreateSystem(void *work, u32 size, BOOL camera, u32 heapId);
// A system working in size bytes of work, its particles drawn with the given polygon IDs, or NULL when all 16 are in
// use
ParticleSystem *Particle_CreateSystemEx(void *work, u32 size, BOOL camera, u32 fixPolyID, u32 minPolyID, u32 maxPolyID,
                                        u32 heapId);
void Particle_DeleteSystem(ParticleSystem *system);
void *Particle_ReadResource(u32 arcId, u32 fileId, u32 heapId);
// Gives the system a resource file, which it uploads now or, through tcbMgr, at the next vertical blank
void Particle_SetResource(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr);
// The same, with a resource the system does not own
void Particle_SetExternalResource(ParticleSystem *system, void *resource, BOOL immediate, TCBManager *tcbMgr);
void Particle_SetResourceNoUpload(ParticleSystem *system, void *resource);
void Particle_UploadResource(ParticleSystem *system);
void Particle_DrawSystem(ParticleSystem *system);
void Particle_UpdateSystem(ParticleSystem *system);
int Particle_GetSystemCount(void);
int Particle_DrawAll(void);
int Particle_UpdateAll(void);
SPLEmitter *Particle_CreateEmitter(ParticleSystem *system, int resourceId, const VecFx32 *pos);
// Creates an emitter, calling callback with param readable through Particle_GetCallbackParam, or returns -1 when the
// system has 24 emitters
SPLEmitter *Particle_CreateEmitterWithCallback(ParticleSystem *system, int resourceId, SPLEmitterCallback callback,
                                               void *param);
int Particle_GetEmitterCount(ParticleSystem *system);
void Particle_DeleteAllEmitters(ParticleSystem *system);
void Particle_DeleteEmitter(ParticleSystem *system, SPLEmitter *emitter);
void *Particle_GetWork(ParticleSystem *system);
// Sets the camera that the particles are drawn with; a NULL projection or vector is the default
void Particle_SetCamera(ParticleSystem *system, G3DCameraProjection *projection, u16 fov, const VecFx32 *position,
                        const VecFx32 *upVector, const VecFx32 *target, u32 heapId);
void Particle_FreeCamera(ParticleSystem *system);
void *Particle_GetCallbackParam(void);
G3DCamera *Particle_GetCamera(ParticleSystem *system);
void Particle_GetCameraPos(ParticleSystem *system, VecFx32 *pos);
void Particle_SetCameraPos(ParticleSystem *system, const VecFx32 *pos);

// An emitter's settings
void ParticleEmitter_GetAxis(SPLEmitter *emitter, VecFx16 *axis);
fx32 ParticleEmitter_GetRadius(SPLEmitter *emitter);
void ParticleEmitter_SetRadius(SPLEmitter *emitter, fx32 radius);
fx32 ParticleEmitter_GetLength(SPLEmitter *emitter);
void ParticleEmitter_SetLength(SPLEmitter *emitter, fx32 length);
fx16 ParticleEmitter_GetInitVelPositionAmplifier(SPLEmitter *emitter);
void ParticleEmitter_SetInitVelPositionAmplifier(SPLEmitter *emitter, fx32 amplifier);
fx16 ParticleEmitter_GetInitVelAxisAmplifier(SPLEmitter *emitter);
void ParticleEmitter_SetInitVelAxisAmplifier(SPLEmitter *emitter, fx32 amplifier);
fx16 ParticleEmitter_GetBaseScale(SPLEmitter *emitter);
u16 ParticleEmitter_GetParticleLifeTime(SPLEmitter *emitter);
void ParticleEmitter_SetPosition(SPLEmitter *emitter, const VecFx32 *pos);
void ParticleEmitter_SetAxis(SPLEmitter *emitter, const VecFx16 *axis);
void ParticleEmitter_SetBaseScale(SPLEmitter *emitter, fx32 scale);
void ParticleEmitter_SetParticleLifeTime(SPLEmitter *emitter, u16 lifeTime);
void ParticleEmitter_SetMagnetTarget(SPLEmitter *emitter, const VecFx32 *target);
void ParticleEmitter_SetSpinAxis(SPLEmitter *emitter, const u16 *axis);
void ParticleEmitter_SetConvergenceTarget(SPLEmitter *emitter, const VecFx32 *target);
void ParticleEmitter_SetUpdateCallback(SPLEmitter *emitter, SPLEmitterUpdateCallback callback);
void ParticleEmitter_SetUserData(SPLEmitter *emitter, void *data);
void *ParticleEmitter_GetUserData(SPLEmitter *emitter);

// How many emitters a resource file defines
u16 Particle_GetResourceEmitterCount(const void *resource);
// The character base of a background layer of the main (0 to 3) or sub (4 to 7) engine
void *Particle_GetBGCharPtr(u32 layer);

#endif // POKEBW2_GFL_PARTICLE_H
