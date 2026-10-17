// Unit tests of port/hal/rom: the host's SHA-1 against FIPS 180's examples, and the ROM's check on the player's dump
// and on variants of it made in a private copy-on-write mapping (a trimmed dump, a changed header, a changed or cut
// file, a changed overlay table or overlay file, another version), the system area it leaves, the reads past its end
// and the card's reads. The tests that need the ROM skip with
// status 77 without it (POKEBW2_ROM, else the repository's orig/baserom_<version>.nds).
//
//   port_rom_unit_tests NAME | all
#include "hal/os/port_app.h"
#include "hal/rom/port_rom.h"
#include "hal/rom/rom_manifest.h"
#include "hal/rom/sha1.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define SKIP 77

static int sFailed;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("  check failed: %s\n", what);
        sFailed = 1;
    }
}

static void CheckContains(const char *text, const char *part) {
    if (strstr(text, part) == NULL) {
        printf("  check failed: \"%s\" doesn't say \"%s\"\n", text, part);
        sFailed = 1;
    }
}

static void Hex(char *out, const uint8_t *bytes, size_t count) {
    for (size_t i = 0; i < count; i++) {
        snprintf(out + i * 2, 3, "%02x", bytes[i]);
    }
}

static void CheckSha1(const char *name, const void *data, size_t size, const char *expected) {
    uint8_t digest[PORT_SHA1_DIGEST_SIZE];
    char text[41];

    port_sha1(data, size, digest);
    Hex(text, digest, sizeof(digest));
    if (strcmp(text, expected) != 0) {
        printf("  check failed: SHA-1 of %s is %s, not %s\n", name, text, expected);
        sFailed = 1;
    }
}

