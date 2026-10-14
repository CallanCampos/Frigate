#include "types.h"
#include "nitro/fx.h"
#include "spl/spl.h"
#include "spl_internal.h"

// The behaviors a resource gives its particles, each applied to every particle every frame: forces added to its
// acceleration, or changes to its position and velocity

void SPLBehavior_ApplyGravity(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter) {
    const SPLGravityBehavior *gravity = behavior;

    acc->x += gravity->magnitude.x;
    acc->y += gravity->magnitude.y;
    acc->z += gravity->magnitude.z;
}

void SPLBehavior_ApplyRandom(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter) {
    const SPLRandomBehavior *random = behavior;

    if (particle->age % random->applyInterval != 0) {
        return;
    }
    acc->x += SPLRandom_Range(random->magnitude.x);
    acc->y += SPLRandom_Range(random->magnitude.y);
    acc->z += SPLRandom_Range(random->magnitude.z);
}

// Pulls towards the target, less the particle's velocity, which damps it
void SPLBehavior_ApplyMagnet(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter) {
    const SPLMagnetBehavior *magnet = behavior;

    acc->x += (magnet->force * (magnet->target.x - particle->position.x - particle->velocity.x)) >> FX32_SHIFT;
    acc->y += (magnet->force * (magnet->target.y - particle->position.y - particle->velocity.y)) >> FX32_SHIFT;
    acc->z += (magnet->force * (magnet->target.z - particle->position.z - particle->velocity.z)) >> FX32_SHIFT;
}

// Turns the particle about one of the emitter's axes
void SPLBehavior_ApplySpin(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter) {
    const SPLSpinBehavior *spin = behavior;
    MtxFx33 rot;

    switch (spin->axis) {
    case 0:
        MTX_RotX33_(&rot, FX_SinIdx(spin->angle), FX_CosIdx(spin->angle));
        break;
    case 1:
        MTX_RotY33_(&rot, FX_SinIdx(spin->angle), FX_CosIdx(spin->angle));
        break;
    case 2:
        MTX_RotZ33_(&rot, FX_SinIdx(spin->angle), FX_CosIdx(spin->angle));
        break;
    }
    MTX_MultVec33(&particle->position, &rot, &particle->position);
}

void SPLBehavior_ApplyCollisionPlane(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter) {
    const SPLCollisionPlaneBehavior *plane = behavior;
    fx32 y = plane->y;
    fx32 emitterY;

    if (emitter->collisionPlaneHeight != FX32_MIN) {
        y = emitter->collisionPlaneHeight;
    }
    switch (plane->type) {
    case SPL_COLLISION_KILL:
        emitterY = particle->emitterPos.y;
        if (emitterY < y && emitterY + particle->position.y > y) {
            particle->position.y = y - emitterY;
            particle->age = particle->lifeTime;
        } else if (emitterY >= y && emitterY + particle->position.y < y) {
            particle->position.y = y - emitterY;
            particle->age = particle->lifeTime;
        }
        break;
    case SPL_COLLISION_BOUNCE:
        emitterY = particle->emitterPos.y;
        if (emitterY < y && emitterY + particle->position.y > y) {
            particle->position.y = y - emitterY;
            particle->velocity.y = -FX_Mul(particle->velocity.y, plane->elasticity);
        } else if (emitterY >= y && emitterY + particle->position.y < y) {
            particle->position.y = y - emitterY;
            particle->velocity.y = -FX_Mul(particle->velocity.y, plane->elasticity);
        }
        break;
    }
}

void SPLBehavior_ApplyConvergence(const void *behavior, SPLParticle *particle, VecFx32 *acc, SPLEmitter *emitter) {
    const SPLConvergenceBehavior *convergence = behavior;

    particle->position.x += FX_Mul(convergence->force, convergence->target.x - particle->position.x);
    particle->position.y += FX_Mul(convergence->force, convergence->target.y - particle->position.y);
    particle->position.z += FX_Mul(convergence->force, convergence->target.z - particle->position.z);
}
