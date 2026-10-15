#include "dwc_aoss_ap.h"
#include "dwc/ext.h"
#include "dwc_wcm.h"
#include "gfl/std.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "nitrowifi/wcm.h"

// The access point layer of the AOSS client (see dwc_aoss_ap.h): the scan for the access points, the connection to one
// with a WEP key, and the disconnection, over DWC_Wcm_*. A message queue turns the WCM's events into waits. The ROM
// names none of it; the names are ours.
//
// MWCC 2.0 lays the statics out in the reverse of their definition order

// The messages of the alarm
#define WIFI_ALARM_SCAN 0x13
#define WIFI_ALARM_CONNECT 0x12

typedef struct WifiWepDesc {
    u8 mode;
    u8 keyId;
    u8 key[4][20];
    u8 padding[14];
} WifiWepDesc;

static void Wifi_ConvertBss(const WMBssDesc *bss, WifiApInfo *ap);
static void Wifi_MakeBss(const WifiConnectParam *param, WMBssDesc *bss);
static void Wifi_ReportConnect(WifiConnectResult *result, const WMBssDesc *bss, int success);
static void Wifi_OnEvent(int event, int param);
static void Wifi_OnAlarm(void *arg);

static WifiWepDesc sWep;
static WMBssDesc sBss;
static OSMessageQueue sAossApQueue;
static OSMessage sAossApMessages[4];
DWCAossFreeFunc gAossApFree;
AossApWork gAossAp;

// The WM's description of an access point to ours
static void Wifi_ConvertBss(const WMBssDesc *bss, WifiApInfo *ap) {
    static struct {
        u16 mask;
        u8 rate;
    } sRates[12] = {
        { 0x0001, 0x02 }, { 0x0002, 0x04 }, { 0x0004, 0x0b }, { 0x0008, 0x0c }, { 0x0010, 0x12 }, { 0x0020, 0x16 },
        { 0x0040, 0x18 }, { 0x0080, 0x24 }, { 0x0100, 0x30 }, { 0x0200, 0x48 }, { 0x0400, 0x60 }, { 0x0800, 0x6c },
    };
    int i;
    int count;

    ap->ssidLength = bss->ssidLength;
    sys_memcpy16(bss->ssid, ap->ssid, 0x20);
    ap->channel = bss->channel;
    sys_memcpy16(bss->bssid, ap->bssid, 6);
    count = 0;
    for (i = 0; i < 12; i++) {
        if (bss->rateSetSupport & sRates[i].mask) {
            ap->rates[count] = sRates[i].rate;
            if (bss->rateSetBasic & sRates[i].mask) {
                ap->rates[count] |= 0x80;
            }
            count++;
        }
    }
    ap->rateCount = count;
    ap->beaconPeriod = bss->beaconPeriod;
    if ((bss->capaInfo & 3) == 1) {
        ap->type = 1;
    } else if ((bss->capaInfo & 3) == 2) {
        ap->type = 2;
    } else {
        ap->type = 0;
    }
}

static void Wifi_MakeBss(const WifiConnectParam *param, WMBssDesc *bss) {
    MI_CpuClear16(bss->ssid, 0x20);
    bss->ssidLength = param->ssidLength;
    sys_memcpy(param->ssid, bss->ssid, bss->ssidLength);
}

static void Wifi_ReportConnect(WifiConnectResult *result, const WMBssDesc *bss, int success) {
    result->success = success;
    Wifi_ConvertBss(bss, &result->ap);
}

static void Wifi_OnEvent(int event, int param) {
    OS_SendMessage(&sAossApQueue, (OSMessage)event, 0);
}

static void Wifi_OnAlarm(void *arg) {
    OS_SendMessage(&sAossApQueue, arg, 0);
}

