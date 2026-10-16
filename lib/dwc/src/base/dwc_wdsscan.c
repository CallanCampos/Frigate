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

// The name of the scan's allocations (see dwc/memfunc.h)
#define MEM_NAME_WDS_SCAN 1

// What DWCi_WdsScan_Process returns
#define SCAN_RUNNING 1
#define SCAN_FOUND 2
#define SCAN_FAILED 3

// Where the scan is. The ones that wait for a callback are named for what is in progress
enum {
    // Nothing started: start the wireless manager
    SCAN_STATE_START,
    SCAN_STATE_STARTING,
    // The wireless manager is up (or the time is not out): start a scan of the channels
    SCAN_STATE_START_SCAN,
    SCAN_STATE_SCANNING,
    // A scan took in its beacons: scan again until the time is out
    SCAN_STATE_SCANNED,
    // The time is out: stop the scan
    SCAN_STATE_STOP_SCAN,
    SCAN_STATE_STOPPING_SCAN,
    // The scan is stopped: read its beacons, and end the wireless manager
    SCAN_STATE_STOPPED,
    SCAN_STATE_ENDING,
    // The wireless manager is down: hand over the strongest access point
    SCAN_STATE_ENDED,
    // Something failed: end the wireless manager
    SCAN_STATE_FAIL,
    // The scan has failed, which every step from here on reports
    SCAN_STATE_FAILED,
};

