#ifndef POKEBW2_DWCUTIL_SCENEOPTMOVCONFIRM1_H
#define POKEBW2_DWCUTIL_SCENEOPTMOVCONFIRM1_H

#include "types.h"

// The connection utility's confirmation before the move of the settings to another console: yes goes on to the
// move's instructions (sceneoptmovconfirm.h), no back to the options.
// The file is the DWC utility library's sceneoptmovconfirm1.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilPromptYesNo_Init(void);

#endif // POKEBW2_DWCUTIL_SCENEOPTMOVCONFIRM1_H
