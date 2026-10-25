#ifndef POKEBW2_DWCUTIL_SCENEOPTMOVDISCONNECT_H
#define POKEBW2_DWCUTIL_SCENEOPTMOVDISCONNECT_H

#include "types.h"

// A notice of the connection utility that closes by itself: once the move of the settings has ended, the message stays
// for 120 frames, and the utility goes back to the options.
// The file is the DWC utility library's sceneoptmovdisconnect.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilTimedNotice_Init(void);

#endif // POKEBW2_DWCUTIL_SCENEOPTMOVDISCONNECT_H
