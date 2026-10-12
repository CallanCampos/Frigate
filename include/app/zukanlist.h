#ifndef POKEBW2_APP_ZUKANLIST_H
#define POKEBW2_APP_ZUKANLIST_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// zukanlist.c: the process of the Pokédex's list (overlay 299). The ROM doesn't name the overlay's files; zukanlist.c
// is a guess after the search screen's zukansearch.c. The names are ours

// What the list returns in ZukanListParam's result: the return button, the exit button, a change of Pokédex that makes
// the list again, a species chosen, the search and the habitat list
enum {
    ZUKAN_LIST_RESULT_BACK,
    ZUKAN_LIST_RESULT_CLOSE,
    ZUKAN_LIST_RESULT_RELOAD,
    ZUKAN_LIST_RESULT_DETAIL,
    ZUKAN_LIST_RESULT_SEARCH,
    ZUKAN_LIST_RESULT_HABITAT,
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
} ZukanListParam;

extern const GameProcFunctions ZukanList_ProcFunctions;

#endif // POKEBW2_APP_ZUKANLIST_H
