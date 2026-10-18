// Unit tests of the host's ARM7 backup service (port/hal/card/backup.c), driven word by word as the ARM9's CARD library
// drives it (lib/nitro/src/card/card_request.c), through a stand-in PXI bus that records the service and its answers:
// the INIT handshake, the identification of the 4 Mbit flash, the flash's reads, page writes, programs, verifies and
// erases on the command block, memory the ARM7 can't reach, and the save file: written whole and atomically, once a
// frame passes without a write or every 60 frames of writes, read back, and never overwritten when it isn't an image;
// a write that fails is reported once and tried again every 60 frames. Then other programs' files: a save as melonDS
// writes it goes through the card layer and back to the file byte for byte; another chip's save, DeSmuME's .dsv, a
// padded image and a directory are refused with what they are; a melonDS save under the ROM's name is pointed out; and
// a save path that is a link to another program's save stays a link while that save gets the writes (macOS and Linux:
// the saves follow no links on Windows, where that test skips with status 77). Needs no ROM.
//
//   port_card_backup_tests NAME | all
#include "hal/card/card_internal.h"
#include "hal/card/port_card.h"
#include "hal/host/host.h"
#include "hal/os/port_app.h"
#include "hal/pxi/pxi_bus.h"
#include "hal/rom/rom_manifest.h"
#include "nitro/card.h"
#include "nitro/pxi.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// A test that can't run on this host
#define SKIP_STATUS 77

static int sFailed;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("  check failed: %s\n", what);
        sFailed = 1;
    }
}

// --- What the ARM9 and the rest of the platform layer give the service --------------------------------------------

CARDiCommon cardi_common;
static CARDiCommandArg sArg;
static _Alignas(32) u8 sPageBuffer[PORT_CARD_PAGE_SIZE];

u8 *port_card_page_buffer(void) {
    return sPageBuffer;
}

// The stand-in bus: the services by tag, and the last answer
static PortPxiService sServices[PXI_MAX_FIFO_TAG];
static int sReplies;
static u32 sReplyTag;
static u32 sReplyData;
static BOOL sReplyErr;

void port_pxi_register_service(u32 tag, PortPxiService service) {
    sServices[tag] = service;
}

void port_pxi_reply(u32 tag, u32 data, BOOL err) {
    sReplies++;
    sReplyTag = tag;
    sReplyData = data & PORT_PXI_DATA_MASK;
    sReplyErr = err;
}

// The frame hook the service adds
static void (*sFrameHook)(void);

void port_os_add_frame_hook(void (*hook)(void)) {
    sFrameHook = hook;
}

// --- Driving the service ------------------------------------------------------------------------------------------

// Sends a word of the FS tag as the ARM9 does (the error bit set), and returns whether the ARM7 answered it
static int Send(u32 data) {
    int before = sReplies;

    sServices[PXI_FIFO_TAG_FS](PXI_FIFO_TAG_FS, data & PORT_PXI_DATA_MASK, TRUE);
    return sReplies == before + 1 && sReplyTag == PXI_FIFO_TAG_FS && sReplyErr;
}

// A request on the command block, as CARDi_Request makes it: the result the ARM7 wrote
static CARDResult Request(u32 request) {
    Check(Send(request), "the ARM7 answered the request");
    return sArg.result;
}

static u32 PageAddress(void) {
    return (u32)(uintptr_t)sPageBuffer;
}

// The spec of the 4 Mbit flash as card_backup.c's CARDi_IdentifyBackupCore fills it in
static void IdentifyFlash(void) {
    memset(&sArg, 0, sizeof(sArg));
    sArg.type = CARD_BACKUP_TYPE_FLASH_4MBITS_EX;
    sArg.spec.totalSize = PORT_CARD_BACKUP_SIZE;
    sArg.spec.sectSize = 0x10000;
    sArg.spec.pageSize = 0x100;
    sArg.spec.caps = 0x203f | CARD_CAPS(CARD_REQ_WRITE_BACKUP) | CARD_CAPS(CARD_REQ_ERASE_PAGE_BACKUP) |
                     CARD_CAPS(CARD_REQ_READ_BACKUP) | CARD_CAPS(CARD_REQ_PROGRAM_BACKUP) |
                     CARD_CAPS(CARD_REQ_VERIFY_BACKUP) | CARD_CAPS(CARD_REQ_ERASE_SECTOR_BACKUP);
    Check(Request(CARD_REQ_IDENTIFY) == CARD_RESULT_SUCCESS, "the 4 Mbit flash answers");
}

static char sSavePath[512];

