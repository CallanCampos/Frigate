// A beacon scanner of DWC (TWL-DWC 5.3, ARM9 main), built on NitroSDK's wireless manager. The file name is a guess,
// from the text "WDS!" that its RC4 key starts with; the ROM names none of it. A scan collects the wireless parents
// whose beacons carry the game ID 0x857; the 0x70 bytes of game information of each one are decrypted with a key of
// "WDS!" and four bytes of the beacon's BSSID, and kept when their CRC-16 is right. Its callers are the scan of
// dwc_wdsscan.c and the connection of dwc_connectinet.c, which decrypts a beacon with func_02058d44 on its own.
//
// The functions are still named after their addresses; the comment above each has the name we would give it.

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

static u8 func_02058840(u8 rssi);
static void func_02058854(void *arg);
static int func_02058c00(int index, WDSEntry *out);
static BOOL func_02058e40(const u8 *text);

static WDSWork *sWork;

// The RC4 key's first four bytes, "WDS!" as a four-character constant (the ROM has it as "!SDW")
static const u32 sKeyPrefix = 'WDS!';

// WDS_RssiToSignal: turns the RSSI of a scan result into the signal strength that the entries keep
static u8 func_02058840(u8 rssi) {
    int strength = rssi >> 2;

    if ((rssi & 2) == 0) {
        strength += 25;
    }
    return strength;
}

// WDS_OnScan: the callback of the scan. It takes in the beacons that are new and well-formed, then tells the caller
static void func_02058854(void *arg) {
    WMStartScanExCallback *result = arg;
    u32 i;
    u32 j;
    BOOL found;

    cp15_invalidateDC(sWork->scanBuf, WDS_SCAN_BUF_SIZE);
    if (result->errcode == WM_ERRCODE_SUCCESS && result->state == WM_STATECODE_PARENT_FOUND) {
        for (i = 0; i < result->bssDescCount; i++) {
            if (result->bssDesc[i]->gameInfo.userGameInfoLength + WDS_GAME_INFO_HEADER_SIZE != sizeof(WMGameInfo) ||
                result->bssDesc[i]->gameInfo.magicNumber != WDS_BEACON_VERSION ||
                result->bssDesc[i]->gameInfo.ggid != WDS_GAME_ID) {
                continue;
            }

            found = FALSE;
            for (j = 0; j < sWork->count; j++) {
                if (sWork->tgids[j] == result->bssDesc[i]->gameInfo.tgid) {
                    found = TRUE;
                    break;
                }
            }
            if (found == TRUE) {
                continue;
            }

            if (func_02058d44(&sWork->rc4, &sWork->crc, &sWork->crcTable, result->bssDesc[i],
                              &sWork->infos[sWork->next])) {
                sWork->signals[sWork->next] = func_02058840(result->bssDesc[i]->rssi);
                sWork->tgids[sWork->next] = result->bssDesc[i]->gameInfo.tgid;
                sWork->next = (sWork->next + 1) % WDS_MAX_ENTRIES;
                sWork->count++;
                if (sWork->count > WDS_MAX_ENTRIES) {
                    sWork->count = WDS_MAX_ENTRIES;
                }
            }
        }
    }

    sWork->state = WDS_STATE_SCANNED;
    if (sWork->callback != NULL) {
        sWork->callback(result);
    }
}

// WDS_GetWorkSize
u32 func_02058a40(void) {
    return (sizeof(WDSWork) + 31) & ~31;
}

// WDS_Initialize
int func_02058a4c(void *work, WMCallbackFunc callback, u16 dmaNo) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWork = work;
    sys_memset(sWork, 0, func_02058a40());
    sWork->state = WDS_STATE_IDLE;
    sWork->crc = 0;
    MATHi_CRC16InitTableRev(&sWork->crcTable, 0xa001);

    result = func_020813c4(sWork, callback, dmaNo);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

// WDS_End
int func_02058ad0(WMCallbackFunc callback) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWork->state = WDS_STATE_ENDING;
    sWork = NULL;

    result = func_02081448(callback);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

