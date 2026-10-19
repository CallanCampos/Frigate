#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_H

#include "types.h"

// The blank background shown while the connection utility changes screens.
// The ROM does not embed this source file's name; dwc_util_screen.c is descriptive.
void DWCUtilScreen_Reset(void);
// Shows the message on the sub screen, with the digit value at its character slot if slot isn't negative
BOOL DWCUtilScreen_Start(int msgIndex, int slot, int value);
BOOL DWCUtilScreen_Stop(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_H
