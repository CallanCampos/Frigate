// The player's ROM (port_rom.h): finding it, mapping it, checking it, and what the DS's boot leaves in main memory.
//
// The check reads the DS ROM header (GBATEK, "DS Cartridge Header"): the game code at 0x0c, the file name table (FNT)
// at 0x40, the file allocation table (FAT) at 0x48 and the ARM9 overlay table at 0x50, each an offset and a size. The
// FAT has 8 bytes a file: the offsets of its start and end. The FNT starts with a directory table of 8 bytes a
// directory (the offset of its entries from the FNT's start, the ID of its first file, and its parent's ID, or the
// number of directories for the root), whose entries are a length byte (0: the end; 1-127: a file with a name that
// long; 129-255: a subdirectory with a name of length - 128, followed by its ID, 0xf000 and up) and the name. Files get
// IDs in order from their directory's first one; the overlays' files come first and have no names. The overlay table
// has 32 bytes an overlay (port/hal/fs/fs_overlay.c): its file's ID at 0x18, and at 0x08 and 0x1c the sizes of its
// image and of the stored image, which is the one in the file when bit 24 of the latter word says it is compressed.
#include "hal/os/port_app.h"
#include "hal/rom/port_rom.h"
#include "hal/rom/rom_manifest.h"
#include "hal/rom/sha1.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PORT_ROM_MAP_FILE 0
#else
#define PORT_ROM_MAP_FILE 1
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// The header's fields that the check and the system area use
#define HEADER_SIZE 0x200
#define HEADER_GAME_CODE 0x0c
#define HEADER_DEVICE_SIZE 0x14
#define HEADER_FNT_OFFSET 0x40
#define HEADER_FAT_OFFSET 0x48
#define HEADER_ARM9_OVERLAY_OFFSET 0x50
// An overlay's header in the overlay table, and its fields that the check reads
#define OVERLAY_HEADER_SIZE 0x20
#define OVERLAY_RAM_SIZE 0x08
#define OVERLAY_FILE_ID 0x18
#define OVERLAY_STORED_SIZE 0x1c
#define OVERLAY_COMPRESSED (1u << 24)
// The FNT's directory IDs: the root's, and the others after it
#define FNT_ROOT_ID 0xf000
// A path in the FNT, as long as the manifest's ever are, with room to spare
#define PATH_MAX_LENGTH 256
// The files a refusal names; the rest are counted
#define REFUSAL_NAMES 4
// The capacity code of the header is the card's size: 128 KiB shifted left by it
#define DEVICE_SIZE_UNIT (128 * 1024)
// The chip ID's maker byte: Macronix (GBATEK, "DS Cartridge Protocol"), any maker serving as well
#define CARD_ID_MAKER 0xc2
// A card's data read can't reach its first 32 KiB: an address below it reads from it on in blocks of 0x200
#define CARD_UNREADABLE_END 0x8000
#define CARD_UNREADABLE_BLOCK 0x200

static struct {
    const uint8_t *data;
    size_t size;
    BOOL mapped; // by mmap, else read into memory
    PortRomVerdict verdict;
} sRom;

#ifndef PORT_ROM_DEV_PATH
#define PORT_ROM_DEV_PATH ""
#endif

