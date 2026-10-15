#ifndef POKEBW2_FIELD_EVENT_CGEAR_POWERON_H
#define POKEBW2_FIELD_EVENT_CGEAR_POWERON_H

// Overlay 12's event that turns the C-Gear on. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

typedef struct {
    // Set once the subscreen has changed to the C-Gear
    BOOL subscreenChanged;
    // Whether to start the game's communication
    BOOL bootComm;
    GameSystem *gsys;
} CGearPowerOnData;

GameEvent *EventCGearPowerOn_Create(GameSystem *gsys, BOOL bootComm);

#endif // POKEBW2_FIELD_EVENT_CGEAR_POWERON_H
