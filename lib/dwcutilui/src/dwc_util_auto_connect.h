#ifndef POKEBW2_DWCUTIL_DWC_UTIL_AUTO_CONNECT_H
#define POKEBW2_DWCUTIL_DWC_UTIL_AUTO_CONNECT_H

#include "types.h"

// The connection utility's connection test with DWC's auto-connect (dwc/ac.h), with the connection being edited or
// with the console's settings, then the message of a success or the error screen.
// The ROM does not embed this source file's name; dwc_util_auto_connect.c is descriptive.
void DWCUtilAutoConnect_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_AUTO_CONNECT_H
