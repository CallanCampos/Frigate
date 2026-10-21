#ifndef POKEBW2_DWCUTIL_DWC_UTIL_KEYBOARD_H
#define POKEBW2_DWCUTIL_DWC_UTIL_KEYBOARD_H

#include "types.h"

// The connection utility's keyboard: four rows of character keys in three modes (lower case, shifted and capitals),
// two keys that switch the mode, a space and a backspace key, and two buttons under them, all chosen with the touch
// panel or the pad. The rows slide up into place when it opens and back down when it closes.
// The ROM does not embed this source file's name; dwc_util_keyboard.c is descriptive.
void DWCUtilKeyboard_Open(void);
// Slides the keyboard down and frees it
void DWCUtilKeyboard_Close(void);
// The code of the key entered this frame: a character, 0x80 for the backspace, the code of one of the two buttons,
// or 0 if none
u8 DWCUtilKeyboard_GetKey(void);
// Turns the backspace key, and the character and space keys, on or off
void DWCUtilKeyboard_SetBackspaceEnabled(u8 enabled);
void DWCUtilKeyboard_SetCharsEnabled(u8 enabled);
// Whether the keyboard is still open
BOOL DWCUtilKeyboard_IsOpen(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_KEYBOARD_H
