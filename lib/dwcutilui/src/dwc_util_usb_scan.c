#include "dwc_util_usb_scan.h"
#include "dwc/ext.h"
#include "dwc_util_heap.h"
#include "dwc_util_task.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

static BOOL func_ov182_021b8934(void);
static void func_ov182_021b89fc(void *arg);
static void func_ov182_021b8a58(const WMStartScanExCallback *cb);
static void func_ov182_021b8b48(const WMStartScanExCallback *cb);
static void func_ov182_021b8c18(DWCUtilTask *task, void *arg);

#define USB_SCAN_CONNECTORS 16
// How long the first scan lists the connectors before it gives a result
#define USB_SCAN_TIME OS_MilliSecondsToTicks(3000)

// A connector that the scan found: its BSSID, and whether it takes a console, which bit 0 of the tenth byte of its SSID
// says
typedef struct DWCUtilUsbConnector {
    u8 bssid[6];
    u8 ready;
} DWCUtilUsbConnector;

typedef struct DWCUtilUsbScan {
    u8 wmBuffer[WM_SYSTEM_BUF_SIZE];
    u8 scanBuffer[0x400];
    DWCUtilUsbConnector connectors[USB_SCAN_CONNECTORS];
    DWCUtilUsbScanCallback callback;
    WMScanExParam param;
    WMStatus status;
    OSTick startTick;
    // The task that gives the result of the first scan
    DWCUtilTask *task;
    // TRUE while the wireless is stopped, 2 once WM_End's callback came: volatile, as the WM callback sets it from
    // the ARM7's interrupt while func_ov182_021b895c waits for it
    volatile u8 stopping;
    // A result found while there was no callback
    u8 pending;
    // Whether the first scan has given its result
    u8 listed;
    u8 unk_1b97[9];
} DWCUtilUsbScan;

