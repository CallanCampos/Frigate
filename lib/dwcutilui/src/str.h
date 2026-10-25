#ifndef POKEBW2_DWCUTIL_STR_H
#define POKEBW2_DWCUTIL_STR_H

#include "types.h"

// The connection utility's strings of bytes, which it widens to the 16-bit characters its fonts draw.
// The file is the DWC utility library's str.o: Black 1's decompilation (aletheia) has its two functions as
// libdwcutil.TWL.HYB.a's str.o, whose names they have.

// The length of the string, at most max
int DWCi_STRlNLen(const u8 *str, int max);
// Copies the string into the size characters at dst, widening each byte, and returns its length
int DWCi_STRlConvertStringAsciiToUnicode(u16 *dst, const u8 *src, int size);

#endif // POKEBW2_DWCUTIL_STR_H
