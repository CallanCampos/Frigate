#ifndef POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MOVE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MOVE_H

#include "types.h"

// The connection utility's question, once a console has downloaded the move's program, whether to send it the
// settings: the screen shows the player's name, and yes goes on with the move (dwc_util_top_menu.h), no ends it. If
// the move reports an event meanwhile, it ends, and the move's notice follows (dwc_util_notice_screen.h).
// The ROM does not embed this source file's name; dwc_util_page_move.c is descriptive.
void func_ov182_021b0198(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_PAGE_MOVE_H
