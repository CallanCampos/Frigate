#ifndef POKEBW2_DWCUTIL_SCENEOPTMOVCOMPLETE_H
#define POKEBW2_DWCUTIL_SCENEOPTMOVCOMPLETE_H

#include "types.h"

// The end of a move of the settings that sent them (sceneoptmovcomm.h), which erases them here and turns the console
// off. The file is the DWC utility library's sceneoptmovcomplete.o (libdwcutil.TWL.HYB.a), as aletheia's map of
// Black 1's SDK code to that library's objects (layout/symbols_sdk.txt) shows it (sceneoptmovcompl, its name cut to
// 16 characters, and its scene DWCi_SceneOptMovComplete).
void DWCi_SceneOptMovComplete(void);

#endif // POKEBW2_DWCUTIL_SCENEOPTMOVCOMPLETE_H
