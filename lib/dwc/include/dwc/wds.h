#ifndef POKEBW2_DWC_WDS_H
#define POKEBW2_DWC_WDS_H

#include "types.h"
#include "nitro/crypto/rc4.h"
#include "nitro/math.h"
#include "nitro/wm.h"

// A beacon scanner of DWC (TWL-DWC 5.3, ARM9 main; wds.c), on NitroSDK's wireless manager. It scans for the wireless
// parents whose beacons carry the game ID 0x857 and a block of 0x70 bytes of game information, encrypted with RC4
// under a key made from the text "WDS!" and four bytes of the beacon's BSSID, and checked with a CRC-16. The ROM names
// none of it; the names are ours, taken from what the code does. The calls are still named after their addresses.
//
// The caller gives a work area of WDS_GetWorkSize() bytes to WDS_Initialize (WM_Initialize), starts a scan with
// WDS_StartScan, and when the scan's callback comes reads the beacons it found with WDS_GetEntries. WDS_EndScan stops
// the scan and WDS_End ends the wireless manager.

// What a beacon says, once it is decrypted: 0x70 bytes. WDS_IsBeaconInfoValid checks three of them, and the last two
// are the CRC-16 of the rest (a beacon that carries 0 there is taken as it is)
typedef struct WDSBeaconInfo {
    u8 unk_00[0x20];
    u8 unk_20[10]; // ten printable characters; the connection code reads the first as a digit
    u8 unk_2a[0x1a];
    u8 unk_44[0x20];
    u8 unk_64;
    u8 unk_65; // at most 3
    u8 flags;  // bit 1 must be set
    u8 unk_67[5];
    u16 unk_6c;
    u16 crc;
} WDSBeaconInfo;

// A beacon that the scan found, as WDS_GetEntries gives it out: whether the entry is filled, the strength of its
// signal and what the beacon says
typedef struct WDSEntry {
    int valid;
    u16 signal;
    WDSBeaconInfo info;
} WDSEntry;

#define WDS_MAX_ENTRIES 16

// The size of the work area that WDS_Initialize takes, a multiple of the cache line of 32 bytes
u32 WDS_GetWorkSize(void);
// Sets up the work area and starts the wireless manager (WM_Initialize); 0 if it began, and the callback gets the rest.
// A callback of NULL gives -1
int WDS_Initialize(void *work, WMCallbackFunc callback, u16 dmaNo);
// Ends the wireless manager (WM_End) and lets go of the work area; the callback gets the result
int WDS_End(WMCallbackFunc callback);
// Starts a scan of the allowed channels (WM_StartScanEx); the callback gets the scan's result once the beacons are
// taken in
int WDS_StartScan(WMCallbackFunc callback);
// Stops the scan (WM_EndScan)
int WDS_EndScan(WMCallbackFunc callback);
// Clears WDS_MAX_ENTRIES entries and fills them with the beacons that the scan took in, in the order it did;
// -1 if there is no finished scan
int WDS_GetEntries(WDSEntry *entries);
// Decrypts the information of a beacon into `out` and checks its CRC-16. The caller gives the cipher's state, a
// CRC-16 accumulator and its table (connectinet.c has them in its own work area). FALSE if the beacon fails, with
// `out` cleared
BOOL WDS_DecryptBeaconInfo(CRYPTORC4Context *context, u16 *crc, MATHCRC16Table *table, const WMBssDesc *desc,
                           WDSBeaconInfo *out);
// Whether the decrypted information of a beacon is well-formed
BOOL WDS_IsBeaconInfoValid(const WDSBeaconInfo *info);

#endif // POKEBW2_DWC_WDS_H
