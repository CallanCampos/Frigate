#ifndef POKEBW2_DWCUTIL_DWC_UTIL_ADDRESS_EDIT_H
#define POKEBW2_DWCUTIL_DWC_UTIL_ADDRESS_EDIT_H

#include "types.h"

// The connection utility's entry of an address of the connection being edited (its IP address, subnet mask,
// gateway or one of its two DNS servers) with the numeric keypad (dwc_util_menu.h), as four numbers of up to three
// digits. A confirmation takes the address into the connection, and the utility goes back to its settings.
// The ROM does not embed this source file's name; dwc_util_address_edit.c is descriptive.
void DWCUtilAddressEdit_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_ADDRESS_EDIT_H
