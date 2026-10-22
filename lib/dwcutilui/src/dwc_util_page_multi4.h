#ifndef POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI4_H
#define POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI4_H

#include "types.h"
#include "dwc_util_auto_connect_intro.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_text.h"

// The connection utility's message after a successful AOSS setup, which stays for 120 frames, before the message of
// the connection test.
// The ROM does not embed this source file's name; dwc_util_page_multi4.c is named after its screen, char/xb4Multi.
void DWCUtilPageMulti4_Init(void);

// The end of the message once both screens have faded out: it goes on to the message of the connection test.
// dwc_util_test_done.c has the same function, endScene1
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

#endif // POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI4_H
