#ifndef POKEBW2_FIELD_GIMMICK_PLASMA_FRIGATE_H
#define POKEBW2_FIELD_GIMMICK_PLASMA_FRIGATE_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of zones 561 and 564 of the Plasma Frigate (overlay 117), which the frigate's script plugin drives:
// four switches that flip four panels each, with the lights that show the result. The name is a guess: the ROM has no
// string for it, and the sound archive calls the frigate SEQ_SE_SW_PLAZMASHIP_*. The other half, zones 553 and 563,
// is overlay 119's

// Overlay 36's gimmick table calls these
void GimmickPlasmaFrigate_Setup(Field *field);
void GimmickPlasmaFrigate_End(Field *field);
void GimmickPlasmaFrigate_Update(Field *field);

// The frigate's script plugin (plugin 12) calls these
// The event that presses a switch, which flips its four panels, or NULL if there are fewer switches
GameEvent *GimmickPlasmaFrigate_CallSwitchEvent(GameSystem *gsys, int switchIndex);
// Shows the two models of the switch's pair of lights in the state that on says: with it, the first animation of the
// first, looping, and the end of the first animation of the second, and without it, the end of the second animation of
// both
void GimmickPlasmaFrigate_SetLightsState(GameSystem *gsys, int index, BOOL on);
// Plays the second animation of the first model of the switch's pair of lights, with a sound
void GimmickPlasmaFrigate_CallPlayGroupASecond(GameSystem *gsys, int index);
// Plays the second animation of the second model of the switch's pair of lights, with a sound
void GimmickPlasmaFrigate_CallPlayGroupBSecond(GameSystem *gsys, int index);

#endif // POKEBW2_FIELD_GIMMICK_PLASMA_FRIGATE_H
