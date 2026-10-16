// DWC's tool that makes and reads the console's Wi-Fi ID (TWL-DWC 5.3, bm). The file name is a guess: util_wifiidtool
// is a name from DWC's sources, recalled and not checked. The pending ID is built from the console's MAC address, the
// time and a salt, and scrambled with a fixed order of its bytes and a substitution of its nibbles.

#include "bm_internal.h"
#include "dwc/bm.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"

#define NINTENDO_OUI 0x0009BF

// Where each of the first five bytes of the scrambled ID goes
static const u8 sByteOrder[8] = { 1, 2, 0, 4, 3, 5, 6, 7 };
// What each nibble of those bytes is replaced by; the inverse, which the descrambling of an ID would use, is the table
// below. The ROM keeps it between the other two, but no code that is left reads it: MWCC drops a table that is static
// and read by nothing, so it is a global
const u8 WifiIdTool_InverseNibbleSubstitution[16] = { 7, 2, 5, 10, 11, 0, 13, 15, 12, 1, 6, 8, 4, 9, 3, 14 };
static const u8 sNibbleSubstitution[16] = { 5, 9, 1, 14, 12, 2, 10, 0, 11, 13, 3, 4, 8, 6, 15, 7 };

static u64 MakeScrambledId(u32 salt, u32 nic, u32 notNintendo, u32 type);

// Makes a pending ID of 43 bits from the salt, the last three bytes of the MAC address, whether the first three are
// not Nintendo's, and a type of 2 bits (always 0), and scrambles it
static u64 MakeScrambledId(u32 salt, u32 nic, u32 notNintendo, u32 type) {
    union {
        u64 value;
        u8 bytes[8];
    } id;
    u8 copy[8];
    int i;

    id.value =
        ((u64)type & 3) | (((u64)notNintendo & 1) << 2) | (((u64)nic & 0xFFFFFF) << 3) | (((u64)salt & 0xFFFF) << 27);
    for (i = 0; i < 6; i++) {
        id.bytes[i] ^= 0xD6;
    }
    for (i = 0; i < 5; i++) {
        id.bytes[i] = (sNibbleSubstitution[(id.bytes[i] >> 4) & 0xF] << 4) | sNibbleSubstitution[id.bytes[i] & 0xF];
    }
    MI_CpuCopy8(id.bytes, copy, 8);
    for (i = 0; i < 5; i++) {
        id.bytes[sByteOrder[i]] = copy[i];
    }
    // Keeps the 43 bits (five bytes and three bits) and rotates them left by one
    id.bytes[7] = 0;
    id.bytes[6] = 0;
    id.bytes[5] &= 7;
    id.value <<= 1;
    id.bytes[0] |= (id.bytes[5] >> 3) & 1;
    for (i = 0; i < 6; i++) {
        id.bytes[i] ^= 0x67;
    }
    id.bytes[7] = 0;
    id.bytes[6] = 0;
    id.bytes[5] &= 7;
    return id.value;
}

BOOL DWCBm_MakePendingWifiId(DWCBmWifiIdInfo *info) {
    u8 mac[6];
    RTCDate date;
    RTCTime time;
    u8 notNintendo;
    u32 seed;
    s64 seconds;
    u32 oui;
    u32 nic;

    DWCBm_GetWifiIdInfo(info);
    RTC_Init();
    if (RTC_GetDate(&date) != 0) {
        return FALSE;
    }
    if (RTC_GetTime(&time) != 0) {
        return FALSE;
    }
    seconds = RTC_ConvertDateTimeToSecond(&date, &time);
    if (seconds < 0) {
        return FALSE;
    }
    if (OS_IsTickAvailable()) {
        // BUG: adds the address of the tick function, not the tick count that its call would give
#ifdef BUGFIX
        seconds += OS_GetTick();
#else
        seconds += (u32)OS_GetTick;
#endif
    }
    OS_GetMacAddress(mac);
    oui = (mac[0] << 16) | (mac[1] << 8) | mac[2];
    notNintendo = oui != NINTENDO_OUI;
    nic = (mac[3] << 16) | (mac[4] << 8) | mac[5];
    seed = (u32)seconds * 0x5D588B65 + 0x269EC3;
    info->number = ((seed >> 16) * 1000) >> 16;
    info->id = 0;
    if (info->salt == 0) {
        info->pendingId = 0;
        while (info->pendingId == 0) {
            seed = seed * 0x5D588B65 + 0x269EC3;
            while (seed == 0) {
                seed = seed * 0x5D588B65 + 0x269EC3;
            }
            info->salt = seed;
            info->pendingId = MakeScrambledId(info->salt, nic, notNintendo, 0);
        }
    } else {
        info->pendingId = 0;
        while (info->pendingId == 0) {
            info->salt++;
            info->pendingId = MakeScrambledId(info->salt, nic, notNintendo, 0);
        }
    }
    return TRUE;
}