int DWC_Aoss_Init(DWCAossAllocFunc alloc, DWCAossFreeFunc free) {
    BOOL ok = TRUE;
    OSMessage message;
    u32 mask;

    func_0207acc4(&sAossApQueue, sAossApMessages, 4);
    if (alloc == NULL || free == NULL) {
        return -1;
    }
    mask = CPU_IRQDisable();
    gAossAp.alloc = alloc;
    gAossApFree = free;
    CPU_SetIRQMask(mask);
    gAossAp.block = gAossAp.alloc(0x5890);
    if (gAossAp.block == NULL) {
        return -1;
    }
    if (DWC_Wcm_Start(Wifi_OnEvent, gAossAp.block, 0x5890) == 0) {
        ok = FALSE;
    }
    if (ok) {
        do {
            OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
            switch ((int)message) {
            case DWC_WCM_EVENT_STARTED:
                return 0;
            case 0:
            case 1:
            case 2:
            case 3:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            default:
                ok = FALSE;
            case 4:
            case 5:
                break;
            }
        } while (ok);
    }
    gAossApFree(gAossAp.block);
    return -1;
}

int DWC_Aoss_Shutdown(void) {
    BOOL ok = TRUE;
    int result = -1;
    OSMessage message;
    u32 mask;

    if (gAossApFree == NULL) {
        return result;
    }
    if (DWC_Wcm_Cleanup() == 0) {
        return -1;
    }
    do {
        OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
        switch ((int)message) {
        case DWC_WCM_EVENT_CLEANED_UP:
            ok = FALSE;
            result = 0;
            gAossApFree(gAossAp.block);
            break;
        case 0:
        case 1:
        case 2:
        case 3:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        default:
            ok = FALSE;
        case 4:
        case 5:
            break;
        }
    } while (ok);
    mask = CPU_IRQDisable();
    gAossAp.alloc = NULL;
    gAossApFree = NULL;
    CPU_SetIRQMask(mask);
    return result;
}

int AossAp_Disconnect(void) {
    int result = -1;
    OSMessage message;
    BOOL more;

    if (DWC_Wcm_Disconnect()) {
        do {
            OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
            if ((int)message == DWC_WCM_EVENT_DISCONNECTED) {
                more = FALSE;
                result = 0;
            } else {
                more = FALSE;
            }
        } while (more);
    }
    return result;
}

static // Searches for access points and lists them; 0 if it ended well
    int
    AossAp_Scan(WifiScanList **list) {
    int result = -1;
    BOOL running = TRUE;
    BOOL started = FALSE;
    int count = 0;
    int waits = 0;
    OSMessage message;
    OSAlarm alarm;
    WMBssDesc *descs;

    if (gAossAp.alloc == NULL || gAossApFree == NULL) {
        return -1;
    }
    descs = gAossAp.alloc(0x3000);
    if (descs == NULL) {
        return -1;
    }
    if (DWC_Wcm_Search(NULL, NULL, 0, 0x30bffe)) {
        func_0207bc24(&alarm);
        func_0207bcf4(&alarm, 0x3fec42, Wifi_OnAlarm, (void *)WIFI_ALARM_SCAN);
        do {
            OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
            switch ((int)message) {
            case WIFI_ALARM_SCAN:
                if (!started) {
                    if (waits != 0) {
                        count = DWC_Wcm_GetApList(descs, 0x40);
                    }
                    if (DWC_Wcm_EndSearch()) {
                        started = TRUE;
                    } else {
                        goto cancel;
                    }
                }
                break;
            case DWC_WCM_EVENT_FOUND_AP:
                if (!started) {
                    if (waits < 8) {
                        waits++;
                    } else {
                        count = DWC_Wcm_GetApList(descs, 0x40);
                        if (DWC_Wcm_EndSearch()) {
                            started = TRUE;
                        } else {
                            goto cancel;
                        }
                    }
                }
                break;
            case DWC_WCM_EVENT_SEARCH_ENDED:
                running = FALSE;
                result = 0;
                break;
            case 4:
            case DWC_WCM_EVENT_SEARCHING:
            case 18:
                break;
            case 0:
            case 1:
            case 2:
            case 3:
            case 6:
            case 7:
            case 9:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            default:
                goto cancel;
            }
        } while (running);
        {
            WifiScanList *scanList;
            int i;

            if (count != 0) {
                scanList = gAossAp.alloc(sizeof(WifiScanList) + (count - 1) * sizeof(WifiApInfo));
                if (scanList == NULL) {
                    goto cancel;
                }
            } else {
                scanList = gAossAp.alloc(sizeof(WifiScanList));
                if (scanList == NULL) {
                    goto cancel;
                }
            }
            *list = scanList;
            scanList->count = count;
            for (i = 0; i < count; i++) {
                Wifi_ConvertBss(&descs[i], &scanList->ap[i]);
            }
        }
    cancel:
        func_0207bd3c(&alarm);
        while (OS_ReceiveMessage(&sAossApQueue, &message, 0) == 1) {
        }
    }
    gAossApFree(descs);
    return result;
}

