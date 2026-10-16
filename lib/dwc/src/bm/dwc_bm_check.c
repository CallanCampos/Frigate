// DWC's check of the Wi-Fi settings in the NVRAM (TWL-DWC 5.3, bm); the file name is a guess, and so is whether this is
// DWC's or TwlWiFi's network configuration layer. It reads the pages of the settings of an NTR console, and of a TWL
// console too, checks the CRC16 of each and what the connection in it holds, and writes back the pages with the bad
// ones emptied and the mask of the connections in use made again. DWC_BM_Init is the only caller of the two checks;
// one more function reads the pages for the game's local configuration (wih_dwc.c).

#include "bm_internal.h"
#include "dwc/bm.h"
#include "nitro/math.h"
#include "nitro/mi.h"

// The NTR settings are four pages, the first three of which each hold a connection, and the TWL's three pages each hold
// one too. The TWL pages are there from version 0x20 of the NVRAM on
#define NTR_PAGE_COUNT 4
#define TWL_PAGE_COUNT 3
#define CONNECTION_COUNT 3
#define TWL_FIRST_VERSION 0x20
#define TWL_FIRST_PAGE 0x11

// What a page's CRC16 is made of: all that comes before it
#define NTR_CRC_SIZE (sizeof(DWCBmNtrPage) - sizeof(u16))
#define TWL_EXTRA_CRC_SIZE (sizeof(DWCBmTwlExtra) - sizeof(u16))

// The work area of the NTR check is 0x700 bytes, and the TWL check's 0x1000: the pages, and the table that makes their
// CRC16
#define NTR_WORK_SIZE 0x700
#define TWL_WORK_SIZE DWC_BM_WORK_SIZE

typedef struct NtrWork {
    DWCBmNtrPage pages[NTR_PAGE_COUNT];
    MATHCRC16Table crcTable;
} NtrWork;

typedef struct TwlWork {
    DWCBmTwlPage twlPages[TWL_PAGE_COUNT];
    DWCBmNtrPage ntrPages[NTR_PAGE_COUNT];
    MATHCRC16Table crcTable;
} TwlWork;

static DWCBmReadResult func_0205e020(DWCBmNtrPage *pages);
static int func_0205e160(DWCBmNtrPage *pages, const MATHCRC16Table *table);
static int func_0205e380(TwlWork *work, const MATHCRC16Table *table);
static void func_0205e540(DWCBmNtrPage *pages, int index);
static void func_0205e56c(DWCBmTwlPage *pages, int index);
static BOOL func_0205e598(DWCBmNtrPage *pages);
static BOOL func_0205e614(DWCBmTwlPage *pages);
static BOOL func_0205e668(DWCBmNtrPage *pages, const MATHCRC16Table *table);
static BOOL func_0205e714(DWCBmTwlPage *pages, const MATHCRC16Table *table);
static BOOL func_0205e7a8(const DWCBmConnection *connection);
static BOOL func_0205e7f8(const DWCBmConnection *connection, const DWCBmTwlExtra *extra);
static BOOL func_0205e870(const DWCBmConnection *connection);
static void func_0205e93c(u32 bits, u8 *mask);
static BOOL func_0205e968(const u8 *ssid);
static BOOL func_0205e990(const u8 *address, const u8 *mask);
static BOOL func_0205ea10(const u8 *address);

// Reads the four pages of the NTR settings
static DWCBmReadResult func_0205e020(DWCBmNtrPage *pages) {
    if (func_0205e598(pages)) {
        return DWC_BM_READ_OK;
    }
    return DWC_BM_READ_FAILED;
}

// Whether the TWL's pages could be read, which is only tried when the console has them and the NTR ones were read
static inline BOOL readTwlPages(TwlWork *twl, BOOL ntrRead) {
    if (func_0205dc78() >= TWL_FIRST_VERSION) {
        if (ntrRead) {
            if (func_0205e614(twl->twlPages)) {
                return TRUE;
            }
        }
        return FALSE;
    }
    return FALSE;
}

// Reads the settings into the work area of the local configuration: the NTR pages at +0x600, and the TWL's at +0
DWCBmReadResult func_0205e038(void *work, int unused) {
    TwlWork *twl = work;
    BOOL ntrRead = func_0205e020(twl->ntrPages) == DWC_BM_READ_OK;

    if (readTwlPages(twl, ntrRead)) {
        return DWC_BM_READ_OK;
    }
    return DWC_BM_READ_FAILED;
}

