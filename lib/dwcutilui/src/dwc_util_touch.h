#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H

#include "types.h"

// The connection utility's input: the keys and the touch panel, sampled once a frame, with a held key or touch
// repeating after 40 frames and then every 7. While the lid is closed, every test reports nothing. A rectangle is
// left, top, right and bottom, as in dwc_util_display.c.
// The ROM does not embed this source file's name; dwc_util_touch.c is descriptive.
void DWCi_IPTlInit(void);
void DWCi_IPTlEnd(void);
void DWCi_IPTlRead(void);
void readKey(void);
void readTouch(void);
// Whether all the keys were pressed, repeated or released this frame
BOOL DWCi_IPTlCheckTrigger(u16 keys);
BOOL DWCi_IPTlCheckRepeat(u16 keys);
BOOL DWCi_IPTlCheckRelease(u16 keys);
// Whether the panel is touched inside the rectangle, or the touch started, repeated or ended there this frame
BOOL DWCi_TPlCheck(const u16 *rect);
BOOL DWCi_TPlCheckTrigger(const u16 *rect);
BOOL DWCi_TPlCheckRepeat(const u16 *rect);
BOOL DWCi_TPlCheckRelease(const u16 *rect);
// DWCi_TPlCheckTrigger for the rectangle of a position and a size
BOOL DWCi_TPlCheckATrigger(const u16 *area);
// Whether the panel is touched, with the point touched, or the last one if it isn't
BOOL DWCi_TPlGet(u16 *point);
// Turns the screens off while the lid is closed, and back on when it opens
void DWCUtilTouch_UpdateLid(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H
