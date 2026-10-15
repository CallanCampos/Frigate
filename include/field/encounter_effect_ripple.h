#ifndef POKEBW2_FIELD_ENCOUNTER_EFFECT_RIPPLE_H
#define POKEBW2_FIELD_ENCOUNTER_EFFECT_RIPPLE_H

// Overlay 151: the encounter effects that ripple the captured screen like the surface of water, drawn as a 32 by 24
// mesh whose vertices rise and fall. Drops land in the middle of the screen while the screen fades out. Overlay 148
// runs the event around it. The file's name is a guess; the ROM has no string for it

#include "types.h"
#include "field/encounter_effect.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The mesh's vertices, one more than its quads in each direction
#define ENCEFF_RIPPLE_COLUMNS 33
#define ENCEFF_RIPPLE_ROWS 25
#define ENCEFF_RIPPLE_VERTICES (ENCEFF_RIPPLE_COLUMNS * ENCEFF_RIPPLE_ROWS)

// The effect's work area, 0x19e0 bytes
typedef struct EncEffRipple {
    // z is the height of the water at the vertex
    VecFx16 vertices[ENCEFF_RIPPLE_VERTICES];
    // How fast each vertex's height changes
    s16 speeds[ENCEFF_RIPPLE_VERTICES];
    // Frames until the next drop, and then until the effect ends
    s16 timer;
    s16 drops;
    u16 dropInterval;
    // Frames to wait after the last drop
    u16 settleTime;
    // The height a drop sets the middle vertex to
    fx32 dropHeight;
    // The screens the fade goes to white or black on (FADE_ENGINE_*)
    u32 fadeMode;
    // Frames before the fade starts
    s32 fadeDelay;
    BOOL fadeStarted;
} EncEffRipple;

// Two white flashes, then drops that ripple the screen as it fades to white
GameEvent *EncEffRipple_CreateWhite(GameSystem *gsys, Field *field);
// Quicker and stronger drops, without the flashes, as the screen fades to black
GameEvent *EncEffRipple_CreateBlack(GameSystem *gsys, Field *field);
// The effect's render function: clears the screen to gray, then draws the ripples with overlay 148
void EncEffRipple_ClearAndDraw(EncEff *effect);

#endif // POKEBW2_FIELD_ENCOUNTER_EFFECT_RIPPLE_H
