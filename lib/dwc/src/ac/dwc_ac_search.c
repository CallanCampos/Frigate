// DWC's auto-connect, the search for an access point (TWL-DWC 5.3, ac). The file name is a guess: its functions are the
// handlers of the states that dwc_ac.c dispatches below 7 (DWCi_AC_SearchAP, the search) and of state 9
// (DWCi_AC_ConnectRetryAP, the end of the connection to a candidate, which goes back to the search), and the scans
// they start. A round of the search has up to three passes: state 3 listens on every other channel for any access
// point, state 4 asks for each candidate found on its own channel, and state 5 asks for the SSID of each target on the
// channels where a beacon came with no SSID. The first round listens on the channels with an even index (channels 1, 3,
// ...), the second, which follows when the first finds nothing to connect to, on those with an odd one. State 6 is the
// end of the search. The file ends where dwc_ac_wepkey.c begins.

#include "dwc/ac.h"
#include "nitro/os.h"

// The options that a scan on one channel takes, by the index of the channel: the bit that says that the channel list
// is given, and the bit of the channel. A scan for any access point adds 0x200000 to it (WCM_UpdateOption's scan
// type), and one that asks for a given SSID or access point 0x300000
static const u32 DWC_AC_SEARCH_CHANNEL[13] = {
    0x8002, 0x8004, 0x8008, 0x8010, 0x8020, 0x8040, 0x8080, 0x8100, 0x8200, 0x8400, 0x8800, 0x9000, 0xa000,
};

// The options of the scan that asks for one candidate on its own channel (the scan of a target takes its channel's
// entry of the table with 0x300000)
// BUG: the cast to u16 cuts the 0x300000 off, and what is left is the channel's number less 1 and not its entry of the
// table: the wireless manager is told neither the channel nor the kind of scan, and keeps those of the scan before
#ifdef BUGFIX
#define CANDIDATE_SCAN_OPTION(channel) (DWC_AC_SEARCH_CHANNEL[(channel) - 1] | 0x300000)
#else
#define CANDIDATE_SCAN_OPTION(channel) ((u16)(((channel) - 1) | 0x300000))
#endif

static u8 NextSearchCheck(ACWORK *work, u8 state);
static u8 CheckStartStealthSearch(ACWORK *work);
static void DWCi_AC_SearchReStart(u8 state);
static u8 SearchStart(ACWORK *work);
static u8 SearchAround2ndLap(ACWORK *work);
static u8 SearchAround(ACWORK *work);
static u8 SearchDifferChannel(ACWORK *work);
static u8 SearchStealth(ACWORK *work);
static u8 SearchEndCheck(ACWORK *work, u8 state);
static s8 DWCAc_FirstChannel(BOOL second);
static s8 DWCAc_NextChannel(s8 channel);
static s8 DWCAc_NextHiddenChannel(void);

// Whether the wireless manager lets the channels of the mask be used
static inline BOOL IsChannelAllowed(u16 mask) {
    u32 allowed = WCM_GetChannelMask();

    if (allowed & mask) {
        return TRUE;
    }
    return FALSE;
}

// The handler of state 9, where the connection to a candidate is over: the wireless manager is brought back to idle
// (its state 3; 6 is a scan, 9 a connection) and then the search goes on from the state it was in, or the connection
// goes to the next target of a candidate that several matched (state 7). The wireless manager's states 11 and 12 are
// errors, and end everything. The state that follows
u8 DWCi_AC_ConnectRetryAP(void) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    u8 state = 9;

    switch (WCM_GetPhase()) {
    case 3:
        state = work->resumeState;
        if (work->sameSsid == 1) {
            work->findList[work->connectNo].state = 0;
            state = 7;
        } else if (state >= 3 && state <= 5) {
            DWCi_AC_SearchReStart(state);
        }
        break;
    case 6:
        WCM_EndSearchAsync();
        break;
    case 9:
        WCM_DisconnectAsync();
        break;
    case 12:
        WCM_TerminateAsync();
        DWCi_AC_SetError(4);
        state = 17;
        break;
    case 11:
        DWCi_AC_SetError(0);
        state = 17;
        break;
    }
    return state;
}

