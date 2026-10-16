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

// The AC's status (func_0205a2dc): 5 once it is connected, 0 or more while it works and a negative number for an error.
// The errors from -10 up are its own failures
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

static void func_0205ac88(int rwin);
static void func_0205ad94(const char *ssid, const u8 *wepKey, int wepMode, u16 mtu, const char *spotInfo, int apType);
static BOOL func_0205b224(void);
static void func_0205b25c(const char *spotInfo, DWCApInfo *apInfo);
static BOOL func_0205b3fc(const u8 *ssid, u16 ssidLength, DWCApInfo *apInfo);
static u32 func_0205b49c(void);
static BOOL func_0205b4a8(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo);

void func_0205ac24(DWCInetControl *control, u16 dmaNo, u16 powerMode, u16 sslPriority) {
    if (sInet.control != NULL) {
        return;
    }
    sys_memset(control, 0, sizeof(DWCInetControl));
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

static void func_0205ac88(int rwin) {
    if (sInet.control == NULL) {
        return;
    }
    if (rwin < RWIN_MIN || rwin > RWIN_MAX) {
        return;
    }
    if (hw_isDSi()) {
        sInet.control->rwin = rwin;
    }
}

void func_0205acd0(void) {
    DWCAcParam param;

    if (sInet.control != NULL) {
        if (sInet.control->state != DWC_CONNECTINET_STATE_IDLE) {
            return;
        }
        sys_memset(&param, 0, sizeof(DWCAcParam));
        param.dmaNo = sInet.control->dmaNo;
        param.radioOn = sInet.control->powerMode;
        param.unk_0c = sInet.control->rwin;
        param.alloc = func_020586e4;
        param.free = func_02058728;
        param.ignoreEula = sInet.control->skipEulaCheck;
        sInet.control->state = DWC_CONNECTINET_STATE_OPERATING;
        if (!func_02059f04(&param)) {
            func_020584cc(DWC_ERROR_FATAL, AC_OWN_ERROR_CODE);
        }
    } else {
        func_020584cc(DWC_ERROR_FATAL, AC_OWN_ERROR_CODE);
    }
}

static void func_0205ad94(const char *ssid, const u8 *wepKey, int wepMode, u16 mtu, const char *spotInfo, int apType) {
    func_0205acd0();
    if (sInet.control != NULL) {
        sInet.control->ended = TRUE;
    }
    if (func_0205af98() == DWC_CONNECTINET_STATE_OPERATING) {
        func_0205a420(ssid, wepKey, wepMode, mtu, spotInfo, apType);
    }
}

BOOL func_0205ae04(void) {
    if (sInet.control == NULL) {
        return FALSE;
    }
    if (sInet.control->result != 0) {
        sInet.control->state = DWC_CONNECTINET_STATE_OPERATED;
        func_0205af98();
        return sInet.control->ended != 0;
    }
    return FALSE;
}

void func_0205ae58(void) {
    if (sInet.control == NULL) {
        return;
    }
    switch (sInet.control->scanStage) {
    case DWC_INET_SCAN_SCANNING:
        sInet.control->result = 0;
        sInet.control->scanStage = func_0205bf04();
        return;
    case DWC_INET_SCAN_FOUND:
        func_0205c1ec();
        func_0205ad94(sInet.control->ap.ssid, sInet.control->ap.wepKey, sInet.control->ap.wepMode,
                      sInet.control->ap.mtu, sInet.control->ap.spotInfo, AP_TYPE_WDS);
        func_0205a590();
        sInet.control->scanStage = DWC_INET_SCAN_NONE;
        return;
    case DWC_INET_SCAN_FAILED:
        func_0205c1ec();
        sInet.control->result = -1;
        sInet.control->ended = TRUE;
        return;
    case DWC_INET_SCAN_REPORTED:
    case DWC_INET_SCAN_5:
        return;
    }
    if (sInet.control->state == DWC_CONNECTINET_STATE_OPERATING) {
        sInet.control->result = func_0205a1f0();
    } else if (sInet.control->state == DWC_CONNECTINET_STATE_CONNECTED) {
        if (sInet.control->watch != 0) {
            if (WCM_GetState() != WCM_STATE_CONNECTED) {
                sInet.control->watch = 0;
                sInet.control->state = DWC_CONNECTINET_STATE_DISCONNECTED;
            }
        }
    }
}

int func_0205af98(void) {
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
        func_0205ae58();
        func_020584cc(ERROR_AC, sInet.lastError);
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
    status = func_0205a2dc();
    if (status == AC_STATUS_CONNECTED) {
        sInet.control->state = DWC_CONNECTINET_STATE_CONNECTED;
        sInet.control->watch = 1;
        sInet.control->ended = TRUE;
        return DWC_CONNECTINET_STATE_CONNECTED;
    }
    if (status < 0) {
        if (status >= AC_STATUS_OWN_ERROR_MIN) {
            sInet.control->ended = TRUE;
            func_020584cc(DWC_ERROR_FATAL, AC_OWN_ERROR_CODE);
            sInet.control->state = DWC_CONNECTINET_STATE_FATAL_ERROR;
            return DWC_CONNECTINET_STATE_FATAL_ERROR;
        }
        // Two failures that end the connection without a retry
        if (status == -50599 || status == -50699) {
            sInet.control->ended = TRUE;
            func_020584cc(ERROR_AC, status);
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
            func_0205b198();
            func_0205ac24(control, dmaNo, 1, sslPriority);
            sInet.control->skipEulaCheck = skipEulaCheck;
            if (rwin != 0) {
                func_0205ac88(rwin);
            }
            func_0205be94(&sInet.control->ap, dmaNo);
            sInet.control->scanStage = DWC_INET_SCAN_SCANNING;
            sInet.control->result = 0;
            return DWC_CONNECTINET_STATE_OPERATING;
        }
        if (sInet.lastError < status) {
            status = sInet.lastError;
        }
        func_020584cc(ERROR_AC, status);
        sInet.control->state = DWC_CONNECTINET_STATE_ERROR;
        return DWC_CONNECTINET_STATE_ERROR;
    }
    return DWC_CONNECTINET_STATE_OPERATING;
}

void func_0205b198(void) {
    if (sInet.control == NULL) {
        return;
    }
    if (sInet.control->state == DWC_CONNECTINET_STATE_IDLE) {
        if (sInet.control->scanStage == DWC_INET_SCAN_SCANNING) {
            int scan;

            func_0205c1d0();
            do {
                scan = func_0205bf04();
            } while ((u32)(scan - 2) > 2);
            func_0205c1ec();
        }
        sInet.control = NULL;
        return;
    }
    if (!func_0205a3e4()) {
        do {
            func_0207aa04(10);
        } while (!func_0205a3e4());
    }
    sInet.control = NULL;
}

static BOOL func_0205b224(void) {
    if (sInet.control != NULL && sInet.control->state == DWC_CONNECTINET_STATE_DISCONNECTED) {
        return TRUE;
    }
    return FALSE;
}

int func_0205b250(void) {
    return WCM_UpdateLinkQuality();
}

static void func_0205b25c(const char *spotInfo, DWCApInfo *apInfo) {
    apInfo->area = (spotInfo[0] >= '0' && spotInfo[0] <= '9') ? spotInfo[0] - '0' : AP_AREA_UNKNOWN;
    sys_memcpy(spotInfo + 1, apInfo->spotInfo, 9);
}

BOOL func_0205b29c(DWCApInfo *apInfo) {
    u16 ssidLength;
    char spotInfo[10];
    u32 irq;
    const u8 *bssid;
    const u8 *ssid;

    sys_memset(apInfo, 0, sizeof(DWCApInfo));
    apInfo->area = AP_AREA_UNKNOWN;
    if (func_0205af98() != DWC_CONNECTINET_STATE_CONNECTED) {
        return FALSE;
    }
    apInfo->apType = func_0205a348();
    if (apInfo->apType == AP_TYPE_NONE || apInfo->apType > AP_TYPE_UNKNOWN) {
        apInfo->apType = AP_TYPE_UNKNOWN;
        return FALSE;
    }
    irq = CPU_IRQDisable();
    bssid = WCM_GetApMacAddress();
    cp15_invalidateDC(bssid, BSSID_SIZE);
    if (bssid == NULL) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    sys_memcpy(bssid, apInfo->bssid, BSSID_SIZE);
    if (apInfo->apType == 0 || apInfo->apType == 1 || apInfo->apType == 2 ||
        (hw_isDSi() && (u32)(apInfo->apType - 10) <= 2)) {
        ssid = WCM_GetConnectedApSsid(&ssidLength);
        cp15_invalidateDC(ssid, SSID_SIZE);
        if (ssid == NULL) {
            CPU_SetIRQMask(irq);
            return FALSE;
        }
        sys_memcpy(ssid, apInfo->essid, ssidLength);
    }
    CPU_SetIRQMask(irq);
    if (!func_0205a378(spotInfo)) {
        apInfo->area = AP_AREA_UNKNOWN;
    } else {
        func_0205b25c(spotInfo, apInfo);
    }
    apInfo->unk8 = func_0205a3d0();
    return TRUE;
}

static BOOL func_0205b3fc(const u8 *ssid, u16 ssidLength, DWCApInfo *apInfo) {
    char spotInfo[10];

    sys_memset(spotInfo, 0, sizeof(spotInfo));
    if (ssid == NULL) {
        return FALSE;
    }
    if (func_0205a54c(ssid, ssidLength)) {
        if (apInfo == NULL) {
            return TRUE;
        }
        if (func_0205cfe8(ssid, spotInfo)) {
            apInfo->apType = AP_TYPE_NINTENDO_WFC;
            func_0205b25c(spotInfo, apInfo);
            return TRUE;
        }
    }
    return FALSE;
}

static u32 func_0205b49c(void) {
    return sizeof(DWCBssWork);
}

static BOOL func_0205b4a8(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo) {
    if (work == NULL || bss == NULL) {
        return FALSE;
    }
    sys_memset(work, 0, sizeof(DWCBssWork));
    if (bss->gameInfo.userGameInfoLength + WDS_GAME_INFO_HEADER_SIZE == WDS_GAME_INFO_SIZE &&
        bss->gameInfo.magicNumber == WDS_MAGIC_NUMBER && bss->gameInfo.ggid == WDS_GGID) {
        work->crc = 0;
        MATHi_CRC16InitTableRev(&work->crcTable, WDS_CRC_POLY);
        if (func_02058d44(&work->rc4, &work->crc, &work->crcTable, bss, &work->record)) {
            if (func_02058e74(&work->record)) {
                if (apInfo != NULL) {
                    apInfo->apType = AP_TYPE_WDS;
                    func_0205b25c((char *)work->record.unk_20, apInfo);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

u32 func_0205b588(void) {
    return func_0205b49c();
}

BOOL func_0205b594(DWCBssWork *work, const WMBssDesc *bss, DWCApInfo *apInfo) {
    if (work == NULL || bss == NULL || apInfo == NULL) {
        return FALSE;
    }
    if (func_0205b3fc(bss->ssid, bss->ssidLength, apInfo)) {
        return TRUE;
    }
    return func_0205b4a8(work, bss, apInfo) != 0;
}

BOOL func_0205b5ec(void) {
    u32 apType;

    func_0205ae58();
    if (func_0205b224()) {
        apType = func_0205a348();
        if (apType == AP_TYPE_NONE || apType > AP_TYPE_UNKNOWN) {
            apType = AP_TYPE_UNKNOWN;
        }
        if (WCM_GetState() == WCM_STATE_ERROR) {
            func_020584cc(DWC_ERROR_FATAL, LINK_LOST_FATAL_CODE - apType);
        } else {
            func_020584cc(ERROR_LINK_LOST, LINK_LOST_CODE - apType);
        }
        return TRUE;
    }
    return FALSE;
}
