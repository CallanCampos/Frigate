// The host ARM7's firmware flash and user settings: what the DS's firmware copies into the shared memory at boot (the
// user settings, NitroSDK's NVRAMConfig, at HW_NVRAM_USER_INFO, with the MAC address after them), which
// OS_GetOwnerInfo, OS_GetMacAddress, OS_GetOwnerRtcOffset and TP_GetUserInfo read; and the NVRAM service (PXI tag
// NVRAM) of the decomp's lib/nitro/src/nvram/nvram.c, over a flash image built from the same settings. Only Wi-Fi
// Connection (DWC) reads and writes the flash.
//
// The flash image is 256 KiB, as the DS's firmware flash, laid out as GBATEK describes it ("DS Firmware Header", "DS
// Firmware Wifi Internet Access Points", "DS Firmware User Settings"): the header's MAC address and user settings
// offset, three access point slots marked as not configured, and the user settings twice at the end, with their update
// counters and CRCs. Everything else reads as erased flash (0xff). Writes stay in memory for the run; nothing is saved
// yet.
//
// The protocol, from nvram.c: a command's first word has the command in bits 8 to 14 and a byte of data, each word
// after it 16 bits; the ARM7 answers with the command and a result in the low byte (0 done, 2 bad parameter, 3 not now,
// 4 busy). Reads (0x23) carry the flash address in 24 bits, the size in 32 and the ARM9's buffer in 32; the status
// register's read (0x22) carries the address of the byte to write it to; page writes (0x25) the address, a size of up
// to a page and the ARM9's data. The ARM9's buffers are found through port_pxi_noted_pointer (64-bit pointers arrive
// cut to 32 bits), from the ones the host's nvram.c noted under the tag (nvram_host.c); one that can't be found fails
// the command with "not now", reported once.
#include "hal/nvram/nvram_settings.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// The user settings keep the DS's layout on the host: the firmware's bytes are copied as they are
_Static_assert(offsetof(NVRAMConfig, owner) == 0x02, "NVRAMConfig.owner");
_Static_assert(offsetof(NVRAMConfig, touch) == 0x58, "NVRAMConfig.touch");
_Static_assert(offsetof(NVRAMConfig, rtcOffset) == 0x68, "NVRAMConfig.rtcOffset");
_Static_assert(offsetof(NVRAMConfig, unk_70) == 0x70, "NVRAMConfig.unk_70");
_Static_assert(offsetof(NVRAMConfig, macAddress) == 0x74, "NVRAMConfig.macAddress");

// The flash: its size, the user settings' two copies (each a page, the CRC covering the first 0x70 bytes), the access
// point slots before them, and the header's fields
#define FLASH_SIZE 0x40000
#define FLASH_PAGE_SIZE 0x100
#define FLASH_USER_SETTINGS_0 0x3fe00
#define FLASH_USER_SETTINGS_1 0x3ff00
#define FLASH_USER_SETTINGS_SIZE 0x70
#define FLASH_ACCESS_POINTS 0x3fa00
#define FLASH_ACCESS_POINT_COUNT 3
#define FLASH_ACCESS_POINT_STATUS 0xe7
#define FLASH_ACCESS_POINT_NOT_CONFIGURED 0xff
#define FLASH_ACCESS_POINT_CRC 0xfe
#define FLASH_HEADER_CONSOLE_TYPE 0x1d
#define FLASH_CONSOLE_DS_LITE 0x20
#define FLASH_HEADER_USER_SETTINGS 0x20
#define FLASH_HEADER_MAC 0x36

// The user settings: version 5, and the flags of settings that the player has completed (bits 10, 11, 13, 14 and 15
// of the halfword with the language), with the year 2012 as the one the date was set in
#define USER_SETTINGS_VERSION 5
#define USER_SETTINGS_FLAGS_DONE 0xec00
#define USER_SETTINGS_DATE_YEAR (2012 - 2000)

// nvram.c's commands and the ARM7's results
#define NVRAM_COMMAND_WRITE_ENABLE 0x20
#define NVRAM_COMMAND_WRITE_DISABLE 0x21
#define NVRAM_COMMAND_READ_STATUS 0x22
#define NVRAM_COMMAND_READ 0x23
#define NVRAM_COMMAND_PAGE_WRITE 0x25
#define NVRAM_COMMAND_CLEAR_ERROR 0x2d
#define NVRAM_RESULT_SUCCESS 0
#define NVRAM_RESULT_INVALID_COMMAND 1
#define NVRAM_RESULT_ILLEGAL_PARAMETER 2
#define NVRAM_RESULT_ILLEGAL_STATUS 3
// The flash's status register: the write enable latch (bit 1); the write in progress (bit 0) is never set, since a
// page is written at once
#define FLASH_STATUS_WRITE_ENABLED 0x02

