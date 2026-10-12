#ifndef POKEBW2_APP_DPC_BMP_H
#define POKEBW2_APP_DPC_BMP_H

#include "types.h"
#include "app/dpc_main.h"

// The Hall of Fame PC's text, in dpc_bmp.c (a guessed name). Our names

void DpcBmp_Init(DendouPCWork *wk);
void DpcBmp_Exit(DendouPCWork *wk);
// Runs the print queue and sends the windows it has finished
void DpcBmp_PrintUpdate(DendouPCWork *wk);
// The page's title and date, and its number out of the pages
void DpcBmp_PutTitle(DendouPCWork *wk);
void DpcBmp_PutPage(DendouPCWork *wk);
// The data of the Pokémon at the front of the ring
void DpcBmp_PutPokeInfo(DendouPCWork *wk);
void DpcBmp_ClearPokeInfo(DendouPCWork *wk);

#endif // POKEBW2_APP_DPC_BMP_H
