#ifndef POKEBW2_DWCUTIL_DWCUTIL_AC_H
#define POKEBW2_DWCUTIL_DWCUTIL_AC_H

#include "types.h"

// The access-point connection of DWC's settings in overlay 182 (dwcutil_ac.c): a second copy of the WCM wrapper, which
// lists the access points the WCM found, searches for one and connects to it. The ROM names none of it, and the file
// name is a guess from the range it covers; the utility's, because ARM9 main has a dwc_ac.c of its own

// Copies up to max descriptions (0xc0 bytes each) of the access points found to dest and counts all of them
int func_ov182_021a7bcc(void *dest, int max);

// Not written yet: the connection test, which runs on a thread of its own and reports its state through a callback
typedef struct DWCUtilAcStatus {
    // 2 and 4 while it works, 6 once it connected, 7 when it failed
    int state;
    int unk_04;
    int unk_08;
} DWCUtilAcStatus;

// The access point that the test connected to
typedef struct DWCUtilAcResult {
    u8 ssid[0x20];
    // 0 for none, 1 to 3 for WEP keys of 40, 104 and 128 bits
    int wepMode;
    int unk_24;
    u8 unk_28[0xc0];
} DWCUtilAcResult;

// Starts the test; TRUE, or a negative error
int func_ov182_021aae5c(u32 priority, int param, void (*callback)(const DWCUtilAcStatus *status),
                        void *(*alloc)(u32 size), void (*free)(void *ptr), u32 stackSize);
BOOL func_ov182_021aaf44(void);
BOOL func_ov182_021ab010(DWCUtilAcResult *result);

#endif // POKEBW2_DWCUTIL_DWCUTIL_AC_H