static PortNvramSettings sSettings = {
    .language = PORT_NVRAM_LANGUAGE_ENGLISH,
    .favoriteColor = 11,
    .birthMonth = 6,
    .birthDay = 23,
    .nickname = { 'P', 'l', 'a', 'y', 'e', 'r' },
    .nicknameLength = 6,
    .commentLength = 0,
    .rtcOffset = 0,
    .macAddress = { 0x02, 0x42, 0x57, 0x32, 0x00, 0x01 },
};

static struct {
    PortArm7SpiCommand command;
    BOOL writeEnabled;
    BOOL reportedPointer;
} sNvram;

static u8 sFlash[FLASH_SIZE];

PortNvramSettings *port_nvram_settings(void) {
    return &sSettings;
}

// The CRC-16 of the firmware (the BIOS's GetCRC16: CRC-16/ARC from the given start)
static u16 Crc16(u16 crc, const u8 *data, u32 size) {
    u32 i;
    int bit;

    for (i = 0; i < size; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? (u16)((crc >> 1) ^ 0xa001) : (u16)(crc >> 1);
        }
    }
    return crc;
}

static void StoreU16(u8 *at, u16 value) {
    at[0] = (u8)value;
    at[1] = (u8)(value >> 8);
}

void port_nvram7_fill_user_info(NVRAMConfig *config) {
    int i;

    memset(config, 0, sizeof(*config));
    config->unk_00 = USER_SETTINGS_VERSION;
    config->owner.favoriteColor = sSettings.favoriteColor & 0xf;
    config->owner.birthday.month = sSettings.birthMonth;
    config->owner.birthday.day = sSettings.birthDay;
    for (i = 0; i < PORT_NVRAM_NICKNAME_MAX; i++) {
        config->owner.nickName[i] = i < sSettings.nicknameLength ? sSettings.nickname[i] : 0;
    }
    config->owner.nickNameLength = sSettings.nicknameLength;
    for (i = 0; i < PORT_NVRAM_COMMENT_MAX; i++) {
        config->owner.comment[i] = i < sSettings.commentLength ? sSettings.comment[i] : 0;
    }
    config->owner.commentLength = sSettings.commentLength;
    port_tp7_calibration(&config->touch);
    config->language = sSettings.language & 7;
    config->unk_64_3 = USER_SETTINGS_FLAGS_DONE >> 3;
    config->unk_66 = USER_SETTINGS_DATE_YEAR;
    config->rtcOffset = sSettings.rtcOffset;
    // The update counter of the newer copy in the flash, and the CRC of the settings
    config->unk_70 = 1 | ((u32)Crc16(0xffff, (const u8 *)config, FLASH_USER_SETTINGS_SIZE) << 16);
    memcpy(config->macAddress, sSettings.macAddress, sizeof(config->macAddress));
}

static void BuildFlash(const NVRAMConfig *config) {
    int i;

    memset(sFlash, 0xff, sizeof(sFlash));
    sFlash[FLASH_HEADER_CONSOLE_TYPE] = FLASH_CONSOLE_DS_LITE;
    StoreU16(&sFlash[FLASH_HEADER_USER_SETTINGS], FLASH_USER_SETTINGS_0 / 8);
    memcpy(&sFlash[FLASH_HEADER_MAC], config->macAddress, sizeof(config->macAddress));
    for (i = 0; i < FLASH_ACCESS_POINT_COUNT; i++) {
        u8 *slot = &sFlash[FLASH_ACCESS_POINTS + i * FLASH_PAGE_SIZE];

        memset(slot, 0, FLASH_PAGE_SIZE);
        slot[FLASH_ACCESS_POINT_STATUS] = FLASH_ACCESS_POINT_NOT_CONFIGURED;
        StoreU16(&slot[FLASH_ACCESS_POINT_CRC], Crc16(0, slot, FLASH_ACCESS_POINT_CRC));
    }
    // The two copies, the second the newer: the settings, their update counter and their CRC
    for (i = 0; i < 2; i++) {
        u8 *copy = &sFlash[i == 0 ? FLASH_USER_SETTINGS_0 : FLASH_USER_SETTINGS_1];

        memcpy(copy, config, FLASH_USER_SETTINGS_SIZE);
        StoreU16(&copy[FLASH_USER_SETTINGS_SIZE], (u16)i);
        StoreU16(&copy[FLASH_USER_SETTINGS_SIZE + 2], (u16)(config->unk_70 >> 16));
    }
}

