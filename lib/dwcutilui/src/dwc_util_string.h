#ifndef POKEBW2_DWCUTIL_DWC_UTIL_STRING_H
#define POKEBW2_DWCUTIL_DWC_UTIL_STRING_H

#include "types.h"

// The connection utility's strings of bytes, which it widens to the 16-bit characters its fonts draw.
// The ROM does not embed this source file's name; dwc_util_string.c is descriptive.

// The length of the string, at most max
int DWCi_MOV_WH_SetUserGameInfo(const u8 *str, int max);
// Copies the string into the size characters at dst, widening each byte, and returns its length
int DWCUtilString_Widen(u16 *dst, const u8 *src, int size);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_STRING_H
