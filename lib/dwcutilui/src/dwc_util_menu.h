#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MENU_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MENU_H

#include "types.h"

// The connection utility's numeric keypad: ten digit keys, two arrow keys and two buttons under them, which slide up
// into place row by row and down again when it closes. A key chosen by touch, by the D-pad and A, or by B, is read
// back as its code (the digit's character, 0x10 and 0x11 for the arrows, 0x12 and 0x13 for the buttons); 0x1f until
// the keypad is up. The digits, the left arrow and the right arrow can each be disabled, which makes them buzz.
// The ROM does not embed this source file's name; dwc_util_menu.c is descriptive.
void DWCUtilMenu_Open(void);
// Slides the keypad down and frees it
void DWCUtilMenu_Close(void);
u8 DWCUtilMenu_GetKey(void);
void DWCUtilMenu_SetLeftEnabled(u8 enabled);
void DWCUtilMenu_SetDigitsEnabled(u8 enabled);
void DWCUtilMenu_SetRightEnabled(u8 enabled);
// Whether the keypad exists, until it has slid down
BOOL DWCUtilMenu_IsOpen(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MENU_H
