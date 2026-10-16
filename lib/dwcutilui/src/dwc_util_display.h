#ifndef POKEBW2_DWCUTIL_DWC_UTIL_DISPLAY_H
#define POKEBW2_DWCUTIL_DWC_UTIL_DISPLAY_H

#include "types.h"

// The connection utility's points and rectangles (u16 x and y; left, top, right and bottom), and the screens' visible
// planes and windows. A screen is 1 for the sub engine's and anything else for the main engine's.
// The ROM does not embed this source file's name; dwc_util_display.c is descriptive.
void func_ov182_021bc1a8(u16 x, u16 y, u16 *point);
void func_ov182_021bc1b0(u16 left, u16 top, u16 right, u16 bottom, u16 *rect);
// The rectangle of a size at a position, both points
void func_ov182_021bc1c0(const void *position, const void *size, void *rect);
void func_ov182_021bc1dc(int screen, int planes);
void func_ov182_021bc228(int screen, int planes);
// Sets window 0 or 1 of the screen to the rectangle
void func_ov182_021bc278(int screen, int window, const u16 *rect);
// The planes and the effect inside window 0, window 1 or the OBJ window, or outside them (window 3)
void func_ov182_021bc354(int screen, int window, int planes, BOOL effect);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_DISPLAY_H
