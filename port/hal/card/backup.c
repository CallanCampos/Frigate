// The ARM7's side of the game card's backup memory, on the host (port_card.h).
//
// The protocol is the one the ARM9's CARD library speaks (lib/nitro/src/card/card_request.c, card_backup.c): the
// ARM9 fills its command block (CARDiCommandArg: the result, the backup type and its spec, a source, a destination and
// a length) and sends the request's number as a word of the file system's PXI tag, with the error bit set; the ARM7
// carries it out, writes the result into the block and answers with a word of the same tag, the error bit set, which
// wakes the ARM9's waiting thread. The first request, CARD_REQ_INIT, is followed by a second word, the address of the
// command block. The block is the ARM9's cardi_arg (card_common.c), which the host reaches through cardi_common.cmd: a
// PXI word carries only 26 bits of data, so the address in it serves only as a check.
//
// Memory: for every request BW2 makes, the ARM9 names to the ARM7 its page buffer (card_backup.c's sCardBackupBuffer),
// in a 32-bit word of the block, which holds the low half of the host pointer. The host's ARM7 reaches that buffer and
// nothing else; another address fails the request with CARD_RESULT_ERROR and a message.
//
// The chip: 4 Mbit of flash, as BW2 identifies it (src/gfl/backup_card.c). A page write (CARD_REQ_WRITE_BACKUP) erases
// and programs, a page program (CARD_REQ_PROGRAM_BACKUP) can only clear bits, and the erases set bytes to 0xff.
// Addresses wrap at the chip's size, as the chip ignores the address bits above it. The work takes no time on the
// virtual clock: a request ends before the PXI word that started it is delivered back.
#include "hal/card/card_internal.h"
#include "hal/card/port_card.h"
#include "hal/os/port_app.h"
#include "hal/os/port_os.h"
#include "hal/rom/rom_manifest.h"
#include "nitro/card.h"
#include "nitro/pxi.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

// The PXI bus (port/hal/pxi), which another part of the platform layer provides: port/cmake/rom.cmake defines
// PORT_CARD_PXI_BUS when the tree has it. Until then the ARM7's card services can't be registered, and the game waits
// at its first backup request for an ARM7 that never answers
#ifndef PORT_CARD_PXI_BUS
#define PORT_CARD_PXI_BUS 0
#endif
#if PORT_CARD_PXI_BUS
#include "hal/pxi/pxi_bus.h"
#endif

// The words of the card's own tag that the ARM9 sends (card_pull_out.c): TERMINATE, when the card was pulled out
#define CARD_PXI_COMMAND_MASK 0x3f
// A flash chip's sector, for the sector erase: the spec's sectSize for every flash BW2 could identify
#define FLASH_SECTOR_SIZE 0x10000
// While the game writes, the image goes to disk at least this often, in frames
#define FLUSH_FRAMES 60
#define SAVE_DIRECTORY_NAME "pokebw2-native"
#define PATH_LENGTH 1024

static struct {
    BOOL booted;
    BOOL expectingBlock; // CARD_REQ_INIT's second word, the command block's address, comes next
    BOOL identified;     // the ARM9 identified the backup as the chip the host has
    BOOL reportedMemory; // the message about memory the ARM7 can't reach was printed
    uint8_t *image;
    BOOL dirty;          // changed since it was last written to its file
    BOOL wroteThisFrame; // changed since the last frame hook
    uint32_t waitFrames; // frames since it was last written to its file, or since a write failed, while it is dirty
    BOOL failing;        // the last write to its file failed, which was reported
    BOOL pathGiven;      // the tests' path, in place of the save directory's
    char path[PATH_LENGTH];
} sCard;

// --- The save file ------------------------------------------------------------------------------------------------

