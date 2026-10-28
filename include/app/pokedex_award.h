#ifndef POKEBW2_APP_POKEDEX_AWARD_H
#define POKEBW2_APP_POKEDEX_AWARD_H

#include "types.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// The Pokédex diplomas: the regional one, overlay 314 (regional_pokedex_award.c), and the national one, overlay 315
// (national_pokedex_award.c)
#define OVERLAY_REGIONAL_POKEDEX_AWARD OVERLAY_ID(314)
#define OVERLAY_NATIONAL_POKEDEX_AWARD OVERLAY_ID(315)

typedef struct {
    GameData *gameData;
    // Whether the diploma is shown again, without its jingle and its slide in
    u32 unk04;
} PokedexAwardParam;

#endif // POKEBW2_APP_POKEDEX_AWARD_H
