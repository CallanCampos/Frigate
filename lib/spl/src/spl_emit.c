#include "types.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "spl/spl.h"
#include "spl_internal.h"

// The emission of particles: where they start, and their velocities, scales, colors, rotations and life times

// The emitter's cross axes: two unit vectors orthogonal to its emission axis and each other
static void SPLEmitter_ComputeOrthogonalAxes(SPLEmitter *emitter) {
    VecFx16 up = { 0, FX16_ONE, 0 };
    VecFx16 axis;
    fx32 dot;

    switch (emitter->resource->header->flags.emissionAxis) {
    case SPL_AXIS_X:
        VEC_Fx16Set(&axis, FX16_ONE, 0, 0);
        break;
    case SPL_AXIS_Y:
        VEC_Fx16Set(&axis, 0, FX16_ONE, 0);
        break;
    case SPL_AXIS_Z:
        VEC_Fx16Set(&axis, 0, 0, FX16_ONE);
        break;
    default:
        VEC_Fx16Normalize(&emitter->axis, &axis);
        break;
    }

    // An up vector along the axis has no cross product with it
    dot = VEC_Fx16DotProduct(&up, &axis);
    if (dot == FX16_ONE || dot == -FX16_ONE) {
        VEC_Fx16Set(&up, FX16_ONE, 0, 0);
    }

    emitter->crossAxis1.x = FX_Mul(axis.y, up.z) - FX_Mul(axis.z, up.y);
    emitter->crossAxis1.y = FX_Mul(axis.z, up.x) - FX_Mul(axis.x, up.z);
    emitter->crossAxis1.z = FX_Mul(axis.x, up.y) - FX_Mul(axis.y, up.x);
    emitter->crossAxis2.x = FX_Mul(axis.y, emitter->crossAxis1.z) - FX_Mul(axis.z, emitter->crossAxis1.y);
    emitter->crossAxis2.y = FX_Mul(axis.z, emitter->crossAxis1.x) - FX_Mul(axis.x, emitter->crossAxis1.z);
    emitter->crossAxis2.z = FX_Mul(axis.x, emitter->crossAxis1.y) - FX_Mul(axis.y, emitter->crossAxis1.x);
    VEC_Fx16Normalize(&emitter->crossAxis1, &emitter->crossAxis1);
    VEC_Fx16Normalize(&emitter->crossAxis2, &emitter->crossAxis2);
}

// Coordinates in the emitter's cross axes and their normal, as a position
static void SPLEmitter_TiltCoordinates(VecFx32 *dest, const VecFx32 *src, SPLEmitter *emitter) {
    VecFx16 normal;

    VEC_Fx16CrossProduct(&emitter->crossAxis1, &emitter->crossAxis2, &normal);
    VEC_Fx16Normalize(&normal, &normal);
    dest->x = FX_Mul(src->x, emitter->crossAxis1.x) + FX_Mul(src->y, emitter->crossAxis2.x) + FX_Mul(src->z, normal.x);
    dest->y = FX_Mul(src->x, emitter->crossAxis1.y) + FX_Mul(src->y, emitter->crossAxis2.y) + FX_Mul(src->z, normal.y);
    dest->z = FX_Mul(src->x, emitter->crossAxis1.z) + FX_Mul(src->y, emitter->crossAxis2.z) + FX_Mul(src->z, normal.z);
}

