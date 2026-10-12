#ifndef POKEBW2_SYSTEM_GFL_USE_H
#define POKEBW2_SYSTEM_GFL_USE_H

#include "types.h"

// The game's setup of the GFL library (gfl_use.c): the heaps, the archive paths, the HBlank and VBlank tasks and the
// linear congruential random numbers. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). The
// tasks are declared in gfl/tcb.h, the random numbers in gfl/random.h, the archive paths in gfl/arc.h and the lid's
// volume callbacks in gfl/sound.h

void GFLAppInit(void);
void GFLAppUpdate(void);
// Does nothing, at the end of the main loop's updates
void func_02005668(void);
// Runs the HBlank tasks, from the HBlank interrupt
void GFL_HBlankTCBUpdate(void);
// Runs the VBlank callback, from the VBlank interrupt
void GFL_VBlankExecCallback(void);
// Runs the VBlank tasks and the library's VBlank updates, from the main loop after the VBlank
void GFL_VBlankUpdate(void);
// Silences the sound and restores it, as the callbacks of closing and opening the lid
void GFL_SndMute(void *work);
void GFL_SndSetMaxVol(void *work);

// Declarations that belong to headers gfl_use.c does not own, to be moved there:
// nitro/os.h. NitroSDK's OS_CreateHeap for the whole arena, which only works on a DS
int mem_bind_static_ram_alloc(int arena);

#endif // POKEBW2_SYSTEM_GFL_USE_H
