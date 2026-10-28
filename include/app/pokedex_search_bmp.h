#ifndef POKEBW2_APP_POKEDEX_SEARCH_BMP_H
#define POKEBW2_APP_POKEDEX_SEARCH_BMP_H

#include "types.h"
#include "app/pokedex_search_main.h"
#include "gfl/str.h"
#include "system/printsys.h"

// pokedex_search_bmp.c: the windows and text of the Pokédex's search screen. The ROM doesn't name this file;
// pokedex_search_bmp.c is a guess after pokedex_search_main.c. The names are ours

void PokedexSearchBmp_Init(PokedexSearchWork *wk);
void PokedexSearchBmp_Exit(PokedexSearchWork *wk);
// Runs the print queue and sends the windows that are printed
void PokedexSearchBmp_Update(PokedexSearchWork *wk);
// The main page: its labels and the criteria's values
void PokedexSearchBmp_PutMain(PokedexSearchWork *wk);
// The line at the top of the touch screen: what to do, or nothing while the search runs
void PokedexSearchBmp_PutInfo(PokedexSearchWork *wk);
void PokedexSearchBmp_ClearInfo(PokedexSearchWork *wk);
// The pages of the lists: the line at the top, and the criterion's name and value on the top screen
void PokedexSearchBmp_PutOrder(PokedexSearchWork *wk);
void PokedexSearchBmp_PutOrderValue(PokedexSearchWork *wk);
void PokedexSearchBmp_PutInitial(PokedexSearchWork *wk);
void PokedexSearchBmp_PutInitialValue(PokedexSearchWork *wk);
void PokedexSearchBmp_PutType(PokedexSearchWork *wk);
void PokedexSearchBmp_PutTypeValue(PokedexSearchWork *wk);
void PokedexSearchBmp_PutColor(PokedexSearchWork *wk);
void PokedexSearchBmp_PutColorValue(PokedexSearchWork *wk);
void PokedexSearchBmp_PutShape(PokedexSearchWork *wk);
// A list's item, into its row's window
void PokedexSearchBmp_PrintItem(PokedexSearchWork *wk, PrintWindow *window, StrBuf *strbuf);
void PokedexSearchBmp_FlushItem(PokedexSearchWork *wk, PrintWindow *window);
// The messages of the search
void PokedexSearchBmp_PutSearching(PokedexSearchWork *wk);
void PokedexSearchBmp_PutFound(PokedexSearchWork *wk);
void PokedexSearchBmp_PutNotFound(PokedexSearchWork *wk);
void PokedexSearchBmp_PutStart(PokedexSearchWork *wk);

#endif // POKEBW2_APP_POKEDEX_SEARCH_BMP_H
