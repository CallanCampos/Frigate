#ifndef POKEBW2_DWCUTIL_MOVE_H
#define POKEBW2_DWCUTIL_MOVE_H

#include "types.h"
#include "nitro/mb.h"

// The connection utility's move of the Wi-Fi settings to another console: it offers dwc:/move/child.srl through DS
// Download Play (dwcutil/dwc_moveid.h) and reports the flow's progress to a callback, with an event of
// DWC_UTIL_MOVE_EVENT_*.
// The file is the DWC utility library's move.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.

enum {
    // A console downloaded the program, whose player DWCi_MOVlGetChild returns
    DWC_UTIL_MOVE_EVENT_ENTERED,
    DWC_UTIL_MOVE_EVENT_CHILD_LEFT,
    DWC_UTIL_MOVE_EVENT_DONE,
    DWC_UTIL_MOVE_EVENT_ENDED,
    DWC_UTIL_MOVE_EVENT_FAILED,
};

typedef void (*DWCUtilMoveCallback)(int event);

void DWCi_MOVlInit(DWCUtilMoveCallback callback);
// Ends the move; DWCi_MOVlIsEnd says when it has ended
void DWCi_MOVlEnd(void);
BOOL DWCi_MOVlIsEnd(void);
void DWCi_MOVlChangeCallback(DWCUtilMoveCallback callback);
void DWCi_MOVlAdmitChild(void);
const MBUserInfo *DWCi_MOVlGetChild(void);

#endif // POKEBW2_DWCUTIL_MOVE_H
