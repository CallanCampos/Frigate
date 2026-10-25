#ifndef POKEBW2_DWCUTIL_RAKURAKU_H
#define POKEBW2_DWCUTIL_RAKURAKU_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's connection test (dwcutil/atermset.h), and what it found, saved into the connection
// being edited.
// The file is the DWC utility library's rakuraku.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.

void DWCi_RAKUlInit(void);
void DWCi_RAKUlEnd(void);
// 0 while the test works, 1 and 2 at its two steps, 3 once it connected with a WEP key or none, 4 when it failed, 5
// when it connected otherwise
int DWCi_RAKUlGetState(void);
void DWCi_RAKUlSetResult(void);

#endif // POKEBW2_DWCUTIL_RAKURAKU_H
