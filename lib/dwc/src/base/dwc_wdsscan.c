#include "dwc/memfunc.h"
#include "dwc/wds.h"
#include "dwc/wdsscan.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// DWC's WDS scan (TWL-DWC 5.3's dwc_wdsscan.c, a guessed name; the ROM names none of it, and the file is ARM code in
// ARM9 main like the rest of the library's base): after the auto connect failed, the connection listens for the
// beacons of Nintendo Zones with the scanner of wds.c for three seconds, and keeps the access point whose signal was
// the strongest. Its steps are a state machine that dwc_connectinet.c polls; the wireless manager's callbacks only
// move the state on.

// How long the scan listens, in milliseconds
#define SCAN_TIME 3000

// Where the scan is. The ones that wait for a callback are named for what is in progress
enum {
    // Nothing started: start the wireless manager
    APP_STATE_WDSINIT,
    APP_STATE_WDSWAITINIT,
    // The wireless manager is up (or the time is not out): start a scan of the channels
    APP_STATE_WDSSCAN,
    APP_STATE_WDSWAITSCAN,
    // A scan took in its beacons: scan again until the time is out
    APP_STATE_WDSCOMPLETESCAN,
    // The time is out: stop the scan
    APP_STATE_WDSENDSCAN,
    APP_STATE_WDSWAITENDSCAN,
    // The scan is stopped: read its beacons, and end the wireless manager
    APP_STATE_WDSCOMPLETEENDSCAN,
    APP_STATE_WDSWAITEND,
    // The wireless manager is down: hand over the strongest access point
    APP_STATE_WDSCOMPLETEEND,
    // Something failed: end the wireless manager
    APP_STATE_ERRORENDPROCESS,
    // The scan has failed, which every step from here on reports
    APP_STATE_ERROREND,
};

// What the scan keeps besides the work of wds.c, 0x794 bytes
typedef struct WdsScanData {
    // Where the access point is put
    DWCWDSData *settings;
    u16 dmaNo;
    // The tick of the first scan
    u64 startTick;
    WDSBriefApInfo entries[WDS_MAX_ENTRIES];
    // The entry with the strongest signal, -1 if there is none
    int best;
} WdsScanData;

// The file's statics
typedef struct DWCWdsScan {
    WdsScanData *data;
    // The work of wds.c
    void *wds;
    int state;
} DWCWdsScan;

static DWCWdsScan sWdsScan;

static void WDS_Initialize_CB(void *arg);
static void WDS_StartScan_CB(void *arg);
static void WDS_EndScan_CB(void *arg);
static void WDS_End_CB(void *arg);
static void WDS_Error_End_CB(void *arg);

BOOL DWC_AC_StartupGetWDSInfo(DWCWDSData *settings, u16 dmaNo) {
    sWdsScan.wds = DWC_Alloc(DWC_ALLOCTYPE_AC, WDS_GetWorkAreaSize());
    sWdsScan.data = DWC_Alloc(DWC_ALLOCTYPE_AC, sizeof(WdsScanData));
    MI_CpuFill8(sWdsScan.data, 0, sizeof(WdsScanData));
    sWdsScan.data->settings = settings;
    sWdsScan.data->dmaNo = dmaNo;
    sWdsScan.state = APP_STATE_WDSINIT;
    return TRUE;
}

