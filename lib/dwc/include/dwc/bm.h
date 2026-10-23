#ifndef POKEBW2_DWC_BM_H
#define POKEBW2_DWC_BM_H

#include "types.h"

// The part of DWC (TWL-DWC 5.3, ARM9 main) that keeps the console's Wi-Fi settings and its Wi-Fi ID in the NVRAM: the
// bm/ files of lib/dwc. The ROM names none of it; the names of the types, their fields and the functions are
// NitroDWC's own, as its libraries' symbols and ntrtwl's reconstruction of NitroDWC have them, where the code shows
// them, and ours elsewhere (the DWCBm_ ones).
//
// The Wi-Fi ID is two 43-bit numbers, a confirmed one and a pending one made on this console, and two more fields that
// the pending one is made from. The NVRAM keeps all of it bit-packed in 14 bytes, at offset 0xf0 of each page of the
// Wi-Fi settings; these calls unpack it into a DWCWiFiInfo and pack it again.

#define DWC_BM_PACKED_SIZE 14

// What the settings keep of one connection (a slot of the console's Wi-Fi settings): the network name, the WEP keys,
// the address settings, the WEP mode, a byte that says whether the connection is in use, its MTU, and in the first two
// pages the mask of the connections in use (bit n is set while connection n is). The first 0xF0 bytes of a page
#define DWC_SETTYPE_NONE 0xFF
typedef struct DWCBMApInfo {
    u8 unk0[0x20];
    u8 unk20[0x20];
    u8 ssid[0x20];
    u8 aossSsid[0x20]; // the SSID of the 40-bit WEP key of an AOSS setup, which has an SSID for each kind of key
    u8 wepKey[4][0x10];
    u8 ip[4]; // all 0 when the address comes from DHCP
    u8 gateway[4];
    u8 dns[2][4];  // the first is all 0 when the DNS servers come from DHCP
    u8 netmask;    // how many of the mask's high bits are set, 0x20 at most
    u8 wep2[4][5]; // the 40-bit WEP keys of an AOSS setup
    u8 unkE5;
    u8 wepMode : 2;     // 0 for no WEP, 1 for a key of 5 bytes, 2 of 13 and 3 of 16
    u8 wepNotation : 6; // 1 when the key was entered as characters, 0 when as hex digits
    u8 setType;         // DWC_SETTYPE_NONE; 0 to 2 are the kinds that an NTR console has, more are the TWL's
    u8 unkE8[2];
    u16 mtu; // 0 for the default
    u8 unkEC[3];
    u8 state;
} DWCBMApInfo;

// A page of the NTR console's Wi-Fi settings, 0x100 bytes: a connection, the packed Wi-Fi ID, and the CRC16 of all that
// comes before it
typedef struct DWCMemPage {
    DWCBMApInfo ap;
    u8 wifi[DWC_BM_PACKED_SIZE];
    u16 crc;
} DWCMemPage;

// The second half of a page of the TWL console's settings: more of the connection, and the CRC16 of its own
typedef struct DWCBmTwlExtra {
    u8 unk0[0x82];
    u8 unk82; // 0 or 1
    u8 unk83; // 0 or 1
    u8 unk84[0x7A];
    u16 crc16;
} DWCBmTwlExtra;

// A page of the TWL console's, 0x200 bytes: a first half laid out like an NTR page, and the extra half
typedef struct DWCBmTwlPage {
    DWCMemPage ntr;
    DWCBmTwlExtra extra;
} DWCBmTwlPage;

typedef struct DWCWiFiInfo {
    u64 attestedUserId; // 43 bits; 0 while no ID has been confirmed
    u64 notAttestedId;  // 43 bits; made from the console's MAC address, the time and the salt
    u16 pass;           // 10 bits; a random number from 0 to 999
    u16 randomHistory;  // 16 bits; part of what the pending ID is made from
} DWCWiFiInfo;

// What the Wi-Fi ID is given out as: the two IDs, and whether the first is set
typedef struct DWCAuthWiFiId {
    u64 uId;
    u64 notAttestedId;
    u32 flag;
} DWCAuthWiFiId;

