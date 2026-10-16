#ifndef POKEBW2_DWC_BM_H
#define POKEBW2_DWC_BM_H

#include "types.h"

// The part of DWC (TWL-DWC 5.3, ARM9 main) that keeps the console's Wi-Fi settings and its Wi-Fi ID in the NVRAM: the
// bm/ files of lib/dwc. The ROM names none of it, so the names of the types and their fields are ours, taken from what
// the code does, and the calls are still named after their addresses.
//
// The Wi-Fi ID is two 43-bit numbers, a confirmed one and a pending one made on this console, and two more fields that
// the pending one is made from. The NVRAM keeps all of it bit-packed in 14 bytes, at offset 0xf0 of each page of the
// Wi-Fi settings; these calls unpack it into a DWCBmWifiIdInfo and pack it again.

#define DWC_BM_PACKED_SIZE 14

// What the settings keep of one connection (a slot of the console's Wi-Fi settings): the network name, the address
// settings, and a byte that says whether the connection is in use. The first 0xEF bytes of a page
#define DWC_BM_CONNECTION_UNUSED 0xFF
typedef struct DWCBmConnection {
    u8 unk0[0x40];
    u8 ssid[0x20];
    u8 unk60[0x60];
    u8 ip[4]; // all 0 when the address comes from DHCP
    u8 gateway[4];
    u8 dns[2][4];   // the first is all 0 when the DNS servers come from DHCP
    u8 netmaskBits; // how many of the mask's high bits are set, 0x20 at most
    u8 unkD1[0x16];
    u8 status; // DWC_BM_CONNECTION_UNUSED; 0 to 2 are the kinds that an NTR console has, more are the TWL's
    u8 unkE8[7];
} DWCBmConnection;

// A page of the NTR console's Wi-Fi settings, 0x100 bytes: a connection, the mask of the connections in use (bit n is
// set while connection n is, kept in the first two pages), the packed Wi-Fi ID, and the CRC16 of all that comes before
// it
typedef struct DWCBmNtrPage {
    DWCBmConnection connection;
    u8 usedMask;
    u8 wifiId[DWC_BM_PACKED_SIZE];
    u16 crc16;
} DWCBmNtrPage;

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
    DWCBmNtrPage ntr;
    DWCBmTwlExtra extra;
} DWCBmTwlPage;

typedef struct DWCBmWifiIdInfo {
    u64 id;        // 43 bits; 0 while no ID has been confirmed
    u64 pendingId; // 43 bits; made from the console's MAC address, the time and the salt
    u16 number;    // 10 bits; a random number from 0 to 999
    u16 salt;      // 16 bits; part of what the pending ID is made from
} DWCBmWifiIdInfo;

// What the Wi-Fi ID is given out as: the two IDs, and whether the first is set
typedef struct DWCBmWifiId {
    u64 id;
    u64 pendingId;
    u32 isSet;
} DWCBmWifiId;

// dwc_backup.c. The pages of the settings are in the NVRAM from page 0 on for an NTR console, and from page 0x11 on for
// a TWL console. A page buffer takes the pages one after the other.
// Reads the first three pages of the NTR console's settings; TRUE when all could be read
BOOL func_02056a58(void *buf);
// The same for the TWL console's
BOOL func_02056a6c(void *buf);
// Unpacks the Wi-Fi ID that the settings were last read with
void func_02056a90(DWCBmWifiIdInfo *info);
// Packs an ID and writes it to each of the first two pages of the NTR console's settings, after the page's CRC16 is
// made again. The work area is 0x400 bytes: a page to read and write, 0x100 bytes of scratch for the write, and the
// 0x200 bytes of the CRC16 table
BOOL func_02056c34(const DWCBmWifiIdInfo *info, void *work);
// Reads the four pages of the NTR console's settings
BOOL func_02056ce8(void *buf);
// Writes the pages of the NTR settings that the four flags select, from a page buffer. The scratch buffer of 0x100
// bytes, 32-byte aligned, is for the write to use
BOOL func_02056cfc(void *buf, const u32 flags[4], void *scratch);
// Keeps the packed Wi-Fi ID of a page, as the settings were last read
void func_02056d58(const void *packed);
// The number of bits that the four bytes of a network mask have set
u8 func_02056d70(const u8 *mask);
// The four bytes of the mask that has the given number of its high bits set
void func_02056db4(u32 bits, u8 *mask);
// Whether an address has bits that the mask leaves to the host, if it is a usable address (func_02056e48)
BOOL func_02056de0(const u8 *address, const u8 *mask);
// Whether the first byte of an address is neither 0, 127 nor above 223
BOOL func_02056e48(const u8 *address);
// Returns TRUE without doing anything; the settings' writes end by it
BOOL func_02056e74(void);
// Packs an ID and returns the packed bytes, DWC_BM_PACKED_SIZE of them, which are kept
const u8 *func_02056e7c(const DWCBmWifiIdInfo *info);
// The packed Wi-Fi ID that the settings were last read with
const u8 *func_02056f3c(void);

// What the checks of dwc_bm_check.c return: 0 when the settings are whole, or were made whole again without losing a
// connection, or a negative number for what failed or was lost. DWC_Init takes any of them as the settings being
// destroyed
typedef enum DWCBmCheckResult {
    DWC_BM_CHECK_WRITE_FAILED = -10000,    // the repaired settings couldn't be written
    DWC_BM_CHECK_READ_FAILED = -10001,     // the NVRAM couldn't be read
    DWC_BM_CHECK_CONNECTION_LOST = -10002, // a connection that was in use is gone
    DWC_BM_CHECK_WIFI_ID_LOST = -10003,    // the first two pages, which both keep the Wi-Fi ID, are gone
} DWCBmCheckResult;

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
int func_02056f48(void *work);

// util_wifiidtool.c
// Makes the pending ID for this console from its MAC address and the time, with a salt that is random the first time
// and counts on from the one in the settings after that, and sets the number; FALSE when the clock can't be read
BOOL func_02057138(DWCBmWifiIdInfo *info);
// Makes the pending ID and writes the settings, with the work area of func_02056c34
BOOL func_02057300(void *work);
// Makes the pending ID the ID, and writes the settings, with the work area of func_02056c34
BOOL func_02057348(DWCBmWifiIdInfo *info, void *work);
// Makes a new pending ID with another salt than the one in the settings, and sets the number; FALSE when the clock
// can't be read
BOOL func_02057398(DWCBmWifiIdInfo *info);
// The Wi-Fi ID of the settings
void func_0205752c(DWCBmWifiId *id);
// Whether the settings hold neither an ID nor a pending one
BOOL func_02057580(void);

#endif // POKEBW2_DWC_BM_H
