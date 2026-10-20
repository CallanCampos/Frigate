#ifndef POKEBW2_DWCUTIL_DWC_UTIL_AP_SEARCH_H
#define POKEBW2_DWCUTIL_DWC_UTIL_AP_SEARCH_H

#include "types.h"

// The connection utility's search for access points: it scans for 300 frames and goes on to the list of the access
// points it found (dwc_util_page_ap_list.h), or shows a dialog when it found none, or only ones with a security that
// the utility can't set; B or the button end it and go back to the choice of the setup of a connection.
// The ROM does not embed this source file's name; dwc_util_ap_search.c is descriptive.
void func_ov182_021b5000(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_AP_SEARCH_H
