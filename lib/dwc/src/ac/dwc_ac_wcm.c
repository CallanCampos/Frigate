// DWC's auto-connect, the part that takes in the wireless manager's notifications (TWL-DWC 5.3, ac). The file name is a
// guess: the file starts at the function that the notification callback calls, and ends where the teardown starts, with
// no data or call across the boundary to prove either end. Its one object of data is the OUI and type of the WPA
// vendor element.

#include "dwc/ac.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// A candidate's place that nothing has been put in is all zeroes
#define IS_EMPTY(info) ((info).unk_00 + (info).id + (info).level + (info).tag + (info).seen == 0)

static void func_02058eb0(const WMBssDesc *bss);
static int func_0205914c(const WMBssDesc *bss, int count, const DWCAcTarget *targets);
static int func_020591c8(const WMBssDesc *bss, DWCAcSearchWork *work);
static int func_0205925c(int id, const WMBssDesc *bss, u8 tag, DWCAcSearchWork *work);
static void func_020593e4(u8 id, const WMBssDesc *bss, u8 tag, DWCAcSearchWork *work);
static void func_0205944c(int slot, const WMBssDesc *bss, u8 tag, DWCAcSearchWork *work);
static void func_020594d4(int slot, DWCAcSearchWork *work);

// Takes in one access point that the wireless manager found: from the state the search is in, it matches the beacon
// with a target (state 3, 5) or with a candidate that is kept (state 4), and adds or refreshes the candidate
static void func_02058eb0(const WMBssDesc *bss) {
    int id = -1;
    u8 tag;
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);
    DWCAcMain *control = func_0205a78c(DWC_AC_WORK_MAIN);

    control->unk_0b_0 = 1;
    switch (func_0205a83c()) {
    case 3:
        tag = work->tag;
        if (bss->ssidLength == 0 || bss->ssid[0] == 0) {
            func_0205c4e4(bss->channel);
        } else if (bss->ssidLength == 1 && bss->ssid[0] == ' ') {
            func_0205c4e4(bss->channel);
            id = func_0205914c(bss, work->targetCount, work->targets);
        } else {
            id = func_0205914c(bss, work->targetCount, work->targets);
        }
        break;
    case 4:
        tag = work->candidates[work->current].channel - 1;
        id = func_020591c8(bss, work);
        if (id >= 0) {
            work->infos[work->current].seen = 1;
        }
        break;
    case 5:
        tag = work->tag;
        id = func_0205914c(bss, 1, &work->targets[work->current]);
        if (id >= 0) {
            work->targets[work->current].found = 1;
        }
        break;
    default:
        return;
    }
    if (id >= 0) {
        func_020594d4(func_0205925c(id, bss, tag, work), work);
    }
}

// The id of the kind of access point that a beacon with a 32-byte SSID is, from the SSID and the kind the search wants
int func_02059028(const WMBssDesc *bss) {
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);

    if (work->mode == 0 || work->mode == 4) {
        if (((bss->capaInfo >> 4) & 1) == 1) {
            if (func_0205d074(bss->ssid) == TRUE) {
                return DWC_AC_ID_NWCUSBAP;
            }
        }
    }
    if (work->mode == 0 || work->mode == 5) {
        if (((bss->capaInfo >> 4) & 1) == 1) {
            if (func_0205cf80(bss->ssid) == TRUE) {
                return DWC_AC_ID_NDWCSHAP;
            }
        }
    }
    return -1;
}

