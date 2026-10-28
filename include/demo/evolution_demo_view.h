#ifndef POKEBW2_DEMO_EVOLUTION_DEMO_VIEW_H
#define POKEBW2_DEMO_EVOLUTION_DEMO_VIEW_H

#include "types.h"

// Defined in evolution_demo_view.c

// Not referenced
extern const u32 EVOLUTION_DEMO_VIEW_UNK_774;
// The helix the pieces of the sprite gather into: its top, its height, and its distance from its axis at its ends and
// at its middle
extern const f32 EVOLUTION_DEMO_HELIX_TOP;
extern const f32 EVOLUTION_DEMO_HELIX_HEIGHT;
extern const f32 EVOLUTION_DEMO_HELIX_RADIUS_MAX;
extern const f32 EVOLUTION_DEMO_HELIX_RADIUS_MIN;

// Reconstructed, not known from the ROM. MWCC puts the constants above in the same section as the view's other data,
// as the original has them, only when some code reads them before their definitions, even code it never emits. These
// accessors are that code. Nothing calls them.
static inline u32 EvolutionDemoView_GetUnk774(void) {
    return EVOLUTION_DEMO_VIEW_UNK_774;
}

static inline f32 EvolutionDemoView_GetHelixTop(void) {
    return EVOLUTION_DEMO_HELIX_TOP;
}

static inline f32 EvolutionDemoView_GetHelixHeight(void) {
    return EVOLUTION_DEMO_HELIX_HEIGHT;
}

static inline f32 EvolutionDemoView_GetHelixRadiusMax(void) {
    return EVOLUTION_DEMO_HELIX_RADIUS_MAX;
}

static inline f32 EvolutionDemoView_GetHelixRadiusMin(void) {
    return EVOLUTION_DEMO_HELIX_RADIUS_MIN;
}

#endif // POKEBW2_DEMO_EVOLUTION_DEMO_VIEW_H
