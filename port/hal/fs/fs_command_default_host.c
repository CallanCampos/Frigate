// NitroSDK's file system, the archives' default commands, on the host: the decomp's fs_command_default.c, compiled as
// it is from its own path through this file (port/hal/fs/replaced.txt keeps it out of the game's own sources). Two of
// its functions keep memory addresses in 32-bit words, which cut a 64-bit host pointer, so they are renamed out of the
// way before the include and written again below:
//   - FS_LoadArchiveTables copies an archive's tables into the memory given and from then on reads them through 32-bit
//     positions (FSArchiveTables' fat and fnt, FSTableReader's position, FSDirPos' position). On the host the tables
//     stay in the archive's storage, which for the ROM archive is the memory-mapped ROM (port/hal/rom): the function
//     returns the size the DS's needs, so that the game sets the same memory aside (SystemInit's FS_TryLoadTable), and
//     the archive goes on reading its tables through its read function, as every DS game does before it loads them.
//     What the game reads is the same; FS_UnloadArchiveTables, the decomp's, finds nothing loaded;
//   - FS_LoadArchive gives an archive without a read or write function of its own the defaults that read and write
//     memory at its base plus a position (FS_GetArchiveOffset), a 32-bit address. The one such archive, the memory
//     archive of FS_CreateFileFromMemory, is read and written by the host's hook with the game's whole pointer
//     (port/hal/fs/fs_memfile_host.c) and never reaches them. The host's defaults stop the program with a message,
//     since the pointer that such an address was cut from is gone.
#include "nitro/fs.h"

#define FS_LoadArchiveTables FSi_DsLoadArchiveTables
#define FS_LoadArchive FSi_DsLoadArchive
// The decomp's file keeps addresses in 32-bit words, as the DS does. Its warnings about them, and those of the warning
// groups the game's own files aren't built with, are silenced for the file alone, as port/CMakeLists.txt does for every
// file of the game (pokebw2_game); the compile probe counts them from the file's own path (port/tools/compile-probe).
// The host's code below keeps every warning of the platform layer
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wswitch"
#include "lib/nitro/src/fs/fs_command_default.c"
#pragma GCC diagnostic pop
#undef FS_LoadArchiveTables
#undef FS_LoadArchive

#include <stdio.h>
#include <stdlib.h>

// An archive in memory at a 32-bit address, which the host can't reach (above)
static void FSi_HostMemoryArchiveStop(FSArchive *arc, u32 position, u32 length) {
    FSArchiveTables *tables = arc->userdata;

    fprintf(stderr,
            "port: fs: an archive without a read function of its own reads its memory at %#x (%u bytes), an address "
            "cut to 32 bits; the host reads memory only through the pointer the game gave (FS_CreateFileFromMemory)\n",
            tables->base + position, length);
    exit(1);
}

static FSResult FSi_HostReadMemCallback(FSArchive *arc, void *buffer, u32 position, u32 length) {
    (void)buffer;
    FSi_HostMemoryArchiveStop(arc, position, length);
    return FS_RESULT_FAILURE;
}

static FSResult FSi_HostWriteMemCallback(FSArchive *arc, const void *buffer, u32 position, u32 length) {
    (void)buffer;
    FSi_HostMemoryArchiveStop(arc, position, length);
    return FS_RESULT_FAILURE;
}

// NitroSDK's FS_LoadArchive, with the host's defaults for the memory's read and write
BOOL FS_LoadArchive(FSArchive *arc, u32 base, u32 fat, u32 fatSize, u32 fnt, u32 fntSize, FS_ARCHIVE_READ_FUNC readFunc,
                    FS_ARCHIVE_WRITE_FUNC writeFunc) {
    BOOL result = FSi_DsLoadArchive(arc, base, fat, fatSize, fnt, fntSize, readFunc, writeFunc);

    if (readFunc == NULL) {
        arc->tables.read_func = FSi_HostReadMemCallback;
    }
    if (writeFunc == NULL) {
        arc->tables.write_func = FSi_HostWriteMemCallback;
    }
    return result;
}

// NitroSDK's FS_LoadArchiveTables: the size of memory the tables need (the FAT, the FNT and 0x20 bytes for the
// alignment, rounded up to 0x20), with the tables left where they are (above)
u32 FS_LoadArchiveTables(FSArchive *arc, void *memory, u32 memorySize) {
    FSArchiveTables *tables = arc->userdata;

    (void)memory;
    (void)memorySize;
    return (tables->fat_size + tables->fnt_size + 0x3f) & ~0x1f;
}
