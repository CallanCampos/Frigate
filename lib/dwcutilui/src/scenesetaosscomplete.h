#ifndef POKEBW2_DWCUTIL_SCENESETAOSSCOMPLETE_H
#define POKEBW2_DWCUTIL_SCENESETAOSSCOMPLETE_H

#include "types.h"
#include "comment.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "scenesettestconfirm.h"

// The connection utility's message after a successful AOSS setup, which stays for 120 frames, before the message of
// the connection test.
// The file is the DWC utility library's scenesetaosscomplete.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCUtilPageMulti4_Init(void);

// The end of the message once both screens have faded out: it goes on to the message of the connection test.
// scenesetrakucomplete.c has the same function, endScene1
static inline void DWCUtilPageMulti4_WaitFadeOutInline(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_FNTlDeleteBgLcd(0);
        DWCi_COMMENTlDelete();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 0);
        DWCi_SetParam2_(0, 1);
        DWCi_ChangeScene(DWCUtilAutoConnectIntro_Init);
    }
}

#endif // POKEBW2_DWCUTIL_SCENESETAOSSCOMPLETE_H
