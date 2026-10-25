#ifndef POKEBW2_DWCUTIL_SCENESETEDIT32_H
#define POKEBW2_DWCUTIL_SCENESETEDIT32_H

#include "types.h"

// The connection utility's entry of an SSID or a WEP key of the connection being edited, with the keyboard
// (keyboard.h), from the connection's settings or from the list of access points. A WEP key is 5, 13 or 16
// characters, or 10, 26 or 32 hexadecimal digits; a confirmation takes the text into the connection.
// The file is the DWC utility library's scenesetedit32.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetEdit32(void);

#endif // POKEBW2_DWCUTIL_SCENESETEDIT32_H
