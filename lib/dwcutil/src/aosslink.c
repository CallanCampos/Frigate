#include "aosslink.h"
#include "apc.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "nitrowifi/wcm.h"

// The access point layer of the AOSS client (see aosslink.h): the scan for the access points, the connection to one
// with a WEP key, and the disconnection, over APC_*. A message queue turns the WCM's events into waits. The file is the
// library's aosslink.o (libdwcutil.TWL.HYB.a), which aletheia's map of Black 1's SDK code to the library's objects
// (layout/symbols_sdk.txt) shows with the same functions in the same order. The ROM names none of it; the names are
// the DWC utility library's own, as that library's symbols and debug information have them, where the code shows them,
// and ours elsewhere.
//
// MWCC 2.0 lays the statics out in the reverse of their definition order. The four globals are defined with an
// initializer, which lets the functions below reach them from one base.

// The messages of the alarm
#define WIFI_ALARM_SCAN 0x13
#define WIFI_ALARM_CONNECT 0x12

typedef struct WCMWepDesc {
    u8 mode;
    u8 keyId;
    u8 key[4][20];
    u8 padding[14];
} WCMWepDesc;

static void Set_WLLA_BSS_INFO(const WMBssDesc *bss, WLLA_BSS_INFO *ap);
static void Get_WLLA_SSID_PARAM(const WLLA_CONNECT_PARAM *param, WMBssDesc *bss);
static void Set_WLLA_LINK_STAT(WLLA_LINK_STAT *result, const WMBssDesc *bss, int success);
static void aoss_apc_callback(int event, int param);
static void alarmCallback(void *arg);

// 32-byte aligned, as the ROM places them, which also keeps AOSSi_WLANConnect's empty loop
static WCMWepDesc sWep ATTRIBUTE_ALIGN(32);
static WMBssDesc sBss ATTRIBUTE_ALIGN(32);
static OSMessageQueue sAossApQueue;
static OSMessage sAossApMessages[4];
DWCAossFreeFunc gAossApFree = NULL;
void *gAossApBlock = NULL;
void (*gAossApOnModeChange)(int mode) = NULL;
DWCAossAllocFunc gAossApAlloc = NULL;

// The WM's description of an access point to ours
static void Set_WLLA_BSS_INFO(const WMBssDesc *bss, WLLA_BSS_INFO *ap) {
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
    MIi_CpuCopy16(bss->ssid, ap->ssid, 0x20);
    ap->Channel = bss->channel;
    MIi_CpuCopy16(bss->bssid, ap->Bssid, 6);
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
    ap->BeaconPeriod = bss->beaconPeriod;
    if ((bss->capaInfo & 3) == 1) {
        ap->Capability = 1;
    } else if ((bss->capaInfo & 3) == 2) {
        ap->Capability = 2;
    } else {
        ap->Capability = 0;
    }
}

static void Get_WLLA_SSID_PARAM(const WLLA_CONNECT_PARAM *param, WMBssDesc *bss) {
    MI_CpuClear16(bss->ssid, 0x20);
    bss->ssidLength = param->ssidLength;
    MI_CpuCopy8(param->ssid, bss->ssid, bss->ssidLength);
}

static void Set_WLLA_LINK_STAT(WLLA_LINK_STAT *result, const WMBssDesc *bss, int success) {
    result->LinkStat = success;
    Set_WLLA_BSS_INFO(bss, &result->BssInfo);
}

static void aoss_apc_callback(int event, int param) {
    OS_SendMessage(&sAossApQueue, (OSMessage)event, 0);
}

static void alarmCallback(void *arg) {
    OS_SendMessage(&sAossApQueue, arg, 0);
}

