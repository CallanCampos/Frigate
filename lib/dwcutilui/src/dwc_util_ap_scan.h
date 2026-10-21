#ifndef POKEBW2_DWCUTIL_DWC_UTIL_AP_SCAN_H
#define POKEBW2_DWCUTIL_DWC_UTIL_AP_SCAN_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's scan for access points, which keeps the first 20 that it finds.
// The ROM does not embed this source file's name; dwc_util_ap_scan.c is descriptive.

// The security of an access point that the scan found
enum {
    DWC_UTIL_AP_OPEN,
    DWC_UTIL_AP_WEP,
    DWC_UTIL_AP_WPA, // one that the utility can't set
};

// An access point that the scan found
struct DWCUtilApEntry {
    u8 ssid[32];
    u8 bssid[6];
    u16 linkLevel;
    u8 security;
};

// Allocates the scan, starts it (TRUE when it started), and stops it (TRUE once it has stopped)
void DWCUtilApScan_Init(void);
BOOL DWCUtilApScan_Start(void);
BOOL DWCUtilApScan_Stop(void);
// Waits for the wireless to end and frees the scan
void DWCUtilApScan_Exit(void);
// The access points found, and how many of them there are
int DWCUtilApScan_GetEntries(DWCUtilApEntry **entries);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_AP_SCAN_H
