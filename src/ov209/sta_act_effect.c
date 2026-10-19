#include "types.h"
#include "app/musical/sta_act_effect.h"
#include "constants/arc.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "gfl/tcb.h"

// Overlay 209's sta_act_effect.c: the stage's particle effects

StaActEffectSys *StaActEffect_InitSystem(HeapID heapId) {
    u8 i;
    StaActEffectSys *sys = GFL_HeapAllocate(heapId, sizeof(StaActEffectSys), FALSE, "sta_act_effect.c", 76);

    sys->heapId = heapId;
    Particle_Init(heapId);
    for (i = 0; i < STA_ACT_EFFECT_MAX; i++) {
        sys->effects[i].active = FALSE;
    }
    return sys;
}

void StaActEffect_TermSystem(StaActEffectSys *sys) {
    u8 i;

    for (i = 0; i < STA_ACT_EFFECT_MAX; i++) {
        if (sys->effects[i].active == TRUE) {
            StaActEffect_DelEffect(sys, &sys->effects[i]);
        }
    }
    Particle_Free();
    GFL_HeapFree(sys);
}

void StaActEffect_UpdateSystem(StaActEffectSys *sys) {
    Particle_UpdateAll();
}

void StaActEffect_DrawSystem(StaActEffectSys *sys) {
    Particle_DrawAll();
}

StaActEffect *StaActEffect_AddEffect(StaActEffectSys *sys, u32 fileId) {
    u8 i;
    StaActEffect *effect = NULL;
    void *res;

    for (i = 0; i < STA_ACT_EFFECT_MAX; i++) {
        if (sys->effects[i].active == FALSE) {
            effect = &sys->effects[i];
        }
    }
    effect->ptc = Particle_CreateSystem(effect->ptcWork, sizeof(effect->ptcWork), FALSE, sys->heapId);
    res = Particle_ReadResource(ARCID_MUSICAL, fileId, sys->heapId);
    Particle_SetResource(effect->ptc, res, FALSE, GFL_VBlankGetTCBMgr());
    for (i = 0; i < STA_ACT_EFFECT_EMITTER_MAX; i++) {
        effect->emitters[i].emitter = NULL;
    }
    effect->active = TRUE;
    return effect;
}

void StaActEffect_DelEffect(StaActEffectSys *sys, StaActEffect *effect) {
    effect->active = FALSE;
    Particle_DeleteAllEmitters(effect->ptc);
    Particle_DeleteSystem(effect->ptc);
}

void StaActEffect_CreateEmitter(StaActEffect *effect, u16 emitterNo, VecFx32 *pos) {
    SPLEmitter *emitter = Particle_CreateEmitter(effect->ptc, emitterNo, pos);

    if (emitter == NULL || emitterNo < STA_ACT_EFFECT_EMITTER_MAX) {
        effect->emitters[emitterNo].emitter = emitter;
        VEC_Set(&effect->emitters[emitterNo].pos, pos->x, pos->y, pos->z);
    }
}

void StaActEffect_DeleteEmitter(StaActEffect *effect, u16 emitterNo) {
    if (effect->emitters[emitterNo].emitter != NULL) {
        Particle_DeleteEmitter(effect->ptc, effect->emitters[emitterNo].emitter);
    }
}

void StaActEffect_SetPosition(StaActEffect *effect, u16 emitterNo, VecFx32 *pos) {
    if (effect->emitters[emitterNo].emitter != NULL) {
        ParticleEmitter_SetPosition(effect->emitters[emitterNo].emitter, pos);
    }
}