// DWC_BM_Init's check for an NTR console: a work area of 0x700 bytes
int func_0205e094(void *work) {
    NtrWork *ntr = work;

    MI_CpuFill8(work, 0, NTR_WORK_SIZE);
    if (func_0205dbc0() != 0) {
        return DWC_BM_CHECK_READ_FAILED;
    }
    MATHi_CRC16InitTableRev(&ntr->crcTable, 0xA001);
    if (!func_0205e598(ntr->pages)) {
        return DWC_BM_CHECK_READ_FAILED;
    }
    return func_0205e160(ntr->pages, &ntr->crcTable);
}

// The same for a TWL console, with a work area of 0x1000 bytes
int func_0205e0f0(void *work) {
    TwlWork *twl = work;

    MI_CpuFill8(work, 0, TWL_WORK_SIZE);
    if (func_0205dbc0() != 0) {
        return DWC_BM_CHECK_READ_FAILED;
    }
    MATHi_CRC16InitTableRev(&twl->crcTable, 0xA001);
    if (!func_0205e598(twl->ntrPages)) {
        return DWC_BM_CHECK_READ_FAILED;
    }
    if (!func_0205e614(twl->twlPages)) {
        return DWC_BM_CHECK_READ_FAILED;
    }
    return func_0205e380(twl, &twl->crcTable);
}

// Checks the four pages of the NTR settings and makes them whole: a page whose CRC16 or connection is wrong is emptied
// (the Wi-Fi ID of the first two pages is copied from the other, when that one is fine), and the mask of the
// connections in use is made again. Returns 0, or what was lost
static int func_0205e160(DWCBmNtrPage *pages, const MATHCRC16Table *table) {
    BOOL valid[NTR_PAGE_COUNT];
    u8 usedMask;
    u8 newUsedMask;
    BOOL lost;
    u16 crc;
    int i;

    MI_CpuFill8(valid, 0, sizeof(valid));
    for (i = 0; i < CONNECTION_COUNT; i++) {
        crc = MATH_CalcCRC16(table, &pages[i], NTR_CRC_SIZE);

        if (crc == pages[i].crc16) {
            if (func_0205e7a8(&pages[i].connection)) {
                valid[i] = TRUE;
            }
        }
    }
    // The fourth page holds no connection
    crc = MATH_CalcCRC16(table, &pages[3], NTR_CRC_SIZE);
    if (crc == pages[3].crc16) {
        valid[3] = TRUE;
    }
    if (valid[0] && valid[1] && valid[2] && valid[3]) {
        return 0;
    }

    usedMask = pages[0].usedMask;
    if (!valid[0]) {
        usedMask = 0;
        func_0205e540(pages, 0);
        if (valid[1]) {
            usedMask = pages[1].usedMask;
            MI_CpuCopy8(pages[1].wifiId, pages[0].wifiId, DWC_BM_PACKED_SIZE);
        }
    }
    if (!valid[1]) {
        func_0205e540(pages, 1);
        if (valid[0]) {
            MI_CpuCopy8(pages[0].wifiId, pages[1].wifiId, DWC_BM_PACKED_SIZE);
        }
    }
    if (!valid[2]) {
        func_0205e540(pages, 2);
    }
    if (!valid[3]) {
        MI_CpuClear16(&pages[3], sizeof(DWCBmNtrPage));
    }

    lost = FALSE;
    newUsedMask = 0;
    for (i = 0; i < CONNECTION_COUNT; i++) {
        if (pages[i].connection.status != DWC_BM_CONNECTION_UNUSED) {
            newUsedMask |= (u8)(1 << i);
        } else if (usedMask & (1 << i)) {
            lost = TRUE;
        }
    }
    pages[0].usedMask = newUsedMask;
    pages[1].usedMask = newUsedMask;
    if (!func_0205e668(pages, table)) {
        return DWC_BM_CHECK_WRITE_FAILED;
    }

    if (!valid[0] && !valid[1] && !valid[2]) {
        return 0;
    }
    if (!valid[0] && !valid[1]) {
        return DWC_BM_CHECK_WIFI_ID_LOST;
    }
    if (lost) {
        return DWC_BM_CHECK_CONNECTION_LOST;
    }
    return 0;
}

