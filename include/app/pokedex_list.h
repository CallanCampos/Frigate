#ifndef POKEBW2_APP_POKEDEX_LIST_H
#define POKEBW2_APP_POKEDEX_LIST_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// pokedex_list.c: the process of the Pokédex's list (overlay 299). The ROM doesn't name the overlay's files;
// pokedex_list.c is a guess after the search screen's pokedex_search.c. The names are ours

// What the list returns in PokedexListParam's result: the return button, the exit button, a change of Pokédex that
// makes the list again, a species chosen, the search and the habitat list
enum {
    POKEDEX_LIST_RESULT_BACK,
    POKEDEX_LIST_RESULT_CLOSE,
    POKEDEX_LIST_RESULT_RELOAD,
    POKEDEX_LIST_RESULT_DETAIL,
    POKEDEX_LIST_RESULT_SEARCH,
    POKEDEX_LIST_RESULT_HABITAT,
};

// What the list is given, 0x14 bytes
typedef struct {
    GameData *gameData;
    PokeDexSave *pokedex;
    // Whether the list is a search's
    u16 searched;
    u16 count;
    u16 *list;
    int result;
} PokedexListParam;

extern const GameProcFunctions PokedexList_ProcFunctions;

#endif // POKEBW2_APP_POKEDEX_LIST_H
