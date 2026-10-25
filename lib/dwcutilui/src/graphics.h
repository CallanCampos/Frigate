#ifndef POKEBW2_DWCUTIL_GRAPHICS_H
#define POKEBW2_DWCUTIL_GRAPHICS_H

#include "types.h"

// The screens' visible planes and windows in the connection utility. A screen is 1 for the sub engine's and anything
// else for the main engine's. The file is the DWC utility library's graphics.o (libdwcutil.TWL.HYB.a), as aletheia's
// map of Black 1's SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_GXlVisible(int screen, int planes);
void DWCi_GXlInvisible(int screen, int planes);
// Sets window 0 or 1 of the screen to the rectangle
void DWCi_GXlSetWndPosition(int screen, int window, const u16 *rect);
// The planes and the effect inside window 0, window 1 or the OBJ window, or outside them (window 3)
void DWCi_GXlSetWndPlane(int screen, int window, int planes, BOOL effect);

#endif // POKEBW2_DWCUTIL_GRAPHICS_H
