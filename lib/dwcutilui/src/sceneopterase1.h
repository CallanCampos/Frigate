#ifndef POKEBW2_DWCUTIL_SCENEOPTERASE1_H
#define POKEBW2_DWCUTIL_SCENEOPTERASE1_H

#include "types.h"

// The connection utility's erasing of the Wi-Fi settings, once confirmed (sceneopterase.h): it erases the connections
// and the Wi-Fi ID and then turns the console off. The file is the DWC utility library's sceneopterase1.o
// (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that library's objects (layout/symbols_sdk.txt)
// shows it.
void DWCUtilInfoMenu_EraseSettings(void);

#endif // POKEBW2_DWCUTIL_SCENEOPTERASE1_H
