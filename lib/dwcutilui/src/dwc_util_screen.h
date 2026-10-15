#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_H

#include "types.h"

// The blank background shown while the connection utility changes screens.
// The ROM does not embed this source file's name; dwc_util_screen.c is descriptive.
void DWCUtilScreen_Reset(void);
BOOL DWCUtilScreen_Start(void *param, int arg1, int arg2);
BOOL DWCUtilScreen_Stop(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_H
