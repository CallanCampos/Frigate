#ifndef POKEBW2_DWCUTIL_FOUNDATION_H
#define POKEBW2_DWCUTIL_FOUNDATION_H

#include "types.h"

// The whole touch screen: left, top, right and bottom
extern const u16 DWCi_R_SCR[4];

// The connection utility's points and rectangles (u16 x and y; left, top, right and bottom). The file is the DWC
// utility library's foundation.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that library's
// objects (layout/symbols_sdk.txt) shows it: its three functions, then graphics.o's.
void DWCi_InputCoord(u16 x, u16 y, u16 *point);
void DWCi_InputRect(int left, int top, int right, int bottom, u16 *rect);
// The rectangle of a size at a position, both points
void DWCi_Coord2Rect(const void *position, const void *size, void *rect);

#endif // POKEBW2_DWCUTIL_FOUNDATION_H
