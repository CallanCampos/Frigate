#ifndef POKEBW2_DWCUTIL_DWC_UTIL_BUTTON_SPRITES_H
#define POKEBW2_DWCUTIL_DWC_UTIL_BUTTON_SPRITES_H

#include "types.h"

// The connection utility's button sprites on the main screen: a pair of them from one of six pairs of cells, or the
// four of cells 6 to 9, at the positions given. Showing one set takes the last one away.
// The ROM does not embed this source file's name; dwc_util_button_sprites.c is descriptive.
void DWCUtilButtonSprites_Init(void);
void DWCUtilButtonSprites_Exit(void);
void DWCUtilButtonSprites_ShowPair(int pair, int x0, int x1, int y);
void DWCUtilButtonSprites_ShowFour(int x0, int x1, int y0, int y1);
void DWCUtilButtonSprites_Hide(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_BUTTON_SPRITES_H