// The handler of the states 2 to 6, the search. In state 2, once the wireless manager is idle, it measures what the
// channels allow, sets how long a scan stays on a channel and starts the first scan, which is state 3. After that, as
// long as the wireless manager is idle or scanning, it connects to a candidate if one is good enough
// (SearchEndCheck), and otherwise lets the scan of the state at hand run its course. An error of the wireless
// manager ends everything. The state that follows
u8 DWCi_AC_SearchAP(void) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    u8 state = DWCi_AC_GetPhase();
    s32 wcmState = WCM_GetPhase();

    if (state == 2 && wcmState == 3) {
        u32 channels = WCM_GetChannelMask();
        int i;

        for (i = 0; i <= 12; i++) {
            if (channels & (0x1000 >> i)) {
                break;
            }
        }
        work->channelMax = 13 - i;
        if (OS_IsRunOnTwl()) {
            if (work->rssiPacked == 1) {
                work->scanTime = 300;
                work->directedTime = 150;
            } else {
                work->scanTime = 278;
                work->directedTime = 203;
            }
        } else {
            work->scanTime = 300;
            work->directedTime = 150;
        }
        state = SearchStart(work);
    } else if (state == 6) {
        state = SearchEndCheck(work, state);
    } else if (wcmState == 3 || wcmState == 6) {
        state = SearchEndCheck(work, state);
        if (state != 7) {
            if (state == 3) {
                state = SearchAround(work);
            } else if (state == 4) {
                state = SearchDifferChannel(work);
            } else if (state == 5) {
                state = SearchStealth(work);
            }
        }
    } else if (wcmState == 11) {
        DWCi_AC_SetError(0);
        return 17;
    } else if (wcmState > 11) {
        DWCi_AC_SetError(4);
        return 17;
    }
    return state;
}

// Notes that a beacon with no SSID came in on the channel (its number from 1), so that the third pass looks on it
void DWCi_AC_SetStealthChannel(u32 channel) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    if (channel > work->channelMax) {
        channel = work->channelMax;
    }
    work->stealthChannel = work->stealthChannel | (1 << (channel - 1));
}

// Starts the scan that a state of the search has on the channel it is at. State 3 listens for any access point, state
// 4 asks the candidate at hand on its own channel (and drops it if the wireless manager does not allow the channel),
// and state 5 asks for the SSID of the target at hand
static void DWCi_AC_SearchReStart(u8 state) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    switch (state) {
    case 3:
        work->timeOut = OS_GetTick();
        WCM_SearchAsync(WCM_Bssid_Any, WCM_Essid_Any, DWC_AC_SEARCH_CHANNEL[work->tag] | 0x200000);
        break;
    case 4:
        if (IsChannelAllowed(1 << (work->bssDesc[work->searchListNo].channel - 1)) == TRUE) {
            work->timeOut = OS_GetTick();
            WCM_SearchAsync(work->bssDesc[work->searchListNo].bssid, work->bssDesc[work->searchListNo].ssid,
                            CANDIDATE_SCAN_OPTION(work->bssDesc[work->searchListNo].channel));
        } else {
            work->findList[work->searchListNo].find = 1;
        }
        break;
    case 5:
        work->timeOut = OS_GetTick();
        WCM_SearchAsync(WCM_Bssid_Any, work->searchList[work->searchListNo].ssid,
                        DWC_AC_SEARCH_CHANNEL[work->tag] | 0x300000);
        break;
    }
}

// Starts the search, in state 2: the first channel to listen on, and the scan. State 3
static u8 SearchStart(ACWORK *work) {
    work->timeOut = OS_GetTick();
    work->tag = DWCAc_FirstChannel(FALSE);
    work->timeOut = OS_GetTick();
    WCM_SearchAsync(WCM_Bssid_Any, WCM_Essid_Any, DWC_AC_SEARCH_CHANNEL[work->tag] | 0x200000);
    return 3;
}

// Begins the second round of the search: the list of targets is made again, and the first channel to listen on is that
// with the index 1. State 3
static u8 SearchAround2ndLap(ACWORK *work) {
    work->attempts = 0;
    work->round++;
    DWCi_AC_MakeSearchList(0);
    work->tag = DWCAc_FirstChannel(TRUE);
    return 3;
}