// A fresh state for a test: the service booted on the stand-in bus, its command block, and a save file in a temporary
// directory of its own
static void Start(const char *name) {
    char dir[480];

    memset(&cardi_common, 0, sizeof(cardi_common));
    memset(&sArg, 0, sizeof(sArg));
    cardi_common.cmd = &sArg;
    snprintf(dir, sizeof(dir), "%s/port_card_%s_%d", port_host_temporary_directory(), name, (int)getpid());
    snprintf(sSavePath, sizeof(sSavePath), "%s/save/%s.sav", dir, port_rom_manifest_game_code);
    remove(sSavePath);
    port_card_set_save_path(sSavePath);
    port_card_boot();
    Check(sServices[PXI_FIFO_TAG_FS] != NULL, "the FS tag has the ARM7's service");
    Check(sServices[PXI_FIFO_TAG_CARD] != NULL, "and the card's tag");
    Check(sFrameHook != NULL, "the service has a frame hook");
}

static long FileSize(const char *path) {
    struct stat info;

    return stat(path, &info) == 0 ? (long)info.st_size : -1;
}

// Takes the test's save file and its directories away, unwritten changes with them
static void Finish(void) {
    char dir[600];

    port_card_set_save_path(NULL);
    remove(sSavePath);
    snprintf(dir, sizeof(dir), "%s", sSavePath);
    *strrchr(dir, '/') = '\0';
    rmdir(dir);
    *strrchr(dir, '/') = '\0';
    rmdir(dir);
}

// --- The tests ----------------------------------------------------------------------------------------------------

// The first request: INIT, then the command block's address in the 26 bits of a word; only then the answer
static int TestInit(void) {
    int replies;

    Start("init");
    replies = sReplies;
    Check(!Send(CARD_REQ_INIT), "INIT alone isn't answered");
    Check(sReplies == replies, "no answer yet");
    sArg.result = CARD_RESULT_TIMEOUT;
    Check(Send((u32)(uintptr_t)&sArg), "the block's address is answered");
    Check(sArg.result == CARD_RESULT_SUCCESS, "INIT succeeds");
    // The next word is a request again
    Check(Request(CARD_REQ_IDENTIFY) == CARD_RESULT_NO_RESPONSE, "an unidentified type has no chip that answers");
    Finish();
    return 0;
}

// Identification: the host's chip is a 4 Mbit flash, whatever the vendor; nothing else answers, and no request works
// before an identification succeeds
static int TestIdentify(void) {
    Start("identify");
    sArg.type = 0;
    sArg.spec.caps = 0xffffffff;
    Check(Request(CARD_REQ_READ_BACKUP) == CARD_RESULT_NO_RESPONSE, "nothing answers before an identification");
    sArg.type = CARD_BACKUP_TYPE_DEFINE(CARD_BACKUP_TYPE_DEVICE_EEPROM, 16, 0);
    Check(Request(CARD_REQ_IDENTIFY) == CARD_RESULT_NO_RESPONSE, "an EEPROM doesn't answer");
    sArg.type = CARD_BACKUP_TYPE_DEFINE(CARD_BACKUP_TYPE_DEVICE_FLASH, 20, 0);
    Check(Request(CARD_REQ_IDENTIFY) == CARD_RESULT_NO_RESPONSE, "an 8 Mbit flash doesn't answer");
    sArg.type = CARD_BACKUP_TYPE_DEFINE(CARD_BACKUP_TYPE_DEVICE_FLASH, 19, 0);
    Check(Request(CARD_REQ_IDENTIFY) == CARD_RESULT_SUCCESS, "a 4 Mbit flash of any vendor answers");
    IdentifyFlash();
    Finish();
    return 0;
}

