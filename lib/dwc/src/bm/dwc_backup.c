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
} sWifiId;

// The buffer that the Wi-Fi ID is written to the NTR console's settings with: a page, a scratch buffer for the write,
// and the table that the page's CRC16 is made with
typedef struct PageWriteBuffer {
    DWCBmNtrPage page;
    u8 scratch[0x100];
    MATHCRC16Table crcTable;
} PageWriteBuffer;

static BOOL ReadNtrPages(DWCBmNtrPage *pages, u32 first, u32 count);
static BOOL WriteNtrPage(const DWCBmNtrPage *page, u32 index, void *scratch);
static BOOL ReadTwlPages(DWCBmTwlPage *pages, u32 first, u32 count);

BOOL DWCBm_ReadNtrSettings(void *buf) {
    return ReadNtrPages(buf, 0, 3);
}

BOOL DWCBm_ReadTwlSettings(void *buf) {
    BOOL result = FALSE;

    if (ReadTwlPages(buf, 0, 3)) {
        result = TRUE;
    }
    return result;
}

void DWCBm_GetWifiIdInfo(DWCBmWifiIdInfo *info) {
    MI_CpuCopy8(sWifiId.packed, &info->id, 6);
    info->id &= WIFI_ID_MASK;
    MI_CpuCopy8(&sWifiId.packed[5], &info->pendingId, 6);
    info->pendingId >>= 3;
    info->pendingId &= WIFI_ID_MASK;
    MI_CpuCopy8(&sWifiId.packed[10], &info->number, 2);
    info->number >>= 6;
    info->number &= 0x3FF;
    MI_CpuCopy8(&sWifiId.packed[12], &info->salt, 2);
}

static BOOL ReadNtrPages(DWCBmNtrPage *pages, u32 first, u32 count) {
    u32 index;

    for (index = first; index < first + count; index++) {
        if (func_0205dca4(&pages[index - first], sizeof(DWCBmNtrPage), index) == -1) {
            return FALSE;
        }
    }
    return TRUE;
}

// The scratch buffer that the callers pass is not used
static BOOL WriteNtrPage(const DWCBmNtrPage *page, u32 index, void *scratch) {
    return func_0205dd40(index, page, sizeof(DWCBmNtrPage)) != -1;
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

BOOL DWCBm_WriteWifiIdInfo(const DWCBmWifiIdInfo *info, void *work) {
    PageWriteBuffer *buf = work;
    int index;

    DWCBm_PackWifiIdInfo(info);
    MATHi_CRC16InitTableRev(&buf->crcTable, 0xA001);
    for (index = 0; index < 2; index++) {
        // The page can't be read back before it is written: the game stops
        if (!ReadNtrPages(&buf->page, index, 1)) {
            OS_Terminate();
            return FALSE;
        }
        MI_CpuCopy8(sWifiId.packed, buf->page.wifiId, DWC_BM_PACKED_SIZE);
        buf->page.crc16 = MATH_CalcCRC16(&buf->crcTable, &buf->page, sizeof(DWCBmNtrPage) - sizeof(u16));
        while (!WriteNtrPage(&buf->page, index, buf->scratch)) {
        }
    }
    return func_02056e74() != 0;
}

BOOL DWCBm_ReadAllNtrPages(void *buf) {
    return ReadNtrPages(buf, 0, 4);
}

BOOL DWCBm_WriteNtrPages(void *buf, const u32 flags[4], void *scratch) {
    DWCBmNtrPage *pages = buf;
    int index;

    for (index = 0; index < 4; index++) {
        if (flags[index] != 0) {
            while (!WriteNtrPage(&pages[index], index, scratch)) {
            }
        }
    }
    return func_02056e74() != 0;
}

void DWCBm_SetPackedWifiIdInfo(const void *packed) {
    MI_CpuCopy8(packed, sWifiId.packed, DWC_BM_PACKED_SIZE);
}

u8 DWCBm_CountMaskBits(const u8 *mask) {
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

void DWCBm_MakeMask(u32 bits, u8 *mask) {
    int i;
    u32 value = 0xFFFFFFFF ^ (0xFFFFFFFF >> bits);

    for (i = 0; i < 4; i++) {
        mask[i] = value >> (24 - i * 8);
    }
}

BOOL DWCBm_HasHostBits(const u8 *address, const u8 *mask) {
    u32 addressBits;
    u32 maskBits;

    if (!DWCBm_IsUsableAddress(address)) {
        return FALSE;
    }
    MI_CpuCopy8(address, &addressBits, 4);
    MI_CpuCopy8(mask, &maskBits, 4);
    return (addressBits & ~maskBits) != 0;
}

BOOL DWCBm_IsUsableAddress(const u8 *address) {
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

const u8 *DWCBm_PackWifiIdInfo(const DWCBmWifiIdInfo *info) {
    u64 pendingId = info->pendingId;

    MI_CpuCopy8(info, sWifiId.packed, 5);
    sWifiId.packed[5] = ((info->id >> 40) & 7) | ((pendingId & 0x1F) << 3);
    pendingId >>= 5;
    MI_CpuCopy8(&pendingId, &sWifiId.packed[6], 4);
    sWifiId.packed[10] = ((pendingId >> 32) & 0x3F) | ((info->number & 3) << 6);
    sWifiId.packed[11] = info->number >> 2;
    MI_CpuCopy8(&info->salt, &sWifiId.packed[12], 2);
    return sWifiId.packed;
}

const u8 *DWCBm_GetPackedWifiIdInfo(void) {
    return sWifiId.packed;
}
