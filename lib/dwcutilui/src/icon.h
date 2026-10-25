#ifndef POKEBW2_DWCUTIL_ICON_H
#define POKEBW2_DWCUTIL_ICON_H

#include "types.h"

// The connection utility's signal strength icon at the top right of the main screen, one of two sets of four cells,
// which a task updates from the wireless link level each frame.
// The file is the DWC utility library's icon.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilLinkIcon_Init(int set);
void DWCi_ICONlEnd(void);

#endif // POKEBW2_DWCUTIL_ICON_H
