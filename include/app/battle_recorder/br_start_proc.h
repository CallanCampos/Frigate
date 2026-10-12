#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_START_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_START_PROC_H

#include "types.h"
#include "gfl/proc.h"

// The Battle Recorder's opening and closing screen (br_start_proc.c, overlay 268), which br_core.c runs as
// BR_PROCID_START with a BrStartProcParam. The name is a guess: the ROM gives none

// What the screen does, in BrStartProcParam's mode
enum {
    // The Battle Recorder starts up, then the menu opens
    BR_START_MODE_OPEN,
    // The Battle Recorder closes
    BR_START_MODE_CLOSE,
    // The menu opens at once, after a battle video has played
    BR_START_MODE_RETURN,
};

extern const GameProcFunctions BR_START_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_START_PROC_H
