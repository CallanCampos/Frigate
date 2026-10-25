#ifndef POKEBW2_DWCUTIL_SCENESETSELECTWAY_H
#define POKEBW2_DWCUTIL_SCENESETSELECTWAY_H

#include "types.h"

// The connection utility's choice of how to set up a connection not in use: a wide button for the search for an access
// point, and below it the AOSS setup, a third setup that only the layout of flag 1 of the utility's param has (in
// Japanese only), and the settings by hand. B goes back to the first screen of the connection settings.
// The file is the DWC utility library's scenesetselectway.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetSelectWay(void);

#endif // POKEBW2_DWCUTIL_SCENESETSELECTWAY_H
