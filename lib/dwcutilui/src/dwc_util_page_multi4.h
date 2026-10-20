#ifndef POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI4_H
#define POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI4_H

#include "types.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_text.h"

// The connection utility's message after a successful AOSS setup, which stays for 120 frames, before the message of
// the connection test.
// The ROM does not embed this source file's name; dwc_util_page_multi4.c is named after its screen, char/xb4Multi.
void func_ov182_021b0ad0(void);

// The end of the message once both screens have faded out: it goes on to the message of the connection test.
// dwc_util_test_done.c has the same function, func_ov182_021b4c58
static inline void func_ov182_021b0bb8Inline(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021bbf7c(0);
        DWCUtilScreen_Stop();
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x15);
        func_ov182_021adfc8(0, 0);
        func_ov182_021adff0(0, 1);
        func_ov182_021adfbc(func_ov182_021b6588);
    }
}

#endif // POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI4_H
