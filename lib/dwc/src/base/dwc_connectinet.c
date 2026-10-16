#include "dwc/ac.h"
#include "dwc/apinfo.h"
#include "dwc/connectinet.h"
#include "dwc/error.h"
#include "dwc/memfunc.h"
#include "dwc/wds.h"
#include "dwc/wdsscan.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/cps.h"
#include "nitrowifi/wcm.h"

// DWC's connection to the internet (TWL-DWC 5.3's dwc_connectinet.c, a guessed name; the ROM names none of it, and the
// file is ARM code in ARM9 main like the rest of the library's base): the control that the game gives the library, the
// connection through the auto connect (AC) and the WDS scan, and the access point information that goes with it. The
// layouts are in dwc/connectinet.h and dwc/apinfo.h; the SDK's names for the functions are in the comments there.

// The kinds of access point (DWCApInfo.apType): one that DWC does not know, the AC's answer when it has none, and the
// two of Nintendo's that carry the area and spot information: a Nintendo Wi-Fi Connection spot, and a Nintendo Zone
// that the WDS scan found
#define AP_TYPE_UNKNOWN 99
#define AP_TYPE_NONE 0xff
#define AP_TYPE_NINTENDO_WFC 4
#define AP_TYPE_WDS 8
// The area of a spot that tells none
#define AP_AREA_UNKNOWN 0xff

// The sizes of an access point's names
#define BSSID_SIZE 6
#define SSID_SIZE 32

// The TCP receive windows that a DSi may ask for
#define RWIN_MIN 0x10c0
#define RWIN_MAX 0xffff

// The AC's status (DWCAc_GetStatus): 5 once it is connected, 0 or more while it works and a negative number for an
// error. The errors from -10 up are its own failures
#define AC_STATUS_CONNECTED 5
#define AC_STATUS_OWN_ERROR_MIN -10
// The code of the fatal error that reports those failures, and a connection that cannot start
#define AC_OWN_ERROR_CODE -50199

// The numbers of the errors that the connection sets: the failure of the AC (DWC_ERROR_AC_ANY in the SDK's order, from
// memory), and the loss of the link while WCM is not in its state for errors; the fatal error is DWC_ERROR_FATAL. The
// loss of the link subtracts the kind of the access point from its code
#define ERROR_AC 5
#define ERROR_LINK_LOST 8
#define LINK_LOST_CODE -54000
#define LINK_LOST_FATAL_CODE -54900

// WCM's state in which the link is up, and the one it enters after its errors, which the connection reports as fatal
#define WCM_STATE_CONNECTED 9
#define WCM_STATE_ERROR 11

// The work that a beacon is parsed with, 0x376 bytes: the context that decrypts the game information, the checksum of
// the decrypted record with its table, and the record
struct DWCBssWork {
    CRYPTORC4Context rc4;
    u16 crc;
    MATHCRC16Table crcTable;
    WDSBeaconInfo record;
};

// What makes a beacon one of a WDS access point: the number and ggid of its game information, its size and the
// polynomial of the checksum
#define WDS_MAGIC_NUMBER 1
#define WDS_GGID 0x857
#define WDS_GAME_INFO_SIZE 0x80
#define WDS_GAME_INFO_HEADER_SIZE 0x10
#define WDS_CRC_POLY 0xa001

// The file's statics
typedef struct DWCInet {
    // The AC's error that the restart of the connection through the WDS scan remembers
    int lastError;
    // The control that the game gave, NULL while there is none
    DWCInetControl *control;
} DWCInet;

static DWCInet sInet;

static void DWCi_Inet_SetRecvWindow(int rwin);
static void DWCi_Inet_ConnectToAp(const char *ssid, const u8 *wepKey, int wepMode, u16 mtu, const char *spotInfo,
                                  int apType);
static BOOL DWCi_Inet_IsLinkLost(void);
static void DWCi_ApInfo_SetSpotInfo(const char *spotInfo, DWCApInfo *apInfo);
static BOOL DWCi_ApInfo_FromNintendoSsid(const u8 *ssid, u16 ssidLength, DWCApInfo *apInfo);
static u32 DWCi_ApInfo_GetWorkSize(void);
static BOOL DWCi_ApInfo_FromWdsBeacon(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo);

void DWC_InitInetEx(DWCInetControl *control, u16 dmaNo, u16 powerMode, u16 sslPriority) {
    if (sInet.control != NULL) {
        return;
    }
    MI_CpuFill8(control, 0, sizeof(DWCInetControl));
    control->dmaNo = dmaNo;
    control->powerMode = 1;
    control->state = DWC_CONNECTINET_STATE_IDLE;
    control->watch = 0;
    control->ended = FALSE;
    control->scanStage = DWC_INET_SCAN_NONE;
    control->skipEulaCheck = 0;
    sInet.control = control;
    CPS_SslSetMaxPriority(sslPriority);
}

