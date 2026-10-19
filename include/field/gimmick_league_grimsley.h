#ifndef POKEBW2_FIELD_GIMMICK_LEAGUE_GRIMSLEY_H
#define POKEBW2_FIELD_GIMMICK_LEAGUE_GRIMSLEY_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of Grimsley's room in the Pokémon League, zone 141 (gimmick 40), overlay 123: seven models, of which
// the first has two animations and the others, in two groups, four each, and a record in the save of which group has
// played. The name is a guess: the ROM has no string for it

// Overlay 36's gimmick table calls these
void GimmickGrimsley_Setup(Field *field);
void GimmickGrimsley_End(Field *field);
void GimmickGrimsley_Update(Field *field);

// The League's script plugin (overlay 52) calls these
// Shows the models of one of the two groups, the first when group is 0, and has them play, and records it in the save
void GimmickGrimsley_PlayGroup(GameSystem *gsys, int group);
// Plays one of model 0's two animations, and pauses the other
void GimmickGrimsley_PlayModel0Animation(GameSystem *gsys, int animation);

#endif // POKEBW2_FIELD_GIMMICK_LEAGUE_GRIMSLEY_H
