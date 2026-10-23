#ifndef POKEBW2_DWCUTIL_DWC_UTIL_ERROR_H
#define POKEBW2_DWCUTIL_DWC_UTIL_ERROR_H

#include "types.h"

// The connection utility's error screen: the DWC error code that DWCi_SetErrorCode set, and the message of its kind.
// A then goes back to the first screen of the connection settings or to the settings of the connection.
// The ROM does not embed this source file's name; dwc_util_error.c is descriptive.
void DWCUtilError_Init(void);
void DWCi_SetErrorCode(int code);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_ERROR_H
