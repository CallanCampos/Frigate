// Unit tests of the host's ARM7 backup service (port/hal/card/backup.c), driven word by word as the ARM9's CARD library
// drives it (lib/nitro/src/card/card_request.c), through a stand-in PXI bus that records the service and its answers:
// the INIT handshake, the identification of the 4 Mbit flash, the flash's reads, page writes, programs, verifies and
// erases on the command block, memory the ARM7 can't reach, and the save file: written whole and atomically, once a
// frame passes without a write or every 60 frames of writes, read back, and never overwritten when it isn't an image;
// a write that fails is reported once and tried again every 60 frames. Needs no ROM.
//
//   port_card_backup_tests NAME | all
#include "hal/card/card_internal.h"
#include "hal/card/port_card.h"
#include "hal/os/port_app.h"
#include "hal/pxi/pxi_bus.h"
#include "hal/rom/rom_manifest.h"
#include "nitro/card.h"
#include "nitro/pxi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
    snprintf(dir, sizeof(dir), "%s/port_card_%s_%d", getenv("TMPDIR") != NULL ? getenv("TMPDIR") : "/tmp", name,
             (int)getpid());
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
    mkdir(dir, 0755);
    strcat(dir, "/save");
    file = fopen(dir, "wb"); // where the save directory should be
    Check(file != NULL, "a file in the save directory's place");
    if (file != NULL) {
        fclose(file);
    }

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
        reason += strstr(line, "Not a directory") != NULL;
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
    mkdir(dir, 0755);
    strcat(dir, "/save");
    mkdir(dir, 0755);
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
    Check(strstr(port_card_save_path(), "pokebw2-native/") != NULL, "else the user's own");
    Check(getenv("HOME") == NULL || strncmp(port_card_save_path(), getenv("HOME"), strlen(getenv("HOME"))) == 0 ||
              getenv("XDG_DATA_HOME") != NULL,
          "under the home directory");
    port_app_set_arguments(0, NULL, NULL, 0);
    return 0;
}

typedef struct Test {
    const char *name;
    int (*run)(void);
} Test;

static const Test sTests[] = {
    { "init", TestInit },          { "identify", TestIdentify },
    { "flash", TestFlash },        { "unreachable", TestUnreachable },
    { "save_file", TestSaveFile }, { "write_failure", TestWriteFailure },
    { "save_path", TestSavePath }, { "bad_file", TestBadFile },
};

int main(int argc, char **argv) {
    int all = argc < 2 || strcmp(argv[1], "all") == 0;
    int ran = 0;

    for (size_t i = 0; i < sizeof(sTests) / sizeof(sTests[0]); i++) {
        if (all ? sTests[i].run == TestBadFile : strcmp(argv[1], sTests[i].name) != 0) {
            continue;
        }
        printf("%s\n", sTests[i].name);
        sFailed = 0;
        if (sTests[i].run() != 0 || sFailed) {
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
