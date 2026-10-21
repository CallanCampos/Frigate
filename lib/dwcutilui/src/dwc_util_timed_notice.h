#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TIMED_NOTICE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TIMED_NOTICE_H

#include "types.h"

// A notice of the connection utility that closes by itself: once the move of the settings has ended, the message stays
// for 120 frames, and the utility goes back to the options.
// The ROM does not embed this source file's name; dwc_util_timed_notice.c is descriptive.
void DWCUtilTimedNotice_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TIMED_NOTICE_H
