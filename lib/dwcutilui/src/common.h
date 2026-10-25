#ifndef POKEBW2_DWCUTIL_COMMON_H
#define POKEBW2_DWCUTIL_COMMON_H

#include "types.h"

// The connection utility's localized resources: a path whose sixth character is the language's letter, the upload of
// a file to VRAM, the row of buttons of the screen's layout, and the text of the main screen's message box.
// The file is the DWC utility library's common.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.

// The path with the language's letter in place of its sixth character, unless that is 'x', or 'y' and the language
// isn't 0
char *DWCi_Char(const char *path);
// Reads the file, localized, and hands it to upload, a GX_Load function
void DWCUtilGfxResource_Load(const char *path, void (*upload)(const void *, u32, u32));
// Shows the buttons of the layout set through DWCi_SetParam_
void DWCi_CMNlStep(int variant);
void DWCi_CMNlButton(int variant);
// Draws the message into the main screen's message box, alone or under a string
void DWCi_CMNlMsgDraw(int msgIndex);
void DWCi_CMNlMsgNameDraw(const void *str, int msgIndex);
// The text flags of the utility's language
u32 DWCi_CMNlFontFlag(void);

#endif // POKEBW2_DWCUTIL_COMMON_H
