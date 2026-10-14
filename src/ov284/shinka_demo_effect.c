#include "types.h"
#include "constants/arc.h"
#include "demo/demo_particle.h"
#include "demo/shinka_demo.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "nitro/fx.h"

// The effects around the evolving Pokémon: particles that play from the start, then a 3D model's animations that play
// forward and back

// The steps of the effect
enum {
    EFFECT_WAIT_START,
    EFFECT_PARTICLES,
    EFFECT_START_3D,
    EFFECT_WAIT_3D,
    EFFECT_HOLD_3D,
    EFFECT_WAIT_3D_REVERSE,
    EFFECT_DONE,
    // Created as already played
    EFFECT_PLAYED,
};

// What the 3D model's animations are doing
enum {
    EFFECT_3D_IDLE,
    EFFECT_3D_FORWARD,
    EFFECT_3D_END,
    EFFECT_3D_REVERSE,
};

typedef struct {
    u16 actor;
    SRTMatrix srt;
    BOOL visible;
} EffectActor;

struct ShinkaDemoEffect {
    HeapID heapId;
    BOOL played;
    s32 state;
    u32 wait;
    DemoParticle *particle;
    G3DManager *g3dManager;
    u16 scene;
    u16 modelActor;
    u16 actorCount;
    EffectActor *actors;
    u32 frame3D;
    u32 state3D;
};

static EffectActor *ShinkaDemoEffect_CreateActors(HeapID heapId, u16 count);
static void ShinkaDemoEffect_FreeActors(EffectActor *actors);
static DemoParticle *ShinkaDemoParticle_Create(HeapID heapId, u16 count, const DemoParticleEvent *events);
static void ShinkaDemoParticle_Free(DemoParticle *particle);
static void ShinkaDemoParticle_Update(DemoParticle *particle);
static void ShinkaDemoParticle_Draw(DemoParticle *particle);
static void ShinkaDemoParticle_Start(DemoParticle *particle);
static void ShinkaDemoParticle_Stop(DemoParticle *particle, s32 stopTimer);
static void ShinkaDemoParticle_SetStopTimer(DemoParticle *particle, s32 stopTimer);
static void ShinkaDemoEffect_Init3D(ShinkaDemoEffect *effect);
static void ShinkaDemoEffect_Free3D(ShinkaDemoEffect *effect);
static void ShinkaDemoEffect_Draw3D(ShinkaDemoEffect *effect);
static void ShinkaDemoEffect_AddScene(ShinkaDemoEffect *effect);
static void ShinkaDemoEffect_DeleteScene(ShinkaDemoEffect *effect);
static void ShinkaDemoEffect_Update3D(ShinkaDemoEffect *effect);
static void ShinkaDemoEffect_Play3D(ShinkaDemoEffect *effect);
static BOOL ShinkaDemoEffect_Is3DAtEnd(ShinkaDemoEffect *effect);
static void ShinkaDemoEffect_Reverse3D(ShinkaDemoEffect *effect);
static BOOL ShinkaDemoEffect_Is3DIdle(ShinkaDemoEffect *effect);

static const G3DSceneResourceSetup sModelResources[] = {
    { ARCID_SHINKA_DEMO, 8, 0 },
    { ARCID_SHINKA_DEMO, 9, 0 },
    { ARCID_SHINKA_DEMO, 6, 0 },
    { ARCID_SHINKA_DEMO, 7, 0 },
};

static const G3DSceneAnimationSetup sModelAnimations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 } };

static const G3DSceneActorSetup sModelActors[] = {
    { 0, 0, 0, 0, sModelAnimations, NELEMS(sModelAnimations) },
};

static const G3DSceneSetup sModelScene = { sModelResources, NELEMS(sModelResources), sModelActors,
                                           NELEMS(sModelActors) };

// Not referenced. Its value, FX32_ONE, is the step of the model's looping animation and the scale of its actor
const fx32 SHINKA_DEMO_EFFECT_UNK_FX32 = FX32_ONE;

static const DemoParticleResource sParticleResources[DEMO_PARTICLE_UNIT_COUNT] = { { ARCID_SHINKA_DEMO, 10 } };

