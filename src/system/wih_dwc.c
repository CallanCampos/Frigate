#include "types.h"
#include "system/wih_dwc.h"
#include "app/comm_tvt_field.h"
#include "gfl/net.h"
#include "gfl/net_whpipe.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/wih.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "system/ctvt_beacon.h"

// What the wireless scan sees around the player, for the icons of the C-Gear and the other screens that show the
// state of the wireless: the access points the console is set up for, the hotspots, the other access points with and
// without a key, the games that can be joined and the signal of the DS Download Play. The name is the ROM's string.

#define _ALLBEACON_MAX 4
// Frames that an access point stays shown after its last beacon
#define AP_KNOWN_FRAMES 600
#define AP_OTHER_FRAMES 400
#define LINK_LEVEL_FRAMES 400
#define LINK_LEVEL_MAX 4
#define WIFI_SLOT_COUNT 3
#define WIFI_SLOT_SSID_COUNT 2
#define WIFI_CONFIG_SIZE 0xa00

enum {
    AP_TIMER_KNOWN,
    AP_TIMER_HOTSPOT,
    AP_TIMER_OPEN,
    AP_TIMER_LOCKED,
    AP_TIMER_COUNT
};

// One of the console's Wi-Fi connection settings
typedef struct {
    u8 unk0[0x40];
    char ssid[WIFI_SLOT_SSID_COUNT][0x20];
    u8 unk80[0x67];
    u8 type;
    u8 unkE8[0x18];
} WifiSlot;

// One of the DSi's own settings, of two parts
typedef struct {
    WifiSlot base;
    WifiSlot ex;
} WifiSlotEx;

typedef struct {
    WifiSlotEx slotsEx[WIFI_SLOT_COUNT];
    WifiSlot slots[WIFI_SLOT_COUNT];
    u8 unk900[0x100];
} WifiConfig;

typedef struct {
    WMBssDesc bss[_ALLBEACON_MAX];
    void *parseWork;
    WifiConfig *config;
    u32 unk308;
    u16 paused;
    // Frames until the link level may change again
    u16 linkLevelWait;
    u16 num;
    u16 linkLevel;
    u16 bestLinkLevel;
    u16 count;
    int timers[AP_TIMER_COUNT];
} WihDwcWork;

static void scanInfoCallback(const void *beacon, void *work, u16 linkLevel);
static BOOL isConfiguredAP(WMBssDesc *bss);
static BOOL isFreeSpot(WMBssDesc *bss);
static BOOL isLocked(WMBssDesc *bss);

static WifiConfig *_localcfg;
static WihDwcWork *_localWork;

void *func_02012908(int num, HeapID heapId) {
    WihDwcWork *work;

    GFL_ASSERT(num <= _ALLBEACON_MAX);
    GFL_ASSERT(_localWork==NULL);
    work = allocConfigDSSoftwareFeature(heapId, sizeof(WihDwcWork), "wih_dwc.c", 73);
    work->num = num;
    work->parseWork = allocConfigDSSoftwareFeature(heapId, func_0205b588(), "wih_dwc.c", 76);
    WH_SetScanInfoCallback(scanInfoCallback);
    _localWork = work;
    work->config = _localcfg;
    GFL_OvlLoad(OVERLAY_ID(31));
    return work;
}

void func_02012994(void *work) {
    WihDwcWork *wk = work;

    GFL_ASSERT(_localWork);
    WH_SetScanInfoCallback(NULL);
    func_02042ed0(wk->parseWork);
    func_02042ed0(wk);
    _localWork = NULL;
    GFL_OvlUnload(OVERLAY_ID(31));
}

void func_020129e0(void) {
    if (_localWork != NULL) {
        _localWork->paused = TRUE;
    }
}

void func_020129f8(void) {
    if (_localWork != NULL) {
        _localWork->paused = FALSE;
    }
}

static void scanInfoCallback(const void *beacon, void *work, u16 linkLevel) {
    if (_localWork->num != 0 && _localWork->count < _localWork->num && _localWork->count < _ALLBEACON_MAX) {
        sys_memcpy(beacon, &_localWork->bss[_localWork->count], sizeof(WMBssDesc));
        _localWork->count++;
    }
}

void func_02012a4c(void) {
    if (_localWork != NULL && _localWork->paused) {
        return;
    }
    if (_localWork != NULL) {
        if (_localWork->timers[AP_TIMER_KNOWN] > 0) {
            _localWork->timers[AP_TIMER_KNOWN]--;
        }
        if (_localWork->timers[AP_TIMER_HOTSPOT] > 0) {
            _localWork->timers[AP_TIMER_HOTSPOT]--;
        }
        if (_localWork->timers[AP_TIMER_OPEN] > 0) {
            _localWork->timers[AP_TIMER_OPEN]--;
        }
        if (_localWork->timers[AP_TIMER_LOCKED] > 0) {
            _localWork->timers[AP_TIMER_LOCKED]--;
        }
    }
}