void SPLEmitter_EmitParticles(SPLEmitter *emitter, SPLList *freeList) {
    SPLResource *resource = emitter->resource;
    SPLResourceHeader *header = resource->header;
    fx32 emissions = emitter->emissionCount + emitter->emissionCountFractional;
    int i;
    int n;
    int type;
    SPLParticle *particle;
    fx32 velPositionAmp, velAxisAmp;
    // The particles emitted around a uniform circle so far
    int circleCount;

    emitter->emissionCountFractional = emissions & FX32_DEC_MASK;
    n = emissions >> FX32_SHIFT;

    type = header->flags.emissionType;
    if (type == SPL_EMISSION_CIRCLE_BORDER || type == SPL_EMISSION_CIRCLE_BORDER_UNIFORM ||
        type == SPL_EMISSION_CIRCLE || type == SPL_EMISSION_CYLINDER_SURFACE || type == SPL_EMISSION_CYLINDER ||
        type == SPL_EMISSION_HEMISPHERE_SURFACE || type == SPL_EMISSION_HEMISPHERE) {
        SPLEmitter_ComputeOrthogonalAxes(emitter);
    }

    circleCount = 0;
    for (i = 0; i < n; i++) {
        particle = (SPLParticle *)SPLList_PopFront(freeList);
        if (particle == NULL) {
            return;
        }
        SPLList_PushFront(&emitter->particles, (SPLNode *)particle);

        switch (header->flags.emissionType) {
        case SPL_EMISSION_POINT:
            particle->position.x = particle->position.y = particle->position.z = 0;
            break;
        case SPL_EMISSION_SPHERE_SURFACE:
            SPLRandom_VecFx32(&particle->position);
            particle->position.x = FX_Mul(particle->position.x, emitter->radius);
            particle->position.y = FX_Mul(particle->position.y, emitter->radius);
            particle->position.z = FX_Mul(particle->position.z, emitter->radius);
            break;
        case SPL_EMISSION_CIRCLE_BORDER: {
            VecFx32 pos;

            SPLRandom_VecFx32_XY(&pos);
            pos.x = FX_Mul(pos.x, emitter->radius);
            pos.y = FX_Mul(pos.y, emitter->radius);
            pos.z = 0;
            SPLEmitter_TiltCoordinates(&particle->position, &pos, emitter);
            break;
        }
        case SPL_EMISSION_CIRCLE_BORDER_UNIFORM: {
            VecFx32 pos;
            int angle = (circleCount * 0x10000) / n;

            circleCount++;
            pos.x = FX_Mul(FX_SinIdx(angle), emitter->radius);
            pos.y = FX_Mul(FX_CosIdx(angle), emitter->radius);
            pos.z = 0;
            SPLEmitter_TiltCoordinates(&particle->position, &pos, emitter);
            break;
        }
        case SPL_EMISSION_SPHERE:
            SPLRandom_VecFx32(&particle->position);
            particle->position.x = FX_Mul(FX_Mul(particle->position.x, emitter->radius), SPLRandom_Range(FX32_ONE));
            particle->position.y = FX_Mul(FX_Mul(particle->position.y, emitter->radius), SPLRandom_Range(FX32_ONE));
            particle->position.z = FX_Mul(FX_Mul(particle->position.z, emitter->radius), SPLRandom_Range(FX32_ONE));
            break;
        case SPL_EMISSION_CIRCLE: {
            VecFx32 pos;

            SPLRandom_VecFx32_XY(&pos);
            pos.x = FX_Mul(FX_Mul(pos.x, emitter->radius), SPLRandom_Range(FX32_ONE));
            pos.y = FX_Mul(FX_Mul(pos.y, emitter->radius), SPLRandom_Range(FX32_ONE));
            SPLEmitter_TiltCoordinates(&particle->position, &pos, emitter);
            break;
        }
        case SPL_EMISSION_HEMISPHERE_SURFACE: {
            VecFx16 normal16;
            VecFx32 normal;

            SPLRandom_VecFx32(&particle->position);
            VEC_Fx16CrossProduct(&emitter->crossAxis1, &emitter->crossAxis2, &normal16);
            normal.x = normal16.x;
            normal.y = normal16.y;
            normal.z = normal16.z;
            if (VEC_DotProduct(&normal, &particle->position) <= 0) {
                particle->position.x = -particle->position.x;
                particle->position.y = -particle->position.y;
                particle->position.z = -particle->position.z;
            }
            particle->position.x = FX_Mul(particle->position.x, emitter->radius);
            particle->position.y = FX_Mul(particle->position.y, emitter->radius);
            particle->position.z = FX_Mul(particle->position.z, emitter->radius);
            break;
        }
        case SPL_EMISSION_HEMISPHERE: {
            VecFx16 normal16;
            VecFx32 normal;

            SPLRandom_VecFx32(&particle->position);
            VEC_Fx16CrossProduct(&emitter->crossAxis1, &emitter->crossAxis2, &normal16);
            normal.x = normal16.x;
            normal.y = normal16.y;
            normal.z = normal16.z;
            if (VEC_DotProduct(&normal, &particle->position) < 0) {
                particle->position.x = -particle->position.x;
                particle->position.y = -particle->position.y;
                particle->position.z = -particle->position.z;
            }
            particle->position.x =
                FX_Mul(FX_Mul(particle->position.x, emitter->radius), (SPLRandom_Range(FX32_ONE) >> 1) + FX32_HALF);
            particle->position.y =
                FX_Mul(FX_Mul(particle->position.y, emitter->radius), (SPLRandom_Range(FX32_ONE) >> 1) + FX32_HALF);
            particle->position.z =
                FX_Mul(FX_Mul(particle->position.z, emitter->radius), (SPLRandom_Range(FX32_ONE) >> 1) + FX32_HALF);
            break;
        }
        // The velocity holds the direction out from the cylinder's axis, until the velocity is set below
        case SPL_EMISSION_CYLINDER_SURFACE: {
            VecFx32 pos;

            SPLRandom_VecFx32_XY(&particle->velocity);
            pos.x = FX_Mul(particle->velocity.x, emitter->radius);
            pos.y = FX_Mul(particle->velocity.y, emitter->radius);
            pos.z = SPLRandom_Range(emitter->length);
            SPLEmitter_TiltCoordinates(&particle->position, &pos, emitter);
            break;
        }
        case SPL_EMISSION_CYLINDER: {
            VecFx32 pos;

            SPLRandom_VecFx32_XY(&particle->velocity);
            pos.x = FX_Mul(FX_Mul(particle->velocity.x, emitter->radius), SPLRandom_Range(FX32_ONE));
            pos.y = FX_Mul(FX_Mul(particle->velocity.y, emitter->radius), SPLRandom_Range(FX32_ONE));
            pos.z = SPLRandom_Range(emitter->length);
            SPLEmitter_TiltCoordinates(&particle->position, &pos, emitter);
            break;
        }
        }

        {
            // Particles move out from the emitter, and along its axis
            VecFx32 dir;

            velPositionAmp = SPLRandom_Vary(emitter->initVelPositionAmplifier, header->velocityVariance);
            velAxisAmp = SPLRandom_Vary(emitter->initVelAxisAmplifier, header->velocityVariance);
            if (header->flags.emissionType == SPL_EMISSION_CYLINDER_SURFACE) {
                VecFx32 out;

                out.x = FX_Mul(particle->velocity.x, emitter->crossAxis1.x) +
                        FX_Mul(particle->velocity.y, emitter->crossAxis2.x);
                out.y = FX_Mul(particle->velocity.x, emitter->crossAxis1.y) +
                        FX_Mul(particle->velocity.y, emitter->crossAxis2.y);
                out.z = FX_Mul(particle->velocity.x, emitter->crossAxis1.z) +
                        FX_Mul(particle->velocity.y, emitter->crossAxis2.z);
                VEC_Normalize(&out, &dir);
            } else if (particle->position.x == 0 && particle->position.y == 0 && particle->position.z == 0) {
                SPLRandom_VecFx32(&dir);
            } else {
                VEC_Normalize(&particle->position, &dir);
            }
            particle->velocity.x =
                emitter->particleInitVelocity.x + (FX_Mul(dir.x, velPositionAmp) + FX_Mul(emitter->axis.x, velAxisAmp));
            particle->velocity.y =
                emitter->particleInitVelocity.y + (FX_Mul(dir.y, velPositionAmp) + FX_Mul(emitter->axis.y, velAxisAmp));
            particle->velocity.z =
                emitter->particleInitVelocity.z + (FX_Mul(dir.z, velPositionAmp) + FX_Mul(emitter->axis.z, velAxisAmp));
            particle->emitterPos = emitter->position;
        }

        particle->baseScale = SPLRandom_Vary(emitter->baseScale, header->scaleVariance);
        particle->animScale = FX16_ONE;

        // A color animation may start at any of its colors
        if (header->flags.hasColorAnim && resource->colorAnim->randomStart) {
            GXRgb colors[3];
            u32 index = (SPLRandom_Next() >> 20) % 3;

            colors[0] = resource->colorAnim->start;
            colors[1] = header->color;
            colors[2] = resource->colorAnim->end;
            particle->color = colors[index];
        } else {
            particle->color = header->color;
        }
        particle->baseAlpha = emitter->baseAlpha;
        particle->animAlpha = 31;

        if (header->flags.randomInitAngle) {
            particle->rotation = SPLRandom_Next();
        } else {
            particle->rotation = emitter->initAngle;
        }
        if (!header->flags.hasRotation) {
            particle->angularVelocity = 0;
        } else {
            particle->angularVelocity = ((header->maxRotation - header->minRotation) * (SPLRandom_Next() >> 20) +
                                         (header->minRotation << FX32_SHIFT)) >>
                                        FX32_SHIFT;
        }

        particle->lifeTime =
            ((emitter->particleLifeTime * (255 - ((header->lifeTimeVariance * (s32)(SPLRandom_Next() >> 24)) >> 8))) >>
             8) +
            1;
        particle->age = 0;

        if (header->flags.hasTextureAnim && resource->textureAnim->randomStart) {
            particle->texture =
                resource->textureAnim->textures[(SPLRandom_Next() >> 20) % resource->textureAnim->count];
        } else if (header->flags.hasTextureAnim && !resource->textureAnim->randomStart) {
            particle->texture = resource->textureAnim->textures[0];
        } else {
            particle->texture = header->texture;
        }

        particle->loopTimeFactor = 0xffff / resource->header->loopTime;
        particle->lifeTimeFactor = 0xffff / particle->lifeTime;
        particle->lifeRateOffset = 0;
        if (header->flags.randomLoopOffset) {
            particle->lifeRateOffset = SPLRandom_Next() >> 24;
        }
    }
}