// The save directory, without the file's name: --save-dir, else the user's own (port_card.h)
static BOOL SaveDirectory(char *out, size_t size) {
    const char *dir = port_app_option("save-dir");
    const char *base;

    if (dir != NULL && dir[0] != '\0') {
        return snprintf(out, size, "%s", dir) < (int)size;
    }
#if defined(_WIN32)
    base = getenv("APPDATA");
    return base != NULL && snprintf(out, size, "%s\\%s", base, SAVE_DIRECTORY_NAME) < (int)size;
#elif defined(__APPLE__)
    base = getenv("HOME");
    return base != NULL &&
           snprintf(out, size, "%s/Library/Application Support/%s", base, SAVE_DIRECTORY_NAME) < (int)size;
#else
    base = getenv("XDG_DATA_HOME");
    if (base != NULL && base[0] == '/') {
        return snprintf(out, size, "%s/%s", base, SAVE_DIRECTORY_NAME) < (int)size;
    }
    base = getenv("HOME");
    return base != NULL && snprintf(out, size, "%s/.local/share/%s", base, SAVE_DIRECTORY_NAME) < (int)size;
#endif
}

const char *port_card_save_path(void) {
    char dir[PATH_LENGTH];

    if (!sCard.pathGiven && sCard.path[0] == '\0') {
        if (!SaveDirectory(dir, sizeof(dir)) || snprintf(sCard.path, sizeof(sCard.path), "%s/%s.sav", dir,
                                                         port_rom_manifest_game_code) >= (int)sizeof(sCard.path)) {
            fprintf(stderr, "port: card: no save directory: give one with --save-dir PATH\n");
            exit(PORT_CARD_EXIT_STATUS);
        }
    }
    return sCard.path;
}

void port_card_set_save_path(const char *path) {
    free(sCard.image);
    sCard.image = NULL;
    sCard.dirty = FALSE;
    sCard.wroteThisFrame = FALSE;
    sCard.waitFrames = 0;
    sCard.failing = FALSE;
    sCard.pathGiven = path != NULL;
    snprintf(sCard.path, sizeof(sCard.path), "%s", path != NULL ? path : "");
}

// Makes the directories of a file's path that don't exist yet
static void MakeParentDirectories(const char *path) {
    char partial[PATH_LENGTH];
    size_t i;

    snprintf(partial, sizeof(partial), "%s", path);
    for (i = 1; partial[i] != '\0'; i++) {
        if (partial[i] == '/' || partial[i] == '\\') {
            char separator = partial[i];
            partial[i] = '\0';
#ifdef _WIN32
            _mkdir(partial);
#else
            mkdir(partial, 0755);
#endif
            partial[i] = separator;
        }
    }
}

// The sizes of the saves of the DS's other backup chips, as melonDS 1.1 sizes them by save type (NDSCart.cpp's
// sramlengths): a file of one of these sizes is the save of another game
static const struct {
    long size;
    const char *chip;
} sOtherChips[] = {
    { 0x200, "a 4 Kbit EEPROM" },   { 0x2000, "a 64 Kbit EEPROM" },   { 0x10000, "a 512 Kbit EEPROM" },
    { 0x20000, "a 1 Mbit EEPROM" }, { 0x40000, "a 2 Mbit flash" },    { 0x100000, "an 8 Mbit flash" },
    { 0x800000, "a 64 Mbit NAND" }, { 0x1000000, "a 128 Mbit NAND" }, { 0x4000000, "a 512 Mbit NAND" },
};

// The last bytes of DeSmuME's save files (.dsv), which put DeSmuME's own data after the chip's image
#define DESMUME_FOOTER_END "|-DESMUME SAVE-|"

// Stops the program over a save file that isn't an image of the chip, saying what it seems to be: the file is never
// overwritten. size is the file's, or -1 when it couldn't be measured
static void RefuseFile(const char *path, FILE *file, long size) {
    char end[sizeof(DESMUME_FOOTER_END) - 1];
    char reason[160];
    size_t i;

    snprintf(reason, sizeof(reason), "it is %ld bytes", size);
    if (size < 0) {
        snprintf(reason, sizeof(reason), "its size can't be read");
    } else if (size == PORT_CARD_BACKUP_SIZE) {
        snprintf(reason, sizeof(reason), "it can't be read whole");
    } else if (size == 0) {
        snprintf(reason, sizeof(reason), "it is empty");
    } else if (size >= (long)sizeof(end) && fseek(file, size - (long)sizeof(end), SEEK_SET) == 0 &&
               fread(end, 1, sizeof(end), file) == sizeof(end) && memcmp(end, DESMUME_FOOTER_END, sizeof(end)) == 0) {
        snprintf(reason, sizeof(reason),
                 "it ends with DeSmuME's footer, so it is a .dsv, the image with DeSmuME's own data after it");
    }
    for (i = 0; i < sizeof(sOtherChips) / sizeof(sOtherChips[0]); i++) {
        if (size == sOtherChips[i].size) {
            snprintf(reason, sizeof(reason), "it is %ld bytes, the save of %s, another game's chip", size,
                     sOtherChips[i].chip);
        }
    }
    fprintf(stderr,
            "port: card: %s isn't a backup image of %u bytes (the 4 Mbit flash of %s, as melonDS's .sav of it is): "
            "%s; move it away, or give another save directory with --save-dir\n",
            path, (unsigned)PORT_CARD_BACKUP_SIZE, port_rom_manifest_title, reason);
    fclose(file);
    exit(PORT_CARD_EXIT_STATUS);
}

