#ifndef POKEBW2_FIELD_BUILDING_ENTER_EFFECT_H
#define POKEBW2_FIELD_BUILDING_ENTER_EFFECT_H

// Overlay 12's building_enter_effect.c: the zoom into a building as the player enters it

#include "types.h"

// Starts the zoom, which runs each VBlank
void setBuildingEnterVBlankCallback(void);
// Ends it and resets BG 2's matrix
void resetBuildingEnterVBlankCallback(void);

#endif // POKEBW2_FIELD_BUILDING_ENTER_EFFECT_H
