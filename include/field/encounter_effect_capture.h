#ifndef POKEBW2_FIELD_ENCOUNTER_EFFECT_CAPTURE_H
#define POKEBW2_FIELD_ENCOUNTER_EFFECT_CAPTURE_H

#include "types.h"
#include "field/encounter_effect.h"
#include "nitro/fx.h"
#include "nnsys/gfd.h"
#include "struct_decls.h"
#include "system/game_event.h"

// Overlay 148, the event around the encounter effects that draw the captured screen in 3D, such as overlay 150's
// grid of cells: it captures the 3D screen to VRAM bank D, maps the bank as a texture, then chains the effect's event.
// The file's name is a guess, since the overlay has no strings

// The event's data, which the effects' render functions get back from EncEff_GetEventData
typedef struct {
    // How many times the screen has flashed white
    s32 flashes;
    u32 texBanks;
    // Whether bank D was added to the texture banks, to be removed again at the end
    BOOL addedTexBank;
    u32 renderMode;
    // Set by the V-blank task once the capture is on
    BOOL captured;
    // Where the captured screen is in the texture VRAM
    NNSGfdTexKey texKey;
    VecFx32 camPos;
    // The effect's work area
    void *effectWork;
    GameEvent *(*init)(GameSystem *gsys);
    void (*render)(EncEffGrid *grid);
} EncEffCaptureWork;

// The event of an effect without the flashes: init makes the effect's event, and render draws it. The event with two
// white flashes first, EncEffCapture_CreateFlashEvent, and the draw function, EncEffCapture_Draw, are declared in
// field/encounter_effect.h
GameEvent *EncEffCapture_CreateEvent(GameSystem *gsys, const VecFx32 *pos, GameEvent *(*init)(GameSystem *gsys),
                                     void (*render)(EncEffGrid *grid));

#endif // POKEBW2_FIELD_ENCOUNTER_EFFECT_CAPTURE_H
