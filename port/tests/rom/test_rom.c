// Tests of the ROM, the card and the file system as the game links them (port/hal/rom, port/hal/card, port/hal/fs over
// the decomp's own FS and CARD libraries), each run on the OS core as the game's main() would run, after the part of
// OS_Init that they need: the PXI bus and the host's ARM7, the arenas, the locks, the threads and the alarms. Each runs
// in a process of its own, since the SDK's libraries initialize once per process (CARD_Init, FS_Init). They need the
// player's ROM and skip (77) without it.
//
//   port_rom_tests NAME
#include "battle/btl_event.h"
#include "field/field_terrain_animator.h"
#include "gfl/backup_card.h"
#include "hal/card/port_card.h"
#include "hal/fs/ovsec.h"
#include "hal/os/port_os.h"
#include "hal/rom/port_rom.h"
#include "nitro/card.h"
// card.h's backup calls as the game's backup_card.c makes them on the host, with the buffer kept whole
#include "hal/card/card_backup_calls.h"
#include "nitro/fs.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SKIP 77

static int sFailed;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("  check failed: %s\n", what);
        sFailed = 1;
    }
}

static u32 Read32(const u8 *p) {
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

// Whole sections, read and written byte by byte without ASan's checks: ASan pads each global with a poisoned redzone,
// which lies inside the sections, as port/hal/fs/ovsec.c says
__attribute__((no_sanitize("address"), noinline)) static void FillSection(u8 *start, size_t size, u8 value) {
    for (size_t i = 0; i < size; i++) {
        ((volatile u8 *)start)[i] = value;
    }
}

__attribute__((no_sanitize("address"), noinline)) static BOOL SectionIs(const u8 *start, size_t size, const u8 *bytes,
                                                                        u8 value) {
    for (size_t i = 0; i < size; i++) {
        if (((const volatile u8 *)start)[i] != (bytes != NULL ? bytes[i] : value)) {
            return FALSE;
        }
    }
    return TRUE;
}

// Two statics of overlay files, for the check that they lie in their overlay's sections and are set back: an
// initialized one of ov036's field_g3d_mapper.c and a zeroed one of ov167's battle_event.c
extern FieldTerrainAnimatorSetup GND_ANIME_WK;
extern BattleEventItem *data_ov167_021db1a0[];

// The part of OS_Init that the file system and the card need, in OS_Init's order, then FS_Init and FS_LoadTable as
// SystemInit does them. Returns the table's memory
static void *StartFileSystem(u32 *tableSize) {
    void *table;

    PXI_Init();
    OS_InitArena();
    OS_InitLock();
    OS_InitIrqTable();
    OS_InitVAlarm();
    OS_InitThread();
    OS_InitTick();
    OS_InitAlarm();
    (void)OS_EnableIrq();
    (void)OS_EnableInterrupts();
    FS_Init(1);
    *tableSize = FS_LoadTable(NULL, 0);
    table = OS_AllocFromArenaLo(OS_ARENA_MAIN, *tableSize, 4);
    Check(table != NULL, "the arena gives the table's memory");
    Check(FS_LoadTable(table, *tableSize) == *tableSize, "FS_LoadTable takes it");
    return table;
}

// The FAT entry of a file of the ROM
static void FileRange(u32 fileId, u32 *start, u32 *end) {
    const u8 *rom = port_rom_data();
    u32 fat = Read32(rom + 0x48);

    *start = Read32(rom + fat + fileId * 8);
    *end = Read32(rom + fat + fileId * 8 + 4);
}

// FS_Init and FS_LoadTable, then files by path through the ROM archive, against the ROM's bytes
static void FileSystemMain(void) {
    const u8 *rom;
    u32 tableSize;
    FSFile file;
    FSFileID id;
    u32 start;
    u32 end;
    u8 *data;
    s32 read;
    u8 header[0x10];

    StartFileSystem(&tableSize);
    rom = port_rom_data();
    Check(rom != NULL, "the CARD library's initialization opened the ROM");
    Check(tableSize == ((Read32(rom + 0x4c) + Read32(rom + 0x44) + 0x3f) & ~0x1fu),
          "FS_LoadTable asks for the DS's size: the FAT, the FNT and the alignment");
    Check(memcmp((void *)HW_ROM_HEADER_BUF, rom, HW_CARD_ROM_HEADER_SIZE) == 0, "the header is at HW_ROM_HEADER_BUF");
    Check(memcmp((void *)HW_CARD_ROM_HEADER, rom, HW_CARD_ROM_HEADER_SIZE) == 0, "and CARD_Init's copy");
    Check(memcmp(CARDi_GetRomAuthCode(), rom + Read32(rom + 0x80), 0x88) == 0,
          "CARDi_InitRom read the bytes after the program");

    // A NARC of the game's archive table, whole
    FS_InitFile(&file);
    Check(FS_ConvertPathToFileID(&id, "a/0/0/0"), "a/0/0/0 has a file ID");
    Check(FS_OpenFile(&file, "a/0/0/0"), "a/0/0/0 opens");
    FileRange(id.file_id, &start, &end);
    Check(FS_GetFileLength(&file) == end - start, "with the FAT's length");
    data = malloc(end - start);
    read = FS_ReadFile(&file, data, (s32)(end - start));
    Check(read == (s32)(end - start) && memcmp(data, rom + start, end - start) == 0, "and the ROM's bytes");
    Check(memcmp(data, "NARC", 4) == 0, "a NARC");
    FS_CloseFile(&file);
    free(data);

    // A range of the sound archive, by seeking
    FS_InitFile(&file);
    Check(FS_OpenFile(&file, "/swan_sound_data.sdat"), "the sound archive opens from the root");
    Check(FS_SeekFile(&file, 0x1000, FS_SEEK_SET), "seeks");
    Check(FS_ReadFile(&file, header, sizeof(header)) == sizeof(header), "reads a range");
    Check(FS_GetFilePosition(&file) == 0x1000 + sizeof(header), "and moves on");
    FS_ConvertPathToFileID(&id, "swan_sound_data.sdat");
    FileRange(id.file_id, &start, &end);
    Check(memcmp(header, rom + start + 0x1000, sizeof(header)) == 0, "the range's bytes");
    FS_CloseFile(&file);

    // Paths that don't exist, and raw ranges of the ROM: the card reads the header and secure area, below 0x8000, as
    // the block at 0x8000
    FS_InitFile(&file);
    Check(!FS_OpenFile(&file, "a/9/9/9"), "a missing file doesn't open");
    Check(romfs_fopen_raw(&file, Read32(rom + 0x40), 4), "a raw range of the ROM opens");
    Check(FS_ReadFile(&file, header, 4) == 4 && memcmp(header, rom + Read32(rom + 0x40), 4) == 0,
          "with the file name table's first bytes");
    FS_CloseFile(&file);
    FS_InitFile(&file);
    Check(romfs_fopen_raw(&file, 0x0c, 4), "a raw range of the header opens");
    Check(FS_ReadFile(&file, header, 4) == 4 && memcmp(header, rom + 0x800c, 4) == 0,
          "and reads the block at 0x8000, as a card does");
    FS_CloseFile(&file);
}

// A card read straight through the CARD library, without waiting, with its callback
static int sCallbacks;

static void CountCallback(void *arg) {
    (void)arg;
    sCallbacks++;
}

static void CardReadMain(void) {
    u32 tableSize;
    u8 buffer[0x300];
    u16 lockId;

    const u8 *rom;
    u64 capacity;

    StartFileSystem(&tableSize);
    rom = port_rom_data();
    capacity = (u64)0x20000 << (rom[0x14] & 0x1f);
    lockId = (u16)OS_GetLockID();
    CARD_LockRom(lockId);
    CARDi_ReadRom(1, 0x81ff, buffer, sizeof(buffer), CountCallback, NULL, TRUE);
    Check(CARDi_TryWaitAsync(), "the card's thread finished the read before the call returned");
    Check(sCallbacks == 1, "its callback ran once");
    Check(memcmp(buffer, rom + 0x81ff, sizeof(buffer)) == 0, "it read the ROM");

    // Below 0x8000 each block of 0x200 bytes reads as the block at 0x8000
    CARDi_ReadRom(1, 0x1f0, buffer, 0x20, NULL, NULL, FALSE);
    Check(memcmp(buffer, rom + 0x81f0, 0x10) == 0 && memcmp(buffer + 0x10, rom + 0x8000, 0x10) == 0,
          "the header's addresses read the block at 0x8000");

    // Past the end: a card of the dump's size wraps to its start, which reads as 0x8000; the rest of a card that a
    // trimmed dump leaves out reads 0xff
    CARDi_ReadRom(1, (u32)port_rom_size() - 0x10, buffer, 0x20, NULL, NULL, FALSE);
    Check(memcmp(buffer, rom + port_rom_size() - 0x10, 0x10) == 0, "a read up to the end of the file");
    if (port_rom_size() == capacity) {
        Check(memcmp(buffer + 0x10, rom + 0x8000, 0x10) == 0, "and on past the card's capacity wraps");
    } else {
        Check(buffer[0x10] == 0xff && buffer[0x1f] == 0xff, "and on past the end of a trimmed dump gives 0xff");
    }
    CARD_UnlockRom(lockId);
    OS_ReleaseLockID(lockId);
}

// GND_ANIME_WK's bytes
static u8 *const sTerrainSetup = (u8 *)&GND_ANIME_WK;

// Overlays: the headers from the ROM's table, and the statics set back on a load
static void OverlayMain(void) {
    const u8 *rom;
    u32 tableSize;
    u32 table;
    FSOverlayInfo info;
    const PortOvsecOverlay *overlay = NULL;
    u32 id;
    u32 start;
    u32 end;

    StartFileSystem(&tableSize);
    rom = port_rom_data();
    table = Read32(rom + 0x50);
    Check(port_ovsec_overlay_count() > 0, "the program has overlay sections");
#if defined(__APPLE__)
    Check(port_ovsec_image_section_count() <= 255, "within Mach-O's 255 sections");
#endif

    // Every header, as the ROM's table has it
    for (id = 0; id < Read32(rom + 0x54) / FS_OVERLAY_HEADER_SIZE; id++) {
        const u8 *header = rom + table + id * FS_OVERLAY_HEADER_SIZE;
        if (!FS_LoadOverlayInfo(&info, MI_PROCESSOR_ARM9, id)) {
            Check(0, "an overlay's header loads");
            break;
        }
        FileRange(info.fileId, &start, &end);
        if (info.id != id || (u32)(uintptr_t)info.ramAddress != Read32(header + 4) ||
            info.ramSize != Read32(header + 8) || info.bssSize != Read32(header + 12) ||
            (u32)(uintptr_t)info.sinitStart != Read32(header + 16) || info.fileId != Read32(header + 24) ||
            info.compressedSize != (Read32(header + 28) & 0xffffff) || info.flag != Read32(header + 28) >> 24 ||
            info.target != MI_PROCESSOR_ARM9 || info.filePos != start || info.fileSize != end - start) {
            printf("  overlay %u's header differs\n", id);
            sFailed = 1;
        }
    }
    Check(!FS_LoadOverlayInfo(&info, MI_PROCESSOR_ARM9, id), "past the table there are none");
    FS_LoadOverlayInfo(&info, MI_PROCESSOR_ARM9, 12);
#ifdef WHITE2
    Check((u32)(uintptr_t)info.ramAddress == 0x02150400, "overlay 12 loads where the field core does");
#else
    Check((u32)(uintptr_t)info.ramAddress == 0x021503c0, "overlay 12 loads where the field core does");
#endif

    // A real overlay's statics in their sections
    overlay = port_ovsec_get(36);
    Check(overlay != NULL && sTerrainSetup >= overlay->data.start &&
              sTerrainSetup < overlay->data.start + overlay->data.size,
          "an initialized global of overlay 36 is in its data section");
    overlay = port_ovsec_get(167);
    Check(overlay != NULL && (u8 *)data_ov167_021db1a0 >= overlay->bss.start &&
              (u8 *)data_ov167_021db1a0 < overlay->bss.start + overlay->bss.size,
          "a zeroed global of overlay 167 is in its bss section");

    // Loading sets the statics back; unloading leaves them; loading again sets them back again
    overlay = port_ovsec_get(36);
    if (overlay != NULL && overlay->bss.start != NULL) {
        u8 initial[8];

        memcpy(initial, sTerrainSetup, sizeof(initial));
        FillSection(overlay->data.start, overlay->data.size, 0xa5);
        FillSection(overlay->bss.start, overlay->bss.size, 0x5a);
        Check(sTerrainSetup[0] == 0xa5, "the overlay's data was dirtied");
        Check(FS_LoadOverlay(MI_PROCESSOR_ARM9, 36), "overlay 36 loads");
        Check(SectionIs(overlay->data.start, overlay->data.size, overlay->snapshot, 0), "its data is set back");
        Check(SectionIs(overlay->bss.start, overlay->bss.size, NULL, 0), "its bss is zeroed");
        Check(memcmp(sTerrainSetup, initial, sizeof(initial)) == 0, "GND_ANIME_WK has its initializer again");
        sTerrainSetup[0] ^= 0xff;
        Check(FS_UnloadOverlay(MI_PROCESSOR_ARM9, 36), "overlay 36 unloads");
        Check(sTerrainSetup[0] == (u8)(initial[0] ^ 0xff), "unloading leaves the statics");
        Check(FS_LoadOverlay(MI_PROCESSOR_ARM9, 36), "and loads again");
        Check(memcmp(sTerrainSetup, initial, sizeof(initial)) == 0, "which sets them back");
    } else {
        Check(0, "overlay 36 has data and bss sections");
    }
    // An overlay without writable statics on the host loads too
    for (id = 0; id < 344 && port_ovsec_get(id) != NULL; id++) {
    }
    Check(id == 344 || FS_LoadOverlay(MI_PROCESSOR_ARM9, id), "an overlay without sections loads");
}

// The backup memory through the ARM9's CARD library and the host's ARM7, and the game's own helpers (backup_card.c),
// with the game's buffers in each kind of memory it has: the main arena, which its heaps come from, the program's
// static data, a thread's stack, and host memory outside both
static int sWriteCallbacks;

static void WriteDone(void *arg) {
    (void)arg;
    sWriteCallbacks++;
}

static char sSavePath[512];
static u8 sStaticBuffer[0x300];

static u8 Pattern(u32 i, u32 seed) {
    return (u8)(i * 13 + seed);
}

// A write of three pages from the buffer at 0x1000 + offset, and a read back into it
static void WriteAndReadBack(u8 *buffer, u32 offset, u32 seed, const char *where) {
    char what[128];
    u32 i;

    for (i = 0; i < 0x300; i++) {
        buffer[i] = Pattern(i, seed);
    }
    sWriteCallbacks = 0;
    snprintf(what, sizeof(what), "a write from %s succeeds", where);
    Check(CARD_WriteAndVerifyBackupAsync(0x1000 + offset, buffer, 0x300, WriteDone, NULL) && CARD_WaitBackupAsync() &&
              CARD_GetResultCode() == CARD_RESULT_SUCCESS,
          what);
    Check(sWriteCallbacks == 1, "its callback ran");
    memset(buffer, 0, 0x300);
    snprintf(what, sizeof(what), "a read into %s", where);
    Check(CARD_ReadBackup(0x1000 + offset, buffer, 0x300), what);
    for (i = 0; i < 0x300 && buffer[i] == Pattern(i, seed); i++) {
    }
    snprintf(what, sizeof(what), "reads back what was written from %s", where);
    Check(i == 0x300, what);
    Check(memcmp(port_card_backup_image() + 0x1000 + offset, buffer, 0x300) == 0, "the chip's image holds it");
}

static void BackupMain(void) {
    u32 tableSize;
    u8 *arena;
    u8 *heap;
    u8 stack[0x300];
    u8 *image;
    u32 word;
    u16 lockId;
    u32 i;

    StartFileSystem(&tableSize);
    port_card_set_save_path(sSavePath);
    arena = OS_AllocFromArenaLo(OS_ARENA_MAIN, 0x400, 32);
    heap = malloc(0x300);

    lockId = (u16)OS_GetLockID();
    CARD_LockBackup(lockId);
    Check(CARD_IdentifyBackup(CARD_BACKUP_TYPE_FLASH_4MBITS_EX), "the 4 Mbit flash is identified");
    Check(CARD_ReadBackup(0x1000, arena, 0x300), "a read of three pages");
    for (i = 0; i < 0x300 && arena[i] == 0xff; i++) {
    }
    Check(i == 0x300, "a new chip reads 0xff");
    WriteAndReadBack(arena, 0, 7, "the arena");
    WriteAndReadBack(sStaticBuffer, 0x400, 9, "static data");
    WriteAndReadBack(stack, 0x800, 11, "a thread's stack");
    WriteAndReadBack(heap, 0xc00, 13, "the host's heap");
    CARD_UnlockBackup(lockId);
    OS_ReleaseLockID(lockId);
    image = port_card_backup_image();

    // The game's own identification, read and write (src/gfl/backup_card.c), whose write first reads the save's first
    // word into a local of the thread's stack
    Check(identifyGameCardBackup(), "identifyGameCardBackup");
    word = 0;
    Check(readFlashWaitSuccess(0x1004, &word, sizeof(word), NULL), "readFlashWaitSuccess into a local");
    Check(memcmp(&word, image + 0x1004, sizeof(word)) == 0, "reads the chip");
    for (i = 0; i < 0x300; i++) {
        heap[i] = Pattern(i, 17);
    }
    Check(func_0203bfd4(0x2000, heap, 0x300, NULL), "the game's write, which reads the first word into a local");
    Check(memcmp(image + 0x2000, heap, 0x300) == 0, "writes the chip");
    memset(arena, 0, 0x10);
    Check(readFlashWaitSuccess(0x2004, arena, 0x10, NULL), "readFlashWaitSuccess into the arena");
    Check(memcmp(arena, image + 0x2004, 0x10) == 0, "reads it back");
    free(heap);

    port_card_flush();
    {
        FILE *file = fopen(sSavePath, "rb");
        u8 bytes[4];
        Check(file != NULL && fseek(file, 0x1000, SEEK_SET) == 0 && fread(bytes, 1, 4, file) == 4 &&
                  memcmp(bytes, image + 0x1000, 4) == 0,
              "the save file has the write");
        if (file != NULL) {
            fclose(file);
        }
    }
    remove(sSavePath);
}

// A backup read whose buffer comes cut to 32 bits, through CARDi_RequestStreamCommand itself rather than card.h's
// calls: the host can't tell which pointer it was cut from, and the program stops, naming why, whatever memory the
// buffer is in
static void BackupCutPointerMain(void) {
    u32 tableSize;
    u8 *buffer;
    u16 lockId;

    StartFileSystem(&tableSize);
    port_card_set_save_path(sSavePath);
    buffer = OS_AllocFromArenaLo(OS_ARENA_MAIN, 0x100, 32);
    lockId = (u16)OS_GetLockID();
    CARD_LockBackup(lockId);
    CARD_IdentifyBackup(CARD_BACKUP_TYPE_FLASH_4MBITS_EX);
    CARDi_RequestStreamCommand(0, (u32)(uintptr_t)buffer, 0x100, NULL, NULL, FALSE, CARD_REQ_READ_BACKUP, 1,
                               CARD_REQUEST_MODE_RECV);
    printf("  the read went on\n");
}

// Files over memory (FS_CreateFileFromMemory), as GFL's archives in memory open them, with the data in each kind of
// memory the game has: read whole, seeked and read, written, and two open at once
static u8 sStaticFile[0x400];

static void CheckMemoryFile(u8 *data, u32 size, const u8 *expected, const char *where) {
    char what[128];
    FSFile file;
    u8 *read = malloc(size);
    u8 bytes[8];

    FS_InitFile(&file);
    snprintf(what, sizeof(what), "a file over %s opens", where);
    Check(FS_CreateFileFromMemory(&file, data, size), what);
    Check(FS_GetFileLength(&file) == size, "with the memory's length");
    snprintf(what, sizeof(what), "a file over %s reads whole", where);
    Check(FS_ReadFile(&file, read, (s32)size) == (s32)size && memcmp(read, expected, size) == 0, what);
    Check(FS_ReadFile(&file, bytes, 1) == 0, "and has nothing more");
    Check(FS_SeekFile(&file, 0x10, FS_SEEK_SET) && FS_ReadFile(&file, bytes, 8) == 8 &&
              memcmp(bytes, expected + 0x10, 8) == 0,
          "seeks and reads a range");
    memcpy(bytes, "PORTTEST", 8);
    Check(FS_SeekFile(&file, 0x20, FS_SEEK_SET) && FS_WriteFile(&file, bytes, 8) == 8 &&
              memcmp(data + 0x20, "PORTTEST", 8) == 0,
          "writes into the memory");
    Check(FS_GetFilePosition(&file) == 0x28, "and moves on");
    memcpy(data + 0x20, expected + 0x20, 8);
    Check(FS_CloseFile(&file), "closes");
    free(read);
}

static void MemoryFilesMain(void) {
    const u8 *rom;
    u32 tableSize;
    u32 start;
    u32 end;
    FSFileID id;
    u32 size;
    u8 *arena;
    u8 *heap;
    u8 stack[0x400];
    FSFile first;
    FSFile second;
    u8 bytes[4];

    StartFileSystem(&tableSize);
    rom = port_rom_data();
    // A NARC of the game's, as GFL_ArcSysCreateMemoryHandle opens one: its first 0x400 bytes
    Check(FS_ConvertPathToFileID(&id, "a/0/0/0"), "a/0/0/0 has a file ID");
    FileRange(id.file_id, &start, &end);
    size = end - start < 0x400 ? end - start : 0x400;
    arena = OS_AllocFromArenaLo(OS_ARENA_MAIN, size, 32);
    heap = malloc(size);
    memcpy(arena, rom + start, size);
    memcpy(heap, rom + start, size);
    memcpy(sStaticFile, rom + start, size);
    memcpy(stack, rom + start, size);
    CheckMemoryFile(arena, size, rom + start, "the arena");
    CheckMemoryFile(sStaticFile, size, rom + start, "static data");
    CheckMemoryFile(stack, size, rom + start, "a thread's stack");
    CheckMemoryFile(heap, size, rom + start, "the host's heap");

    // Two files open at once, each read from its own memory
    heap[0] = 'X';
    FS_InitFile(&first);
    FS_InitFile(&second);
    Check(FS_CreateFileFromMemory(&first, arena, size) && FS_CreateFileFromMemory(&second, heap, size),
          "two files over memory open at once");
    Check(FS_ReadFile(&second, bytes, 4) == 4 && bytes[0] == 'X', "the second reads its memory");
    Check(FS_ReadFile(&first, bytes, 4) == 4 && memcmp(bytes, "NARC", 4) == 0, "the first reads its own");
    FS_CloseFile(&first);
    FS_CloseFile(&second);
    free(heap);
}

typedef struct Test {
    const char *name;
    void (*run)(void);
} Test;

static const Test sTests[] = {
    { "file_system", FileSystemMain },
    { "card_read", CardReadMain },
    { "overlays", OverlayMain },
    { "backup", BackupMain },
    { "backup_cut_pointer", BackupCutPointerMain },
    { "memory_files", MemoryFilesMain },
};

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s NAME\n", argv[0]);
        return 2;
    }
    if (port_rom_default_path() == NULL) {
        printf("no ROM (" PORT_ROM_ENVIRONMENT_VARIABLE ", or the repository's orig/baserom_<version>.nds): skipped\n");
        return SKIP;
    }
    snprintf(sSavePath, sizeof(sSavePath), "%s/port_rom_tests_%d.sav",
             getenv("TMPDIR") != NULL ? getenv("TMPDIR") : "/tmp", (int)getpid());
    for (size_t i = 0; i < sizeof(sTests) / sizeof(sTests[0]); i++) {
        if (strcmp(argv[1], sTests[i].name) == 0) {
            printf("%s\n", sTests[i].name);
            Check(port_os_run(sTests[i].run) == PORT_OS_RETURNED, "the run ended normally");
            port_card_set_save_path(NULL);
            remove(sSavePath);
            if (sFailed) {
                printf("FAIL %s\n", sTests[i].name);
                return 1;
            }
            return 0;
        }
    }
    printf("no test named %s\n", argv[1]);
    return 2;
}