static uint32_t Read32(const uint8_t *data) {
    return (uint32_t)data[0] | (uint32_t)data[1] << 8 | (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

static uint16_t Read16(const uint8_t *data) {
    return (uint16_t)(data[0] | data[1] << 8);
}

static void PrintSha1(char out[41], const uint8_t sha1[20]) {
    for (int i = 0; i < 20; i++) {
        snprintf(out + i * 2, 3, "%02x", sha1[i]);
    }
}

// The files of a dump against the manifest: found tells which manifest entries the FNT has; mismatches count what
// doesn't match, and names lists the first of them
typedef struct ManifestCheck {
    const uint8_t *data;
    size_t size;
    uint32_t fnt;
    uint32_t fntSize;
    uint32_t fat;
    uint32_t fatSize;
    BOOL *found;
    int mismatches;
    char names[REFUSAL_NAMES][PATH_MAX_LENGTH + 32];
    BOOL broken;              // the tables themselves don't hold together
    const char *overlayTable; // what is wrong with the ARM9 overlay table, or NULL
    int overlayMismatches;    // the overlays whose file has another length than their header gives
    uint32_t overlayNames[REFUSAL_NAMES];
} ManifestCheck;

static void AddMismatch(ManifestCheck *check, const char *path, const char *why) {
    if (check->mismatches < REFUSAL_NAMES) {
        snprintf(check->names[check->mismatches], sizeof(check->names[0]), "%s (%s)", path, why);
    }
    check->mismatches++;
}

// Checks one named file of the FNT against the manifest's entry of its path, if it has one
static void CheckFile(ManifestCheck *check, const char *path, uint32_t id) {
    uint8_t sha1[PORT_SHA1_DIGEST_SIZE];
    const PortRomManifestFile *entry = NULL;
    uint32_t start;
    uint32_t end;
    size_t i;

    for (i = 0; i < port_rom_manifest_count; i++) {
        if (strcmp(port_rom_manifest_files[i].path, path) == 0) {
            entry = &port_rom_manifest_files[i];
            break;
        }
    }
    if (entry == NULL) {
        return; // a file the port doesn't know of, which the game never opens
    }
    check->found[i] = TRUE;
    if ((uint64_t)id * 8 + 8 > check->fatSize) {
        AddMismatch(check, path, "not in the allocation table");
        return;
    }
    start = Read32(check->data + check->fat + id * 8);
    end = Read32(check->data + check->fat + id * 8 + 4);
    if (end < start || end - start != entry->size) {
        AddMismatch(check, path, "another size");
        return;
    }
    if (end > check->size) {
        AddMismatch(check, path, "cut off by the end of the file");
        return;
    }
    port_sha1(check->data + start, end - start, sha1);
    if (memcmp(sha1, entry->sha1, sizeof(sha1)) != 0) {
        AddMismatch(check, path, "other contents");
    }
}

// Walks a directory of the FNT and its subdirectories, with path holding the directory's own path (empty for the root)
static void WalkDirectory(ManifestCheck *check, uint32_t directoryId, char *path, size_t pathLength, int depth) {
    uint32_t index = directoryId - FNT_ROOT_ID;
    uint32_t entries;
    uint32_t fileId;

    if (depth > 64 || (uint64_t)index * 8 + 8 > check->fntSize) {
        check->broken = TRUE;
        return;
    }
    entries = check->fnt + Read32(check->data + check->fnt + index * 8);
    fileId = Read16(check->data + check->fnt + index * 8 + 4);
    for (;;) {
        uint8_t kind;
        size_t nameLength;
        size_t length;

        if (entries >= check->fnt + check->fntSize) {
            check->broken = TRUE;
            return;
        }
        kind = check->data[entries++];
        if (kind == 0) {
            return;
        }
        nameLength = kind & 0x7f;
        if (entries + nameLength + ((kind & 0x80) ? 2 : 0) > check->fnt + check->fntSize ||
            pathLength + nameLength + 2 > PATH_MAX_LENGTH) {
            check->broken = TRUE;
            return;
        }
        length = pathLength;
        if (length != 0) {
            path[length++] = '/';
        }
        memcpy(path + length, check->data + entries, nameLength);
        length += nameLength;
        path[length] = '\0';
        entries += (uint32_t)nameLength;
        if (kind & 0x80) {
            uint16_t subdirectory = Read16(check->data + entries);
            entries += 2;
            if (subdirectory < FNT_ROOT_ID) {
                check->broken = TRUE;
                return;
            }
            WalkDirectory(check, subdirectory, path, length, depth + 1);
            if (check->broken) {
                return;
            }
        } else {
            CheckFile(check, path, fileId++);
        }
        path[pathLength] = '\0';
    }
}

// The ARM9 overlay table against the manifest, and then each overlay's file against its header: FS_LoadOverlayInfo
// reads the headers from the table, and FS_LoadOverlayImage needs a file at least as long as the stored image, which
// the decomp's dump has exactly that long. Only the files' lengths in the FAT matter: the host never reads an overlay's
// image, whose code is in the program
static void CheckOverlays(ManifestCheck *check) {
    uint8_t sha1[PORT_SHA1_DIGEST_SIZE];
    uint32_t table = Read32(check->data + HEADER_ARM9_OVERLAY_OFFSET);
    uint32_t tableSize = Read32(check->data + HEADER_ARM9_OVERLAY_OFFSET + 4);
    uint32_t i;

    if (tableSize != port_rom_manifest_overlay_table_size) {
        check->overlayTable = "its ARM9 overlay table has another size";
        return;
    }
    if ((uint64_t)table + tableSize > check->size) {
        check->overlayTable = "its ARM9 overlay table lies past the end of the file";
        return;
    }
    port_sha1(check->data + table, tableSize, sha1);
    if (memcmp(sha1, port_rom_manifest_overlay_table_sha1, sizeof(sha1)) != 0) {
        check->overlayTable = "its ARM9 overlay table has other contents";
        return;
    }
    for (i = 0; i < tableSize / OVERLAY_HEADER_SIZE; i++) {
        const uint8_t *header = check->data + table + i * OVERLAY_HEADER_SIZE;
        uint32_t fileId = Read32(header + OVERLAY_FILE_ID);
        uint32_t stored = Read32(header + OVERLAY_STORED_SIZE);
        uint32_t expected = (stored & OVERLAY_COMPRESSED) ? stored & 0xffffff : Read32(header + OVERLAY_RAM_SIZE);
        uint32_t start;
        uint32_t end;

        if ((uint64_t)fileId * 8 + 8 <= check->fatSize) {
            start = Read32(check->data + check->fat + fileId * 8);
            end = Read32(check->data + check->fat + fileId * 8 + 4);
            if (end >= start && end - start == expected) {
                continue;
            }
        }
        if (check->overlayMismatches < REFUSAL_NAMES) {
            check->overlayNames[check->overlayMismatches] = i;
        }
        check->overlayMismatches++;
    }
}

// Appends the text that printf's format gives to the message at used, as far as it fits
static void Append(char *message, size_t messageSize, size_t *used, const char *format, ...)
    __attribute__((format(printf, 4, 5)));

static void Append(char *message, size_t messageSize, size_t *used, const char *format, ...) {
    va_list args;
    int length;

    if (*used >= messageSize) {
        return;
    }
    va_start(args, format);
    length = vsnprintf(message + *used, messageSize - *used, format, args);
    va_end(args);
    if (length > 0) {
        *used += (size_t)length;
    }
}

// Whether every NitroFS file of the manifest is in the dump, as the manifest has it, and the overlays are
// (CheckOverlays). Writes the refusal to message
static BOOL CheckFiles(const uint8_t *data, size_t size, char *message, size_t messageSize, const char *sha1Text) {
    ManifestCheck check;
    char path[PATH_MAX_LENGTH + 1] = "";
    char expected[41];
    size_t i;
    size_t used;

    memset(&check, 0, sizeof(check));
    check.data = data;
    check.size = size;
    check.fnt = Read32(data + HEADER_FNT_OFFSET);
    check.fntSize = Read32(data + HEADER_FNT_OFFSET + 4);
    check.fat = Read32(data + HEADER_FAT_OFFSET);
    check.fatSize = Read32(data + HEADER_FAT_OFFSET + 4);
    PrintSha1(expected, port_rom_manifest_sha1);
    if ((uint64_t)check.fnt + check.fntSize > size || (uint64_t)check.fat + check.fatSize > size) {
        snprintf(message, messageSize,
                 "its SHA-1 is %s, not the decomp's %s, and its file tables lie past the end of the file", sha1Text,
                 expected);
        return FALSE;
    }
    check.found = calloc(port_rom_manifest_count, sizeof(BOOL));
    if (check.found == NULL) {
        snprintf(message, messageSize, "no memory to check its files");
        return FALSE;
    }
    WalkDirectory(&check, FNT_ROOT_ID, path, 0, 0);
    if (!check.broken) {
        for (i = 0; i < port_rom_manifest_count; i++) {
            if (!check.found[i]) {
                AddMismatch(&check, port_rom_manifest_files[i].path, "missing");
            }
        }
    }
    free(check.found);
    if (check.broken) {
        snprintf(message, messageSize, "its SHA-1 is %s, not the decomp's %s, and its file name table is damaged",
                 sha1Text, expected);
        return FALSE;
    }
    CheckOverlays(&check);
    if (check.mismatches == 0 && check.overlayTable == NULL && check.overlayMismatches == 0) {
        return TRUE;
    }
    used = 0;
    Append(message, messageSize, &used, "its SHA-1 is %s, not the decomp's %s, and", sha1Text, expected);
    if (check.mismatches != 0) {
        Append(message, messageSize, &used,
               " the port's manifest finds %d of the %zu files the game reads changed:", check.mismatches,
               port_rom_manifest_count);
        for (i = 0; i < REFUSAL_NAMES && i < (size_t)check.mismatches; i++) {
            Append(message, messageSize, &used, "%s %s", i == 0 ? "" : ",", check.names[i]);
        }
        if (check.mismatches > REFUSAL_NAMES) {
            Append(message, messageSize, &used, " and %d more", check.mismatches - REFUSAL_NAMES);
        }
    }
    if (check.overlayTable != NULL) {
        Append(message, messageSize, &used, "%s %s, which the game loads its overlays by",
               check.mismatches != 0 ? ";" : "", check.overlayTable);
    }
    if (check.overlayMismatches != 0) {
        Append(message, messageSize, &used,
               "%s the FAT gives the files of %d of the %u overlays another length than their headers do:",
               check.mismatches != 0 ? ";" : "", check.overlayMismatches,
               port_rom_manifest_overlay_table_size / OVERLAY_HEADER_SIZE);
        for (i = 0; i < REFUSAL_NAMES && i < (size_t)check.overlayMismatches; i++) {
            Append(message, messageSize, &used, "%s overlay %u", i == 0 ? "" : ",", check.overlayNames[i]);
        }
        if (check.overlayMismatches > REFUSAL_NAMES) {
            Append(message, messageSize, &used, " and %d more", check.overlayMismatches - REFUSAL_NAMES);
        }
    }
    return FALSE;
}

// The name of a game code's version, for a message about a ROM of another one: Black 2 is IRE?, White 2 IRD?, and the
// fourth letter is the region (O for the US)
static const char *DescribeGameCode(const char *code) {
    if (memcmp(code, "IREO", 4) == 0) {
        return "Pokémon Black 2 (US)";
    }
    if (memcmp(code, "IRDO", 4) == 0) {
        return "Pokémon White 2 (US)";
    }
    if (memcmp(code, "IRE", 3) == 0) {
        return "Pokémon Black 2 of another region";
    }
    if (memcmp(code, "IRD", 3) == 0) {
        return "Pokémon White 2 of another region";
    }
    return "another game";
}

PortRomVerdict port_rom_check(const uint8_t *data, size_t size, char *message, size_t messageSize) {
    uint8_t sha1[PORT_SHA1_DIGEST_SIZE];
    char sha1Text[41];
    char code[5];

    if (messageSize != 0) {
        message[0] = '\0';
    }
    if (size < HEADER_SIZE) {
        snprintf(message, messageSize, "too small for a DS ROM (%zu bytes)", size);
        return PORT_ROM_REFUSED;
    }
    memcpy(code, data + HEADER_GAME_CODE, 4);
    code[4] = '\0';
    for (int i = 0; i < 4; i++) {
        if (code[i] < 0x20 || code[i] > 0x7e) {
            code[i] = '?';
        }
    }
    if (memcmp(code, port_rom_manifest_game_code, 4) != 0) {
        snprintf(message, messageSize, "its game code %s is %s; this program plays %s (%s)", code,
                 DescribeGameCode(code), port_rom_manifest_title, port_rom_manifest_game_code);
        return PORT_ROM_REFUSED;
    }
    port_sha1(data, size, sha1);
    if (memcmp(sha1, port_rom_manifest_sha1, sizeof(sha1)) == 0) {
        return PORT_ROM_EXACT;
    }
    PrintSha1(sha1Text, sha1);
    return CheckFiles(data, size, message, messageSize, sha1Text) ? PORT_ROM_FILES : PORT_ROM_REFUSED;
}

static void CloseRom(void) {
    if (sRom.data != NULL) {
#if PORT_ROM_MAP_FILE
        if (sRom.mapped) {
            munmap((void *)sRom.data, sRom.size);
        } else
#endif
        {
            free((void *)sRom.data);
        }
    }
    memset(&sRom, 0, sizeof(sRom));
}

// Maps the file read-only, or reads it into memory where mapping isn't written yet. Returns FALSE with the reason in
// message
static BOOL MapFile(const char *path, char *message, size_t messageSize) {
#if PORT_ROM_MAP_FILE
    struct stat info;
    void *data;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        snprintf(message, messageSize, "can't open it: %s", strerror(errno));
        return FALSE;
    }
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode)) {
        snprintf(message, messageSize, "not a file");
        close(fd);
        return FALSE;
    }
    if (info.st_size == 0) {
        snprintf(message, messageSize, "empty");
        close(fd);
        return FALSE;
    }
    data = mmap(NULL, (size_t)info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (data == MAP_FAILED) {
        snprintf(message, messageSize, "can't map it: %s", strerror(errno));
        return FALSE;
    }
    sRom.data = data;
    sRom.size = (size_t)info.st_size;
    sRom.mapped = TRUE;
    return TRUE;
#else
    FILE *file = fopen(path, "rb");
    long size;
    uint8_t *data;

    if (file == NULL) {
        snprintf(message, messageSize, "can't open it: %s", strerror(errno));
        return FALSE;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        snprintf(message, messageSize, "can't read its size");
        fclose(file);
        return FALSE;
    }
    data = malloc((size_t)size);
    if (data == NULL || fread(data, 1, (size_t)size, file) != (size_t)size) {
        snprintf(message, messageSize, "can't read it");
        free(data);
        fclose(file);
        return FALSE;
    }
    fclose(file);
    sRom.data = data;
    sRom.size = (size_t)size;
    sRom.mapped = FALSE;
    return TRUE;
#endif
}

