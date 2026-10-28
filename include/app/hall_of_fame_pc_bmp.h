#ifndef POKEBW2_APP_HALL_OF_FAME_PC_BMP_H
#define POKEBW2_APP_HALL_OF_FAME_PC_BMP_H

#include "types.h"
#include "app/hall_of_fame_pc_main.h"

// The Hall of Fame PC's text, in hall_of_fame_pc_bmp.c (a guessed name). Our names

void HallOfFamePCBmp_Init(HallOfFamePCWork *wk);
void HallOfFamePCBmp_Exit(HallOfFamePCWork *wk);
// Runs the print queue and sends the windows it has finished
void HallOfFamePCBmp_PrintUpdate(HallOfFamePCWork *wk);
// The page's title and date, and its number out of the pages
void HallOfFamePCBmp_PutTitle(HallOfFamePCWork *wk);
void HallOfFamePCBmp_PutPage(HallOfFamePCWork *wk);
// The data of the Pokémon at the front of the ring
void HallOfFamePCBmp_PutPokeInfo(HallOfFamePCWork *wk);
void HallOfFamePCBmp_ClearPokeInfo(HallOfFamePCWork *wk);

#endif // POKEBW2_APP_HALL_OF_FAME_PC_BMP_H