// The same for a TWL console, with the NTR pages checked first and their result kept unless it is 0. When the first two
// TWL pages are both bad, all three connections are taken as having been in use
static int func_0205e380(TwlWork *work, const MATHCRC16Table *table) {
    DWCBmTwlPage *pages = work->twlPages;
    int ntrResult = func_0205e160(work->ntrPages, table);
    BOOL valid[TWL_PAGE_COUNT];
    u8 usedMask;
    u8 newUsedMask;
    BOOL lost;
    int i;

    MI_CpuFill8(valid, 0, sizeof(valid));
    for (i = 0; i < TWL_PAGE_COUNT; i++) {
        u16 ntrCrc = MATH_CalcCRC16(table, &pages[i].ntr, NTR_CRC_SIZE);
        u16 extraCrc = MATH_CalcCRC16(table, &pages[i].extra, TWL_EXTRA_CRC_SIZE);

        if (ntrCrc == pages[i].ntr.crc16 && extraCrc == pages[i].extra.crc16) {
            if (func_0205e7f8(&pages[i].ntr.connection, &pages[i].extra)) {
                valid[i] = TRUE;
            }
        }
    }
    if (valid[0] && valid[1] && valid[2]) {
        return ntrResult;
    }

    usedMask = pages[0].ntr.usedMask;
    if (!valid[0]) {
        func_0205e56c(pages, 0);
        if (valid[1]) {
            usedMask = pages[1].ntr.usedMask;
        } else {
            usedMask = 7;
        }
    }
    if (!valid[1]) {
        func_0205e56c(pages, 1);
    }
    if (!valid[2]) {
        func_0205e56c(pages, 2);
    }

    lost = FALSE;
    newUsedMask = 0;
    for (i = 0; i < CONNECTION_COUNT; i++) {
        if (pages[i].ntr.connection.status != DWC_BM_CONNECTION_UNUSED) {
            newUsedMask |= (u8)(1 << i);
        } else if (usedMask & (1 << i)) {
            lost = TRUE;
        }
    }
    pages[0].ntr.usedMask = newUsedMask;
    pages[1].ntr.usedMask = newUsedMask;
    if (!func_0205e714(pages, table)) {
        return DWC_BM_CHECK_WRITE_FAILED;
    }

    if (!valid[0] && !valid[1] && !valid[2]) {
        lost = FALSE;
    }
    if (ntrResult == 0) {
        if (lost) {
            return DWC_BM_CHECK_CONNECTION_LOST;
        }
        return 0;
    }
    return ntrResult;
}

// Empties a page, and marks its connection as not in use
static void func_0205e540(DWCBmNtrPage *pages, int index) {
    MI_CpuClear16(&pages[index], sizeof(DWCBmNtrPage));
    pages[index].connection.status = DWC_BM_CONNECTION_UNUSED;
}

static void func_0205e56c(DWCBmTwlPage *pages, int index) {
    MI_CpuClear16(&pages[index], sizeof(DWCBmTwlPage));
    pages[index].ntr.connection.status = DWC_BM_CONNECTION_UNUSED;
}

// Reads the four pages of the NTR settings; TRUE when all could be read
static BOOL func_0205e598(DWCBmNtrPage *pages) {
    BOOL ok = TRUE;
    int i;

    for (i = 0; i < CONNECTION_COUNT; i++) {
        BOOL read = FALSE;

        if (ok) {
            int size = func_0205dca4(&pages[i], sizeof(DWCBmNtrPage), i);

            if (size >= 0) {
                read = TRUE;
            }
        }
        ok = read;
    }
    if (ok) {
        if (func_0205dca4(&pages[3], sizeof(DWCBmNtrPage), 3) >= 0) {
            return TRUE;
        }
    }
    return FALSE;
}

// Reads the three pages of the TWL settings
static BOOL func_0205e614(DWCBmTwlPage *pages) {
    int i;
    BOOL ok = TRUE;

    for (i = 0; i < TWL_PAGE_COUNT; i++) {
        ok = ok && func_0205dca4(&pages[i], sizeof(DWCBmTwlPage), i + TWL_FIRST_PAGE) >= 0;
    }
    return ok;
}

