#ifndef POKEBW2_DWCUTIL_DWC_AOSS_AP_H
#define POKEBW2_DWCUTIL_DWC_AOSS_AP_H

#include "types.h"
#include "dwc_aoss.h"

// The access point layer of the AOSS client (dwc_aoss_ap.c): it scans, connects and disconnects through DWC_Wcm_*, one
// operation at a time, and holds the memory functions that the client allocates with. The ROM names none of it; the
// names are ours

// An access point of a scan, and the scan
typedef struct WifiApInfo {
    u32 ssidLength; // 0x00
    u8 ssid[0x20];  // 0x04
    u32 channel;    // 0x24
    u32 unk_28[2];
    u8 bssid[6];      // 0x30
    u32 rateCount;    // 0x38
    u8 rates[16];     // 0x3c
    u32 beaconPeriod; // 0x4c
    u32 type;         // 0x50: 1 for an infrastructure network, 2 for an ad hoc one
} WifiApInfo;

typedef struct WifiScanList {
    u32 count;
    WifiApInfo ap[1];
} WifiScanList;

typedef struct WifiConnectResult {
    u32 success;
    WifiApInfo ap;
} WifiConnectResult;

// The access point to connect to
typedef struct WifiConnectParam {
    u32 ssidLength; // 0x00
    u8 ssid[0x20];  // 0x04
    u32 authMode;   // 0x24
    u32 keyLength;  // 0x28
    u8 key[0x10];   // 0x2c
} WifiConnectParam;

// The memory functions the client was started with, the block it gave the WCM, and a function to tell when the mode
// of the client changes (never set)
typedef struct AossApWork {
    DWCAossAllocFunc alloc;         // 0x00
    void (*onModeChange)(int mode); // 0x04
    void *block;                    // 0x08
} AossApWork;

extern AossApWork gAossAp;
extern DWCAossFreeFunc gAossApFree;

// Scans for access points and lists them in memory from alloc; 0 if it ended well
int AossAp_Scan(WifiScanList **list);
// Connects to the access point and reports in result; 0 if it connected
int AossAp_Connect(const WifiConnectParam *param, WifiConnectResult *result);
// Disconnects; 0 if it did
int AossAp_Disconnect(void);
// Waits for some milliseconds
void AossAp_Sleep(u32 milliseconds);
// Tells that the client went to a mode (an SetMode value)
int AossAp_NotifyModeChange(int mode);

#endif // POKEBW2_DWCUTIL_DWC_AOSS_AP_H
