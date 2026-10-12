#ifndef POKEBW2_APP_ZKNSEARCH_BMP_H
#define POKEBW2_APP_ZKNSEARCH_BMP_H

#include "types.h"
#include "app/zknsearch_main.h"
#include "gfl/str.h"
#include "system/printsys.h"

// zknsearch_bmp.c: the windows and text of the Pokédex's search screen. The ROM doesn't name this file;
// zknsearch_bmp.c is a guess after zknsearch_main.c. The names are ours

void ZknSearchBmp_Init(ZknSearchWork *wk);
void ZknSearchBmp_Exit(ZknSearchWork *wk);
// Runs the print queue and sends the windows that are printed
void ZknSearchBmp_Update(ZknSearchWork *wk);
// The main page: its labels and the criteria's values
void ZknSearchBmp_PutMain(ZknSearchWork *wk);
// The line at the top of the touch screen: what to do, or nothing while the search runs
void ZknSearchBmp_PutInfo(ZknSearchWork *wk);
void ZknSearchBmp_ClearInfo(ZknSearchWork *wk);
// The pages of the lists: the line at the top, and the criterion's name and value on the top screen
void ZknSearchBmp_PutOrder(ZknSearchWork *wk);
void ZknSearchBmp_PutOrderValue(ZknSearchWork *wk);
void ZknSearchBmp_PutInitial(ZknSearchWork *wk);
void ZknSearchBmp_PutInitialValue(ZknSearchWork *wk);
void ZknSearchBmp_PutType(ZknSearchWork *wk);
void ZknSearchBmp_PutTypeValue(ZknSearchWork *wk);
void ZknSearchBmp_PutColor(ZknSearchWork *wk);
void ZknSearchBmp_PutColorValue(ZknSearchWork *wk);
void ZknSearchBmp_PutShape(ZknSearchWork *wk);
// A list's item, into its row's window
void ZknSearchBmp_PrintItem(ZknSearchWork *wk, PrintWindow *window, StrBuf *strbuf);
void ZknSearchBmp_FlushItem(ZknSearchWork *wk, PrintWindow *window);
// The messages of the search
void ZknSearchBmp_PutSearching(ZknSearchWork *wk);
void ZknSearchBmp_PutFound(ZknSearchWork *wk);
void ZknSearchBmp_PutNotFound(ZknSearchWork *wk);
void ZknSearchBmp_PutStart(ZknSearchWork *wk);

#endif // POKEBW2_APP_ZKNSEARCH_BMP_H
