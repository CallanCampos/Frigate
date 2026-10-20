#ifndef POKEBW2_DWCUTIL_DWC_UTIL_GFX_RESOURCE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_GFX_RESOURCE_H

#include "types.h"

// The connection utility's localized resources: a path whose sixth character is the language's letter, the upload of
// a file to VRAM, the row of buttons of the screen's layout, and the text of the main screen's message box.
// The ROM does not embed this source file's name; dwc_util_gfx_resource.c is descriptive.

// The path with the language's letter in place of its sixth character, unless that is 'x', or 'y' and the language
// isn't 0
char *DWCUtilGfxResource_LocalizePath(const char *path);
// Reads the file, localized, and hands it to upload, a GX_Load function
void DWCUtilGfxResource_Load(const char *path, void (*upload)(const void *, u32, u32));
// Shows the buttons of the layout set through DWCUtilMain_SetLayout
void DWCUtilGfxResource_ShowStep(int variant);
void DWCUtilGfxResource_ShowButtons(int variant);
// Draws the message into the main screen's message box, alone or under a string
void DWCUtilGfxResource_ShowMessage(int msgIndex);
void DWCUtilGfxResource_ShowMessageWithString(const void *str, int msgIndex);
// The text flags of the utility's language
u32 DWCUtilGfxResource_GetTextFlags(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_GFX_RESOURCE_H
