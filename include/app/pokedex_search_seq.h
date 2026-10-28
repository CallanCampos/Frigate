#ifndef POKEBW2_APP_POKEDEX_SEARCH_SEQ_H
#define POKEBW2_APP_POKEDEX_SEARCH_SEQ_H

#include "types.h"
#include "app/pokedex_search_main.h"

// pokedex_search_seq.c: the sequences of the Pokédex's search screen. The ROM doesn't name this file;
// pokedex_search_seq.c is a guess after pokedex_search_main.c. The names are ours

// Runs a frame; FALSE once the screen has ended
BOOL PokedexSearchSeq_Main(PokedexSearchWork *wk);

#endif // POKEBW2_APP_POKEDEX_SEARCH_SEQ_H
