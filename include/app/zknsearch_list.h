#ifndef POKEBW2_APP_ZKNSEARCH_LIST_H
#define POKEBW2_APP_ZKNSEARCH_LIST_H

#include "types.h"
#include "app/zknsearch_main.h"

// zknsearch_list.c: the lists of the criteria of the Pokédex's search screen. The ROM doesn't name this file;
// zknsearch_list.c is a guess after zknsearch_main.c. The names are ours

void ZknSearchList_InitOrder(ZknSearchWork *wk);
void ZknSearchList_InitInitial(ZknSearchWork *wk);
void ZknSearchList_InitType(ZknSearchWork *wk);
void ZknSearchList_InitColor(ZknSearchWork *wk);
void ZknSearchList_InitShape(ZknSearchWork *wk);
void ZknSearchList_Exit(ZknSearchWork *wk);

#endif // POKEBW2_APP_ZKNSEARCH_LIST_H