static const DemoParticleEvent sParticleEvents[] = {
    { 0, 0, 1 },   { 0, 0, 0 },   { 0, 0, 2 },   { 0, 0, 3 },   { 0, 0, 4 },
    { 500, 0, 5 }, { 500, 0, 6 }, { 755, 0, 7 }, { 783, 0, 8 },
};

static EffectActor *ShinkaDemoEffect_CreateActors(HeapID heapId, u16 count) {
    EffectActor *actors = GFL_HeapAllocate(heapId, sizeof(EffectActor) * count, TRUE, "shinka_demo_effect.c", 232);
    u16 i;

    for (i = 0; i < count; i++) {
        actors[i].srt.translation.x = 0;
        actors[i].srt.translation.y = 0;
        actors[i].srt.translation.z = 0;
        actors[i].srt.scale.x = FX32_ONE;
        actors[i].srt.scale.y = FX32_ONE;
        actors[i].srt.scale.z = FX32_ONE;
        MTX_Identity33_(&actors[i].srt.rotation);
    }
    return actors;
}

static void ShinkaDemoEffect_FreeActors(EffectActor *actors) {
    GFL_HeapFree(actors);
}

ShinkaDemoEffect *ShinkaDemoEffect_Create(HeapID heapId, BOOL played) {
    ShinkaDemoEffect *effect = GFL_HeapAllocate(heapId, sizeof(ShinkaDemoEffect), TRUE, "shinka_demo_effect.c", 336);

    effect->heapId = heapId;
    effect->played = played;
    if (played == FALSE) {
        effect->state = EFFECT_WAIT_START;
    } else {
        effect->state = EFFECT_PLAYED;
    }
    effect->wait = 0;
    effect->particle = ShinkaDemoParticle_Create(effect->heapId, NELEMS(sParticleEvents), sParticleEvents);
    ShinkaDemoEffect_Init3D(effect);
    ShinkaDemoEffect_AddScene(effect);
    return effect;
}

void ShinkaDemoEffect_Free(ShinkaDemoEffect *effect) {
    ShinkaDemoEffect_DeleteScene(effect);
    ShinkaDemoEffect_Free3D(effect);
    ShinkaDemoParticle_Free(effect->particle);
    GFL_HeapFree(effect);
}

void ShinkaDemoEffect_Update(ShinkaDemoEffect *effect) {
    switch (effect->state) {
    case EFFECT_WAIT_START:
    case EFFECT_PARTICLES:
        break;
    case EFFECT_START_3D:
        ShinkaDemoEffect_Play3D(effect);
        effect->state = EFFECT_WAIT_3D;
        break;
    case EFFECT_WAIT_3D:
        if (ShinkaDemoEffect_Is3DAtEnd(effect)) {
            effect->state = EFFECT_HOLD_3D;
            effect->wait = 3;
            ShinkaDemoParticle_SetStopTimer(effect->particle, 0);
        }
        break;
    case EFFECT_HOLD_3D:
        if (effect->wait == 0) {
            ShinkaDemoEffect_Reverse3D(effect);
            effect->state = EFFECT_WAIT_3D_REVERSE;
        } else {
            effect->wait--;
        }
        break;
    case EFFECT_WAIT_3D_REVERSE:
        if (ShinkaDemoEffect_Is3DIdle(effect)) {
            effect->state = EFFECT_DONE;
        }
        break;
    case EFFECT_DONE:
    case EFFECT_PLAYED:
        break;
    }
    ShinkaDemoEffect_Update3D(effect);
    ShinkaDemoParticle_Update(effect->particle);
}

void ShinkaDemoEffect_Draw(ShinkaDemoEffect *effect) {
    ShinkaDemoEffect_Draw3D(effect);
    ShinkaDemoParticle_Draw(effect->particle);
}

// Stops the particles and plays the 3D model when the evolution is cancelled
void ShinkaDemoEffect_Cancel(ShinkaDemoEffect *effect) {
    ShinkaDemoParticle_Stop(effect->particle, 30);
    if (effect->state == EFFECT_PARTICLES) {
        effect->state = EFFECT_START_3D;
    }
}

void ShinkaDemoEffect_Start(ShinkaDemoEffect *effect) {
    ShinkaDemoParticle_Start(effect->particle);
    effect->state = EFFECT_PARTICLES;
    effect->wait = 620;
}

