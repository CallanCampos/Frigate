#ifndef POKEBW2_DWC_WDSSCAN_H
#define POKEBW2_DWC_WDSSCAN_H

#include "types.h"
#include "dwc/connectinet.h"

// The WDS scan (TWL-DWC 5.3's dwc_wdsscan.c, a guessed name, in ARM9 main): when the auto connect fails, DWC listens
// for the beacons of a Nintendo Zone for three seconds and hands the connection the access point whose signal is the
// strongest. Only the connection of dwc/connectinet.h uses it. The ROM names none of it. The scan itself, on the
// wireless manager, is dwc/wds.h.

// Starts the scan, allocating its work: the result goes in settings, and dmaNo is the DMA channel of the wireless
// manager. The return value is always TRUE, and the caller ignores it
BOOL func_0205be94(DWCInetApSettings *settings, u16 dmaNo);
// Takes a step of the scan: 1 while it runs, 2 once it found an access point (in the settings), 3 if it failed or was
// never started
int func_0205bf04(void);
// Asks the scan to stop: a scan that is waiting for its time to run out ends at the next step
void func_0205c1d0(void);
// Frees the scan's work
void func_0205c1ec(void);

#endif // POKEBW2_DWC_WDSSCAN_H
