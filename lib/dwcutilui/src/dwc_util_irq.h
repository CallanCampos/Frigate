#ifndef POKEBW2_DWCUTIL_DWC_UTIL_IRQ_H
#define POKEBW2_DWCUTIL_DWC_UTIL_IRQ_H

#include "types.h"

// The connection utility's interrupts: while it runs, its own VBlank handler runs the VBlank tasks, and the game's
// interrupt mask and VBlank handler are put back when it ends.
// The ROM does not embed this source file's name; dwc_util_irq.c is descriptive.
void DWCUtilIrq_Init(void);
void DWCUtilIrq_Exit(void);
void DWCUtilIrq_VBlank(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_IRQ_H
