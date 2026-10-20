#ifndef POKEBW2_DWCUTIL_DWC_UTIL_PAGE_STEP3_H
#define POKEBW2_DWCUTIL_DWC_UTIL_PAGE_STEP3_H

#include "types.h"

// The connection utility's AOSS screen (dwc_util_aoss_session.h): it runs the setup, which B or closing the lid
// cancels, and after a success keeps its message for 1080 frames, then saves the keys and goes on to the connection
// test (dwc_util_page_multi4.h). A failure shows a dialog, and the utility goes back to the choice of the setup.
// The ROM does not embed this source file's name; dwc_util_page_step3.c is named after its screen's characters,
// char/ybBgStep3.
void func_ov182_021b06f4(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_PAGE_STEP3_H
