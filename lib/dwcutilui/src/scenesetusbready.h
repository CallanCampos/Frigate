#ifndef POKEBW2_DWCUTIL_SCENESETUSBREADY_H
#define POKEBW2_DWCUTIL_SCENESETUSBREADY_H

#include "types.h"

// The connection utility's screen before the setup of the Nintendo Wi-Fi USB Connector, after the first screen of the
// connection settings' wide button: yes goes on to the setup (scenesetusbready1.h), no back to the first screen.
// The file is the DWC utility library's scenesetusbready.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetUsbReady(void);

#endif // POKEBW2_DWCUTIL_SCENESETUSBREADY_H