// An unused BSSID, the start of the connector's SSID, and the scan's parameters: every channel, any BSSID, the SSIDs
// that start with the first 8 bytes of the console's own (NWCUSBAP, 1 in its tenth byte and the console's nickname)
static const u8 sUsbScanNoBssid[6] = { 0 };
static const u8 sUsbScanSsid[] = "NWCUSBAP";
static const WMScanExParam sUsbScanParam = {
    NULL, 0x400, 0x3fff, 0, { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 2, 32, { 0 }, 8,
};

static DWCUtilUsbScan *sUsbScan;

BOOL func_ov182_021b8834(DWCUtilUsbScanCallback callback) {
    OSOwnerInfo info;

    sUsbScan = DWCUtilHeap_AllocClear(sizeof(DWCUtilUsbScan), 32);
    sUsbScan->callback = callback;
    sUsbScan->startTick = OS_GetTick();
    if (WM_Initialize(sUsbScan, func_ov182_021b89fc, 3) == WM_ERRCODE_OPERATING) {
        do {
            WM_ReadStatus(&sUsbScan->status);
        } while (sUsbScan->status.state != WM_STATE_IDLE);
        sUsbScan->param = sUsbScanParam;
        sUsbScan->param.scanBuf = (WMBssDesc *)sUsbScan->scanBuffer;
        sUsbScan->param.maxChannelTime = WM_GetDispersionScanPeriod();
        OS_GetOwnerInfo(&info);
        MI_CpuCopy8(sUsbScanSsid, sUsbScan->param.ssid, 8);
        sUsbScan->param.ssid[9] = TRUE;
        MI_CpuCopy8(info.nickName, &sUsbScan->param.ssid[12], info.nickNameLength * 2);
        if (func_ov182_021b8934()) {
            sUsbScan->task = DWCUtilTask_Create(0, func_ov182_021b8c18, NULL, 120);
            return TRUE;
        }
    }
    DWCUtilHeap_FreeAndNull(&sUsbScan);
    return FALSE;
}

static BOOL func_ov182_021b8934(void) {
    if (WM_StartScanEx(func_ov182_021b89fc, &sUsbScan->param) == WM_ERRCODE_OPERATING) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov182_021b895c(void) {
    sUsbScan->stopping = TRUE;
    WM_ReadStatus(&sUsbScan->status);
    if (sUsbScan->status.state != WM_STATE_IDLE) {
        if (WM_Reset(func_ov182_021b89fc) != WM_ERRCODE_OPERATING) {
            return FALSE;
        }
        do {
            WM_ReadStatus(&sUsbScan->status);
        } while (sUsbScan->status.state != WM_STATE_IDLE);
    }
    if (WM_End(func_ov182_021b89fc) != WM_ERRCODE_OPERATING) {
        return FALSE;
    }
    if (sUsbScan->task != NULL) {
        DWCUtilTask_Remove(0, sUsbScan->task);
    }
    while (sUsbScan->stopping != 2) {
    }
    DWCUtilHeap_FreeAndNull(&sUsbScan);
    return TRUE;
}

void func_ov182_021b89e8(DWCUtilUsbScanCallback callback) {
    sUsbScan->callback = callback;
}

// The callback of the wireless: the scan is started again after each channel, and takes the connectors found
static void func_ov182_021b89fc(void *arg) {
    WMStartScanExCallback *cb = arg;

    if (cb->errcode == WM_ERRCODE_SUCCESS) {
        if (sUsbScan->stopping) {
            if (cb->apiid == WM_APIID_END) {
                sUsbScan->stopping = 2;
            }
        } else if (cb->apiid == WM_APIID_START_SCAN_EX) {
            switch (cb->state) {
            case WM_STATECODE_PARENT_FOUND:
                if (sUsbScan->listed) {
                    func_ov182_021b8b48(cb);
                } else {
                    func_ov182_021b8a58(cb);
                }
                func_ov182_021b8934();
                break;
            case WM_STATECODE_PARENT_NOT_FOUND:
                func_ov182_021b8934();
                break;
            default:
                OS_Terminate();
                break;
            }
        }
    }
}

// The first scan: keeps the connectors found, and gives the result at once when one that didn't take the console
// starts to
static void func_ov182_021b8a58(const WMStartScanExCallback *cb) {
    int i;
    int j;

    for (i = 0; i < cb->bssDescCount; i++) {
        WMBssDesc *desc = cb->bssDesc[i];

        DC_InvalidateRange(desc, sizeof(WMBssDesc));
        if (memcmp(desc->ssid, sUsbScanSsid, 8) == 0) {
            DWCUtilUsbScan *scan = sUsbScan;

            for (j = 0; j < USB_SCAN_CONNECTORS; j++) {
                if (memcmp(desc->bssid, scan->connectors[j].bssid, 6) == 0) {
                    if (!scan->connectors[j].ready && (desc->ssid[9] & 1)) {
                        if (scan->callback == NULL) {
                            return;
                        }
                        scan->callback(DWC_UTIL_USB_SCAN_FOUND);
                        return;
                    }
                    break;
                }
            }
            if (j == USB_SCAN_CONNECTORS) {
                for (j = 0; j < USB_SCAN_CONNECTORS; j++) {
                    if (memcmp(scan->connectors[j].bssid, sUsbScanNoBssid, 6) == 0) {
                        MI_CpuCopy8(desc->bssid, scan->connectors[j].bssid, 6);
                        sUsbScan->connectors[j].ready = (desc->ssid[9] & 1) != 0;
                        break;
                    }
                }
            }
        }
    }
}

// The scans after the first: a connector that didn't take the console and now does is the result
static void func_ov182_021b8b48(const WMStartScanExCallback *cb) {
    int i;
    DWCUtilUsbScan *scan;
    int count;
    int j;

    if (sUsbScan->pending) {
        if (sUsbScan->callback != NULL) {
            sUsbScan->callback(sUsbScan->pending);
        }
        return;
    }
    DC_InvalidateRange(sUsbScan->scanBuffer, sizeof(sUsbScan->scanBuffer));
    i = 0;
    count = cb->bssDescCount;
    scan = sUsbScan;
    for (; i < count; i++) {
        WMBssDesc *desc = cb->bssDesc[i];

        if (memcmp(desc->ssid, sUsbScanSsid, 8) == 0 && (desc->ssid[9] & 1)) {
            for (j = 0; j < USB_SCAN_CONNECTORS; j++) {
                if (memcmp(desc->bssid, scan->connectors[j].bssid, 6) == 0) {
                    if (!scan->connectors[j].ready) {
                        if (scan->callback != NULL) {
                            scan->callback(DWC_UTIL_USB_SCAN_FOUND);
                            return;
                        }
                        scan->pending = DWC_UTIL_USB_SCAN_FOUND;
                        return;
                    }
                    break;
                }
            }
        }
    }
}

// The result of the first scan, once it has run for USB_SCAN_TIME: a connector that takes the console, some that do
// and some that don't, or none; with only connectors that don't, the scans go on
static void func_ov182_021b8c18(DWCUtilTask *task, void *arg) {
    DWCUtilUsbScan *scan;
    OSTick now = OS_GetTick();
    BOOL ready;
    BOOL notReady;

    scan = sUsbScan;
    notReady = FALSE;
    if (now >= scan->startTick + USB_SCAN_TIME) {
        int i;

        ready = FALSE;

        for (i = 0; i < USB_SCAN_CONNECTORS; i++) {
            if (memcmp(scan->connectors[i].bssid, sUsbScanNoBssid, 6) != 0) {
                if (scan->connectors[i].ready) {
                    ready = TRUE;
                } else {
                    notReady = TRUE;
                }
            }
        }
        if (ready && notReady) {
            if (scan->callback != NULL) {
                scan->callback(DWC_UTIL_USB_SCAN_SEVERAL);
            }
        } else if (ready) {
            if (scan->callback != NULL) {
                scan->callback(DWC_UTIL_USB_SCAN_FOUND);
            }
        } else if (!notReady) {
            if (scan->callback != NULL) {
                scan->callback(DWC_UTIL_USB_SCAN_NONE);
            }
        }
        sUsbScan->task = NULL;
        sUsbScan->listed = TRUE;
        DWCUtilTask_QueueRemoval(0, task);
    }
}