int AOSSi_InitLocal(DWCAossAllocFunc alloc, DWCAossFreeFunc free) {
    BOOL ok = TRUE;
    OSMessage message;
    u32 mask;

    OS_InitMessageQueue(&sAossApQueue, sAossApMessages, 4);
    if (alloc == NULL || free == NULL) {
        return -1;
    }
    mask = OS_DisableInterrupts();
    gAossApAlloc = alloc;
    gAossApFree = free;
    OS_RestoreInterrupts(mask);
    gAossApBlock = gAossApAlloc(0x5890);
    if (gAossApBlock == NULL) {
        return -1;
    }
    if (APC_Init(aoss_apc_callback, gAossApBlock, 0x5890) == 0) {
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
    gAossApFree(gAossApBlock);
    return -1;
}

int AOSSi_EndLocal(void) {
    BOOL ok = TRUE;
    int result = -1;
    OSMessage message;
    u32 mask;

    if (gAossApFree == NULL) {
        return result;
    }
    if (APC_End() == 0) {
        return -1;
    }
    do {
        OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
        switch ((int)message) {
        case DWC_WCM_EVENT_CLEANED_UP:
            ok = FALSE;
            result = 0;
            gAossApFree(gAossApBlock);
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
    mask = OS_DisableInterrupts();
    gAossApAlloc = NULL;
    gAossApFree = NULL;
    OS_RestoreInterrupts(mask);
    return result;
}

int AOSSi_Disconnect(void) {
    int result = -1;
    OSMessage message;
    BOOL more;

    if (APC_Disconnect()) {
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

// Searches for access points and lists them; 0 if it ended well
int AOSSi_WLANGetBSSList(WLLA_BSS_LIST **list) {
    int result = -1;
    BOOL running = TRUE;
    BOOL started = FALSE;
    int count = 0;
    int waits = 0;
    OSMessage message;
    OSAlarm alarm;
    WMBssDesc *descs;
    // The start of the descriptions, for the free, while descs walks them
    WMBssDesc *buffer;

    if (gAossApAlloc == NULL || gAossApFree == NULL) {
        return -1;
    }
    descs = gAossApAlloc(0x3000);
    if (descs == NULL) {
        return -1;
    }
    buffer = descs;
    if (APC_SearchStart(NULL, NULL, 0, 0x30bffe)) {
        OS_CreateAlarm(&alarm);
        OS_SetAlarm(&alarm, 0x3fec42, alarmCallback, (void *)WIFI_ALARM_SCAN);
        do {
            OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
            switch ((int)message) {
            case WIFI_ALARM_SCAN:
                if (!started) {
                    if (waits != 0) {
                        count = APC_GetAPList(descs, 0x40);
                    }
                    if (APC_SearchEnd()) {
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
                        count = APC_GetAPList(descs, 0x40);
                        if (APC_SearchEnd()) {
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
            case WIFI_ALARM_CONNECT:
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
            WLLA_BSS_LIST *scanList;
            int i;

            if (count != 0) {
                scanList = gAossApAlloc(sizeof(WLLA_BSS_LIST) + (count - 1) * sizeof(WLLA_BSS_INFO));
                if (scanList == NULL) {
                    goto cancel;
                }
            } else {
                scanList = gAossApAlloc(sizeof(WLLA_BSS_LIST));
                if (scanList == NULL) {
                    goto cancel;
                }
            }
            *list = scanList;
            scanList->Num = count;
            for (i = 0; i < count; i++) {
                Set_WLLA_BSS_INFO(descs, &scanList->BssInfo[i]);
                descs++;
            }
        }
    cancel:
        OS_CancelAlarm(&alarm);
        while (OS_ReceiveMessage(&sAossApQueue, &message, 0) == 1) {
        }
    }
    gAossApFree(buffer);
    return result;
}

// Connects to the access point; 0 if it did
int AOSSi_WLANConnect(const WLLA_CONNECT_PARAM *param, WLLA_LINK_STAT *result) {
    BOOL running = TRUE;
    int status = -1;
    int connected = 0;
    int tries;
    u32 option;
    OSMessage message;
    OSAlarm alarm;
    int i;

    if (param->AuthAlgo == 0) {
        option = 1 << 19;
    } else if (param->AuthAlgo == 1) {
        option = 3 << 18;
    }
    option |= 3 << 16;
    MI_CpuFill8(&sWep, 0, sizeof(sWep));
    if (param->keyLength == 5) {
        sWep.mode = 1;
    } else if (param->keyLength == 13) {
        sWep.mode = 2;
    } else if (param->keyLength == 16) {
        sWep.mode = 3;
    } else {
        return -1;
    }
    sWep.keyId = connected;
    MI_CpuCopy8(param->key, sWep.key[connected], param->keyLength);
    WCM_ClearApList();
    if (APC_SearchStart(NULL, param->ssid, param->ssidLength, 0x30bffe)) {
        tries = 0;
        OS_CreateAlarm(&alarm);
        OS_SetAlarm(&alarm, 0x3fec42, alarmCallback, (void *)WIFI_ALARM_CONNECT);
        while (running) {
            OS_ReceiveMessage(&sAossApQueue, &message, OS_MESSAGE_BLOCK);
            switch ((int)message) {
            case WIFI_ALARM_CONNECT:
                if (!connected) {
                    running = FALSE;
                }
                break;
            case WIFI_ALARM_SCAN:
                // A debug print, compiled out: the scan's alarm has its own case here, where the scan puts the
                // connection's alarm among the events it ignores
                (void)0;
                break;
            case 4:
            case DWC_WCM_EVENT_SEARCHING:
                break;
            case DWC_WCM_EVENT_FOUND_AP:
                if (!connected) {
                    OS_CancelAlarm(&alarm);
                    if (APC_GetAPList(&sBss, 1) != 1) {
                        running = FALSE;
                        break;
                    }
                    Get_WLLA_SSID_PARAM(param, &sBss);
                    for (i = 0; i < sBss.ssidLength; i++) {
                    }
                    if (!APC_Connect(&sBss, &sWep, option)) {
                        running = FALSE;
                    } else {
                        connected = 1;
                    }
                }
                break;
            case DWC_WCM_EVENT_SEARCH_ENDED:
                Get_WLLA_SSID_PARAM(param, &sBss);
                if (!APC_Connect(&sBss, &sWep, option)) {
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
                    if (!APC_Connect(&sBss, &sWep, option)) {
                        running = FALSE;
                    }
                } else {
                    running = FALSE;
                }
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
            default:
                running = FALSE;
                break;
            }
        }
        OS_CancelAlarm(&alarm);
        while (OS_ReceiveMessage(&sAossApQueue, &message, 0) == 1) {
        }
    }
    Set_WLLA_LINK_STAT(result, &sBss, status == 0);
    return status;
}

void AOSSi_Sleep(u32 milliseconds) {
    OS_Sleep(milliseconds);
}

int AOSSi_Status(int mode) {
    if (gAossApOnModeChange != NULL) {
        gAossApOnModeChange(mode);
    }
    return 0;
}