// The flash's requests on the command block, through the page buffer
static int TestFlash(void) {
    u8 *image;
    u32 i;

    Start("flash");
    IdentifyFlash();
    image = port_card_backup_image();
    Check(image[0] == 0xff && image[PORT_CARD_BACKUP_SIZE - 1] == 0xff, "a new chip is erased");

    // A page write of 256 bytes at 0x100
    for (i = 0; i < PORT_CARD_PAGE_SIZE; i++) {
        sPageBuffer[i] = (u8)(i * 7 + 1);
    }
    sArg.src = PageAddress();
    sArg.dst = 0x100;
    sArg.len = PORT_CARD_PAGE_SIZE;
    Check(Request(CARD_REQ_WRITE_BACKUP) == CARD_RESULT_SUCCESS, "a page write succeeds");
    Check(memcmp(image + 0x100, sPageBuffer, PORT_CARD_PAGE_SIZE) == 0, "the chip holds the page");
    Check(Request(CARD_REQ_VERIFY_BACKUP) == CARD_RESULT_SUCCESS, "it verifies");
    sPageBuffer[10] ^= 1;
    Check(Request(CARD_REQ_VERIFY_BACKUP) == CARD_RESULT_FAILURE, "other bytes don't verify");

    // A read of 16 bytes from 0x108
    memset(sPageBuffer, 0, sizeof(sPageBuffer));
    sArg.src = 0x108;
    sArg.dst = PageAddress();
    sArg.len = 16;
    Check(Request(CARD_REQ_READ_BACKUP) == CARD_RESULT_SUCCESS, "a read succeeds");
    Check(memcmp(sPageBuffer, image + 0x108, 16) == 0 && sPageBuffer[16] == 0, "it reads what the chip holds");

    // A program only clears bits
    sPageBuffer[0] = 0x0f;
    sArg.src = PageAddress();
    sArg.dst = 0x200;
    sArg.len = 1;
    image[0x200] = 0x3c;
    Check(Request(CARD_REQ_PROGRAM_BACKUP) == CARD_RESULT_SUCCESS, "a program succeeds");
    Check(image[0x200] == 0x0c, "a program clears bits only");

    // Erases: a page's bytes, and a whole sector from its start
    sArg.dst = 0x104;
    sArg.len = 4;
    Check(Request(CARD_REQ_ERASE_PAGE_BACKUP) == CARD_RESULT_SUCCESS, "a page erase succeeds");
    Check(image[0x103] != 0xff && image[0x104] == 0xff && image[0x107] == 0xff && image[0x108] != 0xff,
          "it erases those bytes");
    sArg.dst = 0x180;
    sArg.len = 0x10000;
    Check(Request(CARD_REQ_ERASE_SECTOR_BACKUP) == CARD_RESULT_SUCCESS, "a sector erase succeeds");
    for (i = 0; i < 0x10000 && image[i] == 0xff; i++) {
    }
    Check(i == 0x10000, "it erases the sector from its start");

    // Addresses wrap at the chip's size
    sPageBuffer[0] = 0x11;
    sPageBuffer[1] = 0x22;
    sArg.src = PageAddress();
    sArg.dst = PORT_CARD_BACKUP_SIZE - 1;
    sArg.len = 2;
    Check(Request(CARD_REQ_WRITE_BACKUP) == CARD_RESULT_SUCCESS, "a write over the end succeeds");
    Check(image[PORT_CARD_BACKUP_SIZE - 1] == 0x11 && image[0] == 0x22, "it wraps to the start");

    // A request the spec doesn't allow, and one the library never sends
    Check(Request(CARD_REQ_ERASE_CHIP_BACKUP) == CARD_RESULT_UNSUPPORTED, "the spec leaves the chip erase out");
    sArg.spec.caps |= CARD_CAPS(5);
    Check(Request(5) == CARD_RESULT_UNSUPPORTED, "an unknown request");
    Finish();
    return 0;
}

// Memory the ARM7 can't reach: only the page buffer is
static int TestUnreachable(void) {
    Start("unreachable");
    IdentifyFlash();
    sArg.src = 0;
    sArg.dst = PageAddress() + 0x1000;
    sArg.len = 4;
    Check(Request(CARD_REQ_READ_BACKUP) == CARD_RESULT_ERROR, "another address fails");
    sArg.dst = PageAddress();
    sArg.len = PORT_CARD_PAGE_SIZE + 1;
    Check(Request(CARD_REQ_READ_BACKUP) == CARD_RESULT_ERROR, "more than the page buffer fails");
    Finish();
    return 0;
}

// The save file: written once a frame passes without a write, or every 60 frames of writes; whole, renamed into place;
// read back by the next start
static int TestSaveFile(void) {
    char temporary[600];
    u8 *image;
    FILE *file;
    u8 *contents;
    int frame;

    Start("save");
    IdentifyFlash();
    snprintf(temporary, sizeof(temporary), "%s.tmp", sSavePath);
    sPageBuffer[0] = 0xa5;
    sArg.src = PageAddress();
    sArg.dst = 0x1234;
    sArg.len = 1;
    Check(Request(CARD_REQ_WRITE_BACKUP) == CARD_RESULT_SUCCESS, "a write");
    Check(FileSize(sSavePath) < 0, "nothing is written at once");
    sFrameHook();
    Check(FileSize(sSavePath) < 0, "nor at the end of the frame that wrote");
    sFrameHook();
    Check(FileSize(sSavePath) == PORT_CARD_BACKUP_SIZE, "a frame without a write writes the image");
    Check(FileSize(temporary) < 0, "and leaves no temporary file");

    // Writes in every frame: written on the 60th
    for (frame = 1; frame <= 60; frame++) {
        sPageBuffer[0] = (u8)frame;
        sArg.dst = 0x2000;
        Request(CARD_REQ_WRITE_BACKUP);
        sFrameHook();
        if (frame == 59) {
            file = fopen(sSavePath, "rb");
            Check(file != NULL && fseek(file, 0x2000, SEEK_SET) == 0 && fgetc(file) == 0xff,
                  "59 frames of writes aren't written yet");
            if (file != NULL) {
                fclose(file);
            }
        }
    }
    file = fopen(sSavePath, "rb");
    Check(file != NULL && fseek(file, 0x2000, SEEK_SET) == 0 && fgetc(file) == 60, "the 60th frame writes");
    if (file != NULL) {
        fclose(file);
    }

    // The next start reads the file
    image = port_card_backup_image();
    contents = malloc(PORT_CARD_BACKUP_SIZE);
    memcpy(contents, image, PORT_CARD_BACKUP_SIZE);
    port_card_set_save_path(sSavePath);
    image = port_card_backup_image();
    Check(memcmp(image, contents, PORT_CARD_BACKUP_SIZE) == 0, "the image comes back from the file");
    Check(image[0x1234] == 0xa5, "with the first write");
    free(contents);
    Finish();
    return 0;
}

