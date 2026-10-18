#ifndef POKEBW2_DWCUTIL_DWC_UTIL_WEP_KEYBOARD_H
#define POKEBW2_DWCUTIL_DWC_UTIL_WEP_KEYBOARD_H

#include "types.h"

// The connection utility's entry of an SSID or a WEP key of the connection being edited, with the keyboard
// (dwc_util_keyboard.h), from the connection's settings or from the list of access points. A WEP key is 5, 13 or 16
// characters, or 10, 26 or 32 hexadecimal digits; a confirmation takes the text into the connection.
// The ROM does not embed this source file's name; dwc_util_wep_keyboard.c is descriptive.
void func_ov182_021b19f4(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_WEP_KEYBOARD_H
