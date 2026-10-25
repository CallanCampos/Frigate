#ifndef POKEBW2_DWCUTIL_WINDOW_H
#define POKEBW2_DWCUTIL_WINDOW_H

#include "types.h"

// The connection utility's dialog over the main screen: a frame with a message (as DWCi_COMMENTlDispEx shows it) and
// the buttons of one of six layouts, which slides up from the bottom of the screen, with the screen darkened behind it
// when fade is TRUE, and slides down again once it is closed. Layout 5 has no buttons and closes after 120 frames.
// The file is the DWC utility library's window.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilDialog_Open(int msgIndex, int layout, BOOL fade, int slot, int value);
// Closes the dialog
void DWCi_WINlEnd(void);
// The button chosen, -2 until the dialog is up and -1 until one is
int DWCi_WINlGet(void);
// TRUE until the dialog has closed
BOOL DWCi_WINlIsInit(void);

// The offset of the dialog's text in its frame, which nothing reads
extern const u16 gDialogTextOffset[2];

#endif // POKEBW2_DWCUTIL_WINDOW_H
