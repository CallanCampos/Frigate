#ifndef POKEBW2_APP_POKEDEX_LIST_BMP_H
#define POKEBW2_APP_POKEDEX_LIST_BMP_H

#include "types.h"
#include "app/pokedex_list_main.h"
#include "gfl/str.h"
#include "system/printsys.h"

// pokedex_list_bmp.c: the windows and text of the Pokédex's list. The ROM doesn't name this file; pokedex_list_bmp.c is
// a guess after the search screen's pokedex_search_bmp.c. The names are ours

void PokedexListBmp_Init(PokedexListWork *wk);
void PokedexListBmp_Exit(PokedexListWork *wk);
void PokedexListBmp_Update(PokedexListWork *wk);
// How many species the Pokédex has seen and caught
void PokedexListBmp_PutCounts(PokedexListWork *wk);
// How many species the search found
void PokedexListBmp_PutSearchCount(PokedexListWork *wk);
// A row of the list: the species' number and its name
void PokedexListBmp_PrintItem(PokedexListWork *wk, PrintWindow *window, StrBuf *name, u32 number);

#endif // POKEBW2_APP_POKEDEX_LIST_BMP_H
