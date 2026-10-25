#ifndef POKEBW2_DWCUTIL_SCENESETRAKUBUTTON_H
#define POKEBW2_DWCUTIL_SCENESETRAKUBUTTON_H

#include "types.h"

// The connection utility's connection test after its first step (rakuraku.h): it waits for the second
// step and goes on to the wait for the connection (scenesetrakubutton1.h). A failure shows a dialog, and B, closing
// the lid or the button end the test and go back to the choice of the setup of a connection.
// The file is the DWC utility library's scenesetrakubutton.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilTestProgress_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETRAKUBUTTON_H
