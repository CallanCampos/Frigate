#ifndef POKEBW2_SYSTEM_MACHINE_USE_H
#define POKEBW2_SYSTEM_MACHINE_USE_H

#include "types.h"

// The game's setup of the console and the SDK (machine_use.c, a guessed name). SystemInit and SystemUpdateVBlank are
// names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

void SystemInit(void);
// Exits the application once the DSi asked for it and nothing blocks the reset
void func_02005430(void);
// Does the VRAM transfers queued during the frame
void SystemUpdateVBlank(void);
// CTRDG_Init, the game's CTRDG_Init, is in nitro/ctrdg.h

#endif // POKEBW2_SYSTEM_MACHINE_USE_H