BOOL DWCBm_MakeAndSaveWifiId(void *work) {
    DWCBmWifiIdInfo info;

    if (DWCBm_MakePendingWifiId(&info) == FALSE) {
        return FALSE;
    }
    return DWCBm_WriteWifiIdInfo(&info, work) != 0;
}

BOOL DWCBm_ConfirmWifiId(DWCBmWifiIdInfo *info, void *work) {
    DWCBmWifiIdInfo stored;

    DWCBm_GetWifiIdInfo(&stored);
    info->id = info->pendingId;
    info->pendingId = stored.pendingId;
    return DWCBm_WriteWifiIdInfo(info, work) != 0;
}

BOOL DWCBm_RenewPendingWifiId(DWCBmWifiIdInfo *info) {
    u8 mac[6] = { 0, 0, 0, 0, 0, 0 };
    RTCDate date;
    RTCTime time;
    u8 notNintendo;
    u32 seed;
    s64 seconds;
    u32 nic;
    u32 oui;

    DWCBm_GetWifiIdInfo(info);
    RTC_Init();
    if (RTC_GetDate(&date) != 0) {
        return FALSE;
    }
    if (RTC_GetTime(&time) != 0) {
        return FALSE;
    }
    seconds = RTC_ConvertDateTimeToSecond(&date, &time);
    if (seconds < 0) {
        return FALSE;
    }
    if (OS_IsTickAvailable()) {
        // BUG: adds the address of the tick function, not the tick count that its call would give
#ifdef BUGFIX
        seconds += OS_GetTick();
#else
        seconds += (u32)OS_GetTick;
#endif
    }
    OS_GetMacAddress(mac);
    oui = (mac[0] << 16) | (mac[1] << 8) | mac[2];
    notNintendo = oui != NINTENDO_OUI;
    nic = (mac[3] << 16) | (mac[4] << 8) | mac[5];
    seed = (u32)seconds * 0x5D588B65 + 0x269EC3;
    info->number = ((seed >> 16) * 1000) >> 16;
    info->pendingId = 0;
    while (info->pendingId == 0) {
        seed = seed * 0x5D588B65 + 0x269EC3;
        while (seed == 0 || info->salt == (u16)seed) {
            seed = seed * 0x5D588B65 + 0x269EC3;
        }
        info->salt = seed;
        info->pendingId = MakeScrambledId(info->salt, nic, notNintendo, 0);
    }
    return TRUE;
}

void DWCBm_GetWifiId(DWCBmWifiId *out) {
    DWCBmWifiIdInfo info;

    DWCBm_GetWifiIdInfo(&info);
    out->id = info.id;
    out->pendingId = info.pendingId;
    if (info.id != 0) {
        out->isSet = TRUE;
    } else {
        out->isSet = FALSE;
    }
}

BOOL DWCBm_HasNoWifiId(void) {
    DWCBmWifiIdInfo info;

    DWCBm_GetWifiIdInfo(&info);
    if (info.pendingId == 0 && info.id == 0) {
        return TRUE;
    }
    return FALSE;
}
