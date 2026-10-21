#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_WAY_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_WAY_H

#include "types.h"

// The connection utility's choice of how to set up a connection not in use: a wide button for the search for an access
// point, and below it the AOSS setup, a third setup that only the layout of flag 1 of the utility's param has (in
// Japanese only), and the settings by hand. B goes back to the first screen of the connection settings.
// The ROM does not embed this source file's name; dwc_util_screen_way.c is named after its screen, char/jb3Way.
void DWCUtilScreenWay_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_WAY_H
