// DWC's auto-connect, the connection to the access point that the search chose (TWL-DWC 5.3, ac). The file name is a
// guess: its one global function runs the states 7 and 8, which dwc_ac.c dispatches, and the file ends where that
// file's public functions begin. State 7 settles the key that the access point at hand needs, state 8 connects to it
// and retries when it fails. The static functions are all called from those two.

#include "dwc/ac.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/wcm.h"

static u8 DWCAc_SettleKey(DWCAcSearchWork *work);
static u8 DWCAc_ConnectCandidate(DWCAcSearchWork *work);
static u8 DWCAc_FindCandidateTarget(DWCAcSearchWork *work);
static u32 DWCAc_GetPowerOption(const DWCAcSearchWork *work);
static u32 DWCAc_GetAuthOption(const DWCAcSearchWork *work);
static BOOL DWCAc_MakeKey(DWCAcSearchWork *work, u8 id, DWCAcKeyDesc *desc);

// Runs the state at hand if it is 7 or 8, and returns the state that follows
u8 DWCAc_StepConnect(void) {
    u8 state = DWCAc_GetState();
    DWCAcSearchWork *work = DWCAc_GetBlock(DWC_AC_WORK_SEARCH);

    switch (state) {
    case 7:
        state = DWCAc_SettleKey(work);
        break;
    case 8:
        state = DWCAc_ConnectCandidate(work);
        break;
    }
    return state;
}

// State 7: settles which target the candidate at hand belongs to, makes its key, and checks that the key and the access
// point fit each other: an access point that keeps its traffic private (the Privacy bit of its capabilities) needs a
// key and an open one does not, and a key makes the first try authenticate with the shared key. The candidate that does
// not fit gets the result "wrong key" and the search goes on to the next (state 9). An NWCUSBAP whose SSID has a zero
// at its tenth byte is passed over too, for a reason that the code does not show
static u8 DWCAc_SettleKey(DWCAcSearchWork *work) {
    WMBssDesc *bss = &work->candidates[work->selected];

    work->kind = DWCAc_FindCandidateTarget(work);
    MI_CpuFill8(&work->keyDesc, 0, sizeof(work->keyDesc));
    if (DWCAc_MakeKey(work, work->kind, &work->keyDesc)) {
        work->sharedKey = 1;
        if (!((bss->capaInfo >> 4) & 1)) {
            work->infos[work->selected].unk_00 = DWC_AC_RESULT_WRONG_KEY;
            return 9;
        }
        if (work->kind == DWC_AC_ID_NWCUSBAP && bss->ssid[9] == 0) {
            work->infos[work->selected].unk_00 = DWC_AC_RESULT_WRONG_KEY;
            return 9;
        }
    } else {
        work->sharedKey = 0;
        if (((bss->capaInfo >> 4) & 1) == 1) {
            work->infos[work->selected].unk_00 = DWC_AC_RESULT_WRONG_KEY;
            return 9;
        }
    }
    work->attempts = 0;
    work->failureReason = 0;
    return 8;
}

// State 8: while the wireless manager is idle, asks it to connect to the candidate, three tries at most, with the
// authentication and the power saving that the work says; the second and third tries look at why the last one failed
// (failureReason): a refused authentication algorithm drops the shared key for an open authentication, a wrong key or a
// full access point ends the candidate. When the connection is up it is state 10; when the wireless manager is in
// trouble, the error is noted and the teardown, state 17, begins
static u8 DWCAc_ConnectCandidate(DWCAcSearchWork *work) {
    s32 state = WCM_GetState();
    WMBssDesc *bss = &work->candidates[work->selected];
    u32 option;

    if (state == 3) {
        option = DWCAc_GetPowerOption(work);
        if (++work->attempts > 3) {
            work->attempts = 0;
            work->infos[work->selected].unk_00 = DWC_AC_RESULT_GAVE_UP;
            return 9;
        }
        if (work->attempts != 1) {
            if (work->failureReason == 1) {
                work->sharedKey = 0;
            } else if (work->failureReason == 2) {
                work->attempts = 0;
                work->infos[work->selected].unk_00 = DWC_AC_RESULT_WRONG_KEY;
                return 9;
            } else if (work->failureReason == 3) {
                work->attempts = 0;
                work->infos[work->selected].unk_00 = DWC_AC_RESULT_FULL;
                return 9;
            } else if (work->attempts == 3) {
                work->sharedKey = 0;
            }
        }
        WCM_ConnectAsync(bss, &work->keyDesc, option | DWCAc_GetAuthOption(work));
    } else if (state == 9) {
        work->attempts = 0;
        work->connectedTime = OS_GetTick();
        return 10;
    } else if (state == 11) {
        DWCAc_SetError(0);
        return 17;
    } else if (state > 11) {
        DWCAc_SetError(4);
        return 17;
    }
    return 8;
}