void ShinkaDemoEffect_Start3D(ShinkaDemoEffect *effect) {
    if (effect->state == EFFECT_PARTICLES) {
        effect->state = EFFECT_START_3D;
    }
}

BOOL ShinkaDemoEffect_Is3DHeld(ShinkaDemoEffect *effect) {
    return effect->state == EFFECT_HOLD_3D;
}

// Whether the 3D model is about to or has started reversing
BOOL ShinkaDemoEffect_Is3DReversing(ShinkaDemoEffect *effect) {
    if (effect->state == EFFECT_HOLD_3D && effect->wait <= 1) {
        return TRUE;
    }
    return effect->state == EFFECT_WAIT_3D_REVERSE;
}

static DemoParticle *ShinkaDemoParticle_Create(HeapID heapId, u16 count, const DemoParticleEvent *events) {
    VecFx32 cameraPosition = { 0, 0, FX32_CONST(13) };
    VecFx32 cameraUp = { 0, FX32_ONE, 0 };
    VecFx32 cameraTarget = { 0, 0, 0 };
    G3DCameraProjection projection;
    DemoParticle *particle;
    void *resource;
    u32 i;

    projection.type = G3DCAM_PROJECTION_PERSPECTIVE;
    projection.param1 = FX_SinIdx(DEG_TO_IDX(20));
    projection.param2 = FX_CosIdx(DEG_TO_IDX(20));
    projection.param3 = FX32_CONST(4.0 / 3.0);
    projection.param4 = 0;
    projection.near = FX32_ONE;
    projection.far = FX32_CONST(1024);
    projection.ndcRangeOverride = 0;
    particle = GFL_HeapAllocate(heapId, sizeof(DemoParticle), TRUE, "shinka_demo_effect.c", 741);
    particle->frame = 0;
    particle->index = 0;
    particle->count = count;
    particle->events = events;
    particle->active = FALSE;
    particle->stopTimer = -1;
    func_0204f918(heapId);
    for (i = 0; i < DEMO_PARTICLE_UNIT_COUNT; i++) {
        particle->units[i].system =
            func_0204f980(particle->units[i].buffer, DEMO_PARTICLE_BUFFER_SIZE, TRUE, 5, 6, 0x3b, heapId);
        func_02050178(particle->units[i].system);
        func_020500cc(particle->units[i].system, &projection, FX32_CONST(2), &cameraPosition, &cameraUp, &cameraTarget,
                      heapId);
        resource = func_0204fdf8(sParticleResources[i].arcId, sParticleResources[i].fileId, heapId);
        particle->units[i].resourceCount = func_020503f0(resource);
        func_0204fe04(particle->units[i].system, resource, TRUE, FALSE);
    }
    return particle;
}

static void ShinkaDemoParticle_Free(DemoParticle *particle) {
    func_0204fb4c();
    GFL_HeapFree(particle);
}

// Emits the particles of the events at the current frame
static void ShinkaDemoParticle_Update(DemoParticle *particle) {
    if (particle->active) {
        VecFx32 pos = { 0, 0, 0 };

        while (particle->index < particle->count) {
            if (particle->frame != particle->events[particle->index].frame) {
                break;
            }
            func_0205006c(particle->units[particle->events[particle->index].unit].system,
                          particle->events[particle->index].emitter, &pos);
            particle->index++;
        }
        particle->frame++;
    }
    if (particle->stopTimer >= 0) {
        if (particle->stopTimer == 0) {
            func_020500b0(particle->units[0].system);
        }
        particle->stopTimer--;
    }
}

static void ShinkaDemoParticle_Draw(DemoParticle *particle) {
    func_0204f954();
}

static void ShinkaDemoParticle_Start(DemoParticle *particle) {
    particle->active = TRUE;
}

static void ShinkaDemoParticle_Stop(DemoParticle *particle, s32 stopTimer) {
    particle->active = FALSE;
    particle->stopTimer = stopTimer;
}

static void ShinkaDemoParticle_SetStopTimer(DemoParticle *particle, s32 stopTimer) {
    particle->stopTimer = stopTimer;
}

static void ShinkaDemoEffect_Init3D(ShinkaDemoEffect *effect) {
    effect->g3dManager = GFL_G3DMgrCreate(4, 1, effect->heapId);
    effect->actorCount = 0;
}

