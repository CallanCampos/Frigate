#ifndef POKEBW2_APP_POKEDEX_LIST_SEQ_H
#define POKEBW2_APP_POKEDEX_LIST_SEQ_H

#include "types.h"
#include "app/pokedex_list_main.h"

// pokedex_list_seq.c: the sequences of the Pokédex's list. The ROM doesn't name this file; pokedex_list_seq.c is a
// guess after the search screen's pokedex_search_seq.c. The names are ours

// Runs a frame; FALSE once the screen has ended
BOOL PokedexListSeq_Main(PokedexListWork *wk);

#endif // POKEBW2_APP_POKEDEX_LIST_SEQ_H