// Whether the access point can be taken for a connection with WPA. A DS cannot tell, and takes it so
BOOL func_020590cc(const WMBssDesc *bss) {
    if (hw_isDSi() && func_02703208((NWMBssDesc *)bss, 0x30) == NULL) {
        u8 oui[4] = { 0x00, 0x50, 0xf2, 0x01 };

        if (func_02703250((NWMBssDesc *)bss, 0xdd, oui) == NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

// The id of the first of `count` targets whose SSID is the beacon's, or that of the kind of access point that the SSID
// shows, or -1
static int func_0205914c(const WMBssDesc *bss, int count, const DWCAcTarget *targets) {
    int i;
    u16 length;

    if (bss->ssidLength == 0x20) {
        int id = func_02059028(bss);

        if (id > 0) {
            return id;
        }
    }
    i = 0;
    if (count > 0) {
        length = bss->ssidLength;
        do {
            if ((u8)length == targets->ssidLength && strncmp(bss->ssid, targets->ssid, length) == 0) {
                return targets->id;
            }
            i++;
            targets++;
        } while (i < count);
    }
    return -1;
}

// The id of the candidate that has the beacon's SSID, or that of the kind of access point that the SSID shows, or -1
static int func_020591c8(const WMBssDesc *bss, DWCAcSearchWork *work) {
    int i;
    s16 count;
    u16 length;

    if (bss->ssidLength == 0x20) {
        int id = func_02059028(bss);

        if (id > 0) {
            return id;
        }
    }
    count = work->candidateCount;
    i = 0;
    if (count > 0) {
        length = bss->ssidLength;
        do {
            if (length == work->candidates[i].ssidLength && strncmp(bss->ssid, work->candidates[i].ssid, length) == 0) {
                return work->infos[i].id;
            }
            i++;
        } while (i < count);
    }
    return -1;
}

// Puts the beacon among the candidates, as a new one or in place of the one it matches, and gives its place: the last
// one, a place past the end of the list that func_020594d4 then sorts it from
static int func_0205925c(int id, const WMBssDesc *bss, u8 tag, DWCAcSearchWork *work) {
    int i;
    s16 count = work->candidateCount;
    int found = -1;

    for (i = 0; i < count; i++) {
        const u8 *bssid = work->candidates[i].bssid;

        if (bss->bssid[0] == bssid[0] && bss->bssid[1] == bssid[1] && bss->bssid[2] == bssid[2] &&
            bss->bssid[3] == bssid[3] && bss->bssid[4] == bssid[4] && bss->bssid[5] == bssid[5] &&
            strncmp(bss->ssid, work->candidates[i].ssid, bss->ssidLength) == 0) {
            found = i;
            break;
        }
    }

    if (found == -1) {
        func_020593e4(id, bss, tag, work);
        if (work->candidateCount < DWC_AC_CANDIDATE_MAX) {
            work->candidateCount++;
        }
        found = DWC_AC_CANDIDATE_MAX;
    } else {
        func_0205944c(found, bss, tag, work);
    }
    return found;
}

// Stores the beacon as a new candidate, in the place past the end of the list
static void func_020593e4(u8 id, const WMBssDesc *bss, u8 tag, DWCAcSearchWork *work) {
    DWCAcCandidate *info = &work->infos[DWC_AC_CANDIDATE_MAX];
    WMBssDesc *dest = &work->candidates[DWC_AC_CANDIDATE_MAX];
    u16 level = bss->rssi;

    info->id = id;
    if (work->rssiPacked == 1) {
        if (level & 2) {
            level = (u8)(level >> 2);
        } else {
            level = (u8)((level >> 2) + 0x19);
        }
    }
    info->level = level;
    info->tag = tag;
    WCM_FillApEntry(bss, dest);
}

// Refreshes a candidate with a beacon of the same access point: its signal and the tag, if the signal is stronger
static void func_0205944c(int slot, const WMBssDesc *bss, u8 tag, DWCAcSearchWork *work) {
    DWCAcCandidate *info = &work->infos[slot];
    WMBssDesc *dest = &work->candidates[slot];
    u16 level = bss->rssi;

    if (work->rssiPacked == 1) {
        if (level & 2) {
            level = (u8)(level >> 2);
        } else {
            level = (u8)((level >> 2) + 0x19);
        }
    }
    if ((u8)level > info->level) {
        info->level = level;
        info->tag = tag;
    }
    sys_memcpy32(bss, dest, sizeof(WMBssDesc));
}

// Moves the candidate in a place toward the front for as long as it is better than the one before, and then clears the
// place past the end of the list
static void func_020594d4(int slot, DWCAcSearchWork *work) {
    DWCAcCandidate *infos = work->infos;
    WMBssDesc *candidates = work->candidates;
    int prev;

    for (prev = slot - 1; prev >= 0; prev--) {
        BOOL swap = FALSE;

        if (work->preferWpa == 1 && func_020590cc(&candidates[slot]) == TRUE) {
            if (infos[slot].level < infos[prev].level) {
                if (func_020590cc(&candidates[prev]) == FALSE) {
                    swap = TRUE;
                }
            } else {
                swap = TRUE;
            }
        } else if (DWC_AC_ID_IS_HOTSPOT(infos[slot].id)) {
            if (infos[slot].level >= infos[prev].level) {
                // The one before is of a kind too, or is an empty place
                if (DWC_AC_ID_IS_HOTSPOT(infos[prev].id) || IS_EMPTY(infos[prev])) {
                    swap = TRUE;
                }
            }
        } else if (infos[slot].level >= infos[prev].level) {
            if (work->preferWpa == 1) {
                if (func_020590cc(&candidates[prev]) == FALSE) {
                    swap = TRUE;
                }
            } else {
                swap = TRUE;
            }
        } else if (DWC_AC_ID_IS_HOTSPOT(infos[prev].id)) {
            swap = TRUE;
        }
        if (swap == TRUE) {
            DWCAcCandidate tmpInfo;
            WMBssDesc tmpBss;

            sys_memcpy32(&infos[prev], &tmpInfo, sizeof(DWCAcCandidate));
            sys_memcpy32(&infos[slot], &infos[prev], sizeof(DWCAcCandidate));
            sys_memcpy32(&tmpInfo, &infos[slot], sizeof(DWCAcCandidate));
            sys_memcpy32(&candidates[prev], &tmpBss, sizeof(WMBssDesc));
            sys_memcpy32(&candidates[slot], &candidates[prev], sizeof(WMBssDesc));
            sys_memcpy32(&tmpBss, &candidates[slot], sizeof(WMBssDesc));
            slot = prev;
        }
    }
    sys_memset32(0, &infos[DWC_AC_CANDIDATE_MAX], sizeof(DWCAcCandidate));
    sys_memset32(0, &candidates[DWC_AC_CANDIDATE_MAX], sizeof(WMBssDesc));
}

// The wireless manager's notification callback of the search: an access point that was found goes to func_02058eb0,
// and the reason that a connection failed to the work
void func_020596c0(WCMNotice *notice) {
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);

    if (notice->notify == WCM_NOTIFY_CONNECT) {
        if (notice->result == 0) {
            return;
        }
        // The status code of the failed attempt, as the access point gave it: 13 is an authentication algorithm that it
        // does not support, 15 a challenge that failed, 17 that it cannot take another station
        switch (((DWCAcWcmNotice *)notice)->detail) {
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
        func_02058eb0((const WMBssDesc *)notice->parameter);
    }
}
