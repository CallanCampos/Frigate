#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TOP_MENU_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TOP_MENU_H

#include "types.h"

// The connection utility's screens of the move of the settings to another console (move.h): the move itself,
// with the signal icon, until the flow reports how it went; the end of a move that sent the settings, which erases
// them here and turns the console off; and the instructions before a move.
// The ROM does not embed this source file's name; dwc_util_top_menu.c is descriptive.
void DWCUtilTopMenu_InitMove(void);
void DWCUtilTopMenu_InitInstructions(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TOP_MENU_H
