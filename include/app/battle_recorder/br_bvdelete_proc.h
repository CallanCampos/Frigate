#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_BVDELETE_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_BVDELETE_PROC_H

#include "types.h"
#include "app/battle_recorder/br_core.h"
#include "gfl/proc.h"

// The Battle Recorder's battle video deletion screen (br_bvdelete_proc.c, overlay 268), which br_core.c runs as
// BR_PROCID_BV_DELETE with a BrBvDeleteProcParam. The name is a guess: the ROM gives none

// The number of saved slots, which the screen's mode is one of. The name is the ROM's, from an assert of the screen
#define BR_BVDELETE_MODE_MAX BR_RECORD_NUM

extern const GameProcFunctions BR_BVDELETE_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_BVDELETE_PROC_H
