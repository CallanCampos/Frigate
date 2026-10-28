// NitroSDK's file system, the archives' default commands, on the host: the decomp's fs_command_default.c, compiled as
// it is from its own path through this file (port/hal/fs/replaced.txt keeps it out of the game's own sources). Two of
// its functions keep memory addresses in 32-bit words, which cut a 64-bit host pointer, so they are renamed out of the
// way before the include and written again below:
//   - FS_LoadArchiveTables keeps FAT/FNT positions as archive offsets, and copies their bytes into the caller's
//     buffer. FSi_HostReadArchiveTable resolves only bounded table spans through the full native load_mem pointer;
//     directory positions never become truncated host addresses. The original unload restores offsets and returns
//     that same caller-owned buffer. SystemInit and DWC's utility archive keep their original allocation lifetimes;
//   - FS_LoadArchive gives an archive without a read or write function of its own the defaults that read and write
//     memory at its base plus a position (FS_GetArchiveOffset), a 32-bit address. The one such archive, the memory
//     archive of FS_CreateFileFromMemory, is read and written by the host's hook with the game's whole pointer
//     (port/hal/fs/fs_memfile_host.c) and never reaches them. The host's defaults stop the program with a message,
//     since the pointer that such an address was cut from is gone.
#include "nitro/fs.h"
#include "port/hal/fs/port_fs_tables.h"
#include <stdint.h>

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

// A table range must be representable by the SDK's archive offsets, without wrapping around zero.
static BOOL FSi_HostTableRange(u32 start, u32 size) {
    return size <= UINT32_MAX - start;
}

static BOOL FSi_HostTableContains(u32 start, u32 size, u32 position, u32 length) {
    return position >= start && position - start <= size && length <= size - (position - start);
}

// Read only the cached table that owns this complete span. Never turn a missing or ambiguous offset into a pointer.
FSResult FSi_HostReadArchiveTable(FSArchive *arc, void *buffer, u32 position, u32 length) {
    const FSArchiveTables *tables = arc->userdata;
    BOOL inFat = FSi_HostTableRange(tables->fat_bak, tables->fat_size) &&
                 FSi_HostTableContains(tables->fat_bak, tables->fat_size, position, length);
    BOOL inFnt = FSi_HostTableRange(tables->fnt_bak, tables->fnt_size) &&
                 FSi_HostTableContains(tables->fnt_bak, tables->fnt_size, position, length);
    uintptr_t aligned;
    const u8 *source;

    // A zero-byte span may belong to both adjacent tables (or an empty table); there is nothing to copy or resolve.
    if (tables->load_mem != NULL && length == 0 && (inFat || inFnt)) {
        return FS_RESULT_SUCCESS;
    }
    if (tables->load_mem == NULL || inFat == inFnt) {
        fprintf(stderr, "port: fs: cached table read has no unique range at %#x (%u bytes)\n", position, length);
        return FS_RESULT_FAILURE;
    }
    aligned = ((uintptr_t)tables->load_mem + 0x1f) & ~(uintptr_t)0x1f;
    source = (const u8 *)aligned;
    if (inFat) {
        source += position - tables->fat_bak;
    } else {
        source += tables->fat_size + (position - tables->fnt_bak);
    }
    MI_CpuCopy8(source, buffer, length);
    return FS_RESULT_SUCCESS;
}

// Load through the same SDK file commands and into the same aligned caller allocation as the DS. Only the cache's
// representation differs: FAT/FNT remain offsets, while load_mem retains the whole native pointer.
u32 FS_LoadArchiveTables(FSArchive *arc, void *memory, u32 memorySize) {
    FSArchiveTables *tables;
    u32 size;
    uintptr_t aligned;
    FSFile file;

    if (memory != NULL && FS_IsArchiveTableLoaded(arc)) {
        FS_UnloadArchiveTables(arc);
    }
    tables = arc->userdata;
    if (tables->fat_size > UINT32_MAX - 0x3f || tables->fnt_size > UINT32_MAX - 0x3f - tables->fat_size) {
        fprintf(stderr, "port: fs: archive table sizes overflow the SDK cache size\n");
        exit(1);
    }
    size = (tables->fat_size + tables->fnt_size + 0x3f) & ~0x1f;
    if (memory == NULL || size > memorySize) {
        return size;
    }
    if (!FSi_HostTableRange(tables->fat, tables->fat_size) || !FSi_HostTableRange(tables->fnt, tables->fnt_size) ||
        (tables->fat_size != 0 && tables->fnt_size != 0 && tables->fat < tables->fnt + tables->fnt_size &&
         tables->fnt < tables->fat + tables->fat_size) ||
        (uintptr_t)memory > UINTPTR_MAX - memorySize) {
        fprintf(stderr, "port: fs: invalid or overlapping archive table cache ranges\n");
        return size;
    }
    aligned = ((uintptr_t)memory + 0x1f) & ~(uintptr_t)0x1f;
    FS_InitFile(&file);
    if (FS_OpenFileDirect(&file, arc, tables->fat, tables->fat + tables->fat_size, -1)) {
        if (FS_ReadFile(&file, (void *)aligned, tables->fat_size) < 0) {
            MI_CpuFill8((void *)aligned, 0, tables->fat_size);
        }
        FS_CloseFile(&file);
    }
    aligned += tables->fat_size;
    if (FS_OpenFileDirect(&file, arc, tables->fnt, tables->fnt + tables->fnt_size, -1)) {
        if (FS_ReadFile(&file, (void *)aligned, tables->fnt_size) < 0) {
            MI_CpuFill8((void *)aligned, 0, tables->fnt_size);
        }
        FS_CloseFile(&file);
    }
    tables->load_mem = memory;
    arc->flag |= FS_ARCHIVE_FLAG_TABLE_LOAD;
    return size;
}
