#ifndef POKEBW2_DWCUTIL_DWC_UTIL_INFO_MENU_H
#define POKEBW2_DWCUTIL_DWC_UTIL_INFO_MENU_H

#include "types.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"

// Two screens of the connection utility's options: the erasing of the Wi-Fi settings, which asks for a confirmation,
// erases the connections and the Wi-Fi ID and then turns the console off, and the console's information, its MAC
// address and its Wi-Fi ID.
// The ROM does not embed this source file's name; dwc_util_info_menu.c is descriptive.
void func_ov182_021aeafc(void);
void func_ov182_021aef34(void);

// The keys of a screen with two buttons: A presses the first button, B the second, once they are idle.
// dwc_util_prompt_yes_no.c, dwc_util_notice_screen.c and dwc_util_usb_intro.c have the same function
static inline void func_ov182_021aec10Inline(void) {
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    }
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

#endif // POKEBW2_DWCUTIL_DWC_UTIL_INFO_MENU_H
