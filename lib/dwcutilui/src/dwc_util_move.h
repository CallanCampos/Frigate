#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MOVE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MOVE_H

#include "types.h"

// The connection utility's move of the Wi-Fi settings to another console: it offers dwc:/move/child.srl through DS
// Download Play (app/download_play_sys.h) and reports the flow's progress to a callback, with an event of
// DWC_UTIL_MOVE_EVENT_*.
// The ROM does not embed this source file's name; dwc_util_move.c is descriptive.

enum {
    // A console downloaded the program, whose player func_ov182_021ae240 returns
    DWC_UTIL_MOVE_EVENT_ENTERED,
    DWC_UTIL_MOVE_EVENT_CHILD_LEFT,
    DWC_UTIL_MOVE_EVENT_DONE,
    DWC_UTIL_MOVE_EVENT_ENDED,
    DWC_UTIL_MOVE_EVENT_FAILED,
};

typedef void (*DWCUtilMoveCallback)(int event);

void func_ov182_021ae134(DWCUtilMoveCallback callback);
// Ends the move; func_ov182_021ae210 says when it has ended
void func_ov182_021ae1d4(void);
BOOL func_ov182_021ae210(void);
void func_ov182_021ae224(DWCUtilMoveCallback callback);
void func_ov182_021ae238(void);
const void *func_ov182_021ae240(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MOVE_H
