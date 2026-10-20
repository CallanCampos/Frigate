#ifndef POKEBW2_DWCUTIL_DWC_UTIL_INFO_MENU_H
#define POKEBW2_DWCUTIL_DWC_UTIL_INFO_MENU_H

#include "types.h"

// Two screens of the connection utility's options: the erasing of the Wi-Fi settings, which asks for a confirmation,
// erases the connections and the Wi-Fi ID and then turns the console off, and the console's information, its MAC
// address and its Wi-Fi ID.
// The ROM does not embed this source file's name; dwc_util_info_menu.c is descriptive.
void func_ov182_021aeafc(void);
void func_ov182_021aef34(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_INFO_MENU_H
