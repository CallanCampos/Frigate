#ifndef POKEBW2_APP_POKEDEX_OPEN_H
#define POKEBW2_APP_POKEDEX_OPEN_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Pokédex's opening, overlay 301 (pokedex_open.c): the Pokédex's cover, with the downloaded skin if there is one,
// whose halves slide apart once the player presses A, touches the screen or waits. B or X leaves it. The Pokédex
// (overlay 302, pokedex.c) queues it. The name is a guess: the ROM has no string for it
#define OVERLAY_POKEDEX_OPEN OVERLAY_ID(301)

// What the Pokédex ends the opening with: B, X or the end of the opening. The Pokédex tells by the result whether to
// go on
enum {
    POKEDEX_OPEN_RESULT_BACK,
    POKEDEX_OPEN_RESULT_CLOSE,
    POKEDEX_OPEN_RESULT_DONE,
};

typedef struct {
    GameData *gameData;
    PlayerInfo *playerInfo;
    // A POKEDEX_OPEN_RESULT_*
    u32 result;
} PokedexOpenParam;

extern const GameProcFunctions POKEDEX_OPEN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_POKEDEX_OPEN_H