// Whether name ends with .sav, in any case
static BOOL HasSaveExtension(const char *name) {
    size_t length = strlen(name);
    const char *extension = ".sav";
    size_t i;

    if (length < 4) {
        return FALSE;
    }
    for (i = 0; i < 4; i++) {
        char c = name[length - 4 + i];
        if ((c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c) != extension[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

// Whether name is a save as the port names its own: a game code of four capital letters and digits, and .sav
static BOOL IsPortSaveName(const char *name) {
    size_t i;

    for (i = 0; i < 4; i++) {
        if (!((name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= '0' && name[i] <= '9'))) {
            return FALSE;
        }
    }
    return strlen(name) == 8 && HasSaveExtension(name);
}

// A file of the save directory that could be this game's save under another name: a .sav file of the image's size.
// Each is named, with what to do with it
static void HintOtherSave(const char *dir, const char *name, int64_t size) {
    if (!HasSaveExtension(name) || IsPortSaveName(name) || size != PORT_CARD_BACKUP_SIZE) {
        return;
    }
    fprintf(stderr,
            "port: card: %s has no %s.sav, so the game starts with a blank backup; %s there is an image of the same "
            "chip under another name (melonDS names its saves after the ROM's file): to play that save, rename it "
            "%s.sav\n",
            dir, port_rom_manifest_game_code, name, port_rom_manifest_game_code);
}

// When the save file isn't there, the saves of the same size in its directory that the game won't see by their
// names: a melonDS .sav copied into the save directory keeps the ROM file's name. Files named as the port names its
// own (a game code, IREO.sav or IRDO.sav) are other games' or the other version's, and are left alone
static void HintOtherSaves(const char *path) {
    char dir[PATH_LENGTH];
    char *separator;

    snprintf(dir, sizeof(dir), "%s", path);
    separator = strrchr(dir, '/');
#ifdef _WIN32
    if (strrchr(dir, '\\') > separator) {
        separator = strrchr(dir, '\\');
    }
#endif
    if (separator == NULL) {
        return;
    }
    *separator = '\0';
#ifdef _WIN32
    {
        char pattern[PATH_LENGTH + 8];
        WIN32_FIND_DATAA found;
        HANDLE search;

        snprintf(pattern, sizeof(pattern), "%s\\*", dir);
        search = FindFirstFileA(pattern, &found);
        if (search == INVALID_HANDLE_VALUE) {
            return;
        }
        do {
            if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                HintOtherSave(dir, found.cFileName, ((int64_t)found.nFileSizeHigh << 32) | (int64_t)found.nFileSizeLow);
            }
        } while (FindNextFileA(search, &found));
        FindClose(search);
    }
#else
    {
        DIR *listing = opendir(dir);
        struct dirent *entry;

        if (listing == NULL) {
            return;
        }
        while ((entry = readdir(listing)) != NULL) {
            char file[PATH_LENGTH + 256];
            struct stat info;

            snprintf(file, sizeof(file), "%s/%s", dir, entry->d_name);
            if (stat(file, &info) == 0 && S_ISREG(info.st_mode)) {
                HintOtherSave(dir, entry->d_name, (int64_t)info.st_size);
            }
        }
        closedir(listing);
    }
#endif
}

// The image, read from its file the first time it is needed: the file holds the chip byte for byte, as melonDS's .sav
// of the game does, and a chip that was never written reads 0xff everywhere. A file of another size is never
// overwritten: the program stops instead
uint8_t *port_card_backup_image(void) {
    const char *path;
    FILE *file;

    if (sCard.image != NULL) {
        return sCard.image;
    }
    path = port_card_save_path();
    sCard.image = malloc(PORT_CARD_BACKUP_SIZE);
    if (sCard.image == NULL) {
        fprintf(stderr, "port: card: no memory for the backup image\n");
        exit(PORT_CARD_EXIT_STATUS);
    }
    memset(sCard.image, 0xff, PORT_CARD_BACKUP_SIZE);
    file = fopen(path, "rb");
    if (file != NULL) {
        long size = -1;
        if (fseek(file, 0, SEEK_END) == 0) {
            size = ftell(file);
        }
        if (size != PORT_CARD_BACKUP_SIZE || fseek(file, 0, SEEK_SET) != 0 ||
            fread(sCard.image, 1, PORT_CARD_BACKUP_SIZE, file) != PORT_CARD_BACKUP_SIZE) {
            RefuseFile(path, file, size);
        }
        fclose(file);
    } else if (errno == ENOENT) {
        HintOtherSaves(path);
    } else {
        fprintf(stderr, "port: card: can't read %s: %s\n", path, strerror(errno));
        exit(PORT_CARD_EXIT_STATUS);
    }
    return sCard.image;
}

// The error of the call that just failed, as errno has it, or EIO when the call didn't set it
static int LastError(void) {
    return errno != 0 ? errno : EIO;
}

// Writes the image to a temporary file beside its own, then renames it over the old one, so that the file is always
// one whole image or the other. Returns 0, or the error of the first call that failed, taken before the temporary
// file's removal can change errno
static int WriteImage(const char *path) {
    char temporary[PATH_LENGTH + 8];
    FILE *file;
    int error = 0;

    if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary)) {
        return ENAMETOOLONG;
    }
    MakeParentDirectories(path);
    errno = 0;
    file = fopen(temporary, "wb");
    if (file == NULL) {
        return LastError();
    }
    if (fwrite(sCard.image, 1, PORT_CARD_BACKUP_SIZE, file) != PORT_CARD_BACKUP_SIZE || fflush(file) != 0) {
        error = LastError();
    }
#ifndef _WIN32
    if (error == 0 && fsync(fileno(file)) != 0) {
        error = LastError();
    }
#endif
    if (fclose(file) != 0 && error == 0) {
        error = LastError();
    }
    if (error == 0) {
#ifdef _WIN32
        if (MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
            error = EIO;
        }
#else
        if (rename(temporary, path) != 0) {
            error = LastError();
        }
#endif
    }
    if (error != 0) {
        remove(temporary);
    }
    return error;
}

void port_card_flush(void) {
    int error;

    if (!sCard.dirty || sCard.image == NULL) {
        return;
    }
    error = WriteImage(port_card_save_path());
    sCard.waitFrames = 0;
    if (error != 0) {
        // The game goes on with the image in memory, and the frame hook tries again every FLUSH_FRAMES frames. The
        // failure is reported once, until a write succeeds
        if (!sCard.failing) {
            sCard.failing = TRUE;
            fprintf(stderr, "port: card: can't write %s: %s; trying again every %d frames\n", port_card_save_path(),
                    strerror(error), FLUSH_FRAMES);
        }
        return;
    }
    if (sCard.failing) {
        sCard.failing = FALSE;
        fprintf(stderr, "port: card: wrote %s\n", port_card_save_path());
    }
    sCard.dirty = FALSE;
}

// The write at exit, which says so when the game's last changes are lost
static void FlushAtExit(void) {
    port_card_flush();
    if (sCard.dirty && sCard.image != NULL) {
        fprintf(stderr, "port: card: the game's last changes to the backup weren't written to %s\n",
                port_card_save_path());
    }
}

// The frame hook: writes the image once a frame passes without the game changing it, or at least every FLUSH_FRAMES
// frames while it keeps changing it (a save spans several frames). After a failed write it tries again every
// FLUSH_FRAMES frames
static void OnFrame(void) {
    if (sCard.dirty) {
        sCard.waitFrames++;
        if (sCard.waitFrames >= FLUSH_FRAMES || (!sCard.failing && !sCard.wroteThisFrame)) {
            port_card_flush();
        }
    }
    sCard.wroteThisFrame = FALSE;
}

// --- The requests -------------------------------------------------------------------------------------------------

static void Changed(void) {
    sCard.dirty = TRUE;
    sCard.wroteThisFrame = TRUE;
}

// The ARM9 memory that a 32-bit address of the command block names, for length bytes: only card_backup.c's page
// buffer (the file's header). NULL, with a message the first time, for anything else
static uint8_t *Arm9Memory(u32 address, u32 length) {
    uint8_t *buffer = port_card_page_buffer();

    if (address == (u32)(uintptr_t)buffer && length <= PORT_CARD_PAGE_SIZE) {
        return buffer;
    }
    if (!sCard.reportedMemory) {
        sCard.reportedMemory = TRUE;
        fprintf(stderr,
                "port: card: a backup request names ARM9 memory at %#x (%u bytes), which the host's ARM7 can't reach: "
                "only the CARD library's page buffer, %#x, is\n",
                address, length, (u32)(uintptr_t)buffer);
    }
    return NULL;
}

// Whether the backup type is the chip the host has: a flash (the device, in the low byte) of 1 << 19 bytes (the size's
// shift, in the next byte); the vendor, in the third, doesn't matter
static BOOL IsHostChip(CARDBackupType type) {
    return (type & 0xff) == CARD_BACKUP_TYPE_DEVICE_FLASH && 1u << ((type >> 8) & 0xff) == PORT_CARD_BACKUP_SIZE;
}

static CARDResult Read(CARDiCommandArg *cmd) {
    uint8_t *image = port_card_backup_image();
    uint8_t *dst = Arm9Memory(cmd->dst, cmd->len);
    u32 i;

    if (dst == NULL) {
        return CARD_RESULT_ERROR;
    }
    for (i = 0; i < cmd->len; i++) {
        dst[i] = image[(cmd->src + i) & (PORT_CARD_BACKUP_SIZE - 1)];
    }
    return CARD_RESULT_SUCCESS;
}

// A page write erases and programs (CARD_REQ_WRITE_BACKUP), a program only clears bits (CARD_REQ_PROGRAM_BACKUP)
static CARDResult Write(CARDiCommandArg *cmd, BOOL program) {
    uint8_t *image = port_card_backup_image();
    const uint8_t *src = Arm9Memory(cmd->src, cmd->len);
    u32 i;

    if (src == NULL) {
        return CARD_RESULT_ERROR;
    }
    for (i = 0; i < cmd->len; i++) {
        uint8_t *byte = &image[(cmd->dst + i) & (PORT_CARD_BACKUP_SIZE - 1)];
        *byte = program ? *byte & src[i] : src[i];
    }
    Changed();
    return CARD_RESULT_SUCCESS;
}

// What the ARM9 wrote against what the chip holds now
static CARDResult Verify(CARDiCommandArg *cmd) {
    uint8_t *image = port_card_backup_image();
    const uint8_t *src = Arm9Memory(cmd->src, cmd->len);
    u32 i;

    if (src == NULL) {
        return CARD_RESULT_ERROR;
    }
    for (i = 0; i < cmd->len; i++) {
        if (image[(cmd->dst + i) & (PORT_CARD_BACKUP_SIZE - 1)] != src[i]) {
            return CARD_RESULT_FAILURE;
        }
    }
    return CARD_RESULT_SUCCESS;
}

// Erases len bytes from start, start rounded down to a multiple of unit (a sector's start, for a sector erase)
static CARDResult Erase(u32 start, u32 len, u32 unit) {
    uint8_t *image = port_card_backup_image();
    u32 i;

    start &= ~(unit - 1);
    for (i = 0; i < len; i++) {
        image[(start + i) & (PORT_CARD_BACKUP_SIZE - 1)] = 0xff;
    }
    Changed();
    return CARD_RESULT_SUCCESS;
}

// Carries out one request on the command block
static CARDResult Execute(CARDiCommandArg *cmd, u32 request) {
    if (request == CARD_REQ_IDENTIFY) {
        sCard.identified = IsHostChip(cmd->type);
        return sCard.identified ? CARD_RESULT_SUCCESS : CARD_RESULT_NO_RESPONSE;
    }
    if (!sCard.identified) {
        return CARD_RESULT_NO_RESPONSE;
    }
    if ((cmd->spec.caps & CARD_CAPS(request)) == 0) {
        return CARD_RESULT_UNSUPPORTED;
    }
    switch (request) {
    case CARD_REQ_READ_BACKUP:
        return Read(cmd);
    case CARD_REQ_WRITE_BACKUP:
        return Write(cmd, FALSE);
    case CARD_REQ_PROGRAM_BACKUP:
        return Write(cmd, TRUE);
    case CARD_REQ_VERIFY_BACKUP:
        return Verify(cmd);
    case CARD_REQ_ERASE_PAGE_BACKUP:
        return Erase(cmd->dst, cmd->len, 1);
    case CARD_REQ_ERASE_SECTOR_BACKUP:
        return Erase(cmd->dst, cmd->len, FLASH_SECTOR_SIZE);
    case CARD_REQ_ERASE_SUBSECTOR_BACKUP:
        return Erase(cmd->dst, cmd->len, cmd->spec.subsectSize != 0 ? cmd->spec.subsectSize : 1);
    case CARD_REQ_ERASE_CHIP_BACKUP:
        return Erase(0, PORT_CARD_BACKUP_SIZE, 1);
    case CARD_REQ_WRITE_STATUS:
        return CARD_RESULT_SUCCESS; // the status register's protection bits, which no write of the host's ever meets
    default:
        // The SDK's requests that BW2's library never sends (the chip's ID and status, raw commands)
        return CARD_RESULT_UNSUPPORTED;
    }
}

// The answer to the ARM9: a word of the tag with the error bit set
static void Acknowledge(u32 request) {
#if PORT_CARD_PXI_BUS
    port_pxi_reply(PXI_FIFO_TAG_FS, request, TRUE);
#else
    (void)request;
#endif
}

void port_card_on_fs_word(uint32_t tag, uint32_t data, int err) {
    CARDiCommandArg *cmd = cardi_common.cmd;

    (void)tag;
    (void)err;
    if (cmd == NULL) {
        fprintf(stderr, "port: card: a backup request before the CARD library set its command block up\n");
        return;
    }
    if (sCard.expectingBlock) {
        // The block's address, in the 26 bits of data a PXI word holds
        u32 expected = (u32)(uintptr_t)cmd & 0x03ffffff;
        sCard.expectingBlock = FALSE;
        if (data != expected) {
            fprintf(stderr, "port: card: the ARM9 named its command block %#x, but cardi_common's is %#x\n", data,
                    expected);
        }
        cmd->result = CARD_RESULT_SUCCESS;
        Acknowledge(CARD_REQ_INIT);
        return;
    }
    if (data == CARD_REQ_INIT) {
        sCard.expectingBlock = TRUE;
        return;
    }
    cmd->result = Execute(cmd, data);
    Acknowledge(data);
}

// The ARM7's side of the card's own tag: the ARM9 sends TERMINATE when the card was pulled out, which a host's card
// never is; nothing is answered
static void OnCardWord(uint32_t tag, uint32_t data, int err) {
    (void)tag;
    (void)err;
    if ((data & CARD_PXI_COMMAND_MASK) != 0) {
        fprintf(stderr, "port: card: the ARM9 sent the card's tag %#x\n", data);
    }
}

// The services are registered at every call, since a reset of the PXI bus (port_pxi_reset) takes them away; the frame
// hook and the write at exit are the process's, once
void port_card_boot(void) {
    sCard.expectingBlock = FALSE;
    sCard.identified = FALSE;
#if PORT_CARD_PXI_BUS
    port_pxi_register_service(PXI_FIFO_TAG_FS, port_card_on_fs_word);
    port_pxi_register_service(PXI_FIFO_TAG_CARD, OnCardWord);
#else
    (void)OnCardWord;
    if (!sCard.booted) {
        fprintf(stderr, "port: card: this build has no PXI bus (port/hal/pxi), so the ARM7's backup service is off; "
                        "the game will wait for it at its first save access\n");
    }
#endif
    if (!sCard.booted) {
        sCard.booted = TRUE;
        port_os_add_frame_hook(OnFrame);
        atexit(FlushAtExit);
    }
}