// Writes the four pages of the NTR settings, each with the CRC16 of what comes before it
static BOOL func_0205e668(DWCBmNtrPage *pages, const MATHCRC16Table *table) {
    int i;
    BOOL ok = TRUE;

    for (i = 0; i < CONNECTION_COUNT; i++) {
        pages[i].crc16 = MATH_CalcCRC16(table, &pages[i], NTR_CRC_SIZE);
        ok = ok && func_0205dd40(i, &pages[i], sizeof(DWCBmNtrPage)) >= 0;
    }
    pages[3].crc16 = MATH_CalcCRC16(table, &pages[3], NTR_CRC_SIZE);
    if (ok) {
        if (func_0205dd40(3, &pages[3], sizeof(DWCBmNtrPage)) >= 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL func_0205e714(DWCBmTwlPage *pages, const MATHCRC16Table *table) {
    int i;
    BOOL ok = TRUE;

    for (i = 0; i < TWL_PAGE_COUNT; i++) {
        pages[i].ntr.crc16 = MATH_CalcCRC16(table, &pages[i].ntr, NTR_CRC_SIZE);
        pages[i].extra.crc16 = MATH_CalcCRC16(table, &pages[i].extra, TWL_EXTRA_CRC_SIZE);
        ok = ok && func_0205dd40(i + TWL_FIRST_PAGE, &pages[i], sizeof(DWCBmTwlPage)) >= 0;
    }
    return ok;
}

// Whether a connection of an NTR page is one that the console could have made: unused, or a kind from 0 to 2 with a
// network name and the address settings in order
static BOOL func_0205e7a8(const DWCBmConnection *connection) {
    if (connection->status == DWC_BM_CONNECTION_UNUSED) {
        return TRUE;
    }
    if (connection->status > 2) {
        return FALSE;
    }
    if (!func_0205e968(connection->ssid)) {
        return FALSE;
    }
    if (func_0205e870(connection)) {
        return TRUE;
    }
    return FALSE;
}

// The same for the first half of a TWL page, whose kinds above 2 also need the flags of the second half to be 0 or 1
static BOOL func_0205e7f8(const DWCBmConnection *connection, const DWCBmTwlExtra *extra) {
    if (connection->status == DWC_BM_CONNECTION_UNUSED) {
        return TRUE;
    }
    if (connection->status <= 2) {
        return func_0205e7a8(connection);
    }
    if (!func_0205e968(connection->ssid)) {
        return FALSE;
    }
    if (!func_0205e870(connection)) {
        return FALSE;
    }
    if (extra->unk82 > 1) {
        return FALSE;
    }
    if (extra->unk83 <= 1) {
        return TRUE;
    }
    return FALSE;
}

// Whether the address settings of a connection are in order: when it has an address of its own, a gateway that can be
// one, a mask length of at most 32 and an address that suits the mask; and when it has DNS servers, either of them
// can be one
static BOOL func_0205e870(const DWCBmConnection *connection) {
    if (connection->ip[0] != 0 || connection->ip[1] != 0 || connection->ip[2] != 0 || connection->ip[3] != 0) {
        u8 mask[4];

        if (!func_0205ea10(connection->gateway)) {
            return FALSE;
        }
        if (connection->netmaskBits > 0x20) {
            return FALSE;
        }
        func_0205e93c(connection->netmaskBits, mask);
        if (!func_0205e990(connection->ip, mask)) {
            return FALSE;
        }
    }
    if (connection->dns[0][0] != 0 || connection->dns[0][1] != 0 || connection->dns[0][2] != 0 ||
        connection->dns[0][3] != 0) {
        if (!func_0205ea10(connection->dns[0]) && !func_0205ea10(connection->dns[1])) {
            return FALSE;
        }
    }
    return TRUE;
}

// The four bytes of the mask that has the given number of its high bits set (a copy of dwc_backup.c's)
static void func_0205e93c(u32 bits, u8 *mask) {
    int i;
    u32 value = 0xFFFFFFFF ^ (0xFFFFFFFF >> bits);

    for (i = 0; i < 4; i++) {
        mask[i] = value >> (24 - i * 8);
    }
}

// Whether any of the 32 bytes of a network name is not 0
static BOOL func_0205e968(const u8 *ssid) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (ssid[i] != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

// Whether an address can be used with a mask: a usable address, not one that makes all but the lowest bit set together
// with the mask, and with a bit that the mask leaves to the host
static BOOL func_0205e990(const u8 *address, const u8 *mask) {
    u32 addressBits;
    u32 maskBits;

    if (!func_0205ea10(address)) {
        return FALSE;
    }
    MI_CpuCopy8(address, &addressBits, 4);
    MI_CpuCopy8(mask, &maskBits, 4);
    if ((addressBits | maskBits) == 0xFFFFFFFE) {
        return FALSE;
    }
    return (addressBits & ~maskBits) != 0;
}

// Whether the first byte of an address is neither 0, 127 nor above 223 (a copy of dwc_backup.c's)
static BOOL func_0205ea10(const u8 *address) {
    u8 first = address[0];

    if (first == 127) {
        return FALSE;
    }
    if (first < 1) {
        return FALSE;
    }
    return first <= 223;
}
