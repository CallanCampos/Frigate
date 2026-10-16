#ifndef POKEBW2_DWCUTIL_DWC_AOSS_H
#define POKEBW2_DWCUTIL_DWC_AOSS_H

#include "types.h"

// The AOSS client of DWC's settings in overlay 182 (dwc_aoss.c): Buffalo's one-touch setup, which finds the access
// point named ESSID-AOSS, talks to it in UDP packets and gets the keys of its network. The ROM names none of it; the
// names are ours. The caller gives the request and takes the keys from the same struct

// The types of security of the keys in AossParam.types
#define AOSS_TYPE_WEP40 0x1
#define AOSS_TYPE_WEP104 0x2
#define AOSS_TYPE_TKIP 0x4
#define AOSS_TYPE_AES 0x8

// The keys and the network name of each type of security. The sizes are the longest that the access point sends
typedef struct AossResult {
    u8 wep40Key[4][6];
    u8 wep40Ssid[33];
    u8 wep104Key[4][14];
    u8 wep104Ssid[33];
    u8 tkipKey[64];
    u8 tkipSsid[33];
    u8 aesKey[64];
    u8 aesSsid[33];
} AossResult;

// A count or a delay of -1 is the default. In the answer, types has the types that were asked for and found
typedef struct AossParam {
    u16 types;  // 0x000
    u8 version; // 0x002
    u8 padding_03;
    u16 length;        // 0x004: of data, which ends with a 0
    u8 data[0x100];    // 0x006
    s16 scanCount;     // 0x106: how many times to search for the access point
    s16 scanDelay;     // 0x108
    s16 sendCount;     // 0x10a: how many times to send a packet again
    s16 sendDelay;     // 0x10c
    s16 timeout;       // 0x10e: of a reply, in milliseconds
    u8 mac[6];         // 0x110: of this console
    u8 error;          // 0x116: why it failed
    AossResult result; // 0x117
} AossParam;

// Memory for the client and the Wi-Fi library, from the caller
typedef void *(*DWCAossAllocFunc)(u32 size);
typedef void (*DWCAossFreeFunc)(void *ptr);

// Runs the setup; returns 0, or -1 with the reason in param->error
int DWC_Aoss_Start(AossParam *param);
// Starts the network layer of the client (the Wi-Fi library and its memory); returns 0, or -1 if it did not start
int DWC_Aoss_Init(DWCAossAllocFunc alloc, DWCAossFreeFunc free);
// Stops it and gives the memory back; returns 0, or -1 if it was not running
int DWC_Aoss_Shutdown(void);

#endif // POKEBW2_DWCUTIL_DWC_AOSS_H
