#ifndef POKEBW2_DWCUTIL_DWC_UTIL_OAM_BUF_H
#define POKEBW2_DWCUTIL_DWC_UTIL_OAM_BUF_H

#include "types.h"
#include "nitro/gx.h"

// The connection utility's OAM buffers, one per screen (0 for the main engine's, 1 for the sub engine's), which a
// VBlank task uploads. Entries 0 to 63 of each are handed out one at a time; dwc_util_oam.c runs the rest.
// The ROM does not embed this source file's name; dwc_util_oam_buf.c is descriptive.
void DWCUtilOamBuf_Init(void);
void DWCUtilOamBuf_Exit(void);
// A free entry of the screen's buffer, or NULL
GXOamAttr *DWCUtilOamBuf_Alloc(int screen);
// Hides the entry and frees it
void DWCUtilOamBuf_Free(GXOamAttr *oam);
GXOamAttr *DWCUtilOamBuf_GetEntry(int screen, int index);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_OAM_BUF_H