PortRomVerdict port_rom_open(const char *path, char *message, size_t messageSize) {
    CloseRom();
    if (!MapFile(path, message, messageSize)) {
        return PORT_ROM_REFUSED;
    }
    sRom.verdict = port_rom_check(sRom.data, sRom.size, message, messageSize);
    if (sRom.verdict == PORT_ROM_REFUSED) {
        CloseRom();
    }
    return sRom.verdict;
}

const char *port_rom_default_path(void) {
    const char *path = port_app_option("rom");
    FILE *file;

    if (path != NULL && path[0] != '\0') {
        return path;
    }
    path = getenv("POKEBW2_ROM");
    if (path != NULL && path[0] != '\0') {
        return path;
    }
    // The development default, only when it is there
    path = PORT_ROM_DEV_PATH;
    if (path[0] != '\0' && (file = fopen(path, "rb")) != NULL) {
        fclose(file);
        return path;
    }
    return NULL;
}

void port_rom_boot(void) {
    if (sRom.data == NULL) {
        char message[1024];
        const char *path = port_rom_default_path();
        PortRomVerdict verdict;

        if (path == NULL) {
            fprintf(stderr,
                    "port: rom: no ROM. Give the path of your dump of %s (game code %s) with --rom PATH, or in the "
                    "POKEBW2_ROM environment variable\n",
                    port_rom_manifest_title, port_rom_manifest_game_code);
            exit(PORT_ROM_EXIT_STATUS);
        }
        verdict = port_rom_open(path, message, sizeof(message));
        if (verdict == PORT_ROM_REFUSED) {
            fprintf(stderr, "port: rom: %s: %s\n", path, message);
            exit(PORT_ROM_EXIT_STATUS);
        }
        if (verdict == PORT_ROM_EXACT) {
            fprintf(stderr, "port: rom: %s: %s, the dump the decomp builds\n", path, port_rom_manifest_title);
        } else {
            fprintf(stderr,
                    "port: rom: %s: %s; not the decomp's dump, but its NitroFS files and overlay table match the "
                    "port's manifest\n",
                    path, port_rom_manifest_title);
        }
    }
    port_rom_place_system_area();
}

