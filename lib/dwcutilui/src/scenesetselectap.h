#ifndef POKEBW2_DWCUTIL_SCENESETSELECTAP_H
#define POKEBW2_DWCUTIL_SCENESETSELECTAP_H

#include "types.h"

// The first screen of the connection utility's connection settings: a button for each of the three connections of the
// console's Wi-Fi settings, with a button below each one in use to erase it, and a wide button below them for the
// screen of DWCUtilUsbIntro_Init. A connection's button goes on to its settings, or to the choice of its setup when it
// isn't in use; B goes back to the main menu, or ends the utility when it was started on this screen.
// The file is the DWC utility library's scenesetselectap.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilScreenAp_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETSELECTAP_H
