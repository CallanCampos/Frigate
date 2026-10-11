#ifndef POKEBW2_FIELD_GIMMICK_GIANT_CHASM_6_H
#define POKEBW2_FIELD_GIMMICK_GIANT_CHASM_6_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Giant Chasm's innermost cave, zone 604 (overlay 116): four models with animations and sounds,
// shown and played by script plugin 14 (overlay 66), and a task that makes an actor hover. The sounds use the
// chasm's Japanese name, SEQ_SE_SW_GIANTHOLE_*. The name is a guess: the ROM has no string for it

// Overlay 36's gimmick table calls these
void GimmickGC6_Setup(Field *field);
void GimmickGC6_End(Field *field);
void GimmickGC6_Update(Field *field);

// Script plugin 14 calls these
// Shows model 0 and hides model 1 when which is 0, and the reverse otherwise
void GimmickGC6_ShowModel01(GameSystem *gsys, u16 which);
// Plays the animations of model 0 when which is 0, or of model 1, with the sounds of the first
void GimmickGC6_PlayModel01(GameSystem *gsys, u16 which);
// Makes the actor of the id hover
void GimmickGC6_CallStartHover(GameSystem *gsys, u32 actorId);
// Hides model 3 and stops its animations
void GimmickGC6_HideModel3(GameSystem *gsys);
// Plays the animations of model 2 if which is TRUE, or of model 3, with footsteps
void GimmickGC6_PlayModel23(GameSystem *gsys, BOOL which);

#endif // POKEBW2_FIELD_GIMMICK_GIANT_CHASM_6_H