// State 3: when the scan has had its time on the channel, goes on to the next channel of the pass, or to what follows
// the pass when there are no more
static u8 SearchAround(ACWORK *work) {
    if (OS_TicksToMilliSeconds(OS_GetTick() - work->timeOut) >= work->scanTime) {
        work->tag = DWCAc_NextChannel(work->tag);
        if (work->tag == -1) {
            return NextSearchCheck(work, 3);
        }
        work->timeOut = OS_GetTick();
        WCM_SearchAsync(WCM_Bssid_Any, WCM_Essid_Any, DWC_AC_SEARCH_CHANNEL[work->tag] | 0x200000);
    }
    return 3;
}

// State 4: when the scan for the candidate at hand has had its time or has found it, goes on to the next candidate
// that was not seen, or to what follows the pass when there are no more
static u8 SearchDifferChannel(ACWORK *work) {
    if (OS_TicksToMilliSeconds(OS_GetTick() - work->timeOut) >= work->directedTime ||
        work->findList[work->searchListNo].find == 1) {
        work->findList[work->searchListNo].find = 1;
        while (work->searchListNo < work->candidateCount) {
            if (!work->findList[work->searchListNo].find) {
                break;
            }
            work->searchListNo++;
        }
        if (work->candidateCount <= work->searchListNo) {
            work->searchListNo = 0;
            return NextSearchCheck(work, 4);
        }
        if (IsChannelAllowed(1 << (work->bssDesc[work->searchListNo].channel - 1)) == TRUE) {
            work->timeOut = OS_GetTick();
            WCM_SearchAsync(work->bssDesc[work->searchListNo].bssid, work->bssDesc[work->searchListNo].ssid,
                            CANDIDATE_SCAN_OPTION(work->bssDesc[work->searchListNo].channel));
        } else {
            work->findList[work->searchListNo].find = 1;
        }
    }
    return 4;
}

// State 5: when the scan for the target at hand has had its time or has found it, goes on to the next target, and
// when they are all done to the next channel where a hidden SSID was seen, or to what follows the pass
static u8 SearchStealth(ACWORK *work) {
    if (OS_TicksToMilliSeconds(OS_GetTick() - work->timeOut) >= work->directedTime ||
        work->searchList[work->searchListNo].find == 1) {
        work->searchList[work->searchListNo].find = 0;
        work->searchListNo++;
        if (work->searchListNum <= work->searchListNo) {
            work->attempts++;
            work->searchListNo = 0;
            work->tag = DWCAc_NextHiddenChannel();
        }
        if (work->tag < 0) {
            work->attempts = 0;
            return NextSearchCheck(work, 5);
        }
        work->timeOut = OS_GetTick();
        WCM_SearchAsync(WCM_Bssid_Any, work->searchList[work->searchListNo].ssid,
                        DWC_AC_SEARCH_CHANNEL[work->tag] | 0x300000);
    }
    return 5;
}

// A pass of the search is over: the state that follows it, whose scan it starts. After state 3 the candidates that were
// found are asked on their own channels (state 4) if there are any, and after that or in its place the channels with
// hidden SSIDs are looked into (state 5); when nothing is left the second round begins, or if it has been the search
// ends (state 6)
static u8 NextSearchCheck(ACWORK *work, u8 state) {
    switch (state) {
    case 3:
        if (work->candidateCount != 0 || work->stealthChannel != 0) {
            if (DWCi_AC_MakeSearchList(1) == TRUE) {
                state = 4;
            } else {
                state = CheckStartStealthSearch(work);
            }
        } else if (work->round >= 1) {
            state = 6;
        } else {
            state = SearchAround2ndLap(work);
        }
        break;
    case 4:
        state = CheckStartStealthSearch(work);
        break;
    case 5:
        if (work->round >= 1) {
            state = 6;
        } else {
            state = SearchAround2ndLap(work);
        }
        break;
    }
    DWCi_AC_SearchReStart(state);
    return state;
}

