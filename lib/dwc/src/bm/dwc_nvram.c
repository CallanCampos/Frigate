// DWC's access to the NVRAM of the console (TWL-DWC 5.3, bm), where the Wi-Fi settings are kept. The file name is a
// guess: the ROM names none of it. It finds the settings in the NVRAM from the header, once, and reads and writes the
// pages of NTR consoles (0 to 3) and of the TWL's (0x11 to 0x13), which the console only has from version 0x20 on,
// with the retries that the calls of NitroSDK's NVRAM library need.

#include "bm_internal.h"
#include "nitro/os.h"

// What the header of the NVRAM holds: the version byte, and the offset of the user settings divided by 8
#define NVRAM_ADDRESS_VERSION 0x1fe
#define NVRAM_ADDRESS_USER_SETTINGS 0x20
// The Wi-Fi settings are four pages of 0x100 bytes before the user settings, and the TWL's three of 0x200 bytes before
// them
#define NVRAM_NTR_PAGE_SIZE 0x100
#define NVRAM_TWL_PAGE_SIZE 0x200
#define NVRAM_WIFI_SETTINGS_SIZE 0x400
#define NVRAM_TWL_PAGES_SIZE 0x600

#define NVRAM_VERSION_NO_EXTRA_PAGES 0x10
#define NVRAM_VERSION_EXTRA_PAGES 0x20
#define NVRAM_TWL_FIRST_PAGE 0x11

#define NVRAM_TRIES 5
#define NVRAM_WRITE_TRIES 3

// The offset of the Wi-Fi settings in the NVRAM, -1 until it is read
static int sSettingsOffset = -1;

// The version byte, and the buffers that the ARM7 reads the header into, a cache line each. The file is built with
// -ipa file: the objects share one section, reached from the byte, and the two buffers lie in the reverse of their
// order
static u8 sVersion;
static u16 sUserSettingsBuffer[0x10] ATTRIBUTE_ALIGN(32);
static u8 sVersionBuffer[0x20] ATTRIBUTE_ALIGN(32);

static BOOL func_0205de18(int page, u32 *address, u32 *size);
static int func_0205dee8(u32 address, u16 size, void *buf);
static int func_0205df50(u32 address, u16 size, const void *src);
static int func_0205dfb8(u32 address, u16 size, const void *src);

int func_0205dbc0(void) {
    int result;

    if (sSettingsOffset != -1) {
        return 0;
    }
    NVRAM_Init();
    result = 0;
    DC_InvalidateRange(sVersionBuffer, sizeof(sVersionBuffer));
    // A header that can't be read ends it all: the second read is skipped
    do {
        if (func_0205dee8(NVRAM_ADDRESS_VERSION, sizeof(sVersionBuffer), sVersionBuffer) == 0) {
            sVersion = sVersionBuffer[0];
        } else {
            result = -1;
            break;
        }
        DC_InvalidateRange(sUserSettingsBuffer, sizeof(sUserSettingsBuffer));
        if (func_0205dee8(NVRAM_ADDRESS_USER_SETTINGS, sizeof(sUserSettingsBuffer), sUserSettingsBuffer) == 0) {
            sSettingsOffset = sUserSettingsBuffer[0] * 8 - NVRAM_WIFI_SETTINGS_SIZE;
        } else {
            result = -1;
        }
    } while (0);
    return result;
}

u32 func_0205dc78(void) {
    if (func_0205dbc0() != 0) {
        return 0;
    }
    if (sVersion == 0xff) {
        return NVRAM_VERSION_NO_EXTRA_PAGES;
    }
    return sVersion;
}

int func_0205dca4(void *dst, u32 size, u32 page) {
    u32 address;
    u32 pageSize;
    int result;

    result = func_0205dbc0();
    if (result != 0) {
        return result;
    }
    if (dst == NULL) {
        return -1;
    }
    if (!func_0205de18(page, &address, &pageSize)) {
        return -1;
    }
    if (pageSize > size) {
        pageSize = size;
    }
    DC_InvalidateRange(dst, pageSize);
    if (func_0205dee8(address, pageSize, dst) == 0) {
        return pageSize;
    }
    return -1;
}

