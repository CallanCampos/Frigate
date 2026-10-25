#ifndef POKEBW2_DWCUTIL_SCENESETUSBREADY1_H
#define POKEBW2_DWCUTIL_SCENESETUSBREADY1_H

#include "types.h"

// The connection utility's setup of the Nintendo Wi-Fi USB Connector: it shows the console's nickname and searches for
// the connector (usbap.h) until the connector takes the console, which goes on to the message of
// scenesetusbcomplete.h. When none is found, it goes back to the screen before (scenesetusbready.h), and B, or closing
// the lid, back to the first screen of the connection settings.
// The file is the DWC utility library's scenesetusbready1.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetUsbReady1(void);

#endif // POKEBW2_DWCUTIL_SCENESETUSBREADY1_H
