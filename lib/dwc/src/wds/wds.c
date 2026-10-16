// A beacon scanner of DWC (TWL-DWC 5.3, ARM9 main), built on NitroSDK's wireless manager. The file name is a guess,
// from the text "WDS!" that its RC4 key starts with; the ROM names none of it. A scan collects the wireless parents
// whose beacons carry the game ID 0x857; the 0x70 bytes of game information of each one are decrypted with a key of
// "WDS!" and four bytes of the beacon's BSSID, and kept when their CRC-16 is right. Its callers are the scan of
// dwc_wdsscan.c and the connection of dwc_connectinet.c, which decrypts a beacon with WDS_DecryptBeaconInfo on its own.
//
// The WDS_ names are ours, taken from what the code does.

#include "dwc/wds.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include <stddef.h>

// What the scan is doing
enum {
    WDS_STATE_IDLE,     // the wireless manager was started
    WDS_STATE_SCANNING, // a scan was started
    WDS_STATE_SCANNED,  // a scan came back, or was stopped; the entries can be read
    WDS_STATE_ENDING,   // the wireless manager is being ended
};

#define WDS_GAME_ID 0x857
#define WDS_BEACON_VERSION 1
// The part of a beacon's game information that comes before the user's data
#define WDS_GAME_INFO_HEADER_SIZE 0x10
#define WDS_SCAN_BUF_SIZE 0x400

typedef struct WDSWork {
    u8 wmBuf[0xf00]; // the wireless manager's system buffer
    u8 scanBuf[WDS_SCAN_BUF_SIZE];
    WMScanExParam scanParam;
    WMCallbackFunc callback; // the caller's, called when a scan is done
    WDSBeaconInfo infos[WDS_MAX_ENTRIES];
    u16 signals[WDS_MAX_ENTRIES];
    u16 tgids[WDS_MAX_ENTRIES]; // the TGID of each beacon taken in, to take each parent in once
    int count;                  // how many entries are in use
    int next;                   // where the next beacon goes; the entries are a ring
    int state;
    u16 crc;
    MATHCRC16Table crcTable;
    CRYPTORC4Context rc4;
} WDSWork;

static u8 WDS_RssiToSignal(u8 rssi);
static void WDS_OnScan(void *arg);
static int WDS_GetEntry(int index, WDSEntry *out);
static BOOL WDS_IsPrintable(const u8 *text);

static WDSWork *sWdsWork;

// The RC4 key's first four bytes, "WDS!" as a four-character constant (the ROM has it as "!SDW")
static const u32 sKeyPrefix = 'WDS!';

// Turns the RSSI of a scan result into the signal strength that the entries keep
static u8 WDS_RssiToSignal(u8 rssi) {
    int strength = rssi >> 2;

    if ((rssi & 2) == 0) {
        strength += 25;
    }
    return strength;
}

// The callback of the scan. It takes in the beacons that are new and well-formed, then tells the caller
static void WDS_OnScan(void *arg) {
    WMStartScanExCallback *result = arg;
    u32 i;
    u32 j;
    BOOL found;

    DC_InvalidateRange(sWdsWork->scanBuf, WDS_SCAN_BUF_SIZE);
    if (result->errcode == WM_ERRCODE_SUCCESS && result->state == WM_STATECODE_PARENT_FOUND) {
        for (i = 0; i < result->bssDescCount; i++) {
            if (result->bssDesc[i]->gameInfo.userGameInfoLength + WDS_GAME_INFO_HEADER_SIZE != sizeof(WMGameInfo) ||
                result->bssDesc[i]->gameInfo.magicNumber != WDS_BEACON_VERSION ||
                result->bssDesc[i]->gameInfo.ggid != WDS_GAME_ID) {
                continue;
            }

            found = FALSE;
            for (j = 0; j < sWdsWork->count; j++) {
                if (sWdsWork->tgids[j] == result->bssDesc[i]->gameInfo.tgid) {
                    found = TRUE;
                    break;
                }
            }
            if (found == TRUE) {
                continue;
            }

            if (WDS_DecryptBeaconInfo(&sWdsWork->rc4, &sWdsWork->crc, &sWdsWork->crcTable, result->bssDesc[i],
                                      &sWdsWork->infos[sWdsWork->next])) {
                sWdsWork->signals[sWdsWork->next] = WDS_RssiToSignal(result->bssDesc[i]->rssi);
                sWdsWork->tgids[sWdsWork->next] = result->bssDesc[i]->gameInfo.tgid;
                sWdsWork->next = (sWdsWork->next + 1) % WDS_MAX_ENTRIES;
                sWdsWork->count++;
                if (sWdsWork->count > WDS_MAX_ENTRIES) {
                    sWdsWork->count = WDS_MAX_ENTRIES;
                }
            }
        }
    }

    sWdsWork->state = WDS_STATE_SCANNED;
    if (sWdsWork->callback != NULL) {
        sWdsWork->callback(result);
    }
}

