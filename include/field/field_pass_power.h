#ifndef POKEBW2_FIELD_FIELD_PASS_POWER_H
#define POKEBW2_FIELD_FIELD_PASS_POWER_H

#include "types.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// Overlay 156: the field's pass power events. The file's name is a guess; the ROM has no string for it.

#define OVERLAY_FIELD_PASS_POWER OVERLAY_ID(156)

// Activates a pass power and runs its script. args is two u32s: the pass power, and whether it is bought with Pass
// Orbs (overlay 14's event_pass_power.c, a GameEventProvider)
GameEvent *EventPassPowerActivate_Create(GameSystem *gsys, void *args);
// The script that says a pass power has run out (overlay 36, a GameEventProvider)
GameEvent *EventPassPowerDepleted_Create(GameSystem *gsys, void *args);
// Flashes the screens white with a sound (script command 0x1DF)
GameEvent *EventPassPowerFlash_Create(GameSystem *gsys);
// Lists the active pass powers and the time each has left, until A or B is pressed
GameEvent *EventPassPowerList_Create(GameSystem *gsys, Field *field);

#endif // POKEBW2_FIELD_FIELD_PASS_POWER_H
