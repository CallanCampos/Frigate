#ifndef POKEBW2_DWCUTIL_SCENESETLISTSAVE_H
#define POKEBW2_DWCUTIL_SCENESETLISTSAVE_H

#include "types.h"

// The connection utility's save of a connection set up by hand: it saves the connection being edited into the
// console's Wi-Fi settings, shows the message of the save, and goes back to the first screen of the connection
// settings.
// The file is the DWC utility library's scenesetlistsave.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilSaveSettings_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETLISTSAVE_H
