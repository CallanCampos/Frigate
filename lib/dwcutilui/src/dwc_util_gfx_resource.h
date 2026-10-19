#ifndef POKEBW2_DWCUTIL_DWC_UTIL_GFX_RESOURCE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_GFX_RESOURCE_H

#include "types.h"

// The connection utility's localized resources: a path whose sixth character is the language's letter, the upload of
// a file to VRAM, the row of buttons of the screen's layout, and the text of the main screen's message box.
// The ROM does not embed this source file's name; dwc_util_gfx_resource.c is descriptive.

// The path with the language's letter in place of its sixth character, unless that is 'x', or 'y' and the language
// isn't 0
char *func_ov182_021ab63c(const char *path);
// Reads the file, localized, and hands it to upload, a GX_Load function
void func_ov182_021ab67c(const char *path, void (*upload)(const void *, u32, u32));
// Shows the buttons of the layout set through func_ov182_021adfc8
void func_ov182_021ab6a4(int variant);
void func_ov182_021ab6d0(int variant);
// Draws the message into the main screen's message box, alone or under a string
void func_ov182_021ab6f0(int msgIndex);
void func_ov182_021ab738(const void *str, int msgIndex);
// The text flags of the utility's language
u32 func_ov182_021ab7b4(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_GFX_RESOURCE_H
