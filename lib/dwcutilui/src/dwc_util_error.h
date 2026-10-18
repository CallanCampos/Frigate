#ifndef POKEBW2_DWCUTIL_DWC_UTIL_ERROR_H
#define POKEBW2_DWCUTIL_DWC_UTIL_ERROR_H

#include "types.h"

// The connection utility's error screen: the DWC error code that func_ov182_021b2d3c set, and the message of its kind.
// A then goes back to the first screen of the connection settings or to the settings of the connection.
// The ROM does not embed this source file's name; dwc_util_error.c is descriptive.
void func_ov182_021b293c(void);
void func_ov182_021b2d3c(int code);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_ERROR_H
