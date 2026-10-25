#ifndef POKEBW2_DWCUTIL_AOSS_H
#define POKEBW2_DWCUTIL_AOSS_H

#include "types.h"

// The AOSS client of DWC's settings in overlay 182 (aoss.c, the library's aoss.o): Buffalo's one-touch setup, which
// finds the access point named ESSID-AOSS, talks to it in UDP packets and gets the keys of its network. The ROM names
// none of it; the names are the DWC utility library's own, as that library's symbols and debug information have them,
// where the code shows them, and ours elsewhere. The caller gives the request and takes the keys from the same struct

// The types of security of the keys in AOSS_INFO.types
#define AOSS_TYPE_WEP40 0x1
#define AOSS_TYPE_WEP104 0x2
#define AOSS_TYPE_TKIP 0x4
#define AOSS_TYPE_AES 0x8

// The keys and the network name of each type of security. The sizes are the longest that the access point sends
typedef struct AOSS_SECURE_INFO {
    u8 wep40Key[4][6];
    u8 WEP40_SSID[33];
    u8 wep104Key[4][14];
    u8 WEP104_SSID[33];
    u8 WPA_PSK_TKIP[64];
    u8 WPA_PSK_TKIP_SSID[33];
    u8 WPA_PSK_AES[64];
    u8 WPA_PSK_AES_SSID[33];
} AOSS_SECURE_INFO;

// A count or a delay of -1 is the default. In the answer, types has the types that were asked for and found
// What the client tells the access point about itself
typedef struct PRODUCT_INFO {
    u8 dataType;
    u8 padding_01;
    u16 dataLen;           // of data, which ends with a 0
    u8 productName[0x100]; // the access point's name
} PRODUCT_INFO;

typedef struct AOSS_INFO {
    u16 supportSecurity;         // 0x000
    PRODUCT_INFO productInfo;    // 0x002
    s16 retryCount_Scan;         // 0x106: how many times to search for the access point
    s16 retryInterval_Scan;      // 0x108
    s16 retryCount_Comu;         // 0x10a: how many times to send a packet again
    s16 retryInterval_Comu;      // 0x10c
    s16 selectInterval;          // 0x10e: of a reply, in milliseconds
    u8 macAddress[6];            // 0x110: of this console
    u8 aossResult;               // 0x116: why it failed
    AOSS_SECURE_INFO secureInfo; // 0x117
} AOSS_INFO;

// Memory for the client and the Wi-Fi library, from the caller
typedef void *(*DWCAossAllocFunc)(u32 size);
typedef void (*DWCAossFreeFunc)(void *ptr);

// Runs the setup; returns 0, or -1 with the reason in param->error
int AOSSi_Init(AOSS_INFO *param);
// Starts the network layer of the client (the Wi-Fi library and its memory); returns 0, or -1 if it did not start
int AOSSi_InitLocal(DWCAossAllocFunc alloc, DWCAossFreeFunc free);
// Stops it and gives the memory back; returns 0, or -1 if it was not running
int AOSSi_EndLocal(void);

#endif // POKEBW2_DWCUTIL_AOSS_H
