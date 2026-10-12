#ifndef POKEBW2_APP_STAFF_ROLL_H
#define POKEBW2_APP_STAFF_ROLL_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The staff roll, overlay 266 (staff_roll_main.c): the credits that the game clear event shows. The names are ours

#define OVERLAY_STAFF_ROLL OVERLAY_ID(266)

// Not from swan: the staff roll's heap
#define HEAPID_STAFF_ROLL 0x8e

typedef struct {
    // TRUE once the game was cleared before: START ends the roll, and it shows no pictures
    BOOL skippable;
    u32 unk04;
    // Its gender picks the pictures
    PlayerInfo *playerInfo;
} StaffRollParam;

extern const GameProcFunctions STAFF_ROLL_PROC_FUNCTIONS;

#endif // POKEBW2_APP_STAFF_ROLL_H