// Decides whether to connect to a candidate now. The first whose result is not set yet is taken when its signal is
// strong enough, it is not of a kind that waits for the end of the search (the ids of DWC_AC_ID_IS_HOTSPOT) and, if the
// settings prefer WPA, it can be taken for a connection with it. The scan is stopped, and the state is kept to go back
// to (resumeState); the state that follows is 7. In state 6, the end of the search, any candidate that is left is taken
// and there being none is an error: 5 when none was found, 6 when all were tried. The state that follows
static u8 SearchEndCheck(ACWORK *work, u8 state) {
    u8 i;
    u8 count;

    if (state == 17) {
        return state;
    }
    count = work->candidateCount;
    for (i = 0; i < count; i++) {
        if (work->findList[i].state == 0) {
            break;
        }
    }
    if (state == 6) {
        if (count == i) {
            if (i == 0) {
                DWCi_AC_SetError(5);
            } else {
                DWCi_AC_SetError(6);
            }
            return 17;
        }
    } else {
        if (count == 0) {
            return state;
        }
        if (count == i) {
            return state;
        }
        if (work->rssiPacked == 1) {
            if (work->findList[i].rssi < 0x14) {
                return state;
            }
        } else if (work->findList[i].rssi < 0x16) {
            return state;
        }
        if (DWC_AC_ID_IS_HOTSPOT(work->findList[i].type)) {
            return state;
        }
        if (work->preferWpa == 1) {
            if (!DWCAc_CheckWpa(&work->bssDesc[i])) {
                return state;
            }
        }
    }
    work->connectNo = i;
    // 1 is the answer when the wireless manager can't stop the scan
    if (WCM_EndSearchAsync() != 1) {
        work->resumeState = state;
        state = 7;
    }
    return state;
}

// What follows the pass of the candidates, or comes in place of it: the third pass, for the channels where a beacon
// with no SSID came, if the list of targets for it has any (DWCi_AC_MakeSearchList with 2); else the second round, or
// the end if it has been. The state that follows
static u8 CheckStartStealthSearch(ACWORK *work) {
    if (work->stealthChannel != 0 && DWCi_AC_MakeSearchList(2) == TRUE) {
        work->tag = DWCAc_NextHiddenChannel();
        if (work->tag >= 0) {
            return 5;
        }
        work->attempts = 0;
        return NextSearchCheck(work, 5);
    }
    if (work->round >= 1) {
        return 6;
    }
    return SearchAround2ndLap(work);
}

// The index of the first channel of a round that the wireless manager allows: the channels with an even index (0, 2,
// ...) for FALSE, those with an odd one for TRUE. If it allows none, the index the round starts at
static s8 DWCAc_FirstChannel(BOOL second) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    int channel;
    u16 mask;

    if (second == FALSE) {
        channel = 0;
        mask = 1;
    } else {
        channel = 1;
        mask = 2;
    }
    for (; channel <= work->channelMax; mask <<= 2, channel += 2) {
        if (IsChannelAllowed(mask) == TRUE) {
            break;
        }
    }
    if (channel > work->channelMax) {
        return second != FALSE;
    }
    return channel;
}

// The index of the channel of the round after the one given (two on) that the wireless manager allows, or -1
static s8 DWCAc_NextChannel(s8 channel) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    u16 mask;

    channel += 2;
    mask = 1 << channel;
    for (; channel <= work->channelMax; mask <<= 2, channel += 2) {
        if (IsChannelAllowed(mask) == TRUE) {
            break;
        }
    }
    if (channel > work->channelMax) {
        channel = -1;
    }
    return channel;
}

// The channel for the third pass: the one of the hidden ones that the pass is at (attempts counts how many it is
// past) if the wireless manager allows it, else the next. -1 when there is none
static s8 DWCAc_NextHiddenChannel(void) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    u16 hidden = work->stealthChannel;
    u8 i;
    u8 count;

    if (hidden == 0) {
        return -1;
    }
    for (i = 0, count = 0; i < work->channelMax; i++) {
        if (hidden & (1 << i)) {
            if (count == work->attempts) {
                if (IsChannelAllowed(1 << i) == TRUE) {
                    return i;
                }
                work->attempts++;
            }
            count++;
        }
    }
    return -1;
}
