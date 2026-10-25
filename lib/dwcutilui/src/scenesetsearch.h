#ifndef POKEBW2_DWCUTIL_SCENESETSEARCH_H
#define POKEBW2_DWCUTIL_SCENESETSEARCH_H

#include "types.h"

// The connection utility's search for access points: it scans for 300 frames and goes on to the list of the access
// points it found (scenesetaplist.h), or shows a dialog when it found none, or only ones with a security that
// the utility can't set; B or the button end it and go back to the choice of the setup of a connection.
// The file is the DWC utility library's scenesetsearch.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilApSearch_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETSEARCH_H
