#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MAIN_MENU_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MAIN_MENU_H

#include "types.h"

// The connection utility's first screen: it opens the language's message file and loads the two screens' graphics,
// then lets the player choose between the connection settings and the options, by touch or with the pad.
// The ROM does not embed this source file's name; dwc_util_main_menu.c is descriptive.

// Ends the utility once the screens have faded out
void func_ov182_021ae4f4(void);
// The utility's first screen, which loads the graphics and the message file
void func_ov182_021ae574(void);
// The choice between the connection settings and the options
void func_ov182_021ae748(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MAIN_MENU_H
