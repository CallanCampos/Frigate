#ifndef POKEBW2_DWCUTIL_DWC_UTIL_LINK_ICON_H
#define POKEBW2_DWCUTIL_DWC_UTIL_LINK_ICON_H

#include "types.h"

// The connection utility's signal strength icon at the top right of the main screen, one of two sets of four cells,
// which a task updates from the wireless link level each frame.
// The ROM does not embed this source file's name; dwc_util_link_icon.c is descriptive.
void DWCUtilLinkIcon_Init(int set);
void DWCi_ICONlEnd(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_LINK_ICON_H
