#ifndef POKEBW2_DWCUTIL_FOCUS_H
#define POKEBW2_DWCUTIL_FOCUS_H

#include "types.h"

// The connection utility's button sprites on the main screen: a pair of them from one of six pairs of cells, or the
// four of cells 6 to 9, at the positions given. Showing one set takes the last one away.
// The file is the DWC utility library's focus.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_FOCUSlInit(void);
void DWCi_FOCUSlEnd(void);
void DWCi_FOCUSlDisp(int pair, int x0, int x1, int y);
void DWCi_FOCUSlDisp4(int x0, int x1, int y0, int y1);
void DWCi_FOCUSlDelete(void);

#endif // POKEBW2_DWCUTIL_FOCUS_H
