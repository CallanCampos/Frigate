#ifndef POKEBW2_FIELD_GIMMICK_NUVEMA_H
#define POKEBW2_FIELD_GIMMICK_NUVEMA_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of Nuvema Town, Accumula Town and Route 1 (zones 389, 397 and 317, gimmick 0x20), overlay 133: a model
// that a script shows, plays once with an opening sound and hides again, with a night version. The name is a guess.
// Overlay 36's gimmick table calls the first three; script commands 0x23B to 0x23D of overlay 12's scrcmd_sp_poke.c
// drive the rest, though no script in the game uses them
#define GIMMICK_NUVEMA 0x20

void GimmickNuvema_Init(Field *field);
void GimmickNuvema_End(Field *field);
void GimmickNuvema_Update(Field *field);
// Shows the model and plays its animations
void GimmickNuvema_PlayModel(Field *field);
// Waits for the animations to finish
GameEvent *GimmickNuvema_CreateWaitEvent(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_NUVEMA_H
