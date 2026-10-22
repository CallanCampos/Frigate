// DWC's auto-connect, the lists of what a search looks for (TWL-DWC 5.3, ac). The file name is a guess: its functions
// make the targets (SSIDs and their ids) from the connection settings that the system keeps and from the SSIDs of the
// special access points, and the candidates still to visit. It ends firmly where dwc_wdsscan.c begins; where it starts
// is not known (dwc_ac_result.c is the file before, or the same one).

#include "dwc/ac.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The SSIDs of the special access points: those of FREESPOT (a prefix, without a zero), of NINTENDOWFC, and the
// prefix of an NWCUSBAP's
static const char FREESPOT_SSID[8] = "FREESPOT";
static const char NINTENDOWFC_SSID[] = "NINTENDOWFC";
static char sNwcUsbApSsid[] = "NWCUSBAP";

static u8 MakeAroundList(ACWORK *work);
static u8 MakeStealthList(ACWORK *work);
static u8 MakeUserList(ACWORK *work);
static u8 MakeDifferChannelList(ACWORK *work);
static u8 CheckDifferChannelStart(ACWORK *work);

// Makes the list that a step of the search works from. Step 0 is that of the first search: the targets of the stored
// settings and FREESPOT. Step 1 counts the candidates that the search found and still has to visit, and has the first
// of them to start with. Step 2 is the targets of the second search: the settings, NWCUSBAP and NINTENDOWFC. TRUE if
// there is something to look for
BOOL DWCi_AC_MakeSearchList(u32 step) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    switch (step) {
    case 0:
        MI_CpuClear32(work->searchList, sizeof(work->searchList));
        work->searchListNum = MakeAroundList(work);
        break;
    case 1:
        work->searchListNum = MakeDifferChannelList(work);
        work->searchListNo = CheckDifferChannelStart(work);
        if (work->searchListNum == 0) {
            return FALSE;
        }
        break;
    case 2:
        MI_CpuClear32(work->searchList, sizeof(work->searchList));
        work->searchListNo = 0;
        work->searchListNum = MakeStealthList(work);
        if (work->searchListNum == 0) {
            return FALSE;
        }
        break;
    }
    // A test of the console whose body is not in this build, most likely a debug print for the DSi
    OS_IsRunOnTwl();
    return TRUE;
}

// The id of FREESPOT if the SSID of the beacon starts with that and the search takes it, else 0
int DWCi_AC_CheckFreespot(const WMBssDesc *bss) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    if (work->mode == 0 || work->mode == 6) {
        if (strncmp(bss->ssid, FREESPOT_SSID, sizeof(FREESPOT_SSID)) == 0) {
            return DWC_AC_ID_FREESPOT;
        }
    }
    return 0;
}

// How many targets there are: the first with an SSID of no length ends them
u8 DWCAc_CountTargets(void) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    u8 i;

    for (i = 0; i < DWC_AC_TARGET_MAX; i++) {
        if (work->searchList[i].length == 0) {
            break;
        }
    }
    return i;
}

// The targets of the first search: those of the settings, and FREESPOT. Returns how many there are
static u8 MakeAroundList(ACWORK *work) {
    u8 count;
    ACSL *target = work->searchList;

    count = MakeUserList(work);
    target += count;

    if (work->mode == 0 || work->mode == 6) {
        MI_CpuCopy8(FREESPOT_SSID, target->ssid, sizeof(FREESPOT_SSID));
        target->length = sizeof(FREESPOT_SSID);
        target->type = DWC_AC_ID_FREESPOT;
        count++;
    }
    return count;
}