static void DWCi_Inet_SetRecvWindow(int rwin) {
    if (sInet.control == NULL) {
        return;
    }
    if (rwin < RWIN_MIN || rwin > RWIN_MAX) {
        return;
    }
    if (OS_IsRunOnTwl()) {
        sInet.control->rwin = rwin;
    }
}

void DWC_ConnectInetAsync(void) {
    DWCAcParam param;

    if (sInet.control != NULL) {
        if (sInet.control->state != DWC_CONNECTINET_STATE_IDLE) {
            return;
        }
        MI_CpuFill8(&param, 0, sizeof(DWCAcParam));
        param.dmaNo = sInet.control->dmaNo;
        param.radioOn = sInet.control->powerMode;
        param.unk_0c = sInet.control->rwin;
        param.alloc = DWC_Alloc;
        param.free = DWC_Free;
        param.ignoreEula = sInet.control->skipEulaCheck;
        sInet.control->state = DWC_CONNECTINET_STATE_OPERATING;
        if (!DWCAc_Create(&param)) {
            DWCi_SetError(DWC_ERROR_FATAL, AC_OWN_ERROR_CODE);
        }
    } else {
        DWCi_SetError(DWC_ERROR_FATAL, AC_OWN_ERROR_CODE);
    }
}

static void DWCi_Inet_ConnectToAp(const char *ssid, const u8 *wepKey, int wepMode, u16 mtu, const char *spotInfo,
                                  int apType) {
    DWC_ConnectInetAsync();
    if (sInet.control != NULL) {
        sInet.control->ended = TRUE;
    }
    if (DWC_GetInetStatus() == DWC_CONNECTINET_STATE_OPERATING) {
        DWCAc_SetGivenAp(ssid, wepKey, wepMode, mtu, spotInfo, apType);
    }
}

BOOL DWC_CheckInet(void) {
    if (sInet.control == NULL) {
        return FALSE;
    }
    if (sInet.control->result != 0) {
        sInet.control->state = DWC_CONNECTINET_STATE_OPERATED;
        DWC_GetInetStatus();
        return sInet.control->ended != 0;
    }
    return FALSE;
}

void DWC_ProcessInet(void) {
    if (sInet.control == NULL) {
        return;
    }
    switch (sInet.control->scanStage) {
    case DWC_INET_SCAN_SCANNING:
        sInet.control->result = 0;
        sInet.control->scanStage = DWCi_WdsScan_Process();
        return;
    case DWC_INET_SCAN_FOUND:
        DWCi_WdsScan_Free();
        DWCi_Inet_ConnectToAp(sInet.control->ap.ssid, sInet.control->ap.wepKey, sInet.control->ap.wepMode,
                              sInet.control->ap.mtu, sInet.control->ap.spotInfo, AP_TYPE_WDS);
        DWCAc_ClearPreferWpa();
        sInet.control->scanStage = DWC_INET_SCAN_NONE;
        return;
    case DWC_INET_SCAN_FAILED:
        DWCi_WdsScan_Free();
        sInet.control->result = -1;
        sInet.control->ended = TRUE;
        return;
    case DWC_INET_SCAN_REPORTED:
    case DWC_INET_SCAN_5:
        return;
    }
    if (sInet.control->state == DWC_CONNECTINET_STATE_OPERATING) {
        sInet.control->result = DWCAc_Process();
    } else if (sInet.control->state == DWC_CONNECTINET_STATE_CONNECTED) {
        if (sInet.control->watch != 0) {
            if (WCM_GetState() != WCM_STATE_CONNECTED) {
                sInet.control->watch = 0;
                sInet.control->state = DWC_CONNECTINET_STATE_DISCONNECTED;
            }
        }
    }
}

