#ifndef POKEBW2_DWCUTIL_DWC_UTIL_BG_BUFFER_H
#define POKEBW2_DWCUTIL_DWC_UTIL_BG_BUFFER_H

#include "types.h"

// The connection utility's copy of a screen of BG 2 of the main screen, which a task loads at the next VBlank once it
// is marked (DWCi_SCREENlRenew), and its loads of BG palettes of the main screen at the next VBlank.
// The file is the DWC utility library's screen.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it; it keeps a name of ours, since dsd takes each file name
// once and lib/dwc/src/gscommon/screen.c, GameSpy's has screen.c.
// Allocates the copy from a screen, and frees it
void DWCi_SCREENlInit(const void *screen);
void DWCi_SCREENlEnd(void);
void DWCi_SCREENlRenew(void);
// Copies rows of a screen of 32 entries a row into the copy, from an entry of the copy on
void DWCi_SCREENlCopy(const u16 *src, int offset, int width, int rows);
// Copies row src of the palette to BG palette row dest of the main screen, and the whole palette as the main screen's
// BG palettes
void DWCi_SCREENlCopyPalette(const void *palette, int src, int dest);
void DWCUtilBgBuffer_LoadPalette(const void *palette);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_BG_BUFFER_H
