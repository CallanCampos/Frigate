#ifndef POKEBW2_DWCUTIL_SCENEOPTMOVUSER_H
#define POKEBW2_DWCUTIL_SCENEOPTMOVUSER_H

#include "types.h"

// The connection utility's question, once a console has downloaded the move's program, whether to send it the
// settings: the screen shows the player's name, and yes goes on with the move (sceneoptmovcomm.h), no ends it. If
// the move reports an event meanwhile, it ends, and the move's notice follows (sceneoptmovfailure.h).
// The file is the DWC utility library's sceneoptmovuser.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilPageMove_Init(void);

#endif // POKEBW2_DWCUTIL_SCENEOPTMOVUSER_H
