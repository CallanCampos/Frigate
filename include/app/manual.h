#ifndef POKEBW2_APP_MANUAL_H
#define POKEBW2_APP_MANUAL_H

#include "types.h"
#include "constants/arc.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The in-game manual, the menu's Help (overlay 319). The ROM names its nine other files; "manual.c" is a guess for
// the process's own. It runs with a GameManualSubwork of field/event_game_manual.h, and sets its result

#define OVERLAY_MANUAL OVERLAY_ID(319)

// Not from swan: the screen's heap, a child of HEAPID_USER
#define HEAPID_MANUAL 0x90

// Not from swan: the upper screen's pictures (a/2/2/7); the manual's graphics and tables are ARCID_MANUAL
#define ARCID_MANUAL_IMAGE 227

// The members of ARCID_MANUAL
enum {
    MANUAL_ARC_BG_NCLR,
    MANUAL_ARC_OBJ_NCLR,
    MANUAL_ARC_BG_NCGR,
    MANUAL_ARC_OBJ_NCGR,
    MANUAL_ARC_TOP_NSCR,
    MANUAL_ARC_LIST_NSCR,
    MANUAL_ARC_EXPLAIN_NSCR,
    MANUAL_ARC_OBJ_NCER,
    MANUAL_ARC_OBJ_NANR,
    MANUAL_ARC_CATEGORY_DATA,
    MANUAL_ARC_CATEGORY_OFFSETS,
    MANUAL_ARC_TITLE_DATA,
    MANUAL_ARC_TITLE_OFFSETS,
};

// What the manual ends with: the menu closed, or back to the menu
enum {
    MANUAL_RESULT_CLOSE,
    MANUAL_RESULT_RETURN,
};

extern const GameProcFunctions gManualProcFunctions;

#endif // POKEBW2_APP_MANUAL_H
