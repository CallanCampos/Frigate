#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_AP_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_AP_H

#include "types.h"

// The first screen of the connection utility's connection settings: a button for each of the three connections of the
// console's Wi-Fi settings, with a button below each one in use to erase it, and a wide button below them for the
// screen of func_ov182_021b688c. A connection's button goes on to its settings, or to the choice of its setup when it
// isn't in use; B goes back to the main menu, or ends the utility when it was started on this screen.
// The ROM does not embed this source file's name; dwc_util_screen_ap.c is named after its screen, char/jb2Ap.
void DWCUtilScreenAp_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SCREEN_AP_H
