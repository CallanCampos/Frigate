#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MOVE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MOVE_H

#include "types.h"
#include "nitro/mb.h"

// The connection utility's move of the Wi-Fi settings to another console: it offers dwc:/move/child.srl through DS
// Download Play (app/download_play_sys.h) and reports the flow's progress to a callback, with an event of
// DWC_UTIL_MOVE_EVENT_*.
// The ROM does not embed this source file's name; dwc_util_move.c is descriptive.

enum {
    // A console downloaded the program, whose player DWCUtilMove_GetChildUser returns
    DWC_UTIL_MOVE_EVENT_ENTERED,
    DWC_UTIL_MOVE_EVENT_CHILD_LEFT,
    DWC_UTIL_MOVE_EVENT_DONE,
    DWC_UTIL_MOVE_EVENT_ENDED,
    DWC_UTIL_MOVE_EVENT_FAILED,
};

typedef void (*DWCUtilMoveCallback)(int event);

void DWCUtilMove_Start(DWCUtilMoveCallback callback);
// Ends the move; DWCUtilMove_HasEnded says when it has ended
void DWCUtilMove_End(void);
BOOL DWCUtilMove_HasEnded(void);
void DWCUtilMove_SetCallback(DWCUtilMoveCallback callback);
void DWCUtilMove_StartSending(void);
const MBUserInfo *DWCUtilMove_GetChildUser(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MOVE_H
