#ifndef POKEBW2_APP_ZKNLIST_BMP_H
#define POKEBW2_APP_ZKNLIST_BMP_H

#include "types.h"
#include "app/zknlist_main.h"
#include "gfl/str.h"
#include "system/printsys.h"

// zknlist_bmp.c: the windows and text of the Pokédex's list. The ROM doesn't name this file; zknlist_bmp.c is a guess
// after the search screen's zknsearch_bmp.c. The names are ours

void ZknListBmp_Init(ZknListWork *wk);
void ZknListBmp_Exit(ZknListWork *wk);
void ZknListBmp_Update(ZknListWork *wk);
// How many species the Pokédex has seen and caught
void ZknListBmp_PutCounts(ZknListWork *wk);
// How many species the search found
void ZknListBmp_PutSearchCount(ZknListWork *wk);
// A row of the list: the species' number and its name
void ZknListBmp_PrintItem(ZknListWork *wk, PrintWindow *window, StrBuf *name, u32 number);

#endif // POKEBW2_APP_ZKNLIST_BMP_H
