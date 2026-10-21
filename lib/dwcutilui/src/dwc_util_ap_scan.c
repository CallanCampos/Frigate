#include "dwc_util_ap_scan.h"
#include "dwc/ext.h"
#include "dwc_util_heap.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

static BOOL DWCUtilApScan_StartScan(void);
static void DWCUtilApScan_WmCallback(void *arg);
static void DWCUtilApScan_StoreResults(const WMStartScanExCallback *cb);

#define DWC_UTIL_AP_SCAN_MAX 20

typedef struct DWCUtilApScan {
    u8 wmBuffer[WM_SYSTEM_BUF_SIZE];
    u8 scanBuffer[0x400];
    DWCUtilApEntry entries[DWC_UTIL_AP_SCAN_MAX];
    WMScanExParam param;
    WMStatus status;
    u8 stopping;
} DWCUtilApScan;

// The OUI of an element of WPA (00:50:F2, type 1), an unused BSSID, and the scan's parameters: every channel, any
// BSSID
static const u8 sApScanWpaOui[4] = { 0x00, 0x50, 0xf2, 0x01 };
static const u8 sApScanNoBssid[6] = { 0 };
static const WMScanExParam sApScanParam = {
    NULL, 0x400, 0x3fff, 0, { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
};

static DWCUtilApScan *sApScan;

void DWCUtilApScan_Init(void) {
    if (sApScan == NULL) {
        sApScan = DWCUtilHeap_AllocClear(sizeof(DWCUtilApScan), 32);
    }
}

void DWCUtilApScan_Exit(void) {
    if (sApScan != NULL) {
        while (WM_GetAllowedChannel() != 0x8000) {
        }
        DWCUtilHeap_FreeAndNull(&sApScan);
    }
}

BOOL DWCUtilApScan_Start(void) {
    MI_CpuClear16(sApScan->entries, sizeof(sApScan->entries));
    if (WM_Initialize(sApScan, DWCUtilApScan_WmCallback, 3) != WM_ERRCODE_OPERATING) {
        return FALSE;
    }
    do {
        WM_ReadStatus(&sApScan->status);
    } while (sApScan->status.state != WM_STATE_IDLE);
    sApScan->param = sApScanParam;
    sApScan->param.scanBuf = (WMBssDesc *)sApScan->scanBuffer;
    sApScan->param.maxChannelTime = WM_GetDispersionScanPeriod();
    if (DWCUtilApScan_StartScan()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL DWCUtilApScan_StartScan(void) {
    if (WM_StartScanEx(DWCUtilApScan_WmCallback, &sApScan->param) == WM_ERRCODE_OPERATING) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWCUtilApScan_Stop(void) {
    sApScan->stopping = TRUE;
    WM_ReadStatus(&sApScan->status);
    if (sApScan->status.state != WM_STATE_IDLE) {
        if (WM_Reset(DWCUtilApScan_WmCallback) != WM_ERRCODE_OPERATING) {
            return FALSE;
        }
        do {
            WM_ReadStatus(&sApScan->status);
        } while (sApScan->status.state != WM_STATE_IDLE);
    }
    if (WM_End(DWCUtilApScan_WmCallback) != WM_ERRCODE_OPERATING) {
        return FALSE;
    }
    return TRUE;
}

int DWCUtilApScan_GetEntries(DWCUtilApEntry **entries) {
    int count = 0;
    DWCUtilApEntry *list = sApScan->entries;
    int i;

    *entries = list;
    for (i = 0; i < DWC_UTIL_AP_SCAN_MAX; i++) {
        if (memcmp(list[i].bssid, sApScanNoBssid, 6) != 0) {
            count++;
        }
    }
    return count;
}

// The callback of the wireless: the scan is started again after each channel, and takes the access points found
static void DWCUtilApScan_WmCallback(void *arg) {
    WMStartScanExCallback *cb = arg;

    if (cb->errcode == WM_ERRCODE_SUCCESS && !sApScan->stopping && cb->apiid == WM_APIID_START_SCAN_EX) {
        switch (cb->state) {
        case WM_STATECODE_PARENT_FOUND:
            DWCUtilApScan_StoreResults(cb);
            DWCUtilApScan_StartScan();
            break;
        case WM_STATECODE_PARENT_NOT_FOUND:
            DWCUtilApScan_StartScan();
            break;
        default:
            OS_Terminate();
            break;
        }
    }
}

// Keeps the access points found that have an SSID and aren't a game's, in the entry of their BSSID or a free one
static void DWCUtilApScan_StoreResults(const WMStartScanExCallback *cb) {
    DWCUtilApEntry *list = sApScan->entries;
    WMOtherElements elements;
    WMBssDesc *desc;
    DWCUtilApEntry *entry;
    int count;
    int i;
    int j;

    DC_InvalidateRange(sApScan->scanBuffer, sizeof(sApScan->scanBuffer));
    for (i = 0; i < cb->bssDescCount; i++) {
        desc = cb->bssDesc[i];
        if (desc->ssid[0] == 0 || desc->gameInfoLength != 0) {
            continue;
        }
        for (j = 0; j < DWC_UTIL_AP_SCAN_MAX; j++) {
            if (memcmp(desc->bssid, list[j].bssid, 6) == 0) {
                break;
            }
        }
        if (j == DWC_UTIL_AP_SCAN_MAX) {
            for (j = 0; j < DWC_UTIL_AP_SCAN_MAX; j++) {
                if (memcmp(list[j].bssid, sApScanNoBssid, 6) == 0) {
                    break;
                }
            }
            if (j == DWC_UTIL_AP_SCAN_MAX) {
                return;
            }
        }
        entry = &list[j];
        MI_CpuCopy8(desc->bssid, entry->bssid, 6);
        MI_CpuCopy8(desc->ssid, entry->ssid, 32);
        entry->linkLevel = cb->linkLevel[i];
        if (!(desc->capaInfo & 0x10)) {
            entry->security = DWC_UTIL_AP_OPEN;
        } else {
            entry->security = DWC_UTIL_AP_WEP;
            elements = WM_GetOtherElements(desc);
            count = elements.count;
            for (j = 0; j < count; j++) {
                if (elements.element[j].id == 0x30) {
                    entry->security = DWC_UTIL_AP_WPA;
                    break;
                }
                if (elements.element[j].id == 0xdd && elements.element[j].length >= 4 &&
                    memcmp(elements.element[j].data, sApScanWpaOui, 4) == 0) {
                    entry->security = DWC_UTIL_AP_WPA;
                    break;
                }
            }
        }
    }
}
