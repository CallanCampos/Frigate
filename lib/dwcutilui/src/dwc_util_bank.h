#ifndef POKEBW2_DWCUTIL_DWC_UTIL_BANK_H
#define POKEBW2_DWCUTIL_DWC_UTIL_BANK_H

#include "types.h"

// The connection utility's VRAM: it takes every bank from the game, but the ARM7's, clears VRAM, OAM and the
// palettes, and gives the banks back when it ends. And two settings of the ARM7 (nitro/pm.h) that it changes.
// The ROM does not embed this source file's name; dwc_util_bank.c is descriptive.
void DWCi_ClearVram(void);
void DWCUtilBank_Exit(void);
void clearVram(void);
void DWCi_SetLedWireless(void);
void DWCi_SetLedNormal(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_BANK_H
