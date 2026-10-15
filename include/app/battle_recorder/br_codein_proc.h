#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_PROC_H

#include "types.h"
#include "gfl/proc.h"

// The Battle Recorder's screen for entering a battle video's number (br_codein_proc.c, overlay 268), which br_core.c
// runs as BR_PROCID_CODEIN with a BrCodeInProcParam. The name is a guess: the ROM gives none

extern const GameProcFunctions BR_CODEIN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_PROC_H
