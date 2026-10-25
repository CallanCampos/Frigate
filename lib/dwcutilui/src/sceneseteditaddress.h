#ifndef POKEBW2_DWCUTIL_SCENESETEDITADDRESS_H
#define POKEBW2_DWCUTIL_SCENESETEDITADDRESS_H

#include "types.h"

// The connection utility's entry of an address of the connection being edited (its IP address, subnet mask,
// gateway or one of its two DNS servers) with the numeric keypad (keyboard10.h), as four numbers of up to three
// digits. A confirmation takes the address into the connection, and the utility goes back to its settings.
// The file is the DWC utility library's sceneseteditaddress.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilAddressEdit_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETEDITADDRESS_H
