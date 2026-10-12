#ifndef POKEBW2_DEMO_STAFF_ROLL_H
#define POKEBW2_DEMO_STAFF_ROLL_H

// The staff roll, the credits after the Hall of Fame, overlay 266. The ROM names staff_roll_main.c

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_STAFF_ROLL OVERLAY_ID(266)

// The parameter of the process, which the game clear event fills (event_game_clear.c)
typedef struct {
    // Whether START skips the credits; they then show no pictures
    BOOL skippable;
    u32 unk04;
    PlayerInfo *playerInfo;
} StaffRollParam;

extern const GameProcFunctions STAFF_ROLL_PROC_FUNCTIONS;

#endif // POKEBW2_DEMO_STAFF_ROLL_H
