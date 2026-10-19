#ifndef POKEBW2_DWCUTIL_DWC_UTIL_MENU_H
#define POKEBW2_DWCUTIL_DWC_UTIL_MENU_H

#include "types.h"

// The connection utility's numeric keypad: ten digit keys, two arrow keys and two buttons under them, which slide up
// into place row by row and down again when it closes. A key chosen by touch, by the D-pad and A, or by B, is read
// back as its code (the digit's character, 0x10 and 0x11 for the arrows, 0x12 and 0x13 for the buttons); 0x1f until
// the keypad is up. The digits, the left arrow and the right arrow can each be disabled, which makes them buzz.
// The ROM does not embed this source file's name; dwc_util_menu.c is descriptive.
void func_ov182_021acd80(void);
// Slides the keypad down and frees it
void func_ov182_021acf7c(void);
u8 func_ov182_021acf9c(void);
void func_ov182_021acfac(u8 enabled);
void func_ov182_021acfbc(u8 enabled);
void func_ov182_021acfcc(u8 enabled);
// Whether the keypad exists, until it has slid down
BOOL func_ov182_021acfdc(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_MENU_H
