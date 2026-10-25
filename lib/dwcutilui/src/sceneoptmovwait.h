#ifndef POKEBW2_DWCUTIL_SCENEOPTMOVWAIT_H
#define POKEBW2_DWCUTIL_SCENEOPTMOVWAIT_H

#include "types.h"

// The connection utility's wait for another console during the move of the settings (move.h): once a console
// has downloaded the move's program, the question follows (sceneoptmovuser.h), and if the move reports another
// event, its notice (sceneoptmovfailure.h). B or closing the lid ends the move and goes back to the options. The file
// is the DWC utility library's sceneoptmovwait.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilPageMulti5_Init(void);

#endif // POKEBW2_DWCUTIL_SCENEOPTMOVWAIT_H
