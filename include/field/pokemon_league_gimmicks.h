#ifndef POKEBW2_FIELD_POKEMON_LEAGUE_GIMMICKS_H
#define POKEBW2_FIELD_POKEMON_LEAGUE_GIMMICKS_H

#include "types.h"
#include "struct_decls.h"
#include "field/gimmick_league_caitlin.h"
#include "field/gimmick_league_champion.h"
#include "field/gimmick_league_marshal.h"
#include "field/gimmick_league_shauntal.h"

// The gimmicks of the Pokémon League's rooms, which the League's script plugin (overlay 52) drives. Their sounds use
// the Elite Four's Japanese names, as SEQ_SE_SW_CATTLEYA_* for Caitlin

// Shauntal's room, zone 140 (overlay 122), is in field/gimmick_league_shauntal.h

// Grimsley's room, zone 141 (overlay 123)
void func_ov123_021eed08(GameSystem *gsys, u16 a1);
void func_ov123_021eed3c(GameSystem *gsys, u16 a1);

// Marshal's room, zone 142 (overlay 124), is in field/gimmick_league_marshal.h

// Caitlin's room, zone 143 (overlay 125), is in field/gimmick_league_caitlin.h

// The Champion's room, zone 144 (overlay 120), is in field/gimmick_league_champion.h

#endif // POKEBW2_FIELD_POKEMON_LEAGUE_GIMMICKS_H