// A save file that can't be written (here its directory is a file, which appears after the image was read, when the
// file wasn't there yet): reported once, with the error of the call that failed, and tried again every 60 frames, not
// every frame; written at the next try once it can be
static int TestWriteFailure(void) {
    char dir[600];
    char log[600];
    char line[1024];
    char probe[700];
    char expected[256];
    int saved;
    int failures = 0;
    int recoveries = 0;
    int reason = 0;
    int frame;
    FILE *file;

    Start("write_failure");
    IdentifyFlash();
    Check(port_card_backup_image() != NULL, "a new image, before the save directory is blocked");
    snprintf(dir, sizeof(dir), "%s", sSavePath);
    *strrchr(dir, '/') = '\0';
    snprintf(log, sizeof(log), "%s.log", dir);
    *strrchr(dir, '/') = '\0';
    port_host_make_directory(dir);
    strcat(dir, "/save");
    file = fopen(dir, "wb"); // where the save directory should be
    Check(file != NULL, "a file in the save directory's place");
    if (file != NULL) {
        fclose(file);
    }
    // The error a file there gets, which the message must give: "Not a directory" on macOS and Linux, Windows' own
    snprintf(probe, sizeof(probe), "%s/probe", dir);
    errno = 0;
    file = fopen(probe, "wb");
    Check(file == NULL, "nothing can be made in the save directory's place");
    if (file != NULL) {
        fclose(file);
        remove(probe);
    }
    snprintf(expected, sizeof(expected), "%s", strerror(errno));

    // stderr to a file of the test's, for the messages
    fflush(stderr);
    saved = dup(2);
    Check(freopen(log, "w", stderr) != NULL, "stderr goes to the test's log");

    sPageBuffer[0] = 0x5a;
    sArg.src = PageAddress();
    sArg.dst = 0x100;
    sArg.len = 1;
    Check(Request(CARD_REQ_WRITE_BACKUP) == CARD_RESULT_SUCCESS, "a write");
    sFrameHook();
    sFrameHook(); // the first try, which fails
    for (frame = 1; frame <= 150; frame++) {
        sFrameHook(); // tries at 60 and 120
    }
    Check(FileSize(sSavePath) < 0, "nothing could be written");
    remove(dir);
    for (frame = 1; frame < 30; frame++) {
        sFrameHook();
    }
    Check(FileSize(sSavePath) < 0, "the next try waits for its 60 frames");
    sFrameHook();
    Check(FileSize(sSavePath) == PORT_CARD_BACKUP_SIZE, "and then writes the image");

    fflush(stderr);
    dup2(saved, 2);
    close(saved);
    file = fopen(log, "r");
    while (file != NULL && fgets(line, sizeof(line), file) != NULL) {
        failures += strstr(line, "can't write") != NULL;
        reason += strstr(line, expected) != NULL;
        recoveries += strstr(line, "port: card: wrote") != NULL;
    }
    if (file != NULL) {
        fclose(file);
    }
    remove(log);
    Check(failures == 1, "the failure is reported once");
    Check(reason == 1, "with the error of the call that failed");
    Check(recoveries == 1, "and the write that succeeded after it");
    Finish();
    return 0;
}

// A file that isn't an image is never overwritten: the program stops (status 65) with a message, which the test's
// registration expects
static int TestBadFile(void) {
    char dir[600];
    FILE *file;

    Start("bad_file");
    atexit(Finish);
    snprintf(dir, sizeof(dir), "%s", sSavePath);
    *strrchr(dir, '/') = '\0';
    *strrchr(dir, '/') = '\0';
    port_host_make_directory(dir);
    strcat(dir, "/save");
    port_host_make_directory(dir);
    file = fopen(sSavePath, "wb");
    Check(file != NULL, "a file of another size");
    if (file != NULL) {
        fwrite("not a save", 1, 10, file);
        fclose(file);
    }
    port_card_set_save_path(sSavePath);
    port_card_backup_image();
    printf("  the program went on\n");
    return 1;
}

// --- Files of other programs --------------------------------------------------------------------------------------

// The save directory of the test's save file, made
static void MakeSaveDirectory(char *dir, size_t size) {
    snprintf(dir, size, "%s", sSavePath);
    *strrchr(dir, '/') = '\0';
    *strrchr(dir, '/') = '\0';
    port_host_make_directory(dir);
    snprintf(dir + strlen(dir), size - strlen(dir), "/save");
    port_host_make_directory(dir);
}

