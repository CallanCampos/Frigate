#ifndef POKEBW2_DWCUTIL_SCENEOPTMENU_H
#define POKEBW2_DWCUTIL_SCENEOPTMENU_H

#include "types.h"

// The connection utility's options: a menu of three buttons, the console's information, the erasing of the Wi-Fi
// settings and the move of the settings to another console. Without a Wi-Fi ID the last two are greyed out.
// The file is the DWC utility library's sceneoptmenu.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code
// to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilOptionMenu_Init(void);

#endif // POKEBW2_DWCUTIL_SCENEOPTMENU_H
