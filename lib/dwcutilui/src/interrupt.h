#ifndef POKEBW2_DWCUTIL_INTERRUPT_H
#define POKEBW2_DWCUTIL_INTERRUPT_H

#include "types.h"

// The connection utility's interrupts: while it runs, its own VBlank handler runs the VBlank tasks, and the game's
// interrupt mask and VBlank handler are put back when it ends.
// The file is the DWC utility library's interrupt.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_ITRlIntr(void);
void DWCi_ITRlEnd(void);
void intrVBlank(void);

#endif // POKEBW2_DWCUTIL_INTERRUPT_H
