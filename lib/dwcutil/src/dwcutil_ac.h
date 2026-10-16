#ifndef POKEBW2_DWCUTIL_DWCUTIL_AC_H
#define POKEBW2_DWCUTIL_DWCUTIL_AC_H

#include "types.h"

// The access-point connection of DWC's settings in overlay 182 (dwcutil_ac.c): a second copy of the WCM wrapper, which
// lists the access points the WCM found, searches for one and connects to it. The ROM names none of it, and the file
// name is a guess from the range it covers; the utility's, because ARM9 main has a dwc_ac.c of its own

// Copies up to max descriptions (0xc0 bytes each) of the access points found to dest and counts all of them
int func_ov182_021a7bcc(void *dest, int max);

#endif // POKEBW2_DWCUTIL_DWCUTIL_AC_H
