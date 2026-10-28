#ifndef POKEBW2_APP_POKEDEX_HABITAT_H
#define POKEBW2_APP_POKEDEX_HABITAT_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The Pokédex's Habitat List, overlay 303: the screen that shows, area by area, the Pokémon that live in the tall
// grass, on the water and at the end of a fishing rod, and stamps an area once all of them are seen or caught. The
// Pokédex (overlay 302, pokedex.c) runs it with a PokedexHabitatListParam (app/pokedex_common.h). The ROM names none of
// the overlay's files: pokedex_habitat.c and pokedex_habitat_graphic.c are guesses, and all the names are ours
#define OVERLAY_POKEDEX_HABITAT OVERLAY_ID(303)

// Not from swan: the screen's heap
#define HEAPID_POKEDEX_HABITAT 0xa5

// What PokedexHabitatListParam's result is set to
enum {
    // The player closed the Pokédex
    POKEDEX_HABITAT_RESULT_EXIT,
    POKEDEX_HABITAT_RESULT_RETURN,
    // The player chose a Pokémon, which is now the Pokédex's species
    POKEDEX_HABITAT_RESULT_INFO,
};

extern const GameProcFunctions PokedexHabitat_ProcFunctions;

#endif // POKEBW2_APP_POKEDEX_HABITAT_H
