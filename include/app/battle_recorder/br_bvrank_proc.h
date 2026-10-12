#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_BVRANK_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_BVRANK_PROC_H

#include "types.h"
#include "gfl/proc.h"

// The Battle Recorder's battle video list screen (br_bvrank_proc.c, overlay 268), which br_core.c runs as
// BR_PROCID_BV_RANK with a BrBvRankProcParam. The name is a guess: the ROM gives none

// The list the screen downloads from the Global Link, in its mode: one of the server's three lists, which the menu
// chooses, or the videos that the search screen asked for
enum {
    BR_BVRANK_MODE_LIST1,
    BR_BVRANK_MODE_LIST3,
    BR_BVRANK_MODE_LIST2,
    BR_BVRANK_MODE_SEARCH,
};

extern const GameProcFunctions BR_BVRANK_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_BVRANK_PROC_H
