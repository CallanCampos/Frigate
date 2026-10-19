#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H

#include "types.h"

// The connection utility's input: the keys and the touch panel, sampled once a frame, with a held key or touch
// repeating after 40 frames and then every 7. While the lid is closed, every test reports nothing. A rectangle is
// left, top, right and bottom, as in dwc_util_display.c.
// The ROM does not embed this source file's name; dwc_util_touch.c is descriptive.
void func_ov182_021bc544(void);
void func_ov182_021bc598(void);
void func_ov182_021bc5bc(void);
void func_ov182_021bc5c8(void);
void func_ov182_021bc670(void);
// Whether all the keys were pressed, repeated or released this frame
BOOL func_ov182_021bc798(u16 keys);
BOOL func_ov182_021bc7c8(u16 keys);
BOOL func_ov182_021bc7f8(u16 keys);
// Whether the panel is touched inside the rectangle, or the touch started, repeated or ended there this frame
BOOL func_ov182_021bc828(const u16 *rect);
BOOL func_ov182_021bc888(const u16 *rect);
BOOL func_ov182_021bc8e8(const u16 *rect);
BOOL func_ov182_021bc948(const u16 *rect);
// func_ov182_021bc888 for the rectangle of a position and a size
BOOL func_ov182_021bc9a8(const u16 *area);
// Whether the panel is touched, with the point touched, or the last one if it isn't
BOOL func_ov182_021bc9e4(u16 *point);
// Turns the screens off while the lid is closed, and back on when it opens
void func_ov182_021bca28(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H
