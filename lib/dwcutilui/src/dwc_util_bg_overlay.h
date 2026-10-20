#ifndef POKEBW2_DWCUTIL_DWC_UTIL_BG_OVERLAY_H
#define POKEBW2_DWCUTIL_DWC_UTIL_BG_OVERLAY_H

#include "types.h"

// The connection utility's highlight on BG 1 of the main screen: one of 20 screens of the highlight's characters,
// copied into a buffer and uploaded at the next VBlank.
// The ROM does not embed this source file's name; dwc_util_bg_overlay.c is descriptive.

// Loads the characters and the first screen of the starting screen's highlight
void DWCUtilBgOverlay_Init(void);
void DWCUtilBgOverlay_Exit(void);
void DWCUtilBgOverlay_Show(int index);
// The highlight of the connection being edited, on its settings' screen and on the list of connections
void DWCUtilBgOverlay_ShowConnectionSet(void);
void DWCUtilBgOverlay_ShowConnectionList(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_BG_OVERLAY_H
