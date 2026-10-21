#ifndef POKEBW2_DWCUTIL_DWC_UTIL_OPTION_MENU_H
#define POKEBW2_DWCUTIL_DWC_UTIL_OPTION_MENU_H

#include "types.h"

// The connection utility's options: a menu of three buttons, the console's information, the erasing of the Wi-Fi
// settings and the move of the settings to another console. Without a Wi-Fi ID the last two are greyed out.
// The ROM does not embed this source file's name; dwc_util_option_menu.c is descriptive.
void DWCUtilOptionMenu_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_OPTION_MENU_H
