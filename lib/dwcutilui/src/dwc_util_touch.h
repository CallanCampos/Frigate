#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H

#include "types.h"

// The connection utility's input: the keys and the touch panel, sampled once a frame, with a held key or touch
// repeating after 40 frames and then every 7. While the lid is closed, every test reports nothing. A rectangle is
// left, top, right and bottom, as in dwc_util_display.c.
// The ROM does not embed this source file's name; dwc_util_touch.c is descriptive.
void DWCUtilTouch_Init(void);
void DWCUtilTouch_Exit(void);
void DWCUtilTouch_Update(void);
void DWCUtilTouch_UpdateKeys(void);
void DWCUtilTouch_UpdatePanel(void);
// Whether all the keys were pressed, repeated or released this frame
BOOL DWCUtilTouch_IsKeyPressed(u16 keys);
BOOL DWCUtilTouch_IsKeyRepeated(u16 keys);
BOOL DWCUtilTouch_IsKeyReleased(u16 keys);
// Whether the panel is touched inside the rectangle, or the touch started, repeated or ended there this frame
BOOL DWCUtilTouch_IsTouchedIn(const u16 *rect);
BOOL DWCUtilTouch_IsTriggeredIn(const u16 *rect);
BOOL DWCUtilTouch_IsRepeatedIn(const u16 *rect);
BOOL DWCUtilTouch_IsReleasedIn(const u16 *rect);
// DWCUtilTouch_IsTriggeredIn for the rectangle of a position and a size
BOOL DWCUtilTouch_IsTriggeredInArea(const u16 *area);
// Whether the panel is touched, with the point touched, or the last one if it isn't
BOOL DWCUtilTouch_GetPoint(u16 *point);
// Turns the screens off while the lid is closed, and back on when it opens
void DWCUtilTouch_UpdateLid(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TOUCH_H