// Writes a file of size bytes, each byte made by a function of its offset
static void WriteFile(const char *path, long size, u8 (*byte)(long offset)) {
    FILE *file = fopen(path, "wb");
    long i;

    Check(file != NULL, "a file the test makes");
    if (file == NULL) {
        return;
    }
    for (i = 0; i < size; i++) {
        fputc(byte(i), file);
    }
    fclose(file);
}

// What a save of the game looks like to the card layer: data in the first 0x4c000 bytes, where BW2 keeps its two
// copies of the main save, and the rest of the chip erased. The data is a sequence of no meaning that no two offsets
// share by chance (each 32-bit word of it a step of a linear congruential generator), so that a byte moved, lost or
// added shows
static u8 SaveByte(long offset) {
    u32 word = (u32)(offset / 4) * 1103515245u + 12345u;

    return offset < 0x4c000 ? (u8)(word >> (offset % 4 * 8)) : 0xff;
}

static u8 ZeroByte(long offset) {
    (void)offset;
    return 0;
}

// The file's bytes, or NULL when it isn't size bytes long
static u8 *ReadFile(const char *path, long size) {
    u8 *contents = malloc((size_t)size + 1);
    FILE *file = fopen(path, "rb");
    size_t read = 0;

    if (file != NULL) {
        read = fread(contents, 1, (size_t)size + 1, file);
        fclose(file);
    }
    if (read != (size_t)size) {
        free(contents);
        return NULL;
    }
    return contents;
}

// A save another program wrote, as melonDS writes its .sav (the chip's 524,288 bytes and nothing else), in the save
// directory under the game's name: the game reads it byte for byte through the ARM7's service; the file isn't written
// while the game only reads; the game's page writes and programs reach the file in place, which stays 524,288 bytes
// with every other byte as it was; and the next start reads the file back as it was left
static int TestRoundTrip(void) {
    char dir[600];
    struct stat before;
    struct stat after;
    u8 *original;
    u8 *expected;
    u8 *contents;
    u8 *image;
    u32 address;
    u32 i;
    int frame;
    int mismatches = 0;

    Start("round_trip");
    MakeSaveDirectory(dir, sizeof(dir));
    WriteFile(sSavePath, PORT_CARD_BACKUP_SIZE, SaveByte);
    original = ReadFile(sSavePath, PORT_CARD_BACKUP_SIZE);
    Check(original != NULL && original[0x4bfff] == SaveByte(0x4bfff) && original[0x4c000] == 0xff,
          "the other program's save is in place");
    if (original == NULL) {
        return 1;
    }
    Check(stat(sSavePath, &before) == 0, "the save file's state before the run");
    port_card_set_save_path(sSavePath);
    IdentifyFlash();

    // The whole chip, read a page at a time as card_backup.c reads it, is the file
    for (address = 0; address < PORT_CARD_BACKUP_SIZE; address += PORT_CARD_PAGE_SIZE) {
        memset(sPageBuffer, 0x5a, sizeof(sPageBuffer));
        sArg.src = address;
        sArg.dst = PageAddress();
        sArg.len = PORT_CARD_PAGE_SIZE;
        if (Request(CARD_REQ_READ_BACKUP) != CARD_RESULT_SUCCESS ||
            memcmp(sPageBuffer, original + address, PORT_CARD_PAGE_SIZE) != 0) {
            mismatches++;
        }
    }
    Check(mismatches == 0, "every page reads as the file has it");
    for (frame = 0; frame < 3 * 60; frame++) {
        sFrameHook();
    }
    Check(stat(sSavePath, &after) == 0 && after.st_ino == before.st_ino && after.st_size == before.st_size,
          "a save the game only reads isn't written");

    // A page write at the start of the backup copy (0x26000), the last page of the chip, and a program of one byte,
    // which only clears bits: 0x3c & 0xf0
    expected = malloc(PORT_CARD_BACKUP_SIZE);
    memcpy(expected, original, PORT_CARD_BACKUP_SIZE);
    for (i = 0; i < PORT_CARD_PAGE_SIZE; i++) {
        sPageBuffer[i] = (u8)(0x80 + i);
        expected[0x26000 + i] = (u8)(0x80 + i);
        expected[PORT_CARD_BACKUP_SIZE - PORT_CARD_PAGE_SIZE + i] = (u8)(0x80 + i);
    }
    sArg.src = PageAddress();
    sArg.len = PORT_CARD_PAGE_SIZE;
    sArg.dst = 0x26000;
    Check(Request(CARD_REQ_WRITE_BACKUP) == CARD_RESULT_SUCCESS, "a page write");
    sArg.dst = PORT_CARD_BACKUP_SIZE - PORT_CARD_PAGE_SIZE;
    Check(Request(CARD_REQ_WRITE_BACKUP) == CARD_RESULT_SUCCESS, "a write of the last page");
    image = port_card_backup_image();
    image[0x1000] = 0x3c;
    expected[0x1000] = 0x30;
    sPageBuffer[0] = 0xf0;
    sArg.dst = 0x1000;
    sArg.len = 1;
    Check(Request(CARD_REQ_PROGRAM_BACKUP) == CARD_RESULT_SUCCESS, "a program");
    sFrameHook();
    sFrameHook();
    contents = ReadFile(sSavePath, PORT_CARD_BACKUP_SIZE);
    Check(contents != NULL, "the file is still the chip's 524,288 bytes, with nothing before or after them");
    Check(contents != NULL && memcmp(contents, expected, PORT_CARD_BACKUP_SIZE) == 0,
          "it holds the writes, and every other byte as it was");

    // The next start reads it back
    port_card_set_save_path(sSavePath);
    Check(memcmp(port_card_backup_image(), expected, PORT_CARD_BACKUP_SIZE) == 0, "the next start reads the file");
    free(contents);
    free(expected);
    free(original);
    Finish();
    return 0;
}

