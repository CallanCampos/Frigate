#ifndef POKEBW2_SYSTEM_TPOKE_DATA_H
#define POKEBW2_SYSTEM_TPOKE_DATA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The field models of walking Pokémon (tpoke_data.c): which object code a species, sex and form use. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except TPOKE_INDEX_NONE and TPOKE_SEX_ANY

// What GetFieldPokemonActorLUTIndex_ returns when no entry fits
#define TPOKE_INDEX_NONE 0xFFFF

TPokeData *LoadTPokeData(HeapID heapId);
void FreeTPokeData(TPokeData *data);
// The object code of the entry that fits the Pokémon, or of entry 0 if none does
u16 GetPokemonFieldOBJCODE(TPokeData *data, u16 species, u16 sex, u16 form);
BOOL IsFieldPokemonSpriteHugeBillboard(GameData *unused, TPokeData *data, u16 species, u16 sex, u16 form);
// The index of the entry that fits the Pokémon, or TPOKE_INDEX_NONE
int GetFieldPokemonActorLUTIndex_(const TPokeData *data, u16 species, u16 sex, u16 form);

#endif // POKEBW2_SYSTEM_TPOKE_DATA_H
