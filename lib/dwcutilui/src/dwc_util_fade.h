#ifndef POKEBW2_DWCUTIL_DWC_UTIL_FADE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_FADE_H

#include "types.h"

// The connection utility's brightness fades of the two screens, one at a time on each, run by a VBlank task. A
// screen is 1 for the sub engine's and anything else for the main engine's. The four fade types are from black, to
// black, from white and to white.
// The ROM does not embed this source file's name; dwc_util_fade.c is descriptive.

// Turns both screens white
void func_ov182_021bba6c(void);
void func_ov182_021bba9c(void);
// Whether a fade of the screen is running
u8 func_ov182_021bbaac(int screen);
// Fades the planes of the screen over length frames, or returns FALSE if a fade of it is running
BOOL func_ov182_021bbac4(int type, int screen, u32 planes, int length);
// Waits length frames on the sub screen's fade, without changing its brightness
BOOL func_ov182_021bbbf4(int length);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_FADE_H
