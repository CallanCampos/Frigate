#ifndef POKEBW2_DWCUTIL_DWCUTIL_AC_H
#define POKEBW2_DWCUTIL_DWCUTIL_AC_H

#include "types.h"

// The access point setup of DWC's settings in overlay 182 (dwcutil_ac.c): on a thread of its own it waits for an
// access point to open its setup, joins it, and takes the name of its network and its keys over UDP. The ROM names
// none of it, and the file name is a guess from the range it covers; the utility's, because ARM9 main has a dwc_ac.c
// of its own

// How the setup is going, as its callback is told
typedef struct ATERMApConfigState {
    // 1 to 5 while it works (scanning, scanned once, the access point found, its hello, its key), 6 once it has the
    // settings, 7 when it failed
    int state;
    // The milliseconds left before it gives up, or 0xffffffff for no limit
    u32 timeout;
    // 1, or the negative error that ended it
    int result;
} ATERMApConfigState;

typedef void (*DWCUtilAcCallback)(const ATERMApConfigState *status);

// The settings that the setup took
typedef struct ATERMApConfigParam {
    char ssid[0x20];
    // 0 for none, 1 to 3 for WEP keys of 40, 104 and 128 bits, 4 and 5 for a passphrase
    int auth_mode;
    // How many WEP keys there are
    int wepkeyid;
    // The four WEP keys, of the length that wepMode says
    u8 wepkey[4][0x20];
    u8 wpakey[0x40];
} ATERMApConfigParam;

// Starts the setup, with room for apCount access points in a scan and the thread's stack from alloc; 1, or a negative
// error (-10 while one is running)
int ATERMi_ApConfigStart(u32 priority, int apCount, DWCUtilAcCallback callback, void *(*alloc)(u32 size),
                         void (*free)(void *ptr), u32 stackSize);
// Stops the setup and the wireless manager; 1, or -10 if it was not started
int ATERMi_ApConfigEnd(void);
// Copies the settings that the setup took; TRUE
BOOL ATERMi_ApConfigGetResult(ATERMApConfigParam *result);

#endif // POKEBW2_DWCUTIL_DWCUTIL_AC_H
