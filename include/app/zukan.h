#ifndef POKEBW2_APP_ZUKAN_H
#define POKEBW2_APP_ZUKAN_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Pokédex, overlay 302 (zukan.c, by the ROM's string, and zukan_common.c and zukan_search_engine.c): it runs the
// Pokédex's screens one after the other as processes of its own, the opening (overlay 301), the list (299), the
// search (300), the detail pages (298), the habitat list (303) and the awards' list (304), and returns once the player
// closes it. The names are ours; swan has none
#define OVERLAY_ZUKAN OVERLAY_ID(302)

// What the Pokédex starts on
enum {
    // The opening, then the list
    ZUKAN_START_OPEN,
    // The list, or the awards' list once the national Pokédex is obtained
    ZUKAN_START_LIST,
    // The detail pages: the info, the habitat map, the cry and the forms
    ZUKAN_START_INFO,
    ZUKAN_START_MAP,
    ZUKAN_START_VOICE,
    ZUKAN_START_FORM,
    ZUKAN_START_SEARCH,
    ZUKAN_START_AWARDS,
    ZUKAN_START_HABITATS,
};

typedef struct {
    GameData *gameData;
    PokeDexSave *pokedex;
    PlayerInfo *playerInfo;
    // A ZUKAN_START_*
    u16 start;
    // 0 if the player left with B or the last action, else 1
    u16 result;
} ZukanParam;

extern const GameProcFunctions ZUKAN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_ZUKAN_H
