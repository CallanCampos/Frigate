#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_BVSAVE_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_BVSAVE_PROC_H

#include "types.h"
#include "gfl/proc.h"

// The Battle Recorder's battle video saving screen (br_bvsave_proc.c, overlay 268), which br_core.c runs as
// BR_PROCID_BV_SAVE with a BrBvSaveProcParam. The name is a guess: the ROM gives none

extern const GameProcFunctions BR_BVSAVE_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_BVSAVE_PROC_H
