#ifndef POKEBW2_DWCUTIL_SUB_H
#define POKEBW2_DWCUTIL_SUB_H

#include "types.h"

// The connection utility's VRAM: it takes every bank from the game, but the ARM7's, clears VRAM, OAM and the
// palettes, and gives the banks back when it ends. And two settings of the ARM7 (nitro/pm.h) that it changes.
// The file is the DWC utility library's sub.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_ClearVram(void);
void DWCi_RepairVram(void);
void clearVram(void);
void DWCi_SetLedWireless(void);
void DWCi_SetLedNormal(void);

#endif // POKEBW2_DWCUTIL_SUB_H
