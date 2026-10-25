#ifndef POKEBW2_DWCUTIL_SCENESETUSBCOMPLETE_H
#define POKEBW2_DWCUTIL_SCENESETUSBCOMPLETE_H

#include "types.h"

// The connection utility's message after the setup of the Nintendo Wi-Fi USB Connector (scenesetusbready1.h), which
// stays for 120 frames, before the message of the connection test with auto-connect.
// The file is the DWC utility library's scenesetusbcomplete.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetUsbComplete(void);

#endif // POKEBW2_DWCUTIL_SCENESETUSBCOMPLETE_H
