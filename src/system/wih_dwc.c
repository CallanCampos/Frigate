#include "types.h"
#include "app/comm_tvt_field.h"
#include "dwc/apinfo.h"
#include "dwc/bm.h"
#include "gfl/net.h"
#include "gfl/net_whpipe.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/wih.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "system/ctvt_beacon.h"
#include "system/wih_dwc.h"

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

enum { AP_TIMER_KNOWN, AP_TIMER_HOTSPOT, AP_TIMER_OPEN, AP_TIMER_LOCKED, AP_TIMER_COUNT };

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

struct WihDwcWork {
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
};

static void scanInfoCallback(const void *beacon, void *work, u16 linkLevel);
static BOOL isConfiguredAP(WMBssDesc *bss);
static BOOL isFreeSpot(WMBssDesc *bss);
static BOOL isLocked(WMBssDesc *bss);

static WifiConfig *_localcfg;
static WihDwcWork *_localWork;

void *WihDwc_Create(int num, HeapID heapId) {
    WihDwcWork *work;

    GFL_ASSERT(num <= _ALLBEACON_MAX);
    // clang-format off
    GFL_ASSERT(_localWork==NULL);
    // clang-format on
    work = allocConfigDSSoftwareFeature(heapId, sizeof(WihDwcWork), "wih_dwc.c", 73);
    work->num = num;
    work->parseWork = allocConfigDSSoftwareFeature(heapId, DWCi_GetBssDescParseWorkSize(), "wih_dwc.c", 76);
    WH_SetScanInfoCallback(scanInfoCallback);
    _localWork = work;
    work->config = _localcfg;
    GFL_OvlLoad(OVERLAY_ID(31));
    return work;
}

void WihDwc_Destroy(WihDwcWork *wk) {
    GFL_ASSERT(_localWork);
    WH_SetScanInfoCallback(NULL);
    GFL_NetFreeAligned32(wk->parseWork);
    GFL_NetFreeAligned32(wk);
    _localWork = NULL;
    GFL_OvlUnload(OVERLAY_ID(31));
}

void WihDwc_Pause(void) {
    if (_localWork != NULL) {
        _localWork->paused = TRUE;
    }
}

void WihDwc_Resume(void) {
    if (_localWork != NULL) {
        _localWork->paused = FALSE;
    }
}

static void scanInfoCallback(const void *beacon, void *work, u16 linkLevel) {
    if (_localWork->num != 0 && _localWork->count < _localWork->num && _localWork->count < _ALLBEACON_MAX) {
        MI_CpuCopy8(beacon, &_localWork->bss[_localWork->count], sizeof(WMBssDesc));
        _localWork->count++;
    }
}

void WihDwc_TickTimers(void) {
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
            MI_CpuCopy8(_localWork->config->slots[i].ssid[j], ssid, 0x20);
            length = GFL_STD_StrLen(ssid);
            if (length > 0 && GFL_STD_MemCmp(ssid, bss->ssid, length) == 0) {
                return TRUE;
            }
            MI_CpuCopy8(_localWork->config->slotsEx[i].base.ssid[j], ssid, 0x20);
            length = GFL_STD_StrLen(ssid);
            if (length > 0 && GFL_STD_MemCmp(ssid, bss->ssid, length) == 0) {
                return TRUE;
            }
            if (_localWork->config->slotsEx[i].base.type == 0x11) {
                MI_CpuCopy8(_localWork->config->slotsEx[i].ex.ssid[j], ssid, 0x20);
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

u32 WihDwc_GetStatus(WifiList *wifiList) {
    u32 status = 0;
    GFLNetInitData *init = GFL_NetGetInitData();
    int i;
    DWCApInfo apInfo;

    if (_localWork == NULL) {
        return status;
    }
    if (WihDwc_FindFriendInvite(wifiList) != -1) {
        status |= 0x8;
    }
    if (init->gameCommandBase == 0x35 && GFL_NetGetNegotiatedCount() > 1) {
        status |= 0x4;
    }
    for (i = 0; i < init->unk62; i++) {
        if (GFL_NetGetBeaconData(i) != NULL) {
            u8 service = NetWhpipe_GetBeaconCommandBase(i);

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
        } else if (DWCi_BssDescToApInfo(_localWork->parseWork, bss, &apInfo)) {
            switch (apInfo.aptype) {
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
    // clang-format off
    GFL_ASSERT(_localcfg==NULL);
    // clang-format on
    _localcfg = allocConfigDSSoftwareFeature(heapId, WIFI_CONFIG_SIZE, "wih_dwc.c", 472);
}

void downloadPlayConfigForWifi(void) {
    GFL_ASSERT(_localcfg);
    func_0205e038(_localcfg, 0);
}

int WihDwc_FindFriendInvite(WifiList *wifiList) {
    int i;
    u8 mac[6];

    if (GFL_NetIsInitialized()) {
        GFL_NetGetInitData();
        OS_GetMacAddress(mac);
        for (i = 0; i < 16; i++) {
            if (NetWhpipe_GetBeaconService(i) == 0x20 &&
                CtvtBeacon_IsFriendInvite(GFL_NetGetBeaconData(i), wifiList, mac)) {
                return i;
            }
        }
    }
    return -1;
}

u16 WihDwc_GetLinkLevel(void) {
    if (_localWork != NULL) {
        return _localWork->linkLevel;
    }
    return 0;
}

void WihDwc_SetBestLinkLevel(u8 linkLevel) {
    if (_localWork != NULL && _localWork->bestLinkLevel < linkLevel && linkLevel < LINK_LEVEL_MAX) {
        _localWork->bestLinkLevel = linkLevel;
    }
}

void WihDwc_OnParentNotFound(void) {
}