int DWC_AC_ProcessGetWDSInfo(void) {
    int i;
    u16 strongest;
    int result;

    if (sWdsScan.wds == NULL || sWdsScan.data == NULL) {
        return DWC_WDS_STATE_FAILED;
    }
    result = DWC_WDS_STATE_PROCESS;
    switch (sWdsScan.state) {
    case APP_STATE_WDSINIT:
        sWdsScan.state = APP_STATE_WDSWAITINIT;
        if (WDS_Initialize(sWdsScan.wds, WDS_Initialize_CB, sWdsScan.data->dmaNo) != 0) {
            sWdsScan.state = APP_STATE_ERROREND;
        }
        break;
    case APP_STATE_WDSSCAN:
        sWdsScan.state = APP_STATE_WDSWAITSCAN;
        if (WDS_StartScan(WDS_StartScan_CB) == 0) {
            if (sWdsScan.data->startTick == 0) {
                sWdsScan.data->startTick = OS_GetTick();
            }
        } else {
            sWdsScan.state = APP_STATE_ERRORENDPROCESS;
        }
        break;
    case APP_STATE_WDSCOMPLETESCAN:
        if (OS_TicksToMilliSeconds(OS_GetTick() - sWdsScan.data->startTick) < SCAN_TIME) {
            sWdsScan.state = APP_STATE_WDSSCAN;
        } else {
            sWdsScan.state = APP_STATE_WDSENDSCAN;
        }
        break;
    case APP_STATE_WDSENDSCAN:
        sWdsScan.state = APP_STATE_WDSWAITENDSCAN;
        if (WDS_EndScan(WDS_EndScan_CB) != 0) {
            sWdsScan.state = APP_STATE_ERRORENDPROCESS;
        }
        break;
    case APP_STATE_WDSCOMPLETEENDSCAN:
        if (WDS_GetApInfoAll(sWdsScan.data->entries) != 0) {
            sWdsScan.state = APP_STATE_ERRORENDPROCESS;
        }
        // Reads the beacons the scan took in, and picks the one of the strongest signal (the last, among equals) that
        // wds.c takes for a Nintendo Zone's. If they cannot be read, the entries are still the zeros the scan started
        // with, so none is picked; the state set for that failure is overwritten below
        i = 0;
        sWdsScan.data->best = -1;
        strongest = 0;
        for (; i < WDS_MAX_ENTRIES; i++) {
            if (WDS_IsBeaconInfoValid(&sWdsScan.data->entries[i].apinfo) && sWdsScan.data->entries[i].isvalid) {
                if (sWdsScan.data->entries[i].rssi >= strongest) {
                    sWdsScan.data->best = i;
                    strongest = sWdsScan.data->entries[i].rssi;
                }
            }
        }
        sWdsScan.state = APP_STATE_WDSWAITEND;
        if (WDS_End(WDS_End_CB) != 0) {
            sWdsScan.state = APP_STATE_ERROREND;
        }
        break;
    case APP_STATE_WDSCOMPLETEEND:
        if (sWdsScan.data->best < 0) {
            sWdsScan.state = APP_STATE_ERROREND;
            break;
        }
        // The wireless manager is down: the strongest access point becomes the settings to connect with, from what its
        // beacon gives
        MI_CpuCopy8(sWdsScan.data->entries[sWdsScan.data->best].apinfo.ssid, sWdsScan.data->settings->ssid,
                    sizeof(sWdsScan.data->settings->ssid));
        MI_CpuCopy8(sWdsScan.data->entries[sWdsScan.data->best].apinfo.wepkey, sWdsScan.data->settings->wep,
                    sizeof(sWdsScan.data->settings->wep));
        sWdsScan.data->settings->wepMode = sWdsScan.data->entries[sWdsScan.data->best].apinfo.encryptflag;
        sWdsScan.data->settings->mtu = sWdsScan.data->entries[sWdsScan.data->best].apinfo.mtu;
        MI_CpuCopy8(sWdsScan.data->entries[sWdsScan.data->best].apinfo.apnum, sWdsScan.data->settings->apnum,
                    sizeof(sWdsScan.data->settings->apnum));
        result = DWC_WDS_STATE_COMPLETED;
        break;
    case APP_STATE_ERRORENDPROCESS:
        sWdsScan.state = APP_STATE_WDSWAITEND;
        if (WDS_End(WDS_Error_End_CB) != 0) {
            sWdsScan.state = APP_STATE_ERROREND;
        }
        break;
    case APP_STATE_ERROREND:
        result = DWC_WDS_STATE_FAILED;
        break;
    }
    return result;
}

void DWCi_WdsScan_Stop(void) {
    if (sWdsScan.state == APP_STATE_WDSCOMPLETESCAN) {
        sWdsScan.state = APP_STATE_WDSENDSCAN;
    }
}

void DWC_AC_CleanupGetWDSInfo(void) {
    if (sWdsScan.wds != NULL) {
        DWC_Free(DWC_ALLOCTYPE_AC, sWdsScan.wds, 0);
        sWdsScan.wds = NULL;
    }
    if (sWdsScan.data != NULL) {
        DWC_Free(DWC_ALLOCTYPE_AC, sWdsScan.data, 0);
        sWdsScan.data = NULL;
    }
}

// The wireless manager's callbacks: the wireless manager is up, a scan has taken in its beacons, the scan is stopped,
// the wireless manager is ended after a scan, and after a failure
static void WDS_Initialize_CB(void *arg) {
    sWdsScan.state = APP_STATE_WDSSCAN;
}

static void WDS_StartScan_CB(void *arg) {
    sWdsScan.state = APP_STATE_WDSCOMPLETESCAN;
}

static void WDS_EndScan_CB(void *arg) {
    sWdsScan.state = APP_STATE_WDSCOMPLETEENDSCAN;
}

static void WDS_End_CB(void *arg) {
    sWdsScan.state = APP_STATE_WDSCOMPLETEEND;
}

static void WDS_Error_End_CB(void *arg) {
    sWdsScan.state = APP_STATE_ERROREND;
}
