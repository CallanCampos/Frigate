#ifndef POKEBW2_DWCUTIL_DWC_UTIL_BG_BUFFER_H
#define POKEBW2_DWCUTIL_DWC_UTIL_BG_BUFFER_H

#include "types.h"

// The connection utility's copy of a screen of BG 2 of the main screen, which a task loads at the next VBlank once it
// is marked (func_ov182_021b6f98), and its loads of BG palettes of the main screen at the next VBlank.
// The ROM does not embed this source file's name; dwc_util_bg_buffer.c is descriptive.
// Allocates the copy from a screen, and frees it
void func_ov182_021b6f38(const void *screen);
void func_ov182_021b6f78(void);
void func_ov182_021b6f98(void);
// Copies rows of a screen of 32 entries a row into the copy, from an entry of the copy on
void func_ov182_021b6fac(const u16 *src, int offset, int width, int rows);
// Copies row src of the palette to BG palette row dest of the main screen, and the whole palette as the main screen's
// BG palettes
void func_ov182_021b7014(const void *palette, int src, int dest);
void func_ov182_021b7060(const void *palette);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_BG_BUFFER_H
