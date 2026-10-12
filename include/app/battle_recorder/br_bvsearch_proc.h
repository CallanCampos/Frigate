#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_BVSEARCH_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_BVSEARCH_PROC_H

#include "types.h"
#include "gfl/proc.h"

// The Battle Recorder's battle video search screen (br_bvsearch_proc.c, overlay 268), which br_core.c runs as
// BR_PROCID_BV_SEARCH with a BrBvSearchProcParam. The name is a guess: the ROM gives none

extern const GameProcFunctions BR_BVSEARCH_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_BVSEARCH_PROC_H
