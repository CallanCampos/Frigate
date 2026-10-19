#ifndef POKEBW2_DWCUTIL_DWC_UTIL_BG_OVERLAY_H
#define POKEBW2_DWCUTIL_DWC_UTIL_BG_OVERLAY_H

#include "types.h"

// The connection utility's highlight on BG 1 of the main screen: one of 20 screens of the highlight's characters,
// copied into a buffer and uploaded at the next VBlank.
// The ROM does not embed this source file's name; dwc_util_bg_overlay.c is descriptive.

// Loads the characters and the first screen of the starting screen's highlight
void func_ov182_021ab90c(void);
void func_ov182_021ab95c(void);
void func_ov182_021ab96c(int index);
// The highlight of the connection being edited, on its settings' screen and on the list of connections
void func_ov182_021ab9ac(void);
void func_ov182_021ab9c0(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_BG_OVERLAY_H
