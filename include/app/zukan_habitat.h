#ifndef POKEBW2_APP_ZUKAN_HABITAT_H
#define POKEBW2_APP_ZUKAN_HABITAT_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The Pokédex's Habitat List, overlay 303: the screen that shows, area by area, the Pokémon that live in the tall
// grass, on the water and at the end of a fishing rod, and stamps an area once all of them are seen or caught. The
// Pokédex (overlay 302, zukan.c) runs it with a ZukanHabitatListParam (app/zukan_common.h). The ROM names none of
// the overlay's files: zukan_habitat.c and zukan_habitat_graphic.c are guesses, and all the names are ours
#define OVERLAY_ZUKAN_HABITAT OVERLAY_ID(303)

// Not from swan: the screen's heap
#define HEAPID_ZUKAN_HABITAT 0xa5

// The shortcut of the Habitat List that the Y button registers
#define SHORTCUT_POKEDEX_HABITAT 32

// What ZukanHabitatListParam's result is set to
enum {
    // The player closed the Pokédex
    ZUKAN_HABITAT_RESULT_EXIT,
    ZUKAN_HABITAT_RESULT_RETURN,
    // The player chose a Pokémon, which is now the Pokédex's species
    ZUKAN_HABITAT_RESULT_INFO,
};

extern const GameProcFunctions ZukanHabitat_ProcFunctions;

#endif // POKEBW2_APP_ZUKAN_HABITAT_H