u32 WDS_GetWorkSize(void) {
    return (sizeof(WDSWork) + 31) & ~31;
}

int WDS_Initialize(void *work, WMCallbackFunc callback, u16 dmaNo) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWdsWork = work;
    MI_CpuFill8(sWdsWork, 0, WDS_GetWorkSize());
    sWdsWork->state = WDS_STATE_IDLE;
    sWdsWork->crc = 0;
    MATHi_CRC16InitTableRev(&sWdsWork->crcTable, 0xa001);

    result = WM_Initialize(sWdsWork, callback, dmaNo);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

int WDS_End(WMCallbackFunc callback) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWdsWork->state = WDS_STATE_ENDING;
    sWdsWork = NULL;

    result = WM_End(callback);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

int WDS_StartScan(WMCallbackFunc callback) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWdsWork->scanParam.scanBuf = (WMBssDesc *)sWdsWork->scanBuf;
    sWdsWork->scanParam.scanBufSize = WDS_SCAN_BUF_SIZE;
    sWdsWork->scanParam.channelList = WM_GetAllowedChannel();
    sWdsWork->scanParam.maxChannelTime = WM_GetDispersionScanPeriod();
    sWdsWork->scanParam.scanType = 1;
    MI_CpuFill8(sWdsWork->scanParam.bssid, 0xff, sizeof(sWdsWork->scanParam.bssid));
    sWdsWork->callback = callback;
    sWdsWork->state = WDS_STATE_SCANNING;

    result = WM_StartScanEx(WDS_OnScan, &sWdsWork->scanParam);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

int WDS_EndScan(WMCallbackFunc callback) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWdsWork->state = WDS_STATE_SCANNED;

    result = WM_EndScan(callback);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

// Gives out the entry at an index of the ring, with its signal strength
static int WDS_GetEntry(int index, WDSEntry *out) {
    if (sWdsWork->state != WDS_STATE_SCANNED) {
        return -1;
    }
    if (index < 0 || index >= sWdsWork->count) {
        return -1;
    }

    MI_CpuFill8(out, 0, sizeof(WDSEntry));
    out->valid = TRUE;
    out->signal = sWdsWork->signals[index];
    MI_CpuCopy8(&sWdsWork->infos[index], &out->info, sizeof(WDSBeaconInfo));
    return 0;
}

int WDS_GetEntries(WDSEntry *entries) {
    int i;

    if (sWdsWork->state != WDS_STATE_SCANNED) {
        return -1;
    }

    MI_CpuFill8(entries, 0, WDS_MAX_ENTRIES * sizeof(WDSEntry));
    for (i = 0; i < WDS_MAX_ENTRIES; i++) {
        entries[i].valid = FALSE;
    }

    for (i = 0; i < sWdsWork->count; i++) {
        if (WDS_GetEntry(i, &entries[i]) == -1) {
            break;
        }
    }
    return 0;
}

BOOL WDS_DecryptBeaconInfo(CRYPTORC4Context *context, u16 *crc, MATHCRC16Table *table, const WMBssDesc *desc,
                           WDSBeaconInfo *out) {
    u8 key[8];
    u16 calculated;

    if (desc == NULL || context == NULL || crc == NULL || table == NULL || out == NULL) {
        return FALSE;
    }

    MI_CpuCopy8(desc->gameInfo.userGameInfo, out, sizeof(WDSBeaconInfo));
    MI_CpuCopy8(&sKeyPrefix, key, 4);
    MI_CpuCopy8(&desc->bssid[2], &key[4], 4);
    CRYPTO_RC4Init(context, key, sizeof(key));
    CRYPTO_RC4Encrypt(context, out, sizeof(WDSBeaconInfo), out);

    // The accumulator is updated here, but the check below does its own calculation
    MATHi_CRC16UpdateRev(table, crc, out, offsetof(WDSBeaconInfo, crc));
    calculated = MATH_CalcCRC16(table, out, offsetof(WDSBeaconInfo, crc));
    if (calculated == out->crc || out->crc == 0) {
        return TRUE;
    }

    MI_CpuFill8(out, 0, sizeof(WDSBeaconInfo));
    return FALSE;
}

// Whether ten bytes are all printable ASCII
static BOOL WDS_IsPrintable(const u8 *text) {
    int i;

    for (i = 0; i < 10; i++) {
        if (text[i] < 0x20 || text[i] > 0x7e) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL WDS_IsBeaconInfoValid(const WDSBeaconInfo *info) {
    if ((info->flags & 2) && WDS_IsPrintable(info->spotInfo) && info->wepMode <= 3) {
        return TRUE;
    }
    return FALSE;
}
