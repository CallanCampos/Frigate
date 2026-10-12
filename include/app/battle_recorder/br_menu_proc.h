#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_MENU_PROC_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_MENU_PROC_H

#include "types.h"
#include "gfl/proc.h"

// The Battle Recorder's menu screen (br_menu_proc.c, overlay 268), which br_core.c runs as BR_PROCID_MENU with a
// BrMenuProcParam. The name is a guess: the ROM gives none

extern const GameProcFunctions BR_MENU_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_MENU_PROC_H