const uint8_t *port_rom_data(void) {
    return sRom.data;
}

size_t port_rom_size(void) {
    return sRom.size;
}

PortRomVerdict port_rom_verdict(void) {
    return sRom.verdict;
}

void port_rom_read(void *dst, uint32_t offset, uint32_t length) {
    uint8_t *out = dst;
    size_t available = 0;

    if (offset < sRom.size) {
        available = sRom.size - offset;
        if (available > length) {
            available = length;
        }
        memcpy(out, sRom.data + offset, available);
    }
    memset(out + available, 0xff, length - available);
}

// The card's capacity, from the header's code: 128 KiB shifted left by it (128 KiB without a ROM)
static uint64_t Capacity(void) {
    return (uint64_t)DEVICE_SIZE_UNIT << (sRom.data != NULL ? sRom.data[HEADER_DEVICE_SIZE] & 0x1f : 0);
}

void port_rom_card_read(void *dst, uint32_t address, uint32_t length) {
    uint8_t *out = dst;
    uint64_t capacity = Capacity();

    while (length != 0) {
        uint64_t at = address % capacity;
        uint64_t run;

        if (at < CARD_UNREADABLE_END) {
            // The block's bytes from 0x8000 on, to the end of the block
            at = CARD_UNREADABLE_END + (at & (CARD_UNREADABLE_BLOCK - 1));
            run = CARD_UNREADABLE_END + CARD_UNREADABLE_BLOCK - at;
        } else {
            run = capacity - at; // to where the address wraps
        }
        if (run > length) {
            run = length;
        }
        port_rom_read(out, (uint32_t)at, (uint32_t)run);
        out += run;
        address += (uint32_t)run;
        length -= (uint32_t)run;
    }
}

uint32_t port_rom_card_id(void) {
    uint64_t capacity = Capacity();
    uint32_t sizeCode;

    // GBATEK's size byte: N + 1 MiB for 0x00-0x7f, (0x100 - N) * 256 MiB for 0xf0-0xff; a card under 1 MiB has 0
    if (capacity >= 256u * 1024 * 1024) {
        sizeCode = 0x100 - (uint32_t)(capacity / (256u * 1024 * 1024));
    } else if (capacity >= 1024 * 1024) {
        sizeCode = (uint32_t)(capacity / (1024 * 1024)) - 1;
    } else {
        sizeCode = 0;
    }
    return CARD_ID_MAKER | (sizeCode & 0xff) << 8;
}

void port_rom_place_system_area(void) {
    if (sRom.data == NULL) {
        return;
    }
    memcpy((void *)HW_ROM_HEADER_BUF, sRom.data, HW_CARD_ROM_HEADER_SIZE);
    memcpy((void *)HW_CARD_ROM_HEADER, sRom.data, HW_CARD_ROM_HEADER_SIZE);
    OS_GetSystemWork()->bootCardId = port_rom_card_id();
}
