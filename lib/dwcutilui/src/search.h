#ifndef POKEBW2_DWCUTIL_SEARCH_H
#define POKEBW2_DWCUTIL_SEARCH_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's scan for access points, which keeps the first 20 that it finds.
// The file is the DWC utility library's search.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.

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
void DWCi_SEARCHlInit(void);
BOOL DWCi_SEARCHlBegin(void);
BOOL DWCi_SEARCHlStop(void);
// Waits for the wireless to end and frees the scan
void DWCi_SEARCHlEnd(void);
// The access points found, and how many of them there are
int DWCi_SEARCHlGetApInfo(DWCUtilApEntry **entries);

#endif // POKEBW2_DWCUTIL_SEARCH_H