// FIPS 180's examples, and a message fed in pieces that straddle the blocks
static int TestSha1(void) {
    static const char twoBlocks[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    uint8_t digest[PORT_SHA1_DIGEST_SIZE];
    char text[41];
    char *million = malloc(1000000);
    PortSha1 sha1;

    CheckSha1("\"\"", "", 0, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    CheckSha1("\"abc\"", "abc", 3, "a9993e364706816aba3e25717850c26c9cd0d89d");
    CheckSha1("the two-block message", twoBlocks, strlen(twoBlocks), "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    memset(million, 'a', 1000000);
    CheckSha1("a million a's", million, 1000000, "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
    port_sha1_init(&sha1);
    for (size_t offset = 0, step = 1; offset < 1000000; offset += step, step = step * 7 % 997 + 1) {
        port_sha1_update(&sha1, million + offset, offset + step <= 1000000 ? step : 1000000 - offset);
    }
    port_sha1_final(&sha1, digest);
    Hex(text, digest, sizeof(digest));
    Check(strcmp(text, "34aa973cd4c4daa4f61eeb2bdbad27316534016f") == 0, "a million a's in uneven pieces");
    free(million);
    return 0;
}

// The player's ROM, mapped privately: the tests change bytes of their copy, which never reach the file
static uint8_t *sRom;
static size_t sRomSize;
static const char *sRomPath;

static int MapRom(void) {
    struct stat info;
    int fd;

    if (sRom != NULL) {
        return 1;
    }
    sRomPath = port_rom_default_path();
    if (sRomPath == NULL) {
        printf("  no ROM (POKEBW2_ROM, or the repository's orig/baserom_%s.nds): skipped\n", port_rom_manifest_version);
        return 0;
    }
    fd = open(sRomPath, O_RDONLY);
    if (fd < 0 || fstat(fd, &info) != 0) {
        printf("  can't open %s: skipped\n", sRomPath);
        return 0;
    }
    sRom = mmap(NULL, (size_t)info.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    if (sRom == MAP_FAILED) {
        sRom = NULL;
        printf("  can't map %s: skipped\n", sRomPath);
        return 0;
    }
    sRomSize = (size_t)info.st_size;
    return 1;
}

static uint32_t Read32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void Write32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

// The end of the last NitroFS file, which a trimmed dump keeps
static uint32_t FilesEnd(void) {
    uint32_t fat = Read32(sRom + 0x48);
    uint32_t fatSize = Read32(sRom + 0x4c);
    uint32_t end = 0;

    for (uint32_t i = 0; i < fatSize / 8; i++) {
        uint32_t fileEnd = Read32(sRom + fat + i * 8 + 4);
        if (fileEnd > end) {
            end = fileEnd;
        }
    }
    return end;
}

static int TestExact(void) {
    char message[512];

    if (!MapRom()) {
        return SKIP;
    }
    Check(port_rom_check(sRom, sRomSize, message, sizeof(message)) == PORT_ROM_EXACT, "the dump is the decomp's");
    return 0;
}

// A dump cut after its last file, as trimming tools make them
static int TestTrimmed(void) {
    char message[512];
    uint32_t end;

    if (!MapRom()) {
        return SKIP;
    }
    end = FilesEnd();
    Check(port_rom_check(sRom, end, message, sizeof(message)) == PORT_ROM_FILES, "a trimmed dump passes by its files");
    Check(port_rom_check(sRom, end - 1, message, sizeof(message)) == PORT_ROM_REFUSED,
          "a dump cut inside its last file is refused");
    CheckContains(message, "cut off by the end of the file");
    CheckContains(message, "finds 1 of the");
    return 0;
}

// A change outside every file (the ARM9's program here): the files still match
static int TestChangedProgram(void) {
    char message[512];
    PortRomVerdict verdict;

    if (!MapRom()) {
        return SKIP;
    }
    sRom[0x4000] ^= 0xff;
    verdict = port_rom_check(sRom, sRomSize, message, sizeof(message));
    sRom[0x4000] ^= 0xff;
    Check(verdict == PORT_ROM_FILES, "a dump with another program but the same files passes by its files");
    return 0;
}

// A change in one file: refused, naming it
static int TestChangedFile(void) {
    char message[1024];
    uint32_t fat;
    uint32_t start;
    PortRomVerdict verdict;

    if (!MapRom()) {
        return SKIP;
    }
    // The first NitroFS file after the overlays' (the overlays have the first file IDs, one each)
    fat = Read32(sRom + 0x48);
    start = Read32(sRom + fat + (Read32(sRom + 0x54) / 32) * 8);
    sRom[start] ^= 0x5a;
    verdict = port_rom_check(sRom, sRomSize, message, sizeof(message));
    sRom[start] ^= 0x5a;
    Check(verdict == PORT_ROM_REFUSED, "a dump with a changed file is refused");
    CheckContains(message, "(other contents)");
    CheckContains(message, "finds 1 of the 318 files");
    printf("  %s\n", message);
    return 0;
}

// A change in the ARM9 overlay table, which no NitroFS file holds: refused
static int TestChangedOverlayTable(void) {
    char message[1024];
    uint32_t table;
    PortRomVerdict verdict;

    if (!MapRom()) {
        return SKIP;
    }
    // Overlay 12's load address
    table = Read32(sRom + 0x50);
    sRom[table + 12 * 32 + 4] ^= 0x01;
    verdict = port_rom_check(sRom, sRomSize, message, sizeof(message));
    sRom[table + 12 * 32 + 4] ^= 0x01;
    Check(verdict == PORT_ROM_REFUSED, "a dump with a changed overlay table is refused");
    CheckContains(message, "its ARM9 overlay table has other contents");
    printf("  %s\n", message);
    return 0;
}

// An overlay's file of another length than its header gives, as the FAT has it: refused, naming the overlay
static int TestChangedOverlayFile(void) {
    char message[1024];
    uint32_t table;
    uint32_t fat;
    uint32_t fileId;
    uint8_t *end;
    uint8_t saved[4];
    PortRomVerdict verdict;

    if (!MapRom()) {
        return SKIP;
    }
    table = Read32(sRom + 0x50);
    fat = Read32(sRom + 0x48);
    fileId = Read32(sRom + table + 40 * 32 + 0x18);
    end = sRom + fat + fileId * 8 + 4;
    memcpy(saved, end, 4);
    Write32(end, Read32(end) - 4);
    verdict = port_rom_check(sRom, sRomSize, message, sizeof(message));
    memcpy(end, saved, 4);
    Check(verdict == PORT_ROM_REFUSED, "a dump whose overlay file is shorter is refused");
    CheckContains(message, "the FAT gives the files of 1 of the 344 overlays another length");
    CheckContains(message, "overlay 40");
    printf("  %s\n", message);
    return 0;
}

// The other version, another game and something too small to be a ROM
static int TestOtherGames(void) {
    char message[512];
    uint8_t code[4];

    if (!MapRom()) {
        return SKIP;
    }
    memcpy(code, sRom + 0x0c, 4);
    memcpy(sRom + 0x0c, port_rom_manifest_game_code[2] == 'E' ? "IRDO" : "IREO", 4);
    Check(port_rom_check(sRom, sRomSize, message, sizeof(message)) == PORT_ROM_REFUSED, "the other version is refused");
    CheckContains(message, port_rom_manifest_game_code[2] == 'E' ? "Pokémon White 2 (US)" : "Pokémon Black 2 (US)");
    CheckContains(message, port_rom_manifest_title);
    memcpy(sRom + 0x0c, "IREJ", 4);
    Check(port_rom_check(sRom, sRomSize, message, sizeof(message)) == PORT_ROM_REFUSED, "another region is refused");
    CheckContains(message, "another region");
    memcpy(sRom + 0x0c, code, 4);
    Check(port_rom_check(sRom, 0x100, message, sizeof(message)) == PORT_ROM_REFUSED, "a tiny file is refused");
    CheckContains(message, "too small");
    return 0;
}

// A file name table that points past itself: refused, not followed
static int TestDamagedTables(void) {
    char message[512];
    uint32_t fnt;
    uint8_t saved[4];
    PortRomVerdict verdict;

    if (!MapRom()) {
        return SKIP;
    }
    sRom[0x4000] ^= 0xff; // so that the SHA-1 fails and the tables are read
    fnt = Read32(sRom + 0x40);
    memcpy(saved, sRom + fnt, 4);
    memset(sRom + fnt, 0xff, 4);
    verdict = port_rom_check(sRom, sRomSize, message, sizeof(message));
    memcpy(sRom + fnt, saved, 4);
    Check(verdict == PORT_ROM_REFUSED, "a damaged file name table is refused");
    CheckContains(message, "file name table is damaged");
    memcpy(saved, sRom + 0x48, 4);
    memset(sRom + 0x48, 0xff, 4);
    verdict = port_rom_check(sRom, sRomSize, message, sizeof(message));
    memcpy(sRom + 0x48, saved, 4);
    sRom[0x4000] ^= 0xff;
    Check(verdict == PORT_ROM_REFUSED, "a file allocation table past the end is refused");
    CheckContains(message, "past the end");
    return 0;
}

// Opening the ROM by its path, the system area, and reads past the end
static int TestOpenAndSystemArea(void) {
    char message[512];
    uint8_t bytes[8];
    const uint8_t *data;

    if (!MapRom()) {
        return SKIP;
    }
    Check(port_rom_open("/nonexistent/rom.nds", message, sizeof(message)) == PORT_ROM_REFUSED, "a missing file");
    CheckContains(message, "can't open it");
    Check(port_rom_data() == NULL, "nothing is open after a refusal");
    Check(port_rom_open(sRomPath, message, sizeof(message)) == PORT_ROM_EXACT, "the ROM opens");
    data = port_rom_data();
    Check(data != NULL && port_rom_size() == sRomSize, "its bytes are mapped");
    Check(port_rom_verdict() == PORT_ROM_EXACT, "its verdict is kept");
    memset((void *)HW_ROM_HEADER_BUF, 0, HW_CARD_ROM_HEADER_SIZE);
    memset((void *)HW_CARD_ROM_HEADER, 0, HW_CARD_ROM_HEADER_SIZE);
    port_rom_place_system_area();
    Check(memcmp((void *)HW_ROM_HEADER_BUF, data, HW_CARD_ROM_HEADER_SIZE) == 0, "the header is at HW_ROM_HEADER_BUF");
    Check(memcmp((void *)HW_CARD_ROM_HEADER, data, HW_CARD_ROM_HEADER_SIZE) == 0, "and at HW_CARD_ROM_HEADER");
    // 512 MiB (capacity code 12): size byte 0x100 - 2, maker 0xc2
    Check(data[0x14] != 12 || port_rom_card_id() == 0xfec2, "a 512 MiB card's ID");
    Check(OS_GetSystemWork()->bootCardId == port_rom_card_id(), "the boot card ID is the card's");
    port_rom_read(bytes, (uint32_t)sRomSize - 4, sizeof(bytes));
    Check(memcmp(bytes, data + sRomSize - 4, 4) == 0, "a read up to the end has the file's bytes");
    Check(bytes[4] == 0xff && bytes[5] == 0xff && bytes[6] == 0xff && bytes[7] == 0xff, "past the end reads 0xff");
    port_rom_read(bytes, 0x10, 4);
    Check(memcmp(bytes, data + 0x10, 4) == 0, "a read inside");
    return 0;
}

// The card's reads (port_rom_card_read): the header and secure area read as the block at 0x8000, and addresses wrap at
// the card's capacity
static int TestCardReads(void) {
    char message[512];
    uint8_t bytes[0x400];
    const uint8_t *data;
    uint64_t capacity;

    if (!MapRom()) {
        return SKIP;
    }
    Check(port_rom_open(sRomPath, message, sizeof(message)) != PORT_ROM_REFUSED, "the ROM opens");
    data = port_rom_data();
    capacity = (uint64_t)0x20000 << (data[0x14] & 0x1f);
    port_rom_card_read(bytes, 0, sizeof(bytes));
    Check(memcmp(bytes, data + 0x8000, 0x200) == 0 && memcmp(bytes + 0x200, data + 0x8000, 0x200) == 0,
          "each block below 0x8000 reads as the block at 0x8000");
    port_rom_card_read(bytes, 0x7ffc, 8);
    Check(memcmp(bytes, data + 0x81fc, 4) == 0 && memcmp(bytes + 4, data + 0x8000, 4) == 0,
          "a read across 0x8000 goes on into the card's own bytes");
    port_rom_card_read(bytes, 0x8100, 0x200);
    Check(memcmp(bytes, data + 0x8100, 0x200) == 0, "from 0x8000 on the card reads its bytes");
    if (capacity <= 0xffffffffu) {
        port_rom_card_read(bytes, (uint32_t)(capacity + 0x9000), 0x10);
        Check(memcmp(bytes, data + 0x9000, 0x10) == 0, "addresses wrap at the capacity");
        port_rom_card_read(bytes, (uint32_t)(capacity - 8), 16);
        Check(port_rom_size() < capacity ? bytes[0] == 0xff && bytes[7] == 0xff
                                         : memcmp(bytes, data + capacity - 8, 8) == 0,
              "the card's last bytes");
        Check(memcmp(bytes + 8, data + 0x8000, 8) == 0, "wrap to its first, which read as 0x8000");
    }
    // The file's own bytes, for the check, stay raw
    port_rom_read(bytes, 0x0c, 4);
    Check(memcmp(bytes, data + 0x0c, 4) == 0, "port_rom_read reads the file as it is");
    return 0;
}

// Where the ROM is looked for: --rom, then POKEBW2_ROM, then the repository's
static int TestDefaultPath(void) {
    static char program[] = "test";
    static char option[] = "--rom=/given/by/option.nds";
    static char *withOption[] = { program, option, NULL };
    static char *without[] = { program, NULL };
    const char *saved = getenv("POKEBW2_ROM");
    char *savedCopy = saved != NULL ? strdup(saved) : NULL;
    const char *path;

    port_app_set_arguments(2, withOption, NULL, 0);
    path = port_rom_default_path();
    Check(path != NULL && strcmp(path, "/given/by/option.nds") == 0, "--rom comes first");
    port_app_set_arguments(1, without, NULL, 0);
    setenv("POKEBW2_ROM", "/given/by/environment.nds", 1);
    path = port_rom_default_path();
    Check(path != NULL && strcmp(path, "/given/by/environment.nds") == 0, "then POKEBW2_ROM");
    unsetenv("POKEBW2_ROM");
    path = port_rom_default_path();
    Check(path == NULL || strstr(path, "orig/baserom_") != NULL, "then the repository's, when it is there");
    if (savedCopy != NULL) {
        setenv("POKEBW2_ROM", savedCopy, 1);
        free(savedCopy);
    }
    port_app_set_arguments(0, NULL, NULL, 0);
    return 0;
}

// The card ID's size byte, for capacities without a ROM open (128 KiB) and the arithmetic's boundaries
static int TestCardIdWithoutRom(void) {
    Check(port_rom_data() == NULL, "no ROM is open");
    Check(port_rom_card_id() == 0x00c2, "a card under 1 MiB has size byte 0");
    return 0;
}

typedef struct Test {
    const char *name;
    int (*run)(void);
} Test;

static const Test sTests[] = {
    { "sha1", TestSha1 },
    { "card_id_without_rom", TestCardIdWithoutRom },
    { "default_path", TestDefaultPath },
    { "exact", TestExact },
    { "trimmed", TestTrimmed },
    { "changed_program", TestChangedProgram },
    { "changed_file", TestChangedFile },
    { "changed_overlay_table", TestChangedOverlayTable },
    { "changed_overlay_file", TestChangedOverlayFile },
    { "other_games", TestOtherGames },
    { "damaged_tables", TestDamagedTables },
    { "open_and_system_area", TestOpenAndSystemArea },
    { "card_reads", TestCardReads },
};

int main(int argc, char **argv) {
    int all = argc < 2 || strcmp(argv[1], "all") == 0;
    int ran = 0;
    int skipped = 0;

    for (size_t i = 0; i < sizeof(sTests) / sizeof(sTests[0]); i++) {
        int status;

        if (!all && strcmp(argv[1], sTests[i].name) != 0) {
            continue;
        }
        printf("%s\n", sTests[i].name);
        sFailed = 0;
        status = sTests[i].run();
        ran++;
        if (status == SKIP) {
            skipped++;
        } else if (sFailed) {
            printf("FAIL %s\n", sTests[i].name);
            return 1;
        }
    }
    if (ran == 0) {
        printf("no test named %s\n", argv[1]);
        return 2;
    }
    return skipped == ran ? SKIP : 0;
}