static // Connects to the access point; 0 if it did
    int
    AossAp_Connect(const WifiConnectParam *param, WifiConnectResult *result) {
    BOOL running = TRUE;
    int status = -1;
    int connected = 0;
    int tries = 0;
    u32 option;
    OSMessage message;
    OSAlarm alarm;
    int i;

    if (param->authMode == 0) {
        option = 1 << 19;
    } else if (param->authMode == 1) {
        option = 3 << 18;
    }
    option |= 3 << 16;
    sys_memset(&sWep, 0, sizeof(sWep));
    switch (param->keyLength) {
    case 5:
        sWep.mode = 1;
        break;
    case 13:
        sWep.mode = 2;
        break;
    case 16:
        sWep.mode = 3;
        break;
    default:
        return -1;
    }
    sWep.keyId = connected;
    sys_memcpy(param->key, sWep.key[connected], param->keyLength);
    WCM_ClearApListBuffer();
    if (DWC_Wcm_Search(NULL, param->ssid, param->ssidLength, 0x30bffe)) {
        func_0207bc24(&alarm);
        func_0207bcf4(&alarm, 0x3fec42, Wifi_OnAlarm, (void *)WIFI_ALARM_CONNECT);
        while (running) {
            OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
            switch ((int)message) {
            case 4:
            case DWC_WCM_EVENT_SEARCHING:
                break;
            case DWC_WCM_EVENT_FOUND_AP:
                if (!connected) {
                    func_0207bd3c(&alarm);
                    if (DWC_Wcm_GetApList(&sBss, 1) != 1) {
                        running = FALSE;
                        break;
                    }
                    Wifi_MakeBss(param, &sBss);
                    for (i = 0; i < sBss.ssidLength; i++) {
                    }
                    if (!DWC_Wcm_Connect(&sBss, &sWep, option)) {
                        running = FALSE;
                    } else {
                        connected = 1;
                    }
                }
                break;
            case DWC_WCM_EVENT_SEARCH_ENDED:
                Wifi_MakeBss(param, &sBss);
                if (!DWC_Wcm_Connect(&sBss, &sWep, option)) {
                    running = FALSE;
                }
                break;
            case DWC_WCM_EVENT_CONNECTED:
                running = FALSE;
                status = 0;
                break;
            case DWC_WCM_EVENT_CONNECT_FAILED:
                tries++;
                if (tries < 3) {
                    if (!DWC_Wcm_Connect(&sBss, &sWep, option)) {
                        running = FALSE;
                    }
                } else {
                    running = FALSE;
                }
                break;
            case 18:
                if (!connected) {
                    running = FALSE;
                }
                break;
            case 19:
                break;
            case 0:
            case 1:
            case 2:
            case 3:
            case 6:
            case 7:
            case 9:
            case 11:
            case 14:
            case 15:
            case 16:
            case 17:
                running = FALSE;
                break;
            default:
                running = FALSE;
                break;
            }
        }
        func_0207bd3c(&alarm);
        while (OS_ReceiveMessage(&sAossApQueue, &message, 0) == 1) {
        }
    }
    Wifi_ReportConnect(result, &sBss, status == 0);
    return status;
}

void AossAp_Sleep(u32 milliseconds) {
    func_0207aa04(milliseconds);
}

int AossAp_NotifyModeChange(int mode) {
    if (gAossAp.onModeChange != NULL) {
        gAossAp.onModeChange(mode);
    }
    return 0;
}
