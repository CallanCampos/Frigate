#ifndef POKEBW2_APP_POKEDEX_H
#define POKEBW2_APP_POKEDEX_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Pokédex, overlay 302 (pokedex.c, zukan.c by the ROM's string, and pokedex_common.c and pokedex_search_engine.c):
// it runs the Pokédex's screens one after the other as processes of its own, the opening (overlay 301), the list (299),
// the search (300), the detail pages (298), the habitat list (303) and the awards' list (304), and returns once the
// player closes it. The names are ours; swan has none
#define OVERLAY_POKEDEX OVERLAY_ID(302)

// What the Pokédex starts on
enum {
    // The opening, then the list
    POKEDEX_START_OPEN,
    // The list, or the awards' list once the national Pokédex is obtained
    POKEDEX_START_LIST,
    // The detail pages: the info, the habitat map, the cry and the forms
    POKEDEX_START_INFO,
    POKEDEX_START_MAP,
    POKEDEX_START_VOICE,
    POKEDEX_START_FORM,
    POKEDEX_START_SEARCH,
    POKEDEX_START_AWARDS,
    POKEDEX_START_HABITATS,
};

typedef struct {
    GameData *gameData;
    PokeDexSave *pokedex;
    PlayerInfo *playerInfo;
    // A POKEDEX_START_*
    u16 start;
    // 0 if the player left with B or the last action, else 1
    u16 result;
} PokedexParam;

extern const GameProcFunctions POKEDEX_PROC_FUNCTIONS;

#endif // POKEBW2_APP_POKEDEX_H
