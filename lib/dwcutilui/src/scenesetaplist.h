#ifndef POKEBW2_DWCUTIL_SCENESETAPLIST_H
#define POKEBW2_DWCUTIL_SCENESETAPLIST_H

#include "types.h"

// The connection utility's list of the access points that the scan found (search.c), four rows on the
// screen with each one's SSID, security and signal, a scroll bar, and a button above the list. Choosing an access
// point takes its SSID into the connection being edited and goes on to its WEP key, or to the connection test when it
// has no security; an access point with a security the utility can't set shows a dialog.
// The file is the DWC utility library's scenesetaplist.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK
// code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetApList(void);

#endif // POKEBW2_DWCUTIL_SCENESETAPLIST_H