// The targets of the second search: those of the settings, an NWCUSBAP's SSID and NINTENDOWFC. Returns how many there
// are
static u8 MakeStealthList(ACWORK *work) {
    u8 count;
    ACSL *target = work->searchList;

    count = MakeUserList(work);
    target += count;

    if (work->mode == 0 || work->mode == 4) {
        MI_CpuCopy8(sNwcUsbApSsid, target->ssid, 8);
        target->length = 8;
        target->type = DWC_AC_ID_NWCUSBAP;
        count++;
        target++;
    }
    if (work->mode == 0 || work->mode == 8) {
        MI_CpuCopy8(NINTENDOWFC_SSID, target->ssid, 11);
        target->length = 11;
        target->type = DWC_AC_ID_NINTENDOWFC;
        count++;
    }
    return count;
}

// Puts the SSID of a stored setting in the target at hand, with the id that the setting's key gives it. FALSE, with the
// target untouched, for a setting with no SSID
static inline BOOL AddTarget(ACSL *target, const u8 *ssid, u8 id) {
    u8 length;

    for (length = 0; length < 32; length++) {
        if (ssid[length] == 0) {
            break;
        }
        target->ssid[length] = ssid[length];
    }
    if (length != 0) {
        target->length = length;
        target->type = id;
        return TRUE;
    }
    return FALSE;
}

// The targets of the stored settings that the search takes (all of them, or the one that its mode names), one for each
// SSID of a setting that is in use. Returns how many there are. The ids are those that the connection reads the keys
// by (ids 0 to 2 the DS settings' first SSIDs, 3 to 5 their second, 6 to 8 the DSi settings' first SSIDs, 9 to 11 their
// second, 12 to 14 and 15 to 17 their SSIDs for WPA)
static u8 MakeUserList(ACWORK *work) {
    u8 count = 0;
    int i;
    const DWCMemPage *ds = work->userInfo;
    const DWCAcDsiSetting *dsi = work->dsiSettings;
    ACSL *target = work->searchList;

    for (i = 0; i < 3; i++, ds++) {
        if (work->mode == 0 || work->mode == i + 1) {
            if (ds->ap.setType != 0xff) {
                if (AddTarget(target, ds->ap.ssid, i)) {
                    count++;
                    target++;
                }
                if (ds->ap.setType == 1) {
                    if (AddTarget(target, ds->ap.aossSsid, i + 3)) {
                        count++;
                        target++;
                    }
                }
            }
        }
    }

    if (OS_IsRunOnTwl()) {
        for (i = 0; i < 3; i++, dsi++) {
            if (work->mode == 0 || work->mode == i + 0xb) {
                if (dsi->base.ap.setType != 0xff) {
                    if (AddTarget(target, dsi->base.ap.ssid, i + 6)) {
                        count++;
                        target++;
                    }
                    if (dsi->base.ap.setType == 1 || dsi->base.ap.setType == 0x11) {
                        if (AddTarget(target, dsi->base.ap.aossSsid, i + 9)) {
                            count++;
                            target++;
                        }
                        if (AddTarget(target, dsi->ssid3, i + 12)) {
                            count++;
                            target++;
                        }
                        if (AddTarget(target, dsi->ssid4, i + 15)) {
                            count++;
                            target++;
                        }
                    }
                }
            }
        }
    }
    return count;
}

// Counts the candidates that are still to be visited: those that no connection was tried on and whose tag is not the
// channel they are on, which means that the search heard them from another channel. The others are marked as seen
static u8 MakeDifferChannelList(ACWORK *work) {
    u8 count = 0;
    int i;

    for (i = 0; i < work->candidateCount; i++) {
        if (work->findList[i].state == 0 && work->bssDesc[i].channel - 1 != work->findList[i].channel) {
            work->findList[i].find = 0;
            count++;
        } else {
            work->findList[i].find = 1;
        }
    }
    return count;
}

// The first candidate that is not marked as seen, else the first of all
static u8 CheckDifferChannelStart(ACWORK *work) {
    u8 i;
    u8 first = 0;
    u8 count = work->candidateCount;

    for (i = 0; i < count; i++) {
        if (work->findList[i].find == 0) {
            first = i;
            break;
        }
    }
    return first;
}
