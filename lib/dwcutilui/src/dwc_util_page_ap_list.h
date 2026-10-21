#ifndef POKEBW2_DWCUTIL_DWC_UTIL_PAGE_AP_LIST_H
#define POKEBW2_DWCUTIL_DWC_UTIL_PAGE_AP_LIST_H

#include "types.h"

// The connection utility's list of the access points that the scan found (dwc_util_ap_scan.c), four rows on the
// screen with each one's SSID, security and signal, a scroll bar, and a button above the list. Choosing an access
// point takes its SSID into the connection being edited and goes on to its WEP key, or to the connection test when it
// has no security; an access point with a security the utility can't set shows a dialog.
// The ROM does not embed this source file's name; dwc_util_page_ap_list.c is named after its screen, char/jb4ApList.
void DWCUtilPageApList_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_PAGE_AP_LIST_H