// Whether the beacon is of an access point that the console has settings for
static BOOL isConfiguredAP(WMBssDesc *bss) {
    int i, j;
    char ssid[0x20 + 1];
    int length;

    ssid[0x20] = '\0';
    for (i = 0; i < WIFI_SLOT_COUNT; i++) {
        for (j = 0; j < WIFI_SLOT_SSID_COUNT; j++) {
            sys_memcpy(_localWork->config->slots[i].ssid[j], ssid, 0x20);
            length = GFL_STD_StrLen(ssid);
            if (length > 0 && GFL_STD_MemCmp(ssid, bss->ssid, length) == 0) {
                return TRUE;
            }
            sys_memcpy(_localWork->config->slotsEx[i].base.ssid[j], ssid, 0x20);
            length = GFL_STD_StrLen(ssid);
            if (length > 0 && GFL_STD_MemCmp(ssid, bss->ssid, length) == 0) {
                return TRUE;
            }
            if (_localWork->config->slotsEx[i].base.type == 0x11) {
                sys_memcpy(_localWork->config->slotsEx[i].ex.ssid[j], ssid, 0x20);
                length = GFL_STD_StrLen(ssid);
                if (length > 0 && GFL_STD_MemCmp(ssid, bss->ssid, length) == 0) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// Whether the beacon is of a FREESPOT hotspot
// BUG: the comparison is the wrong way round, so that it is every open access point with a name of eight letters
// but FREESPOT
static BOOL isFreeSpot(WMBssDesc *bss) {
#ifdef BUGFIX
    if (bss->ssidLength == 8 && GFL_STD_MemCmp("FREESPOT", bss->ssid, 8) == 0 && !(bss->capaInfo & 0x10)) {
#else
    if (bss->ssidLength == 8 && GFL_STD_MemCmp("FREESPOT", bss->ssid, 8) != 0 && !(bss->capaInfo & 0x10)) {
#endif
        return TRUE;
    }
    return FALSE;
}

// Whether the access point asks for a key
static BOOL isLocked(WMBssDesc *bss) {
    if (bss->capaInfo & 0x10) {
        return TRUE;
    }
    return FALSE;
}

u32 func_02012be4(WifiList *wifiList) {
    u32 status = 0;
    GFLNetInitData *init = func_02042e84();
    int i;
    DWCApInfo apInfo;

    if (_localWork == NULL) {
        return status;
    }
    if (func_02012e54(wifiList) != -1) {
        status |= 0x8;
    }
    if (init->gameCommandBase == 0x35 && func_02042a78() > 1) {
        status |= 0x4;
    }
    for (i = 0; i < init->unk62; i++) {
        if (func_020428a8(i) != NULL) {
            u8 service = func_ov030_021740a4(i);

            if (service >= 0x14 && service <= 0x1b) {
                status |= 0x10;
            } else if (service == 0x9) {
                status |= 0x20;
            }
        }
    }
    for (i = 0; i < _localWork->count; i++) {
        WMBssDesc *bss = &_localWork->bss[i];

        if (isConfiguredAP(bss)) {
            _localWork->timers[AP_TIMER_KNOWN] = AP_KNOWN_FRAMES;
        } else if (isFreeSpot(bss)) {
            _localWork->timers[AP_TIMER_HOTSPOT] = AP_KNOWN_FRAMES;
        } else if (func_0205b594(_localWork->parseWork, bss, &apInfo)) {
            switch (apInfo.apType) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 10:
            case 11:
            case 12:
                _localWork->timers[AP_TIMER_KNOWN] = AP_KNOWN_FRAMES;
                break;
            case 4:
            case 5:
            case 6:
            case 8:
                _localWork->timers[AP_TIMER_HOTSPOT] = AP_KNOWN_FRAMES;
                break;
            }
        } else if (bss->ssidLength != 0) {
            if (!isLocked(bss)) {
                _localWork->timers[AP_TIMER_OPEN] = AP_OTHER_FRAMES;
            } else {
                _localWork->timers[AP_TIMER_LOCKED] = AP_OTHER_FRAMES;
            }
        }
    }
    if (_localWork->timers[AP_TIMER_KNOWN] > 0) {
        status |= 0x40;
    }
    if (_localWork->timers[AP_TIMER_HOTSPOT] > 0) {
        status |= 0x80;
    }
    if (_localWork->timers[AP_TIMER_OPEN] > 0) {
        status |= 0x100;
    }
    if (_localWork->timers[AP_TIMER_LOCKED] > 0) {
        status |= 0x200;
    }
    if (_localWork->linkLevelWait != 0) {
        _localWork->linkLevelWait--;
    }
    if (status == 0) {
        _localWork->linkLevel = 0;
    } else if (_localWork->bestLinkLevel != 0 && _localWork->linkLevelWait == 0) {
        _localWork->linkLevel = _localWork->bestLinkLevel;
        _localWork->linkLevelWait = LINK_LEVEL_FRAMES;
        _localWork->bestLinkLevel = 0;
    }
    _localWork->count = 0;
    return status | 0x2;
}

void setupDownloadPlayConfig(HeapID heapId) {
    GFL_ASSERT(_localcfg==NULL);
    _localcfg = allocConfigDSSoftwareFeature(heapId, WIFI_CONFIG_SIZE, "wih_dwc.c", 472);
}

void downloadPlayConfigForWifi(void) {
    GFL_ASSERT(_localcfg);
    func_0205e038(_localcfg, 0);
}

int func_02012e54(WifiList *wifiList) {
    int i;
    u8 mac[6];

    if (func_02042788()) {
        func_02042e84();
        func_0207c33c(mac);
        for (i = 0; i < 16; i++) {
            if (func_ov030_02173b78(i) == 0x20 && CtvtBeacon_IsFriendInvite(func_020428a8(i), wifiList, mac)) {
                return i;
            }
        }
    }
    return -1;
}

u16 func_02012ea0(void) {
    if (_localWork != NULL) {
        return _localWork->linkLevel;
    }
    return 0;
}

void func_02012ebc(u8 linkLevel) {
    if (_localWork != NULL && _localWork->bestLinkLevel < linkLevel && linkLevel < LINK_LEVEL_MAX) {
        _localWork->bestLinkLevel = linkLevel;
    }
}

void func_02012edc(void) {
}
