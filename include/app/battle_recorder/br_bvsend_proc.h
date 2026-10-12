#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_BVSEND_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_BVSEND_PROC_H

#include "types.h"
#include "gfl/proc.h"

// The Battle Recorder's battle video upload screen (br_bvsend_proc.c, overlay 268), which br_core.c runs as
// BR_PROCID_BV_SEND with a BrBvSendProcParam. The name is a guess: the ROM gives none

extern const GameProcFunctions BR_BVSEND_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_BVSEND_PROC_H
