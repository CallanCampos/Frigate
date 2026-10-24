// The work of the game's particle systems on a host (src/gfl/particle.c). A system's SPL manager allocates its manager,
// its 24 emitters and 200 particles, and the resource file's resource, behavior and texture tables from one buffer, a
// bump allocator that never frees. The 13 files that create systems size that buffer for the DS's structs, and every
// one of those structs holds pointers, so on a host it overflows (the intro's 0x4800 bytes against 20,992 for the
// emitters and particles alone). Rather than resizing each caller, Particle_CreateSystemEx (src/gfl/particle.c, the
// function that creates every particle system) asks for work here.
//
// Twice the caller's size always suffices: a struct's host size is at most twice its DS size (each field at most
// doubles, a pointer from 4 to 8 bytes, and each offset with it, since no alignment more than doubles), every struct
// SPL allocates holds a pointer, so its host size is a multiple of 8 and needs none of the allocator's rounding to 4,
// and so each allocation takes at most twice what it takes on the DS, where they all fit in the caller's size. It is
// the bound port/hal/gfl/heap_policy.c uses for child heaps, here exact rather than measured.
//
// The work comes from the host's heap, not the game's: the caller still allocates its DS-sized buffer from its game
// heap, as on the DS, so the game's heaps hold what they held there. The buffer is freed with its system
// (Particle_DeleteSystem, the function that deletes one), by the system's ID, which no other live system shares. Every
// particle effect of the game is drawn from work given here.
#include "types.h"
#include "gfl/particle.h"
#include "spl/spl.h"
#include <stdio.h>
#include <stdlib.h>

// The bound above, and the work's alignment for the structs' pointers, hold only if every struct SPL allocates has a
// size that is a multiple of a pointer's
_Static_assert(sizeof(SPLManager) % sizeof(void *) == 0 && sizeof(SPLEmitter) % sizeof(void *) == 0 &&
                   sizeof(SPLParticle) % sizeof(void *) == 0 && sizeof(SPLResource) % sizeof(void *) == 0 &&
                   sizeof(SPLBehavior) % sizeof(void *) == 0 && sizeof(SPLTexture) % sizeof(void *) == 0,
               "an SPL struct's size is not a multiple of a pointer's");

static void *sWork[PARTICLE_SYSTEM_MAX];

u32 port_particle_work_size(u32 size) {
    return size * 2;
}

void *port_particle_work(u32 id, u32 size) {
    if (id >= PARTICLE_SYSTEM_MAX || sWork[id] != NULL) {
        fprintf(stderr, "port: particle system %u created while its work is in use\n", id);
        abort();
    }
    // Zeroed, as Particle_CreateSystemEx zeroes the caller's buffer
    sWork[id] = calloc(1, size);
    if (sWork[id] == NULL) {
        fprintf(stderr, "port: no memory for particle system %u's %u bytes of work\n", id, size);
        abort();
    }
    return sWork[id];
}

void port_particle_work_free(u32 id) {
    if (id < PARTICLE_SYSTEM_MAX) {
        free(sWork[id]);
        sWork[id] = NULL;
    }
}
