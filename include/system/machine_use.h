#ifndef POKEBW2_SYSTEM_MACHINE_USE_H
#define POKEBW2_SYSTEM_MACHINE_USE_H

#include "types.h"

// The game's setup of the console and the SDK (machine_use.c, a guessed name). SystemInit and SystemUpdateVBlank are
// names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

void SystemInit(void);
// Exits the application once the DSi asked for it and nothing blocks the reset
void Machine_ProcessExitRequest(void);
// Does the VRAM transfers queued during the frame
void SystemUpdateVBlank(void);
// The game's CTRDG_Init, which replaces the SDK's weak one, is declared in nitro/ctrdg.h

#endif // POKEBW2_SYSTEM_MACHINE_USE_H