void SPLEmitter_EmitChildren(SPLParticle *parent, SPLEmitter *emitter, SPLList *freeList) {
    SPLParticle *particle;
    SPLChildResource *child = emitter->resource->childResource;
    fx32 velocityRatio = FX_Mul(child->velocityRatio << FX32_SHIFT, FX32_ONE / 256);
    int i;
    fx32 velocity;
    fx32 scale;

    for (i = 0; i < child->emissionCount; i++) {
        particle = (SPLParticle *)SPLList_PopFront(freeList);
        if (particle == NULL) {
            return;
        }
        SPLList_PushFront(&emitter->childParticles, (SPLNode *)particle);

        particle->position = parent->position;
        velocity = FX_Mul(parent->velocity.x, velocityRatio);
        particle->velocity.x = velocity + SPLRandom_Range(child->randomInitVelMag);
        velocity = FX_Mul(parent->velocity.y, velocityRatio);
        particle->velocity.y = velocity + SPLRandom_Range(child->randomInitVelMag);
        velocity = FX_Mul(parent->velocity.z, velocityRatio);
        particle->velocity.z = velocity + SPLRandom_Range(child->randomInitVelMag);
        particle->emitterPos = parent->emitterPos;

        scale = (parent->baseScale * parent->animScale) >> FX32_SHIFT;
        particle->baseScale = (scale * (child->scaleRatio + 1)) >> 6;
        particle->animScale = FX16_ONE;
        if (child->useChildColor) {
            particle->color = child->color;
        } else {
            particle->color = parent->color;
        }
        particle->baseAlpha = (parent->baseAlpha * (parent->animAlpha + 1)) >> 5;
        particle->animAlpha = 31;

        switch (child->rotationType) {
        case SPL_CHILD_ROTATION_NONE:
            particle->rotation = 0;
            particle->angularVelocity = 0;
            break;
        case SPL_CHILD_ROTATION_ANGLE:
            particle->rotation = parent->rotation;
            particle->angularVelocity = 0;
            break;
        case SPL_CHILD_ROTATION_ANGLE_AND_VELOCITY:
            particle->rotation = parent->rotation;
            particle->angularVelocity = parent->angularVelocity;
            break;
        }

        particle->lifeTime = child->lifeTime;
        particle->age = 0;
        particle->texture = child->texture;
        // From the parent's life time, not the child's
        particle->loopTimeFactor = 0xffff / (parent->lifeTime / 2);
        particle->lifeTimeFactor = 0xffff / parent->lifeTime;
        particle->lifeRateOffset = 0;
    }
}
