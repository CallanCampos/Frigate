#ifndef POKEBW2_DWC_DWC_NONPORT_H
#define POKEBW2_DWC_DWC_NONPORT_H

#include "types.h"

// DWC's functions that differ by console (overlay 11's dwc_nonport.c), which the login gives GP; the SDK's names. The
// header isn't dwc/nonport.h, which is GameSpy's.

// The console's ID (always 0 on the DS) and name ("NDS")
u64 DWCi_Np_GetConsoleId(void);
const char *DWCi_Np_GetConsoleName(void);
// The game's code from the ROM's header, as a text
char *DWCi_Np_GetAppGameName(void);

#endif // POKEBW2_DWC_DWC_NONPORT_H