int DWC_GetInetStatus(void) {
    int status;

    if (sInet.control == NULL) {
        return DWC_CONNECTINET_STATE_NOT_INITIALIZED;
    }
    switch (sInet.control->scanStage) {
    case DWC_INET_SCAN_SCANNING:
        return DWC_CONNECTINET_STATE_OPERATING;
    case DWC_INET_SCAN_FOUND:
        return DWC_CONNECTINET_STATE_OPERATING;
    case DWC_INET_SCAN_FAILED:
        DWC_ProcessInet();
        DWCi_SetError(ERROR_AC, sInet.lastError);
        sInet.control->state = DWC_CONNECTINET_STATE_ERROR;
        sInet.control->scanStage = DWC_INET_SCAN_REPORTED;
        return DWC_CONNECTINET_STATE_ERROR;
    case DWC_INET_SCAN_REPORTED:
        return DWC_CONNECTINET_STATE_ERROR;
    case DWC_INET_SCAN_5:
        return DWC_CONNECTINET_STATE_DISCONNECTING;
    }
    if (sInet.control->state == DWC_CONNECTINET_STATE_IDLE) {
        return sInet.control->state;
    }
    status = DWCAc_GetStatus();
    if (status == AC_STATUS_CONNECTED) {
        sInet.control->state = DWC_CONNECTINET_STATE_CONNECTED;
        sInet.control->watch = 1;
        sInet.control->ended = TRUE;
        return DWC_CONNECTINET_STATE_CONNECTED;
    }
    if (status < 0) {
        if (status >= AC_STATUS_OWN_ERROR_MIN) {
            sInet.control->ended = TRUE;
            DWCi_SetError(DWC_ERROR_FATAL, AC_OWN_ERROR_CODE);
            sInet.control->state = DWC_CONNECTINET_STATE_FATAL_ERROR;
            return DWC_CONNECTINET_STATE_FATAL_ERROR;
        }
        // Two failures that end the connection without a retry
        if (status == -50599 || status == -50699) {
            sInet.control->ended = TRUE;
            DWCi_SetError(ERROR_AC, status);
            sInet.control->state = DWC_CONNECTINET_STATE_ERROR;
            return DWC_CONNECTINET_STATE_ERROR;
        }
        if (!sInet.control->ended) {
            DWCInetControl *control = sInet.control;
            u16 dmaNo = control->dmaNo;
            u32 sslPriority = CPS_SslGetMaxPriority();
            u8 skipEulaCheck = sInet.control->skipEulaCheck;
            int rwin = sInet.control->rwin;

            sInet.lastError = status;
            DWC_CleanupInet();
            DWC_InitInetEx(control, dmaNo, 1, sslPriority);
            sInet.control->skipEulaCheck = skipEulaCheck;
            if (rwin != 0) {
                DWCi_Inet_SetRecvWindow(rwin);
            }
            DWCi_WdsScan_Start(&sInet.control->ap, dmaNo);
            sInet.control->scanStage = DWC_INET_SCAN_SCANNING;
            sInet.control->result = 0;
            return DWC_CONNECTINET_STATE_OPERATING;
        }
        if (sInet.lastError < status) {
            status = sInet.lastError;
        }
        DWCi_SetError(ERROR_AC, status);
        sInet.control->state = DWC_CONNECTINET_STATE_ERROR;
        return DWC_CONNECTINET_STATE_ERROR;
    }
    return DWC_CONNECTINET_STATE_OPERATING;
}

void DWC_CleanupInet(void) {
    if (sInet.control == NULL) {
        return;
    }
    if (sInet.control->state == DWC_CONNECTINET_STATE_IDLE) {
        if (sInet.control->scanStage == DWC_INET_SCAN_SCANNING) {
            int scan;

            DWCi_WdsScan_Stop();
            do {
                scan = DWCi_WdsScan_Process();
            } while ((u32)(scan - 2) > 2);
            DWCi_WdsScan_Free();
        }
        sInet.control = NULL;
        return;
    }
    if (!DWCAc_Destroy()) {
        do {
            OS_Sleep(10);
        } while (!DWCAc_Destroy());
    }
    sInet.control = NULL;
}

static BOOL DWCi_Inet_IsLinkLost(void) {
    if (sInet.control != NULL && sInet.control->state == DWC_CONNECTINET_STATE_DISCONNECTED) {
        return TRUE;
    }
    return FALSE;
}

int DWC_GetLinkLevel(void) {
    return WCM_UpdateLinkQuality();
}

static void DWCi_ApInfo_SetSpotInfo(const char *spotInfo, DWCApInfo *apInfo) {
    apInfo->area = (spotInfo[0] >= '0' && spotInfo[0] <= '9') ? spotInfo[0] - '0' : AP_AREA_UNKNOWN;
    MI_CpuCopy8(spotInfo + 1, apInfo->spotInfo, 9);
}