int func_0205dd40(u32 page, const void *src, u32 size) {
    u32 address;
    u32 pageSize;
    int result;
    int tries;

    result = func_0205dbc0();
    if (result != 0) {
        return result;
    }
    if (src == NULL) {
        result = -1;
    } else if (!func_0205de18(page, &address, &pageSize)) {
        result = -1;
    } else {
        if (pageSize > size) {
            pageSize = size;
        }
        DC_StoreRange(src, pageSize);
        result = -1;
        for (tries = 0; tries < NVRAM_WRITE_TRIES; tries++) {
            int verify;

            if (func_0205df50(address, pageSize, src) != 0) {
                result = -1;
                break;
            }
            verify = func_0205dfb8(address, pageSize, src);
            if (verify == NVRAM_RESULT_SUCCESS) {
                result = pageSize;
                break;
            }
            if (verify != NVRAM_RESULT_VERIFY_ERROR) {
                result = -1;
                break;
            }
        }
    }
    return result;
}

// Where a page is in the NVRAM, and how long it is; FALSE when there is no such page
static BOOL func_0205de18(int page, u32 *address, u32 *size) {
    switch (page) {
    case 0:
    case 1:
    case 2:
    case 3:
        *address = sSettingsOffset + page * NVRAM_NTR_PAGE_SIZE;
        *size = NVRAM_NTR_PAGE_SIZE;
        return TRUE;
    case NVRAM_TWL_FIRST_PAGE:
    case NVRAM_TWL_FIRST_PAGE + 1:
    case NVRAM_TWL_FIRST_PAGE + 2:
        if (func_0205dc78() < NVRAM_VERSION_EXTRA_PAGES) {
            *address = -1;
            *size = 0;
            return FALSE;
        }
        *address = sSettingsOffset - NVRAM_TWL_PAGES_SIZE + (page - NVRAM_TWL_FIRST_PAGE) * NVRAM_TWL_PAGE_SIZE;
        *size = NVRAM_TWL_PAGE_SIZE;
        return TRUE;
    default:
        *address = -1;
        *size = 0;
        return FALSE;
    }
}

// The calls of the NVRAM library, tried again while another call keeps it busy, with a millisecond's wait if threads
// and alarms are there to wait with
static int func_0205dee8(u32 address, u16 size, void *buf) {
    int result;
    int tries;

    for (tries = 0; tries < NVRAM_TRIES; tries++) {
        result = NVRAM_ReadDataBytes(address, size, buf);
        if (result != NVRAM_RESULT_BUSY) {
            break;
        }
        if (OS_IsTickAvailable() && OS_IsAlarmAvailable()) {
            OS_Sleep(1);
        }
    }
    return result;
}

static int func_0205df50(u32 address, u16 size, const void *src) {
    int result;
    int tries;

    for (tries = 0; tries < NVRAM_TRIES; tries++) {
        result = NVRAM_WriteDataBytes(address, size, src);
        if (result != NVRAM_RESULT_BUSY) {
            break;
        }
        if (OS_IsTickAvailable() && OS_IsAlarmAvailable()) {
            OS_Sleep(1);
        }
    }
    return result;
}

static int func_0205dfb8(u32 address, u16 size, const void *src) {
    int result;
    int tries;

    for (tries = 0; tries < NVRAM_TRIES; tries++) {
        result = NVRAM_VerifyDataBytes(address, size, src);
        if (result != NVRAM_RESULT_BUSY) {
            break;
        }
        if (OS_IsTickAvailable() && OS_IsAlarmAvailable()) {
            OS_Sleep(1);
        }
    }
    return result;
}
