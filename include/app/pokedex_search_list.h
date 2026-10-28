#ifndef POKEBW2_APP_POKEDEX_SEARCH_LIST_H
#define POKEBW2_APP_POKEDEX_SEARCH_LIST_H

#include "types.h"
#include "app/pokedex_search_main.h"

// pokedex_search_list.c: the lists of the criteria of the Pokédex's search screen. The ROM doesn't name this file;
// pokedex_search_list.c is a guess after pokedex_search_main.c. The names are ours

void PokedexSearchList_InitOrder(PokedexSearchWork *wk);
void PokedexSearchList_InitInitial(PokedexSearchWork *wk);
void PokedexSearchList_InitType(PokedexSearchWork *wk);
void PokedexSearchList_InitColor(PokedexSearchWork *wk);
void PokedexSearchList_InitShape(PokedexSearchWork *wk);
void PokedexSearchList_Exit(PokedexSearchWork *wk);

#endif // POKEBW2_APP_POKEDEX_SEARCH_LIST_H
