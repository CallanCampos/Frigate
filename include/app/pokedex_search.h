#ifndef POKEBW2_APP_POKEDEX_SEARCH_H
#define POKEBW2_APP_POKEDEX_SEARCH_H

#include "types.h"
#include "app/pokedex_common.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// pokedex_search.c: the process of the Pokédex's search screen (overlay 300). The ROM doesn't name this file;
// pokedex_search.c is a guess after pokedex_search_main.c. The names are ours

// What the search returns in PokedexSearchParam's result
enum {
    POKEDEX_SEARCH_RESULT_FOUND,
    POKEDEX_SEARCH_RESULT_CANCEL,
    POKEDEX_SEARCH_RESULT_CLOSE,
};

// What the search is given, 0x18 bytes: the criteria it edits, and the species it found
typedef struct {
    GameData *gameData;
    PokeDexSave *pokedex;
    PokedexSearchCriteria *criteria;
    // A POKEDEX_SEARCH_RESULT_*
    int result;
    u16 *list;
    u16 count;
} PokedexSearchParam;

extern const GameProcFunctions PokedexSearch_ProcFunctions;

#endif // POKEBW2_APP_POKEDEX_SEARCH_H