// WDS_StartScan
int func_02058b10(WMCallbackFunc callback) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWork->scanParam.scanBuf = (WMBssDesc *)sWork->scanBuf;
    sWork->scanParam.scanBufSize = WDS_SCAN_BUF_SIZE;
    sWork->scanParam.channelList = func_020810cc();
    sWork->scanParam.maxChannelTime = func_020811a4();
    sWork->scanParam.scanType = 1;
    sys_memset(sWork->scanParam.bssid, 0xff, sizeof(sWork->scanParam.bssid));
    sWork->callback = callback;
    sWork->state = WDS_STATE_SCANNING;

    result = func_020815c8(func_02058854, &sWork->scanParam);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

// WDS_EndScan
int func_02058bc8(WMCallbackFunc callback) {
    int result;

    if (callback == NULL) {
        return -1;
    }

    sWork->state = WDS_STATE_SCANNED;

    result = func_020816b4(callback);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

// WDS_GetEntry: gives out the entry at an index of the ring, with its signal strength
static int func_02058c00(int index, WDSEntry *out) {
    if (sWork->state != WDS_STATE_SCANNED) {
        return -1;
    }
    if (index < 0 || index >= sWork->count) {
        return -1;
    }

    sys_memset(out, 0, sizeof(WDSEntry));
    out->valid = TRUE;
    out->signal = sWork->signals[index];
    sys_memcpy(&sWork->infos[index], &out->info, sizeof(WDSBeaconInfo));
    return 0;
}

// WDS_GetEntries
int func_02058c9c(WDSEntry *entries) {
    int i;

    if (sWork->state != WDS_STATE_SCANNED) {
        return -1;
    }

    sys_memset(entries, 0, WDS_MAX_ENTRIES * sizeof(WDSEntry));
    for (i = 0; i < WDS_MAX_ENTRIES; i++) {
        entries[i].valid = FALSE;
    }

    for (i = 0; i < sWork->count; i++) {
        if (func_02058c00(i, &entries[i]) == -1) {
            break;
        }
    }
    return 0;
}

// WDS_DecryptBeaconInfo
BOOL func_02058d44(CRYPTORC4Context *context, u16 *crc, MATHCRC16Table *table, const WMBssDesc *desc,
                   WDSBeaconInfo *out) {
    u8 key[8];
    u16 calculated;

    if (desc == NULL || context == NULL || crc == NULL || table == NULL || out == NULL) {
        return FALSE;
    }

    sys_memcpy(desc->gameInfo.userGameInfo, out, sizeof(WDSBeaconInfo));
    sys_memcpy(&sKeyPrefix, key, 4);
    sys_memcpy(&desc->bssid[2], &key[4], 4);
    CRYPTO_RC4Init(context, key, sizeof(key));
    CRYPTO_RC4Encrypt(context, out, sizeof(WDSBeaconInfo), out);

    // The accumulator is updated here, but the check below does its own calculation
    MATHi_CRC16UpdateRev(table, crc, out, offsetof(WDSBeaconInfo, crc));
    calculated = MATH_CalcCRC16(table, out, offsetof(WDSBeaconInfo, crc));
    if (calculated == out->crc || out->crc == 0) {
        return TRUE;
    }

    sys_memset(out, 0, sizeof(WDSBeaconInfo));
    return FALSE;
}

// WDS_IsPrintable: whether ten bytes are all printable ASCII
static BOOL func_02058e40(const u8 *text) {
    int i;

    for (i = 0; i < 10; i++) {
        if (text[i] < 0x20 || text[i] > 0x7e) {
            return FALSE;
        }
    }
    return TRUE;
}

// WDS_IsBeaconInfoValid
BOOL func_02058e74(const WDSBeaconInfo *info) {
    if ((info->flags & 2) && func_02058e40(info->unk_20) && info->unk_65 <= 3) {
        return TRUE;
    }
    return FALSE;
}