// A file of another chip's size, DeSmuME's .dsv or a padded image is never overwritten: the program stops (status 65)
// with a message that says what the file seems to be, which the tests' registrations expect. end, of endSize bytes,
// goes after the file's size bytes
static int RefuseFile(long size, u8 (*byte)(long offset), const u8 *end, size_t endSize) {
    char dir[600];
    FILE *file;

    Start("refuse");
    atexit(Finish);
    MakeSaveDirectory(dir, sizeof(dir));
    WriteFile(sSavePath, size, byte);
    if (end != NULL) {
        file = fopen(sSavePath, "ab");
        Check(file != NULL, "the file's end");
        if (file != NULL) {
            fwrite(end, 1, endSize, file);
            fclose(file);
        }
    }
    port_card_set_save_path(sSavePath);
    port_card_backup_image();
    printf("  the program went on\n");
    return 1;
}

// A save of a 512 Kbit EEPROM, 64 KiB
static int TestRefuseEeprom(void) {
    return RefuseFile(0x10000, ZeroByte, NULL, 0);
}

static void PutWord(u8 *at, u32 value) {
    at[0] = (u8)value;
    at[1] = (u8)(value >> 8);
    at[2] = (u8)(value >> 16);
    at[3] = (u8)(value >> 24);
}

// DeSmuME's .dsv of this chip: the chip's 524,288 bytes, then the footer as DeSmuME's src/mc.cpp writes it
// (BackupDevice::ensure): its text, six 32-bit words (the size written, the padded size, the save type, the address
// size, the memory's size, the footer's version 0) and the cookie
static int TestRefuseDsv(void) {
    static const char text[] = "|<--Snip above here to create a raw sav by excluding this DeSmuME savedata footer:";
    static const char cookie[] = "|-DESMUME SAVE-|";
    static const u32 words[6] = { PORT_CARD_BACKUP_SIZE, PORT_CARD_BACKUP_SIZE, 0, 3, PORT_CARD_BACKUP_SIZE, 0 };
    u8 footer[sizeof(text) - 1 + sizeof(words) + sizeof(cookie) - 1];
    size_t i;

    memcpy(footer, text, sizeof(text) - 1);
    for (i = 0; i < 6; i++) {
        PutWord(footer + sizeof(text) - 1 + 4 * i, words[i]);
    }
    memcpy(footer + sizeof(text) - 1 + sizeof(words), cookie, sizeof(cookie) - 1);
    return RefuseFile(PORT_CARD_BACKUP_SIZE, SaveByte, footer, sizeof(footer));
}

// A save of this chip padded with 0xff to 1 MiB, the size of an 8 Mbit flash's save, as a tool that pads saves leaves
// it: refused, with how to make it the chip's image
static int TestRefusePadded(void) {
    static u8 padding[PORT_CARD_BACKUP_SIZE];

    memset(padding, 0xff, sizeof(padding));
    return RefuseFile(PORT_CARD_BACKUP_SIZE, SaveByte, padding, sizeof(padding));
}

// The same size with data after the chip's 524,288 bytes: only the size is named, as another chip's
static int TestRefuseLarger(void) {
    static u8 rest[PORT_CARD_BACKUP_SIZE];

    memset(rest, 0xff, sizeof(rest));
    rest[sizeof(rest) - 1] = 0;
    return RefuseFile(PORT_CARD_BACKUP_SIZE, SaveByte, rest, sizeof(rest));
}

// A directory at the save path, which some hosts open as a file with a size of its own
static int TestRefuseDirectory(void) {
    char dir[600];

    Start("refuse_directory");
    atexit(Finish);
    MakeSaveDirectory(dir, sizeof(dir));
    port_host_make_directory(sSavePath);
    port_card_set_save_path(sSavePath);
    port_card_backup_image();
    printf("  the program went on\n");
    return 1;
}

