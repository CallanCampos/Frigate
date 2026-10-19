#ifndef POKEBW2_FIELD_GIMMICK_LEAGUE_STATUE_H
#define POKEBW2_FIELD_GIMMICK_LEAGUE_STATUE_H

#include "types.h"
#include "struct_decls.h"

// The statue gimmick of the Pokémon League, overlay 105. Overlay 36's gimmick table calls these
void LeagueStatue_Init(Field *field);
void LeagueStatue_End(Field *field);
void LeagueStatue_Update(Field *field);

#endif // POKEBW2_FIELD_GIMMICK_LEAGUE_STATUE_H
