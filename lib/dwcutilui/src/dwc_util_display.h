#ifndef POKEBW2_DWCUTIL_DWC_UTIL_DISPLAY_H
#define POKEBW2_DWCUTIL_DWC_UTIL_DISPLAY_H

#include "types.h"

// The connection utility's points and rectangles (u16 x and y; left, top, right and bottom), and the screens' visible
// planes and windows. A screen is 1 for the sub engine's and anything else for the main engine's.
// The ROM does not embed this source file's name; dwc_util_display.c is descriptive.
void DWCUtilDisplay_SetPoint(u16 x, u16 y, u16 *point);
void DWCUtilDisplay_SetRect(int left, int top, int right, int bottom, u16 *rect);
// The rectangle of a size at a position, both points
void DWCUtilDisplay_SetRectFromSize(const void *position, const void *size, void *rect);
void DWCi_GXlInvisible(int screen, int planes);
void DWCUtilDisplay_HidePlanes(int screen, int planes);
// Sets window 0 or 1 of the screen to the rectangle
void DWCi_GXlSetWndPosition(int screen, int window, const u16 *rect);
// The planes and the effect inside window 0, window 1 or the OBJ window, or outside them (window 3)
void DWCi_GXlSetWndPlane(int screen, int window, int planes, BOOL effect);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_DISPLAY_H