// Without the game's file, a .sav file of the image's size under another name in the save directory, such as a
// melonDS save named after the ROM (a title, or a release number), is named with how to play it; the game starts with
// a blank backup and the files stay as they were. Files named as the port names this game's saves (the other version's
// game code, another region's), of other sizes or other extensions aren't named
#define OTHER_FILES 6
static int TestOtherSaves(void) {
    char other[16];
    const char *names[OTHER_FILES] = {
        "Pokemon Black 2 (USA).SAV", "6149.sav", other, "IREJ.sav", "small.sav", "image.bin"
    };
    const int expected[OTHER_FILES] = { 1, 1, 0, 0, 0, 0 };
    char dir[600];
    char path[700];
    char log[700];
    char line[2048];
    int named[OTHER_FILES] = { 0 };
    int lines = 0;
    int saved;
    u8 *image;
    u32 i;
    FILE *file;

    snprintf(other, sizeof(other), "%s.sav", strcmp(port_rom_manifest_game_code, "IREO") == 0 ? "IRDO" : "IREO");
    Start("other_saves");
    MakeSaveDirectory(dir, sizeof(dir));
    for (i = 0; i < OTHER_FILES; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        WriteFile(path, i == 4 ? 100 : PORT_CARD_BACKUP_SIZE, SaveByte);
    }
    snprintf(log, sizeof(log), "%s.log", dir);
    fflush(stderr);
    saved = dup(2);
    Check(freopen(log, "w", stderr) != NULL, "stderr goes to the test's log");
    port_card_set_save_path(sSavePath);
    image = port_card_backup_image();
    fflush(stderr);
    dup2(saved, 2);
    close(saved);

    file = fopen(log, "r");
    while (file != NULL && fgets(line, sizeof(line), file) != NULL) {
        lines++;
        for (i = 0; i < OTHER_FILES; i++) {
            named[i] += strstr(line, names[i]) != NULL;
        }
    }
    if (file != NULL) {
        fclose(file);
    }
    remove(log);
    Check(lines == 2, "two files are named");
    for (i = 0; i < OTHER_FILES; i++) {
        if (named[i] != expected[i]) {
            printf("  %s: named %d times\n", names[i], named[i]);
        }
        Check(named[i] == expected[i], "each of the melonDS saves is named once, and no other file");
    }
    for (i = 0; i < PORT_CARD_BACKUP_SIZE && image[i] == 0xff; i++) {
    }
    Check(i == PORT_CARD_BACKUP_SIZE, "the game starts with a blank backup");
    Check(FileSize(sSavePath) < 0, "and nothing is written for it");
    for (i = 0; i < OTHER_FILES; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        Check(FileSize(path) == (i == 4 ? 100 : PORT_CARD_BACKUP_SIZE), "the files stay as they were");
        remove(path);
    }
    Finish();
    return 0;
}

// A page write of the pattern of seed at address, then the frames that write the image to its file
static void WritePage(u32 address, u8 seed) {
    u32 i;

    for (i = 0; i < PORT_CARD_PAGE_SIZE; i++) {
        sPageBuffer[i] = (u8)(seed + i);
    }
    sArg.src = PageAddress();
    sArg.dst = address;
    sArg.len = PORT_CARD_PAGE_SIZE;
    Check(Request(CARD_REQ_WRITE_BACKUP) == CARD_RESULT_SUCCESS, "a page write");
    sFrameHook();
    sFrameHook();
}

// Whether a file holds the pattern of seed at address and nothing else but erased flash or SaveByte's data
static BOOL HoldsPage(const char *path, u32 address, u8 seed, BOOL saveData) {
    u8 *contents = ReadFile(path, PORT_CARD_BACKUP_SIZE);
    BOOL ok = contents != NULL;
    u32 i;

    for (i = 0; ok && i < PORT_CARD_BACKUP_SIZE; i++) {
        u8 wanted = i >= address && i < address + PORT_CARD_PAGE_SIZE ? (u8)(seed + i - address)
                    : saveData                                        ? SaveByte(i)
                                                                      : 0xff;
        ok = contents[i] == wanted;
    }
    free(contents);
    return ok;
}

static BOOL IsLink(const char *path) {
    return port_host_is_symbolic_link(path);
}

