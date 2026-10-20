#ifndef POKEBW2_DWCUTIL_DWC_UTIL_AOSS_SESSION_H
#define POKEBW2_DWCUTIL_DWC_UTIL_AOSS_SESSION_H

#include "types.h"

// The connection utility's AOSS setup (dwcutil/dwc_aoss.h): it asks the access point for the keys of its WEP
// networks, and saves them into the connection being edited when the setup succeeded.
// The ROM does not embed this source file's name; dwc_util_aoss_session.c is descriptive.
void DWCUtilAossSession_Init(void);
void DWCUtilAossSession_Exit(BOOL saveKeys);
// Runs the setup: 1 when it succeeded, 0 when it failed in a way that can be tried again, 2 otherwise
int DWCUtilAossSession_Run(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_AOSS_SESSION_H
