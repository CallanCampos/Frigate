#ifndef POKEBW2_FIELD_GIMMICK_FUNFEST_LEVELS_H
#define POKEBW2_FIELD_GIMMICK_FUNFEST_LEVELS_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Funfest's Entralink room (overlay 130): the Entralink levels of the player whose beacon is
// shown, as lit models, and the door effects, whose sounds are SEQ_SE_SW_FES_*. The name is a guess: the ROM has no
// string for it, and the zone is not known

// Overlay 36's gimmick table calls these
void GimmickFunfestLevels_Setup(Field *field);
void GimmickFunfestLevels_End(Field *field);
void GimmickFunfestLevels_Update(Field *field);

// The Funfest mission's start event calls these, the first on starting and the second 70 frames later
void GimmickFunfestLevels_PlayStartEffect(GameSystem *gsys);
void GimmickFunfestLevels_PlayLeaveEffect(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_FUNFEST_LEVELS_H
