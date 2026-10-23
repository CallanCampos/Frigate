#ifndef POKEBW2_APP_ZUKANSEARCH_H
#define POKEBW2_APP_ZUKANSEARCH_H

#include "types.h"
#include "app/zukan_common.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// zukansearch.c: the process of the Pokédex's search screen (overlay 300). The ROM doesn't name this file;
// zukansearch.c is a guess after zknsearch_main.c. The names are ours

// What the search returns in ZukanSearchParam's result
enum {
    ZUKAN_SEARCH_RESULT_FOUND,
    ZUKAN_SEARCH_RESULT_CANCEL,
    ZUKAN_SEARCH_RESULT_CLOSE,
};

// What the search is given, 0x18 bytes: the criteria it edits, and the species it found
typedef struct {
    GameData *gameData;
    PokeDexSave *pokedex;
    ZukanSearchCriteria *criteria;
    // A ZUKAN_SEARCH_RESULT_*
    int result;
    u16 *list;
    u16 count;
} ZukanSearchParam;

extern const GameProcFunctions ZukanSearch_ProcFunctions;

#endif // POKEBW2_APP_ZUKANSEARCH_H
