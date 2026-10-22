#ifndef POKEBW2_DWCUTIL_DWC_AOSS_AP_H
#define POKEBW2_DWCUTIL_DWC_AOSS_AP_H

#include "types.h"
#include "dwcutil/dwc_aoss.h"

// The access point layer of the AOSS client (dwc_aoss_ap.c): it scans, connects and disconnects through APC_*, one
// operation at a time, and holds the memory functions that the client allocates with. The ROM names none of it; the
// names are the DWC utility library's own, as that library's symbols and debug information have them, where the code
// shows them, and ours elsewhere

// An access point of a scan, and the scan
typedef struct WLLA_BSS_INFO {
    u32 ssidLength; // 0x00
    u8 ssid[0x20];  // 0x04
    u32 Channel;    // 0x24
    u32 Noise[2];
    u8 Bssid[6];      // 0x30
    u32 rateCount;    // 0x38
    u8 rates[16];     // 0x3c
    u32 BeaconPeriod; // 0x4c
    u32 Capability;   // 0x50: 1 for an infrastructure network, 2 for an ad hoc one
} WLLA_BSS_INFO;

typedef struct WLLA_BSS_LIST {
    u32 Num;
    WLLA_BSS_INFO BssInfo[1];
} WLLA_BSS_LIST;

typedef struct WLLA_LINK_STAT {
    u32 LinkStat;
    WLLA_BSS_INFO BssInfo;
} WLLA_LINK_STAT;

// The access point to connect to
typedef struct WLLA_CONNECT_PARAM {
    u32 ssidLength; // 0x00
    u8 ssid[0x20];  // 0x04
    u32 AuthAlgo;   // 0x24
    u32 keyLength;  // 0x28
    u8 key[0x10];   // 0x2c
} WLLA_CONNECT_PARAM;

// The memory functions the client was started with, the block it gave the WCM, and a function to tell when the mode
// of the client changes (never set)
extern DWCAossAllocFunc gAossApAlloc;
extern void (*gAossApOnModeChange)(int mode);
extern void *gAossApBlock;
extern DWCAossFreeFunc gAossApFree;

// Scans for access points and lists them in memory from alloc; 0 if it ended well
int AOSSi_WLANGetBSSList(WLLA_BSS_LIST **list);
// Connects to the access point and reports in result; 0 if it connected
int AOSSi_WLANConnect(const WLLA_CONNECT_PARAM *param, WLLA_LINK_STAT *result);
// Disconnects; 0 if it did
int AOSSi_Disconnect(void);
// Waits for some milliseconds
void AOSSi_Sleep(u32 milliseconds);
// Tells that the client went to a mode (an SetMode value)
int AOSSi_Status(int mode);

#endif // POKEBW2_DWCUTIL_DWC_AOSS_AP_H
