// DWC's auto-connect, the part that takes in the wireless manager's notifications (TWL-DWC 5.3, ac). The file name is a
// guess: the file starts at the function that the notification callback calls, and ends where the teardown starts, with
// no data or call across the boundary to prove either end. Its one object of data is the OUI and type of the WPA
// vendor element.

#include "dwc/ac.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// MSL's <string.h>, not dwc/ext.h's declarations: with it MWCC hoists the loads around AddList's strncmp
// out of its loop, as the ROM does
#include <string.h>

// A candidate's place that nothing has been put in is all zeroes
#define IS_EMPTY(info) ((info).state + (info).type + (info).rssi + (info).channel + (info).find == 0)

static void DWCi_AC_GetBeacon(const WMBssDesc *bss);
static int CompareList(const WMBssDesc *bss, int count, const ACSL *targets);
static int CompareListDiff(const WMBssDesc *bss, ACWORK *work);
static int AddList(int id, const WMBssDesc *bss, u8 tag, ACWORK *work);
static void SetDataListTail(u8 id, const WMBssDesc *bss, u8 tag, ACWORK *work);
static void UpDateList(int slot, const WMBssDesc *bss, u8 tag, ACWORK *work);
static void SortList(int slot, ACWORK *work);

// Takes in one access point that the wireless manager found: from the state the search is in, it matches the beacon
// with a target (state 3, 5) or with a candidate that is kept (state 4), and adds or refreshes the candidate
static void DWCi_AC_GetBeacon(const WMBssDesc *bss) {
    int id = -1;
    u8 tag;
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    ACHOLD *control = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);

    control->findAP = 1;
    switch (DWCi_AC_GetPhase()) {
    case 3:
        tag = work->tag;
        if (bss->ssidLength == 0 || bss->ssid[0] == 0) {
            DWCi_AC_SetStealthChannel(bss->channel);
        } else if (bss->ssidLength == 1 && bss->ssid[0] == ' ') {
            DWCi_AC_SetStealthChannel(bss->channel);
            id = CompareList(bss, work->searchListNum, work->searchList);
        } else {
            id = CompareList(bss, work->searchListNum, work->searchList);
        }
        break;
    case 4:
        tag = work->bssDesc[work->searchListNo].channel - 1;
        id = CompareListDiff(bss, work);
        if (id >= 0) {
            work->findList[work->searchListNo].find = 1;
        }
        break;
    case 5:
        tag = work->tag;
        id = CompareList(bss, 1, &work->searchList[work->searchListNo]);
        if (id >= 0) {
            work->searchList[work->searchListNo].find = 1;
        }
        break;
    default:
        return;
    }
    if (id >= 0) {
        SortList(AddList(id, bss, tag, work), work);
    }
}

// The id of the kind of access point that a beacon with a 32-byte SSID is, from the SSID and the kind the search wants
int DWCi_AC_CheckNintendoSSID(const WMBssDesc *bss) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    if (work->mode == 0 || work->mode == 4) {
        if (((bss->capaInfo >> 4) & 1) == 1) {
            if (DWCi_AC_CheckNintendoUsbAP(bss->ssid) == TRUE) {
                return DWC_AC_ID_NWCUSBAP;
            }
        }
    }
    if (work->mode == 0 || work->mode == 5) {
        if (((bss->capaInfo >> 4) & 1) == 1) {
            if (DWCi_AC_CheckNintendoShopAP(bss->ssid) == TRUE) {
                return DWC_AC_ID_NDWCSHAP;
            }
        }
    }
    return -1;
}

