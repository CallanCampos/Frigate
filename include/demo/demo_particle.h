#ifndef POKEBW2_DEMO_DEMO_PARTICLE_H
#define POKEBW2_DEMO_DEMO_PARTICLE_H

#include "types.h"
#include "gfl/particle.h"

// The particles of the evolution and egg hatching demos (ov284 and ov307). Each overlay has its own copy of the
// functions over this layout, which emit particles at set frames through the particle system of gfl/particle.h

#define DEMO_PARTICLE_BUFFER_SIZE 0x4800
#define DEMO_PARTICLE_UNIT_COUNT 1

// Particles to emit at a frame, from an emitter of a unit
typedef struct {
    u16 frame;
    u8 unit;
    u8 emitter;
} DemoParticleEvent;

// A particle system with its resource
typedef struct {
    u8 buffer[DEMO_PARTICLE_BUFFER_SIZE];
    ParticleSystem *system;
    u8 resourceCount;
} DemoParticleUnit;

typedef struct {
    u16 frame;
    u16 index;
    u16 count;
    const DemoParticleEvent *events;
    DemoParticleUnit units[DEMO_PARTICLE_UNIT_COUNT];
    BOOL active;
    // Stops the first unit when it reaches 0
    s32 stopTimer;
} DemoParticle;

// The archive and file of a unit's resource
typedef struct {
    u32 arcId;
    u32 fileId;
} DemoParticleResource;

#endif // POKEBW2_DEMO_DEMO_PARTICLE_H
