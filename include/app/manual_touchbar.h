#ifndef POKEBW2_APP_MANUAL_TOUCHBAR_H
#define POKEBW2_APP_MANUAL_TOUCHBAR_H

// The manual's touch bar (overlay 319), named after the ROM's "manual_touchbar.c": the close and return icons, a
// line of text, and the explanation's page arrows

#include "types.h"
#include "app/manual_common.h"
#include "app/ui/touchbar.h"

// What the bar shows, by the screen it is under
enum {
    MANUAL_TOUCHBAR_MODE_TOP,
    MANUAL_TOUCHBAR_MODE_CATEGORY,
    MANUAL_TOUCHBAR_MODE_TITLE,
    MANUAL_TOUCHBAR_MODE_EXPLAIN,
    MANUAL_TOUCHBAR_MODE_NONE,
};

ManualTouchBar *ManualTouchBar_Create(ManualCommon *common);
void ManualTouchBar_Free(ManualTouchBar *bar);
void ManualTouchBar_Main(ManualTouchBar *bar);
void ManualTouchBar_SetMode(ManualTouchBar *bar, u32 mode);
// Shows the page number and the arrows of MANUAL_TOUCHBAR_MODE_EXPLAIN
void ManualTouchBar_SetPage(ManualTouchBar *bar, u8 pageCount, u8 page);
// The icon whose animation has ended, and the icon being touched, or TOUCHBAR_ICON_NONE: the bar's close and return
// icons, and the explanation's page arrows as TOUCHBAR_ICON_LEFT and TOUCHBAR_ICON_RIGHT
u32 ManualTouchBar_GetDecided(ManualTouchBar *bar);
u32 ManualTouchBar_GetTouched(ManualTouchBar *bar);

#endif // POKEBW2_APP_MANUAL_TOUCHBAR_H
