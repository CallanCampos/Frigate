#ifndef POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI5_H
#define POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI5_H

#include "types.h"

// The connection utility's wait for another console during the move of the settings (dwc_util_move.h): once a console
// has downloaded the move's program, the question follows (dwc_util_page_move.h), and if the move reports another
// event, its notice (dwc_util_notice_screen.h). B or closing the lid ends the move and goes back to the options. The
// ROM does not embed this source file's name; dwc_util_page_multi5.c is named after its screen, char/yb5Multi.
void func_ov182_021b043c(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MULTI5_H
