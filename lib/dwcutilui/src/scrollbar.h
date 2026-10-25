#ifndef POKEBW2_DWCUTIL_SCROLLBAR_H
#define POKEBW2_DWCUTIL_SCROLLBAR_H

#include "types.h"

// The events of the scroll bar, from DWCi_SBlGetState
enum {
    DWC_UTIL_SCROLLBAR_NONE,
    DWC_UTIL_SCROLLBAR_GRAB,     // the handle was touched
    DWC_UTIL_SCROLLBAR_DRAG,     // and moved
    DWC_UTIL_SCROLLBAR_RELEASE,  // and let go, or the bar was touched and the handle jumped there
    DWC_UTIL_SCROLLBAR_UP,       // the arrow above the bar was touched
    DWC_UTIL_SCROLLBAR_UP_END,   // and let go
    DWC_UTIL_SCROLLBAR_DOWN,     // the arrow below it
    DWC_UTIL_SCROLLBAR_DOWN_END, // and let go
};

// The connection utility's scroll bar of a list, on the main screen: a handle of one of three lengths (kind 0 to 2,
// one that can't be dragged for 0) that travels range pixels down from x, y, an arrow above and an arrow below.
// The file is the DWC utility library's scrollbar.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SBlInit(int kind, int range, u16 x, u16 y, int pos);
void DWCi_SBlEnd(void);
// The handle's position, in pixels from the top of the bar, and the event of this frame
u8 DWCi_SBlGet(void);
u8 DWCi_SBlGetState(void);
void DWCi_SBlSet(int pos);
// Unlocks and locks the scroll bar
void DWCi_SBlEnable(void);
void DWCi_SBlDisable(void);

#endif // POKEBW2_DWCUTIL_SCROLLBAR_H
