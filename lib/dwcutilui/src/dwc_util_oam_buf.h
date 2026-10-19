#ifndef POKEBW2_DWCUTIL_DWC_UTIL_OAM_BUF_H
#define POKEBW2_DWCUTIL_DWC_UTIL_OAM_BUF_H

#include "types.h"
#include "nitro/gx.h"

// The connection utility's OAM buffers, one per screen (0 for the main engine's, 1 for the sub engine's), which a
// VBlank task uploads. Entries 0 to 63 of each are handed out one at a time; dwc_util_oam.c runs the rest.
// The ROM does not embed this source file's name; dwc_util_oam_buf.c is descriptive.
void func_ov182_021bcbc8(void);
void func_ov182_021bcc5c(void);
// A free entry of the screen's buffer, or NULL
GXOamAttr *func_ov182_021bcc80(int screen);
// Hides the entry and frees it
void func_ov182_021bcc9c(GXOamAttr *oam);
GXOamAttr *func_ov182_021bccd8(int screen, int index);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_OAM_BUF_H
