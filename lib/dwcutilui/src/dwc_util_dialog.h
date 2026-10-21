#ifndef POKEBW2_DWCUTIL_DWC_UTIL_DIALOG_H
#define POKEBW2_DWCUTIL_DWC_UTIL_DIALOG_H

#include "types.h"

// The connection utility's dialog over the main screen: a frame with a message (as DWCUtilScreen_Start shows it) and
// the buttons of one of six layouts, which slides up from the bottom of the screen, with the screen darkened behind it
// when fade is TRUE, and slides down again once it is closed. Layout 5 has no buttons and closes after 120 frames.
// The ROM does not embed this source file's name; dwc_util_dialog.c is descriptive.
void DWCUtilDialog_Open(int msgIndex, int layout, BOOL fade, int slot, int value);
// Closes the dialog
void DWCUtilDialog_Close(void);
// The button chosen, -2 until the dialog is up and -1 until one is
int DWCUtilDialog_GetChoice(void);
// TRUE until the dialog has closed
BOOL DWCUtilDialog_IsOpen(void);

// The offset of the dialog's text in its frame, which nothing reads
extern const u16 gDialogTextOffset[2];

#endif // POKEBW2_DWCUTIL_DWC_UTIL_DIALOG_H
