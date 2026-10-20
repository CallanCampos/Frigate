#ifndef POKEBW2_DWCUTIL_DWC_UTIL_FADE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_FADE_H

#include "types.h"
#include "nitro/gx.h"

// The connection utility's brightness fades of the two screens, one at a time on each, run by a VBlank task. A
// screen is 1 for the sub engine's and anything else for the main engine's. The four fade types are from black, to
// black, from white and to white.
// The ROM does not embed this source file's name; dwc_util_fade.c is descriptive.

// The fade types
enum {
    DWC_UTIL_FADE_FROM_BLACK,
    DWC_UTIL_FADE_TO_BLACK,
    DWC_UTIL_FADE_FROM_WHITE,
    DWC_UTIL_FADE_TO_WHITE,
};

// Every plane and the backdrop
#define DWC_UTIL_FADE_PLANES                                                                                           \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |               \
     GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD)

// Turns both screens white
void DWCUtilFade_Init(void);
void DWCUtilFade_Exit(void);
// Whether a fade of the screen is running
u8 DWCUtilFade_IsActive(int screen);
// Fades the planes of the screen over length frames, or returns FALSE if a fade of it is running
BOOL DWCUtilFade_Start(int type, int screen, u32 planes, int length);
// Waits length frames on the sub screen's fade, without changing its brightness
BOOL DWCUtilFade_StartWait(int length);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_FADE_H
