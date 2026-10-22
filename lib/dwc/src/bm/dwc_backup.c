// DWC's access to the Wi-Fi settings in the console's NVRAM (TWL-DWC 5.3, bm). The file name is a guess: the ROM names
// none of it. It reads and writes the pages of the settings of an NTR console (0 to 3) and of a TWL console (from page
// 0x11), keeps the Wi-Fi ID that the settings were last read with, packed into 14 bytes, and has the helpers for IP
// addresses that the Wi-Fi settings screens of overlay 182 call.

#include "bm_internal.h"
#include "dwc/bm.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"

#define WIFI_ID_BITS 43
#define WIFI_ID_MASK ((1ULL << WIFI_ID_BITS) - 1)

// The packed Wi-Fi ID, kept from the last time the settings were read. The word before it is read by no code that the
// link kept
static struct {
    u32 unk0;
    u8 packed[DWC_BM_PACKED_SIZE];
} Wifi;

// The buffer that the Wi-Fi ID is written to the NTR console's settings with: a page, a scratch buffer for the write,
// and the table that the page's CRC16 is made with
typedef struct MEMSWI {
    DWCMemPage mem;
    u8 work[0x100];
    MATHCRC16Table table;
} MEMSWI;

static BOOL ReadNtrPages(DWCMemPage *pages, u32 first, u32 count);
static BOOL WriteNtrPage(const DWCMemPage *page, u32 index, void *scratch);
static BOOL ReadTwlPages(DWCBmTwlPage *pages, u32 first, u32 count);

BOOL DWCi_BM_GetApInfo(void *buf) {
    return ReadNtrPages(buf, 0, 3);
}

BOOL DWCBm_ReadTwlSettings(void *buf) {
    return ReadTwlPages(buf, 0, 3) != FALSE;
}

void DWCi_BM_GetWiFiInfo(DWCWiFiInfo *info) {
    MI_CpuCopy8(Wifi.packed, &info->attestedUserId, 6);
    info->attestedUserId &= WIFI_ID_MASK;
    MI_CpuCopy8(&Wifi.packed[5], &info->notAttestedId, 6);
    info->notAttestedId >>= 3;
    info->notAttestedId &= WIFI_ID_MASK;
    MI_CpuCopy8(&Wifi.packed[10], &info->pass, 2);
    info->pass >>= 6;
    info->pass &= 0x3FF;
    MI_CpuCopy8(&Wifi.packed[12], &info->randomHistory, 2);
}

static BOOL ReadNtrPages(DWCMemPage *pages, u32 first, u32 count) {
    u32 index;

    for (index = first; index < first + count; index++) {
        if (func_0205dca4(&pages[index - first], sizeof(DWCMemPage), index) == -1) {
            return FALSE;
        }
    }
    return TRUE;
}

// The scratch buffer that the callers pass is not used
static BOOL WriteNtrPage(const DWCMemPage *page, u32 index, void *scratch) {
    return func_0205dd40(index, page, sizeof(DWCMemPage)) != -1;
}

static BOOL ReadTwlPages(DWCBmTwlPage *pages, u32 first, u32 count) {
    u32 index;

    for (index = first; index < first + count; index++) {
        if (func_0205dca4(&pages[index - first], sizeof(DWCBmTwlPage), index + 0x11) == -1) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL DWCi_BM_SetWiFiInfo(const DWCWiFiInfo *info, void *work) {
    MEMSWI *buf = work;
    int index;

    DWCi_BACKUPlConvWifiInfo(info);
    MATHi_CRC16InitTableRev(&buf->table, 0xA001);
    for (index = 0; index < 2; index++) {
        // The page can't be read back before it is written: the game stops
        if (!ReadNtrPages(&buf->mem, index, 1)) {
            OS_Terminate();
            return FALSE;
        }
        MI_CpuCopy8(Wifi.packed, buf->mem.wifi, DWC_BM_PACKED_SIZE);
        buf->mem.crc = MATH_CalcCRC16(&buf->table, &buf->mem, sizeof(DWCMemPage) - sizeof(u16));
        while (!WriteNtrPage(&buf->mem, index, buf->work)) {
        }
    }
    return func_02056e74() != 0;
}

BOOL DWCBm_ReadAllNtrPages(void *buf) {
    return ReadNtrPages(buf, 0, 4);
}

BOOL DWCBm_WriteNtrPages(void *buf, const BOOL flags[4], void *scratch) {
    DWCMemPage *pages = buf;
    int index;

    for (index = 0; index < 4; index++) {
        if (flags[index] != 0) {
            while (!WriteNtrPage(&pages[index], index, scratch)) {
            }
        }
    }
    return func_02056e74() != 0;
}

void DWCi_BACKUPlSetWiFi(const u8 *packed) {
    MI_CpuCopy8(packed, Wifi.packed, DWC_BM_PACKED_SIZE);
}

u8 DWCi_BACKUPlConvMaskCidr(const u8 *mask) {
    int bits;
    int i;
    int bit;

    for (i = 0, bits = 0; i < 4; i++) {
        for (bit = 0; bit < 8; bit++) {
            if ((mask[i] >> bit) & 1) {
                bits++;
            }
        }
    }
    return bits;
}

void DWCi_BACKUPlConvMaskAddr(int bits, u8 *mask) {
    int i;
    u32 value = 0xFFFFFFFF ^ (0xFFFFFFFF >> bits);

    for (i = 0; i < 4; i++) {
        mask[i] = value >> (24 - i * 8);
    }
}

BOOL DWC_BACKUPlCheckIp(const u8 *address, const u8 *mask) {
    u32 addressBits;
    u32 maskBits;

    if (!DWC_BACKUPlCheckAddress(address)) {
        return FALSE;
    }
    MI_CpuCopy8(address, &addressBits, 4);
    MI_CpuCopy8(mask, &maskBits, 4);
    return (addressBits & ~maskBits) != 0;
}

BOOL DWC_BACKUPlCheckAddress(const u8 *address) {
    u8 first = address[0];

    if (first == 127) {
        return FALSE;
    }
    if (first < 1) {
        return FALSE;
    }
    return first <= 223;
}

// Does nothing and says it did: the writes end by it, perhaps where a flush of the NVRAM once was
BOOL func_02056e74(void) {
    return TRUE;
}

const u8 *DWCi_BACKUPlConvWifiInfo(const DWCWiFiInfo *info) {
    u64 pendingId = info->notAttestedId;

    MI_CpuCopy8(info, Wifi.packed, 5);
    Wifi.packed[5] = ((info->attestedUserId >> 40) & 7) | ((pendingId & 0x1F) << 3);
    pendingId >>= 5;
    MI_CpuCopy8(&pendingId, &Wifi.packed[6], 4);
    Wifi.packed[10] = ((pendingId >> 32) & 0x3F) | ((info->pass & 3) << 6);
    Wifi.packed[11] = info->pass >> 2;
    MI_CpuCopy8(&info->randomHistory, &Wifi.packed[12], 2);
    return Wifi.packed;
}

const void *DWCi_BACKUPlGetWifi(void) {
    return Wifi.packed;
}
