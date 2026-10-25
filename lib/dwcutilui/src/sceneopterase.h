#ifndef POKEBW2_DWCUTIL_SCENEOPTERASE_H
#define POKEBW2_DWCUTIL_SCENEOPTERASE_H

#include "types.h"
#include "button.h"
#include "input.h"

// The connection utility's option that erases the Wi-Fi settings: it asks for a confirmation, then goes on to the
// erasing (sceneopterase1.h). The file is the DWC utility library's sceneopterase.o (libdwcutil.TWL.HYB.a), as
// aletheia's map of Black 1's SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilInfoMenu_InitErase(void);

// The keys of a screen with two buttons: A presses the first button, B the second, once they are idle.
// sceneoptmovconfirm1.c, sceneoptmovfailure.c and scenesetusbready.c have the same function
static inline void DWCUtilInfoMenu_EraseHandleInputInline(void) {
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(1);
    }
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    }
}

#endif // POKEBW2_DWCUTIL_SCENEOPTERASE_H
