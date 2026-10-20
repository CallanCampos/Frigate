#ifndef POKEBW2_FIELD_BADGE_GATE_H
#define POKEBW2_FIELD_BADGE_GATE_H

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The badge gates on the way to Victory Road, in Victory Road's gimmick (overlay 103)

// Victory Road's gimmick callbacks
void BadgeGate_Setup(Field *field);
void BadgeGate_End(Field *field);
void BadgeGate_Update(Field *field);
// Plays the gate of badge (0 to 7) checking its badge
GameEvent *BadgeGate_CreateCheckEvent(GameSystem *gsys, u8 badge);
// Plays the last gate, after the eight badge gates, with the camera
GameEvent *BadgeGate_CreateLastGateEvent(GameSystem *gsys);

#endif // POKEBW2_FIELD_BADGE_GATE_H