static void Answer(u32 command, u32 result) {
    port_pxi_reply(PXI_FIFO_TAG_NVRAM, PORT_ARM7_SPI_ANSWER(command, result), FALSE);
}

// The ARM9's buffer of size bytes at the 32 bits of address, or NULL (reported once)
static u8 *ArmBuffer(u32 address, u32 size) {
    const u8 *end;
    u8 *buffer = port_pxi_noted_pointer(PXI_FIFO_TAG_NVRAM, address, &end);

    if (buffer == NULL || size > (u32)(end - buffer)) {
        if (!sNvram.reportedPointer) {
            fprintf(stderr,
                    "port: NVRAM: the ARM9's buffer at %#x (%u bytes) can't be found in host memory; the "
                    "command fails\n",
                    address, size);
            sNvram.reportedPointer = TRUE;
        }
        return NULL;
    }
    return buffer;
}

static u32 ReadStatus(const PortArm7SpiCommand *command) {
    u32 address = ((u32)command->data[0] << 24) | ((u32)command->data[1] << 8) | (command->data[2] >> 8);
    u8 *status = ArmBuffer(address, 1);

    if (status == NULL) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    *status = sNvram.writeEnabled ? FLASH_STATUS_WRITE_ENABLED : 0;
    return NVRAM_RESULT_SUCCESS;
}

static u32 Read(const PortArm7SpiCommand *command) {
    u32 address = ((u32)command->data[0] << 16) | command->data[1];
    u32 size = ((u32)command->data[2] << 16) | command->data[3];
    u8 *buffer;

    if (address >= FLASH_SIZE || size > FLASH_SIZE - address) {
        return NVRAM_RESULT_ILLEGAL_PARAMETER;
    }
    buffer = ArmBuffer(((u32)command->data[4] << 16) | command->data[5], size);
    if (buffer == NULL) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    memcpy(buffer, &sFlash[address], size);
    return NVRAM_RESULT_SUCCESS;
}

// A page write: up to the end of the page, with the write enabled, which the flash disables again
static u32 PageWrite(const PortArm7SpiCommand *command) {
    u32 address = ((u32)command->data[0] << 16) | command->data[1];
    u32 size = command->data[2];
    const u8 *source;

    if (!sNvram.writeEnabled) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    if (address >= FLASH_SIZE || size == 0 || size > FLASH_PAGE_SIZE - (address % FLASH_PAGE_SIZE)) {
        return NVRAM_RESULT_ILLEGAL_PARAMETER;
    }
    source = ArmBuffer(((u32)command->data[3] << 16) | command->data[4], size);
    if (source == NULL) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    memcpy(&sFlash[address], source, size);
    sNvram.writeEnabled = FALSE;
    return NVRAM_RESULT_SUCCESS;
}

static void NvramService(u32 tag, u32 data, BOOL err) {
    u32 command;

    if (err || !port_arm7_spi_collect(&sNvram.command, data)) {
        return;
    }
    command = sNvram.command.command;
    switch (command) {
    case NVRAM_COMMAND_WRITE_ENABLE:
        sNvram.writeEnabled = TRUE;
        Answer(command, NVRAM_RESULT_SUCCESS);
        break;
    case NVRAM_COMMAND_WRITE_DISABLE:
        sNvram.writeEnabled = FALSE;
        Answer(command, NVRAM_RESULT_SUCCESS);
        break;
    case NVRAM_COMMAND_READ_STATUS:
        Answer(command, sNvram.command.words == 3 ? ReadStatus(&sNvram.command) : NVRAM_RESULT_ILLEGAL_PARAMETER);
        break;
    case NVRAM_COMMAND_READ:
        Answer(command, sNvram.command.words == 6 ? Read(&sNvram.command) : NVRAM_RESULT_ILLEGAL_PARAMETER);
        break;
    case NVRAM_COMMAND_PAGE_WRITE:
        Answer(command, sNvram.command.words == 5 ? PageWrite(&sNvram.command) : NVRAM_RESULT_ILLEGAL_PARAMETER);
        break;
    case NVRAM_COMMAND_CLEAR_ERROR:
        Answer(command, NVRAM_RESULT_SUCCESS);
        break;
    default:
        Answer(command, NVRAM_RESULT_INVALID_COMMAND);
        break;
    }
}

void port_nvram7_start(void) {
    NVRAMConfig *config = (NVRAMConfig *)HW_NVRAM_USER_INFO;

    sNvram.command.count = 0;
    sNvram.writeEnabled = FALSE;
    sNvram.reportedPointer = FALSE;
    port_nvram7_fill_user_info(config);
    BuildFlash(config);
    port_pxi_register_service(PXI_FIFO_TAG_NVRAM, NvramService);
}