// dwc_backup.c. The pages of the settings are in the NVRAM from page 0 on for an NTR console, and from page 0x11 on for
// a TWL console. A page buffer takes the pages one after the other.
// Reads the first three pages of the NTR console's settings; TRUE when all could be read
BOOL DWCi_BM_GetApInfo(void *buf);
// The same for the TWL console's
BOOL DWCBm_ReadTwlSettings(void *buf);
// Unpacks the Wi-Fi ID that the settings were last read with
void DWCi_BM_GetWiFiInfo(DWCWiFiInfo *info);
// Packs an ID and writes it to each of the first two pages of the NTR console's settings, after the page's CRC16 is
// made again. The work area is 0x400 bytes: a page to read and write, 0x100 bytes of scratch for the write, and the
// 0x200 bytes of the CRC16 table
BOOL DWCi_BM_SetWiFiInfo(const DWCWiFiInfo *info, void *work);
// Reads the four pages of the NTR console's settings
BOOL DWCBm_ReadAllNtrPages(void *buf);
// Writes the pages of the NTR settings that the four flags select, from a page buffer. The scratch buffer of 0x100
// bytes, 32-byte aligned, is for the write to use
BOOL DWCBm_WriteNtrPages(void *buf, const BOOL flags[4], void *scratch);
// Keeps the packed Wi-Fi ID of a page, as the settings were last read
void DWCi_BACKUPlSetWiFi(const u8 *packed);
// The number of bits that the four bytes of a network mask have set
u8 DWCi_BACKUPlConvMaskCidr(const u8 *mask);
// The four bytes of the mask that has the given number of its high bits set
void DWCi_BACKUPlConvMaskAddr(int bits, u8 *mask);
// Whether an address has bits that the mask leaves to the host, if it is a usable address (DWC_BACKUPlCheckAddress)
BOOL DWC_BACKUPlCheckIp(const u8 *address, const u8 *mask);
// Whether the first byte of an address is neither 0, 127 nor above 223
BOOL DWC_BACKUPlCheckAddress(const u8 *address);
// Returns TRUE without doing anything; the settings' writes end by it
BOOL writeDisable(void);
// Packs an ID and returns the packed bytes, DWC_BM_PACKED_SIZE of them, which are kept
const u8 *DWCi_BACKUPlConvWifiInfo(const DWCWiFiInfo *info);
// The packed Wi-Fi ID that the settings were last read with
const void *DWCi_BACKUPlGetWifi(void);

// What the checks of dwc_bm_check.c return: 0 when the settings are whole, or were made whole again without losing a
// connection, or a negative number for what failed or was lost. DWC_InitForProduction takes any of them as the settings
// being destroyed
typedef enum DWCBMInitResult {
    DWC_BM_INIT_WRITE_ERROR = -10000, // the repaired settings couldn't be written
    DWC_BM_INIT_READ_ERROR = -10001,  // the NVRAM couldn't be read
    DWC_BM_INIT_OK_CLEAR = -10002,    // a connection that was in use is gone
    DWC_BM_INIT_OK_INIT = -10003,     // the first two pages, which both keep the Wi-Fi ID, are gone
} DWCBMInitResult;

// dwc_bm_check.c. Reads the NTR settings into a work area of 0x1000 bytes at +0x600 and, when the console has them and
// the NTR ones could be read, the TWL's at +0. The second argument is not used (the caller passes 0)
typedef enum DWCBmReadResult {
    DWC_BM_READ_FAILED = -1,
    DWC_BM_READ_OK,
} DWCBmReadResult;
DWCBmReadResult func_0205e038(void *work, int unused);

// dwc_bm_init.c, DWC_BM_Init. The same checks for the console the game runs on, then the Wi-Fi ID that the settings
// hold is kept. The work area is 0x1000 bytes. Returns the result of the checks, or -10001 when the four pages of the
// NTR console's settings can't be read
// The size of the work area that DWC_BM_Init is given and clears
#define DWC_BM_INIT_WORK_SIZE 0x1000
int DWC_BM_Init(void *work);

// util_wifiidtool.c
// Makes the pending ID for this console from its MAC address and the time, with a salt that is random the first time
// and counts on from the one in the settings after that, and sets the number; FALSE when the clock can't be read
BOOL DWCi_AUTH_GetNewWiFiInfo(DWCWiFiInfo *info);
// Makes the pending ID and writes the settings, with the work area of DWCi_BM_SetWiFiInfo
BOOL DWCi_AUTH_MakeWiFiID(void *work);
// Makes the pending ID the ID, and writes the settings, with the work area of DWCi_BM_SetWiFiInfo
BOOL DWCi_AUTH_UpDateWiFiID(DWCWiFiInfo *info, void *work);
// Makes a new pending ID with another salt than the one in the settings, and sets the number; FALSE when the clock
// can't be read
BOOL DWCi_AUTH_RemakeWiFiID(DWCWiFiInfo *info);
// The Wi-Fi ID of the settings
void DWC_Auth_GetId(DWCAuthWiFiId *id);
// Whether the settings hold neither an ID nor a pending one
BOOL DWC_Auth_CheckWiFiIDNeedCreate(void);

#endif // POKEBW2_DWC_BM_H
