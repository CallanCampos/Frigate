#ifndef POKEBW2_DWCUTIL_DWC_UTIL_BANK_H
#define POKEBW2_DWCUTIL_DWC_UTIL_BANK_H

#include "types.h"

// The connection utility's VRAM: it takes every bank from the game, but the ARM7's, clears VRAM, OAM and the
// palettes, and gives the banks back when it ends. And two settings of the ARM7 (nitro/pm.h) that it changes.
// The ROM does not embed this source file's name; dwc_util_bank.c is descriptive.
void func_ov182_021bcf38(void);
void func_ov182_021bcf9c(void);
void func_ov182_021bd040(void);
void func_ov182_021bd0ac(void);
void func_ov182_021bd0c8(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_BANK_H
