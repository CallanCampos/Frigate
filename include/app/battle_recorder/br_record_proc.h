#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_RECORD_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_RECORD_PROC_H

#include "types.h"
#include "app/battle_recorder/br_core.h"
#include "gfl/proc.h"

// The Battle Recorder's battle video screen (br_record_proc.c, overlay 268), which br_core.c runs as BR_PROCID_RECORD
// with a BrRecordProcParam. The name is a guess: the ROM gives none

// The ROM's name for the last of the saved slots, BR_RECORD_MODE_OTHER3, which an assert of the screen keeps
#define BR_RECODE_PROC_OTHER_02 BR_RECORD_MODE_OTHER3

extern const GameProcFunctions BR_RECORD_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_RECORD_PROC_H