static void ShinkaDemoEffect_Free3D(ShinkaDemoEffect *effect) {
    GFL_G3DMgrFree(effect->g3dManager);
}

static void ShinkaDemoEffect_Draw3D(ShinkaDemoEffect *effect) {
    u16 i;
    EffectActor *actor;

    for (i = 0; i < effect->actorCount; i++) {
        actor = &effect->actors[i];
        if (actor->visible) {
            GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(effect->g3dManager, actor->actor), &actor->srt);
        }
    }
}

static void ShinkaDemoEffect_AddScene(ShinkaDemoEffect *effect) {
    u16 i;
    u16 first;
    EffectActor *actor;
    G3DActor *model;
    fx32 frame;

    effect->scene = GFL_G3DMgrNewScene(effect->g3dManager, &sModelScene);
    effect->actorCount = 1;
    effect->actors = ShinkaDemoEffect_CreateActors(effect->heapId, effect->actorCount);
    first = GFL_G3DMgrGetSceneFirstActorIdx(effect->g3dManager, effect->scene);
    effect->modelActor = 0;
    actor = &effect->actors[0];
    actor->actor = first;
    actor->srt.translation.x = 0;
    actor->srt.translation.y = 0;
    actor->srt.translation.z = 0;
    actor->visible = TRUE;
    for (i = 0; i < effect->actorCount; i++) {
        GFL_G3DActorBindAnm(GFL_G3DMgrGetActor(effect->g3dManager, effect->actors[i].actor), 0);
    }
    effect->frame3D = 0;
    model = GFL_G3DMgrGetActor(effect->g3dManager, effect->actors[effect->modelActor].actor);
    frame = 0;
    effect->state3D = EFFECT_3D_IDLE;
    GFL_G3DActorBindAnm(model, 1);
    GFL_G3DActorBindAnm(model, 2);
    GFL_G3DActorSetAnmFrame(model, 1, &frame);
    GFL_G3DActorSetAnmFrame(model, 2, &frame);
}

static void ShinkaDemoEffect_DeleteScene(ShinkaDemoEffect *effect) {
    GFL_G3DMgrDeleteScene(effect->g3dManager, effect->scene);
    ShinkaDemoEffect_FreeActors(effect->actors);
    effect->actorCount = 0;
}

static void ShinkaDemoEffect_Update3D(ShinkaDemoEffect *effect) {
    u16 i;
    G3DActor *model;
    BOOL running1;
    BOOL running2;

    for (i = 0; i < effect->actorCount; i++) {
        GFL_G3DActorStepAnmFrameLoop(GFL_G3DMgrGetActor(effect->g3dManager, effect->actors[i].actor), 0, FX32_ONE);
    }
    effect->frame3D++;
    model = GFL_G3DMgrGetActor(effect->g3dManager, effect->actors[effect->modelActor].actor);
    if (effect->state3D == EFFECT_3D_FORWARD) {
        running1 = GFL_G3DActorStepAnmFrame(model, 1, FX32_CONST(5));
        running2 = GFL_G3DActorStepAnmFrame(model, 2, FX32_CONST(5));
        if (running1 == FALSE && running2 == FALSE) {
            effect->state3D = EFFECT_3D_END;
        }
    } else if (effect->state3D == EFFECT_3D_REVERSE) {
        running1 = GFL_G3DActorStepAnmFrame(model, 1, -FX32_CONST(2));
        running2 = GFL_G3DActorStepAnmFrame(model, 2, -FX32_CONST(2));
        if (running1 == FALSE && running2 == FALSE) {
            effect->state3D = EFFECT_3D_IDLE;
        }
    }
}

static void ShinkaDemoEffect_Play3D(ShinkaDemoEffect *effect) {
    effect->state3D = EFFECT_3D_FORWARD;
}

static BOOL ShinkaDemoEffect_Is3DAtEnd(ShinkaDemoEffect *effect) {
    return effect->state3D == EFFECT_3D_END;
}

static void ShinkaDemoEffect_Reverse3D(ShinkaDemoEffect *effect) {
    effect->state3D = EFFECT_3D_REVERSE;
}

static BOOL ShinkaDemoEffect_Is3DIdle(ShinkaDemoEffect *effect) {
    return effect->state3D == EFFECT_3D_IDLE;
}