// A save path that is a symbolic link to another program's save (melonDS's .sav, linked to keep one save for both
// programs): the game reads that file, its writes reach it, and the link stays a link with no temporary file left
// beside either. A relative link to a file not made yet leads to where that file is made
static int TestLinkedSave(void) {
    char dir[600];
    char other[600];
    char target[700];
    char temporary[720];
    char relative[64];
    int linked;

    Start("linked_save");
    MakeSaveDirectory(dir, sizeof(dir));
    snprintf(other, sizeof(other), "%s/../melonds", dir);
    port_host_make_directory(other);
    snprintf(target, sizeof(target), "%s/Pokemon Black 2.sav", other);
    WriteFile(target, PORT_CARD_BACKUP_SIZE, SaveByte);
    linked = port_host_symbolic_link(target, sSavePath);
    if (linked != 0 && errno == ENOSYS) {
        printf("  skipped: the saves follow no symbolic links on this host\n");
        remove(target);
        rmdir(other);
        Finish();
        return SKIP_STATUS;
    }
    Check(linked == 0, "the save path links to melonDS's save");
    port_card_set_save_path(sSavePath);
    IdentifyFlash();
    Check(port_card_backup_image()[0x100] == SaveByte(0x100), "the game reads the linked file");
    WritePage(0x26000, 0x40);
    Check(IsLink(sSavePath), "the save path is still a link");
    Check(HoldsPage(target, 0x26000, 0x40, TRUE), "the linked file got the write, and keeps every other byte");
    snprintf(temporary, sizeof(temporary), "%s.tmp", target);
    Check(FileSize(temporary) < 0, "no temporary file beside it");
    snprintf(temporary, sizeof(temporary), "%s.tmp", sSavePath);
    Check(FileSize(temporary) < 0, "nor beside the link");
    remove(sSavePath);
    remove(target);

    // A relative link to a file that doesn't exist yet: the first write makes it there
    snprintf(relative, sizeof(relative), "../melonds/new.sav");
    snprintf(target, sizeof(target), "%s/new.sav", other);
    Check(port_host_symbolic_link(relative, sSavePath) == 0, "a relative link to a file not made yet");
    port_card_set_save_path(sSavePath);
    IdentifyFlash();
    WritePage(0x100, 0x90);
    Check(IsLink(sSavePath), "the link stays");
    Check(HoldsPage(target, 0x100, 0x90, FALSE), "the write made the file it links to");
    remove(sSavePath);
    remove(target);
    rmdir(other);
    Finish();
    return 0;
}

static int StartsWith(const char *text, const char *start) {
    return start != NULL && strncmp(text, start, strlen(start)) == 0;
}

// The save directory: --save-dir, else the user's
static int TestSavePath(void) {
    static char program[] = "test";
    static char option[] = "--save-dir=/given/saves";
    static char *withOption[] = { program, option, NULL };
    static char *without[] = { program, NULL };
    char expected[600];

    port_app_set_arguments(2, withOption, NULL, 0);
    port_card_set_save_path(NULL);
    snprintf(expected, sizeof(expected), "/given/saves/%s.sav", port_rom_manifest_game_code);
    Check(strcmp(port_card_save_path(), expected) == 0, "--save-dir names the directory");
    port_app_set_arguments(1, without, NULL, 0);
    port_card_set_save_path(NULL);
    Check(strstr(port_card_save_path(), "/" PORT_PRODUCT_NAME "/") != NULL ||
              strstr(port_card_save_path(), "\\" PORT_PRODUCT_NAME "/") != NULL,
          "else the user's own, named Frigate");
    // In the home directory on macOS and Linux, in the user's application data on Windows (port_card.h)
    Check(getenv("HOME") == NULL || StartsWith(port_card_save_path(), getenv("HOME")) ||
              StartsWith(port_card_save_path(), getenv("APPDATA")) || getenv("XDG_DATA_HOME") != NULL,
          "under the home directory");
    port_app_set_arguments(0, NULL, NULL, 0);
    return 0;
}

typedef struct Test {
    const char *name;
    int (*run)(void);
} Test;

static const Test sTests[] = {
    { "init", TestInit },
    { "identify", TestIdentify },
    { "flash", TestFlash },
    { "unreachable", TestUnreachable },
    { "save_file", TestSaveFile },
    { "write_failure", TestWriteFailure },
    { "round_trip", TestRoundTrip },
    { "other_saves", TestOtherSaves },
    { "linked_save", TestLinkedSave },
    { "save_path", TestSavePath },
    { "bad_file", TestBadFile },
    { "refuse_eeprom", TestRefuseEeprom },
    { "refuse_dsv", TestRefuseDsv },
    { "refuse_padded", TestRefusePadded },
    { "refuse_larger", TestRefuseLarger },
    { "refuse_directory", TestRefuseDirectory },
};

// The tests that end the program, which "all" leaves out
static int IsStopping(int (*run)(void)) {
    return run == TestBadFile || run == TestRefuseEeprom || run == TestRefuseDsv || run == TestRefusePadded ||
           run == TestRefuseLarger || run == TestRefuseDirectory;
}

int main(int argc, char **argv) {
    int all = argc < 2 || strcmp(argv[1], "all") == 0;
    int ran = 0;
    int status;

    for (size_t i = 0; i < sizeof(sTests) / sizeof(sTests[0]); i++) {
        if (all ? IsStopping(sTests[i].run) : strcmp(argv[1], sTests[i].name) != 0) {
            continue;
        }
        printf("%s\n", sTests[i].name);
        sFailed = 0;
        status = sTests[i].run();
        if (status == SKIP_STATUS && !sFailed && !all) {
            return SKIP_STATUS;
        }
        if ((status != 0 && status != SKIP_STATUS) || sFailed) {
            printf("FAIL %s\n", sTests[i].name);
            return 1;
        }
        ran++;
    }
    if (ran == 0 && !all) {
        printf("no test named %s\n", argv[1]);
        return 2;
    }
    return 0;
}