// Whether the access point can be taken for a connection with WPA. A DS cannot tell, and takes it so
BOOL DWCAc_CheckWpa(const WMBssDesc *bss) {
    if (OS_IsRunOnTwl() && func_02703208((NWMBssDesc *)bss, 0x30) == NULL) {
        u8 oui[4] = { 0x00, 0x50, 0xf2, 0x01 };

        if (func_02703250((NWMBssDesc *)bss, 0xdd, oui) == NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

// The id of the first of `count` targets whose SSID is the beacon's, or that of the kind of access point that the SSID
// shows, or -1
static int CompareList(const WMBssDesc *bss, int count, const ACSL *targets) {
    int i;
    u16 length;

    if (bss->ssidLength == 0x20) {
        int id = DWCi_AC_CheckNintendoSSID(bss);

        if (id > 0) {
            return id;
        }
    }
    i = 0;
    if (count > 0) {
        length = bss->ssidLength;
        do {
            if ((u8)length == targets->length && strncmp(bss->ssid, targets->ssid, length) == 0) {
                return targets->type;
            }
            i++;
            targets++;
        } while (i < count);
    }
    return -1;
}

// The id of the candidate that has the beacon's SSID, or that of the kind of access point that the SSID shows, or -1
static int CompareListDiff(const WMBssDesc *bss, ACWORK *work) {
    int i;
    s16 count;
    u16 length;

    if (bss->ssidLength == 0x20) {
        int id = DWCi_AC_CheckNintendoSSID(bss);

        if (id > 0) {
            return id;
        }
    }
    count = work->candidateCount;
    i = 0;
    if (count > 0) {
        length = bss->ssidLength;
        do {
            if (length == work->bssDesc[i].ssidLength && strncmp(bss->ssid, work->bssDesc[i].ssid, length) == 0) {
                return work->findList[i].type;
            }
            i++;
        } while (i < count);
    }
    return -1;
}

// Whether two BSSIDs are the same. The candidate's is passed as a `const void *`: as a `const u8 *`, MWCC folds the
// list entry's offset into each byte's load, which the original doesn't
static inline BOOL DWCAc_IsSameBssid(const u8 *a, const void *b) {
    return a[0] == ((const u8 *)b)[0] && a[1] == ((const u8 *)b)[1] && a[2] == ((const u8 *)b)[2] &&
           a[3] == ((const u8 *)b)[3] && a[4] == ((const u8 *)b)[4] && a[5] == ((const u8 *)b)[5];
}

// Puts the beacon among the candidates, as a new one or in place of the one it matches, and gives its place: the last
// one, a place past the end of the list that SortList then sorts it from
static int AddList(int id, const WMBssDesc *bss, u8 tag, ACWORK *work) {
    int i;
    s16 count = work->candidateCount;
    int found = -1;

    for (i = 0; i < count; i++) {
        if (DWCAc_IsSameBssid(bss->bssid, work->bssDesc[i].bssid) &&
            strncmp(bss->ssid, work->bssDesc[i].ssid, bss->ssidLength) == 0) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        SetDataListTail(id, bss, tag, work);
        if (work->candidateCount < DWC_AC_CANDIDATE_MAX) {
            work->candidateCount++;
        }
        found = DWC_AC_CANDIDATE_MAX;
    } else {
        UpDateList(found, bss, tag, work);
    }
    return found;
}

// Stores the beacon as a new candidate, in the place past the end of the list
static void SetDataListTail(u8 id, const WMBssDesc *bss, u8 tag, ACWORK *work) {
    ACFL *info = &work->findList[DWC_AC_CANDIDATE_MAX];
    WMBssDesc *dest = &work->bssDesc[DWC_AC_CANDIDATE_MAX];
    u16 level = bss->rssi;

    info->type = id;
    if (work->rssiPacked == 1) {
        if (level & 2) {
            level = (u8)(level >> 2);
        } else {
            level = (u8)((level >> 2) + 0x19);
        }
    }
    info->rssi = level;
    info->channel = tag;
    WCM_FillApEntry(bss, dest);
}

// Refreshes a candidate with a beacon of the same access point: its signal and the tag, if the signal is stronger
static void UpDateList(int slot, const WMBssDesc *bss, u8 tag, ACWORK *work) {
    ACFL *info = &work->findList[slot];
    WMBssDesc *dest = &work->bssDesc[slot];
    u16 level = bss->rssi;

    if (work->rssiPacked == 1) {
        if (level & 2) {
            level = (u8)(level >> 2);
        } else {
            level = (u8)((level >> 2) + 0x19);
        }
    }
    if ((u8)level > info->rssi) {
        info->rssi = level;
        info->channel = tag;
    }
    MIi_CpuCopy32(bss, dest, sizeof(WMBssDesc));
}

// Moves the candidate in a place toward the front for as long as it is better than the one before, and then clears the
// place past the end of the list
static void SortList(int slot, ACWORK *work) {
    ACFL *infos = work->findList;
    WMBssDesc *candidates = work->bssDesc;
    int prev;

    for (prev = slot - 1; prev >= 0; prev--) {
        BOOL swap = FALSE;

        if (work->preferWpa == 1 && DWCAc_CheckWpa(&candidates[slot]) == TRUE) {
            if (infos[slot].rssi < infos[prev].rssi) {
                if (DWCAc_CheckWpa(&candidates[prev]) == FALSE) {
                    swap = TRUE;
                }
            } else {
                swap = TRUE;
            }
        } else if (DWC_AC_ID_IS_HOTSPOT(infos[slot].type)) {
            if (infos[slot].rssi >= infos[prev].rssi) {
                // The one before is of a kind too, or is an empty place
                if (DWC_AC_ID_IS_HOTSPOT(infos[prev].type) || IS_EMPTY(infos[prev])) {
                    swap = TRUE;
                }
            }
        } else if (infos[slot].rssi >= infos[prev].rssi) {
            if (work->preferWpa == 1) {
                if (DWCAc_CheckWpa(&candidates[prev]) == FALSE) {
                    swap = TRUE;
                }
            } else {
                swap = TRUE;
            }
        } else if (DWC_AC_ID_IS_HOTSPOT(infos[prev].type)) {
            swap = TRUE;
        }
        if (swap == TRUE) {
            ACFL tmpInfo;
            WMBssDesc tmpBss;

            MIi_CpuCopy32(&infos[prev], &tmpInfo, sizeof(ACFL));
            MIi_CpuCopy32(&infos[slot], &infos[prev], sizeof(ACFL));
            MIi_CpuCopy32(&tmpInfo, &infos[slot], sizeof(ACFL));
            MIi_CpuCopy32(&candidates[prev], &tmpBss, sizeof(WMBssDesc));
            MIi_CpuCopy32(&candidates[slot], &candidates[prev], sizeof(WMBssDesc));
            MIi_CpuCopy32(&tmpBss, &candidates[slot], sizeof(WMBssDesc));
            slot = prev;
        }
    }
    MI_CpuClear32(&infos[DWC_AC_CANDIDATE_MAX], sizeof(ACFL));
    MI_CpuClear32(&candidates[DWC_AC_CANDIDATE_MAX], sizeof(WMBssDesc));
}

// The wireless manager's notification callback of the search: an access point that was found goes to DWCi_AC_GetBeacon,
// and the reason that a connection failed to the work
void DWCi_AC_CallBackWCM(WCMNotice *notice) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    if (notice->notify == WCM_NOTIFY_CONNECT) {
        if (notice->result == 0) {
            return;
        }
        // The status code of the failed attempt, as the access point gave it: 13 is an authentication algorithm that it
        // does not support, 15 a challenge that failed, 17 that it cannot take another station
        switch (notice->parameter[1].n) {
        case 13:
            work->failureReason = 1;
            break;
        case 15:
            work->failureReason = 2;
            break;
        case 17:
            work->failureReason = 3;
            break;
        default:
            work->failureReason = 4;
            break;
        }
    } else if (notice->notify == WCM_NOTIFY_FOUND_AP) {
        DWCi_AC_GetBeacon(notice->parameter[0].p);
    }
}