// Finds which target the candidate at hand is: an access point told by its SSID (NWCUSBAP, NDWCSHAP, FREESPOT), or the
// first target with the same SSID. When several targets have it, the first is taken and the others are marked, and
// the candidate is visited again for each of them. The id of the target, or 0
static u8 DWCAc_FindCandidateTarget(DWCAcSearchWork *work) {
    WMBssDesc *bss = &work->candidates[work->selected];
    int id = 0;
    int i;
    int matches = 0;
    u8 count = DWCAc_CountTargets();

    if (!work->sameSsid) {
        if (bss->ssidLength == 0x20) {
            id = DWCAc_GetSpecialApId(bss);
            if (id > 0) {
                matches++;
            } else {
                id = 0;
            }
        } else if (bss->ssidLength == 8) {
            id = DWCAc_GetFreeSpotId(bss);
            if (id != 0) {
                matches++;
            } else {
                id = 0;
            }
        }
        for (i = 0; i < count; i++) {
            if (bss->ssidLength == work->targets[i].ssidLength &&
                strncmp(bss->ssid, work->targets[i].ssid, bss->ssidLength) == 0) {
                if (matches == 0) {
                    id = work->targets[i].id;
                } else {
                    work->targets[i].pending = 1;
                    work->sameSsid = 1;
                }
                matches++;
            }
        }
    } else {
        for (i = 0, matches = 0; i < count; i++) {
            if (work->targets[i].pending == 1) {
                if (matches == 0) {
                    work->targets[i].pending = 0;
                    id = work->targets[i].id;
                }
                matches++;
            }
        }
        if (matches == 1) {
            work->sameSsid = 0;
        }
    }
    return id;
}

// The authentication and power saving of the connection, as the option of WCM_ConnectAsync: with the radio always on,
// or saving power
static u32 DWCAc_GetPowerOption(const DWCAcSearchWork *work) {
    return work->radioOn == 1 ? WCM_OPTION_POWER_ACTIVE : WCM_OPTION_POWER_SAVE;
}

// and with a shared key, or open
static u32 DWCAc_GetAuthOption(const DWCAcSearchWork *work) {
    return work->sharedKey == 1 ? WCM_OPTION_AUTH_SHARED : WCM_OPTION_AUTH_OPEN;
}

// Fills the key for WCM_ConnectAsync of the target `id`: 0 to 2 the WEP key of the three settings of the DS, 3 to 5
// their key of 40 bits, 6 to 8 the settings of the DSi, with their WPA key if they say they use WPA, 9 to 11 their key
// of 40 bits, 12 to 14 their WPA key with the mode 4 and 15 to 17 the other one with the mode 6, then NWCUSBAP and
// NDWCSHAP, whose key comes from the SSID. FREESPOT has none. TRUE if there is a key (the mode is not 0). The cases of
// a group fall through, one setting further on for each, to the one that the group starts with
static BOOL DWCAc_MakeKey(DWCAcSearchWork *work, u8 id, DWCAcKeyDesc *desc) {
    const DWCAcDsSetting *ds = work->dsSettings;
    const DWCAcDsiSetting *dsi = work->dsiSettings;

    switch (id) {
    case 2:
        ds++;
    case 1:
        ds++;
    case 0:
        desc->mode = ds->wepMode;
        MI_CpuCopy8(ds->wepKey, desc->key, 16);
        break;
    case 5:
        ds++;
    case 4:
        ds++;
    case 3:
        desc->mode = 1;
        MI_CpuCopy8(ds->wepKey40, desc->key, 5);
        break;
    case 8:
        dsi++;
    case 7:
        dsi++;
    case 6:
        if (dsi->base.security == 0x10 || dsi->base.security == 0x12 || dsi->base.security == 0x13) {
            desc->mode = dsi->wcmMode;
            desc->length = 32;
            MI_CpuCopy8(dsi->wpaKey, desc->key, 32);
            desc->key[32] = 0;
        } else {
            desc->mode = dsi->base.wepMode;
            MI_CpuCopy8(dsi->base.wepKey, desc->key, 16);
        }
        break;
    case 11:
        dsi++;
    case 10:
        dsi++;
    case 9:
        desc->mode = 1;
        MI_CpuCopy8(dsi->base.wepKey40, desc->key, 5);
        break;
    case 14:
        dsi++;
    case 13:
        dsi++;
    case 12:
        desc->mode = 4;
        desc->length = 32;
        MI_CpuCopy8(dsi->wpaKey, desc->key, 32);
        desc->key[32] = 0;
        break;
    case 17:
        dsi++;
    case 16:
        dsi++;
    case 15:
        desc->mode = 6;
        desc->length = 32;
        MI_CpuCopy8(dsi->wpaKey2, desc->key, 32);
        desc->key[32] = 0;
        break;
    case DWC_AC_ID_NWCUSBAP:
        desc->mode = 2;
        DWCAc_MakeNwcUsbApKey(work->candidates[work->selected].ssid, desc->key);
        break;
    case DWC_AC_ID_NDWCSHAP:
        desc->mode = 2;
        DWCAc_MakeNdwcShapKey(work->candidates[work->selected].ssid, desc->key);
        break;
    case DWC_AC_ID_FREESPOT:
        break;
    }
    return desc->mode != 0;
}
