#ifndef POKEBW2_DWCUTIL_LIBAOSS_H
#define POKEBW2_DWCUTIL_LIBAOSS_H

#include "types.h"

// The connection utility's AOSS setup (dwcutil/aoss.h): it asks the access point for the keys of its WEP
// networks, and saves them into the connection being edited when the setup succeeded.
// The file is the DWC utility library's libaoss.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_AOSSlInit(void);
void DWCUtilAossSession_Exit(BOOL saveKeys);
// Runs the setup: 1 when it succeeded, 0 when it failed in a way that can be tried again, 2 otherwise
int DWCUtilAossSession_Run(void);

#endif // POKEBW2_DWCUTIL_LIBAOSS_H