// What the scan keeps besides the work of wds.c, 0x794 bytes
typedef struct WdsScanData {
    // Where the access point is put
    DWCInetApSettings *settings;
    u16 dmaNo;
    // The tick of the first scan
    u64 startTick;
    WDSEntry entries[WDS_MAX_ENTRIES];
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

static void DWCi_WdsScan_OnInitialized(void *arg);
static void DWCi_WdsScan_OnScanned(void *arg);
static void DWCi_WdsScan_OnScanStopped(void *arg);
static void DWCi_WdsScan_OnEnded(void *arg);
static void DWCi_WdsScan_OnFailed(void *arg);

BOOL DWCi_WdsScan_Start(DWCInetApSettings *settings, u16 dmaNo) {
    sWdsScan.wds = DWC_Alloc(MEM_NAME_WDS_SCAN, WDS_GetWorkSize());
    sWdsScan.data = DWC_Alloc(MEM_NAME_WDS_SCAN, sizeof(WdsScanData));
    MI_CpuFill8(sWdsScan.data, 0, sizeof(WdsScanData));
    sWdsScan.data->settings = settings;
    sWdsScan.data->dmaNo = dmaNo;
    sWdsScan.state = SCAN_STATE_START;
    return TRUE;
}

int DWCi_WdsScan_Process(void) {
    int i;
    u16 strongest;
    int result;

    if (sWdsScan.wds == NULL || sWdsScan.data == NULL) {
        return SCAN_FAILED;
    }
    result = SCAN_RUNNING;
    switch (sWdsScan.state) {
    case SCAN_STATE_START:
        sWdsScan.state = SCAN_STATE_STARTING;
        if (WDS_Initialize(sWdsScan.wds, DWCi_WdsScan_OnInitialized, sWdsScan.data->dmaNo) != 0) {
            sWdsScan.state = SCAN_STATE_FAILED;
        }
        break;
    case SCAN_STATE_START_SCAN:
        sWdsScan.state = SCAN_STATE_SCANNING;
        if (WDS_StartScan(DWCi_WdsScan_OnScanned) == 0) {
            if (sWdsScan.data->startTick == 0) {
                sWdsScan.data->startTick = OS_GetTick();
            }
        } else {
            sWdsScan.state = SCAN_STATE_FAIL;
        }
        break;
    case SCAN_STATE_SCANNED:
        if (OS_TicksToMilliSeconds(OS_GetTick() - sWdsScan.data->startTick) < SCAN_TIME) {
            sWdsScan.state = SCAN_STATE_START_SCAN;
        } else {
            sWdsScan.state = SCAN_STATE_STOP_SCAN;
        }
        break;
    case SCAN_STATE_STOP_SCAN:
        sWdsScan.state = SCAN_STATE_STOPPING_SCAN;
        if (WDS_EndScan(DWCi_WdsScan_OnScanStopped) != 0) {
            sWdsScan.state = SCAN_STATE_FAIL;
        }
        break;
    case SCAN_STATE_STOPPED:
        if (WDS_GetEntries(sWdsScan.data->entries) != 0) {
            sWdsScan.state = SCAN_STATE_FAIL;
        }
        // Reads the beacons the scan took in, and picks the one of the strongest signal (the last, among equals) that
        // wds.c takes for a Nintendo Zone's. If they cannot be read, the entries are still the zeros the scan started
        // with, so none is picked; the state set for that failure is overwritten below
        i = 0;
        sWdsScan.data->best = -1;
        strongest = 0;
        for (; i < WDS_MAX_ENTRIES; i++) {
            if (WDS_IsBeaconInfoValid(&sWdsScan.data->entries[i].info) && sWdsScan.data->entries[i].valid) {
                if (sWdsScan.data->entries[i].signal >= strongest) {
                    sWdsScan.data->best = i;
                    strongest = sWdsScan.data->entries[i].signal;
                }
            }
        }
        sWdsScan.state = SCAN_STATE_ENDING;
        if (WDS_End(DWCi_WdsScan_OnEnded) != 0) {
            sWdsScan.state = SCAN_STATE_FAILED;
        }
        break;
    case SCAN_STATE_ENDED:
        if (sWdsScan.data->best < 0) {
            sWdsScan.state = SCAN_STATE_FAILED;
            break;
        }
        // The wireless manager is down: the strongest access point becomes the settings to connect with. Its beacon
        // gives them in the fields wds.h has not named yet: unk_00 is the SSID, unk_44 the WEP key, unk_65 the WEP
        // mode, unk_6c the MTU and unk_20 the spot information
        MI_CpuCopy8(sWdsScan.data->entries[sWdsScan.data->best].info.unk_00, sWdsScan.data->settings->ssid,
                    sizeof(sWdsScan.data->settings->ssid));
        MI_CpuCopy8(sWdsScan.data->entries[sWdsScan.data->best].info.unk_44, sWdsScan.data->settings->wepKey,
                    sizeof(sWdsScan.data->settings->wepKey));
        sWdsScan.data->settings->wepMode = sWdsScan.data->entries[sWdsScan.data->best].info.unk_65;
        sWdsScan.data->settings->mtu = sWdsScan.data->entries[sWdsScan.data->best].info.unk_6c;
        MI_CpuCopy8(sWdsScan.data->entries[sWdsScan.data->best].info.unk_20, sWdsScan.data->settings->spotInfo,
                    sizeof(sWdsScan.data->settings->spotInfo));
        result = SCAN_FOUND;
        break;
    case SCAN_STATE_FAIL:
        sWdsScan.state = SCAN_STATE_ENDING;
        if (WDS_End(DWCi_WdsScan_OnFailed) != 0) {
            sWdsScan.state = SCAN_STATE_FAILED;
        }
        break;
    case SCAN_STATE_FAILED:
        result = SCAN_FAILED;
        break;
    }
    return result;
}

void DWCi_WdsScan_Stop(void) {
    if (sWdsScan.state == SCAN_STATE_SCANNED) {
        sWdsScan.state = SCAN_STATE_STOP_SCAN;
    }
}

void DWCi_WdsScan_Free(void) {
    if (sWdsScan.wds != NULL) {
        DWC_Free(MEM_NAME_WDS_SCAN, sWdsScan.wds, 0);
        sWdsScan.wds = NULL;
    }
    if (sWdsScan.data != NULL) {
        DWC_Free(MEM_NAME_WDS_SCAN, sWdsScan.data, 0);
        sWdsScan.data = NULL;
    }
}

// The wireless manager's callbacks: the wireless manager is up, a scan has taken in its beacons, the scan is stopped,
// the wireless manager is ended after a scan, and after a failure
static void DWCi_WdsScan_OnInitialized(void *arg) {
    sWdsScan.state = SCAN_STATE_START_SCAN;
}

static void DWCi_WdsScan_OnScanned(void *arg) {
    sWdsScan.state = SCAN_STATE_SCANNED;
}

static void DWCi_WdsScan_OnScanStopped(void *arg) {
    sWdsScan.state = SCAN_STATE_STOPPED;
}

static void DWCi_WdsScan_OnEnded(void *arg) {
    sWdsScan.state = SCAN_STATE_ENDED;
}

static void DWCi_WdsScan_OnFailed(void *arg) {
    sWdsScan.state = SCAN_STATE_FAILED;
}
