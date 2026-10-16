#ifndef POKEBW2_DWC_APINFO_H
#define POKEBW2_DWC_APINFO_H

#include "types.h"
#include "nitro/wm.h"

// The access point information of DWC (TWL-DWC 5.3's dwc_connectinet.c, in ARM9 main): what kind of access point the
// console is connected through, and what kind a beacon is. The ROM names none of it; the SDK's names are in the
// comments, where the code shows them.

// A DWCApInfo of the SDK. Its size is 0x40
typedef struct DWCApInfo {
    // The kind of the access point (the SDK's aptype): 0 to 3 and 10 to 12 are the console's own settings, 4 to 6 and 8
    // hotspots (the game's wireless scan takes them for those), and 99 one that DWC does not know. Only 4 and 8 have an
    // area and spot information
    int apType;
    // The digit of the area in a Nintendo access point's information, 0xff if there is none
    int area;
    // A byte of the connection's information
    int unk8;
    // The nine characters of the spot's name (the SDK's spotinfo)
    char spotInfo[10];
    u8 essid[33];
    u8 bssid[6];
} DWCApInfo;

// Gives the information of the access point the console is connected through, FALSE if it is not connected or the
// access point is not one that DWC knows: DWC_GetApInfo
BOOL func_0205b29c(DWCApInfo *apInfo);

// The work that func_0205b594 parses a beacon with, which the caller allocates; its size is func_0205b588's
typedef struct DWCBssWork DWCBssWork;
u32 func_0205b588(void);
// Gives the information of the access point that a beacon is of, FALSE if it is not one that DWC knows
BOOL func_0205b594(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo);

#endif // POKEBW2_DWC_APINFO_H
