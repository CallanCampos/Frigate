#ifndef POKEBW2_DWC_APINFO_H
#define POKEBW2_DWC_APINFO_H

#include "types.h"
#include "nitro/wm.h"

// The access point information of DWC (TWL-DWC 5.3's dwc_connectinet.c, in ARM9 main): what kind of access point the
// console is connected through, and what kind a beacon is. The ROM names none of it: DWC_GetApInfo and DWCApInfo are
// the SDK's names, and DWCi_GetBssDescParseWorkSize and DWCi_BssDescToApInfo are ours, from what they do. They are a
// header of their own, apart from dwc/connectinet.h, to keep nitro/wm.h out of that header's includers.

// The kinds of access point, the SDK's DWCApInfoType as NitroDWC's base/dwc_connectinet.h has it: 0 to 3 (and 10 to 12,
// which it doesn't name) are the console's own settings, 4 to 6 and 8 hotspots (the game's wireless scan takes them
// for those), and 99 one that DWC does not know. Only 4 and 8 have an area and spot information
typedef enum DWCApInfoType {
    DWC_APINFO_TYPE_USER0 = 0,
    DWC_APINFO_TYPE_USER1,
    DWC_APINFO_TYPE_USER2,
    DWC_APINFO_TYPE_USB,
    DWC_APINFO_TYPE_SHOP,
    DWC_APINFO_TYPE_FREESPOT,
    DWC_APINFO_TYPE_WAYPORT,
    DWC_APINFO_TYPE_OTHER,
    DWC_APINFO_TYPE_NINTENDOWFC = DWC_APINFO_TYPE_OTHER,
    DWC_APINFO_TYPE_NINTENDOSPOT,
    DWC_APINFO_TYPE_UNKNOWN = 99,
} DWCApInfoType;

// The area of a Nintendo access point, the SDK's DWCApInfoArea
typedef enum DWCApInfoArea {
    DWC_APINFO_AREA_JPN = 0,
    DWC_APINFO_AREA_USA,
    DWC_APINFO_AREA_EUR,
    DWC_APINFO_AREA_AUS,
    DWC_APINFO_AREA_KOR,
    DWC_APINFO_AREA_UNKNOWN = 0xff,
} DWCApInfoArea;

// A DWCApInfo of the SDK. Its size is 0x40
typedef struct DWCApInfo {
    // The kind of the access point (the SDK's aptype)
    DWCApInfoType aptype;
    // The digit of the area in a Nintendo access point's information, 0xff if there is none
    DWCApInfoArea area;
    // The key mode of the setting that the connection is made with (DWCAc_GetKeyMode)
    int keyMode;
    // The nine characters of the spot's name (the SDK's spotinfo)
    char spotInfo[10];
    char essid[33];
    u8 bssid[6];
} DWCApInfo;

// Gives the information of the access point the console is connected through, FALSE if it is not connected or the
// access point is not one that DWC knows: DWC_GetApInfo
BOOL DWC_GetApInfo(DWCApInfo *apInfo);

// The work that DWCi_BssDescToApInfo parses a beacon with, which the caller allocates; its size is
// DWCi_GetBssDescParseWorkSize's
typedef struct DWCBssWork DWCBssWork;
u32 DWCi_GetBssDescParseWorkSize(void);
// Gives the information of the access point that a beacon is of, FALSE if it is not one that DWC knows
BOOL DWCi_BssDescToApInfo(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo);

#endif // POKEBW2_DWC_APINFO_H
