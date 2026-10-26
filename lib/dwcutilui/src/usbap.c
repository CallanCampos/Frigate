#include "usbap.h"
#include "dwc/ext.h"
#include "heap.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "task.h"

static BOOL startScan(void);
static void cbWm(void *arg);
static void scanFirst(const WMStartScanExCallback *cb);
static void scanWait(const WMStartScanExCallback *cb);
static void taskFirst(DWCUtilTask *task, void *arg);

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
    // the ARM7's interrupt while DWCi_USBAPlEnd waits for it
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

BOOL DWCi_USBAPlBegin(DWCUtilUsbScanCallback callback) {
    OSOwnerInfo info;
    OSTick startTick;

    sUsbScan = DWCi_HEAPlAllocEx(sizeof(DWCUtilUsbScan), 32);
    sUsbScan->callback = callback;
    startTick = OS_GetTick();
    sUsbScan->startTick = startTick;
    // The work is read again for WM_Initialize, through a volatile read, as Black 1's version of this function writes
    // it; the ROM loads it again there where every plain form reuses the stored pointer
    if (WM_Initialize(*(DWCUtilUsbScan *volatile *)&sUsbScan, cbWm, 3) == WM_ERRCODE_OPERATING) {
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
        if (startScan()) {
            sUsbScan->task = DWCi_TSKlForm(0, taskFirst, NULL, 120);
            return TRUE;
        }
    }
    DWCi_HEAPlFree_(&sUsbScan);
    return FALSE;
}

static BOOL startScan(void) {
    if (WM_StartScanEx(cbWm, &sUsbScan->param) == WM_ERRCODE_OPERATING) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWCi_USBAPlEnd(void) {
    sUsbScan->stopping = TRUE;
    WM_ReadStatus(&sUsbScan->status);
    if (sUsbScan->status.state != WM_STATE_IDLE) {
        if (WM_Reset(cbWm) != WM_ERRCODE_OPERATING) {
            return FALSE;
        }
        do {
            WM_ReadStatus(&sUsbScan->status);
        } while (sUsbScan->status.state != WM_STATE_IDLE);
    }
    if (WM_End(cbWm) != WM_ERRCODE_OPERATING) {
        return FALSE;
    }
    if (sUsbScan->task != NULL) {
        DWCi_TSKlDeleteEx(0, sUsbScan->task);
    }
    while (sUsbScan->stopping != 2) {
    }
    DWCi_HEAPlFree_(&sUsbScan);
    return TRUE;
}

void DWCi_USBAPlSetCallback(DWCUtilUsbScanCallback callback) {
    sUsbScan->callback = callback;
}

// The callback of the wireless: the scan is started again after each channel, and takes the connectors found
static void cbWm(void *arg) {
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
                    scanWait(cb);
                } else {
                    scanFirst(cb);
                }
                startScan();
                break;
            case WM_STATECODE_PARENT_NOT_FOUND:
                startScan();
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
static void scanFirst(const WMStartScanExCallback *cb) {
    int i;
    WMBssDesc *desc;
    int j;
    DWCUtilUsbScan *scan;

    for (i = 0; i < cb->bssDescCount; i++) {
        desc = cb->bssDesc[i];

        DC_InvalidateRange(desc, sizeof(WMBssDesc));
        if (memcmp(desc->ssid, sUsbScanSsid, 8) == 0) {
            for (j = 0, scan = sUsbScan; j < USB_SCAN_CONNECTORS; j++) {
                if (memcmp(desc->bssid, scan->connectors[j].bssid, 6) == 0) {
                    if (!scan->connectors[j].ready && (desc->ssid[9] & 1)) {
                        if (scan->callback == NULL) {
                            return;
                        }
                        scan->callback(DWC_UTIL_USB_SCAN_FOUND);
                        return;
                    }
                    // The original goes on to the next connector here without the search for a free entry
                    goto next; // style-ok: goto-return skips the search for a free entry
                }
            }
            for (j = 0; j < USB_SCAN_CONNECTORS; j++) {
                if (memcmp(scan->connectors[j].bssid, sUsbScanNoBssid, 6) == 0) {
                    // The array is cast to its own element type: MWCC otherwise reuses the hoisted scan->connectors
                    // where the original computes the destination again
                    MI_CpuCopy8(desc->bssid, ((DWCUtilUsbConnector *)scan->connectors + j)->bssid, 6);
                    sUsbScan->connectors[j].ready = (desc->ssid[9] & 1) != 0;
                    break;
                }
            }
        }
    next:;
    }
}

// The scans after the first: a connector that didn't take the console and now does is the result
static void scanWait(const WMStartScanExCallback *cb) {
    int i;
    WMBssDesc *desc;
    DWCUtilUsbScan *scan;
    int count;
    s32 j;

    if (sUsbScan->pending) {
        if (sUsbScan->callback != NULL) {
            sUsbScan->callback(sUsbScan->pending);
        }
        return;
    }
    DC_InvalidateRange(sUsbScan->scanBuffer, sizeof(sUsbScan->scanBuffer));
    i = 0;
    count = cb->bssDescCount;
    if (count <= 0) {
        return;
    }
    scan = sUsbScan;
    do {
        desc = cb->bssDesc[i];

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
        i++;
    } while (i < count);
}

// The result of the first scan, once it has run for USB_SCAN_TIME: a connector that takes the console, some that do
// and some that don't, or none; with only connectors that don't, the scans go on
static void taskFirst(DWCUtilTask *task, void *arg) {
    DWCUtilUsbScan *scan;
    OSTick now = OS_GetTick();
    int i;
    _Bool ready, notReady;

    scan = sUsbScan;
    if (now >= scan->startTick + USB_SCAN_TIME) {
        notReady = FALSE;
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
        DWCi_TSKlDelete(0, task);
    }
}