BOOL DWC_GetApInfo(DWCApInfo *apInfo) {
    u16 ssidLength;
    char spotInfo[10];
    u32 irq;
    const u8 *bssid;
    const u8 *ssid;

    MI_CpuFill8(apInfo, 0, sizeof(DWCApInfo));
    apInfo->area = AP_AREA_UNKNOWN;
    if (DWC_GetInetStatus() != DWC_CONNECTINET_STATE_CONNECTED) {
        return FALSE;
    }
    apInfo->apType = DWCAc_GetApType();
    if (apInfo->apType == AP_TYPE_NONE || apInfo->apType > AP_TYPE_UNKNOWN) {
        apInfo->apType = AP_TYPE_UNKNOWN;
        return FALSE;
    }
    irq = OS_DisableInterrupts();
    bssid = WCM_GetApMacAddress();
    DC_InvalidateRange(bssid, BSSID_SIZE);
    if (bssid == NULL) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    MI_CpuCopy8(bssid, apInfo->bssid, BSSID_SIZE);
    if (apInfo->apType == 0 || apInfo->apType == 1 || apInfo->apType == 2 ||
        (OS_IsRunOnTwl() && (u32)(apInfo->apType - 10) <= 2)) {
        ssid = WCM_GetConnectedApSsid(&ssidLength);
        DC_InvalidateRange(ssid, SSID_SIZE);
        if (ssid == NULL) {
            OS_RestoreInterrupts(irq);
            return FALSE;
        }
        MI_CpuCopy8(ssid, apInfo->essid, ssidLength);
    }
    OS_RestoreInterrupts(irq);
    if (!DWCAc_GetApSpotInfo(spotInfo)) {
        apInfo->area = AP_AREA_UNKNOWN;
    } else {
        DWCi_ApInfo_SetSpotInfo(spotInfo, apInfo);
    }
    apInfo->unk8 = DWCAc_GetKeyMode();
    return TRUE;
}

static BOOL DWCi_ApInfo_FromNintendoSsid(const u8 *ssid, u16 ssidLength, DWCApInfo *apInfo) {
    char spotInfo[10];

    MI_CpuFill8(spotInfo, 0, sizeof(spotInfo));
    if (ssid == NULL) {
        return FALSE;
    }
    if (DWCAc_IsShopSsid(ssid, ssidLength)) {
        if (apInfo == NULL) {
            return TRUE;
        }
        if (DWCAc_GetNdwcShapSpotInfo(ssid, spotInfo)) {
            apInfo->apType = AP_TYPE_NINTENDO_WFC;
            DWCi_ApInfo_SetSpotInfo(spotInfo, apInfo);
            return TRUE;
        }
    }
    return FALSE;
}

static u32 DWCi_ApInfo_GetWorkSize(void) {
    return sizeof(DWCBssWork);
}

static BOOL DWCi_ApInfo_FromWdsBeacon(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo) {
    if (work == NULL || bss == NULL) {
        return FALSE;
    }
    MI_CpuFill8(work, 0, sizeof(DWCBssWork));
    if (bss->gameInfo.userGameInfoLength + WDS_GAME_INFO_HEADER_SIZE == WDS_GAME_INFO_SIZE &&
        bss->gameInfo.magicNumber == WDS_MAGIC_NUMBER && bss->gameInfo.ggid == WDS_GGID) {
        work->crc = 0;
        MATHi_CRC16InitTableRev(&work->crcTable, WDS_CRC_POLY);
        if (WDS_DecryptBeaconInfo(&work->rc4, &work->crc, &work->crcTable, bss, &work->record)) {
            if (WDS_IsBeaconInfoValid(&work->record)) {
                if (apInfo != NULL) {
                    apInfo->apType = AP_TYPE_WDS;
                    DWCi_ApInfo_SetSpotInfo((char *)work->record.unk_20, apInfo);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

u32 DWCi_GetBssDescParseWorkSize(void) {
    return DWCi_ApInfo_GetWorkSize();
}

BOOL DWCi_BssDescToApInfo(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo) {
    if (work == NULL || bss == NULL || apInfo == NULL) {
        return FALSE;
    }
    if (DWCi_ApInfo_FromNintendoSsid(bss->ssid, bss->ssidLength, apInfo)) {
        return TRUE;
    }
    return DWCi_ApInfo_FromWdsBeacon(work, bss, apInfo) != 0;
}

BOOL DWC_UpdateConnection(void) {
    u32 apType;

    DWC_ProcessInet();
    if (DWCi_Inet_IsLinkLost()) {
        apType = DWCAc_GetApType();
        if (apType == AP_TYPE_NONE || apType > AP_TYPE_UNKNOWN) {
            apType = AP_TYPE_UNKNOWN;
        }
        if (WCM_GetState() == WCM_STATE_ERROR) {
            DWCi_SetError(DWC_ERROR_FATAL, LINK_LOST_FATAL_CODE - apType);
        } else {
            DWCi_SetError(ERROR_LINK_LOST, LINK_LOST_CODE - apType);
        }
        return TRUE;
    }
    return FALSE;
}
