#ifndef POKEBW2_DWCUTIL_SCENESETRAKUREADY_H
#define POKEBW2_DWCUTIL_SCENESETRAKUREADY_H

#include "types.h"

// The connection utility's start of the connection test (rakuraku.h): it starts the test, waits for its
// first step and goes on to the wait for the second (scenesetrakubutton.h). A failure shows a dialog, and B,
// closing the lid or the button end the test and go back to the choice of the setup of a connection.
// The file is the DWC utility library's scenesetrakuready.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilTestStart_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETRAKUREADY_H
