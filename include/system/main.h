#ifndef POKEBW2_SYSTEM_MAIN_H
#define POKEBW2_SYSTEM_MAIN_H

#include "types.h"
#include "nitro/os.h"
// For Machine_ProcessExitRequest, which this header declared before machine_use.c was told apart from main.c
#include "system/machine_use.h"

// The game's main file, with main, GameInit and GameUpdate. main is a name from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

void main(void);
// Keeps the main loop from giving the ARM9 priority on the main memory while it is set
void SetMainMemoryPriorityBlocked(BOOL blocked);

// Declarations that belong to headers main.c does not own, to be moved there:
// gfl/sound.h. The sound thread of snd_sys.c
extern OSThread g_GFLSndThread;

#endif // POKEBW2_SYSTEM_MAIN_H
