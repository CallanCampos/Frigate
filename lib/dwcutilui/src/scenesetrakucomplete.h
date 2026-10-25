#ifndef POKEBW2_DWCUTIL_SCENESETRAKUCOMPLETE_H
#define POKEBW2_DWCUTIL_SCENESETRAKUCOMPLETE_H

#include "types.h"

// The connection utility's message after the connection test of rakuraku.h connected, which stays for 120
// frames, before the message of the connection test with auto-connect (as scenesetaosscomplete.h's after an AOSS
// setup).
// The file is the DWC utility library's scenesetrakucomplete.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilTestDone_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETRAKUCOMPLETE_H
