#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fs.h"

// A NARC starts with a header whose size is at offset 0xc. The blocks follow it: FATB, the start and end of each file
// in FIMG, then FNTB and FIMG. Each block starts with its magic and size
#define NARC_HEADER_SIZE_OFFSET 0xc
#define NARC_BLOCK_SIZE_OFFSET 4
#define NARC_FATB_ENTRIES_OFFSET 0xc
#define NARC_FIMG_DATA_OFFSET 8

typedef struct {
    u32 size;
    u16 fileCount;
} NarcFATBHeader;

typedef struct {
    u32 start;
    u32 end;
} NarcFATBEntry;

struct ArcTool {
    FSFile file;
    u32 fimgOffset;
    u16 fatbOffset;
    u16 fileCount;
};

typedef struct {
    // The path of each archive, or the start and end of each in memory
    union {
        const char **paths;
        const NarcFATBEntry *ranges;
    } table;
    u32 count;
    BOOL inMemory;
} ArcSys;

static void GFL_ArcSysReadImpl(void *dest, u32 arcId, u32 fileId, u32 offset, u32 size);
static void *GFL_ArcSysReadHeapNewImpl(u32 arcId, u32 fileId, HeapID heapId, u32 offset, u32 size);
static void GFL_ArcSysInitArcHandle(FSFile *file, u32 arcId);
static u32 ArchiveMoveImageTop(FSFile *file, u32 fileId, u32 offset, u32 size);
static void GFL_ArcToolInit(ArcTool *handle);

static ArcSys sArcSys;

// The file name the allocators pass. The assert's strings of ArchiveMoveImageTop follow it: its file name, the empty
// string plus its padding, and its format, in one array whose leading four zero bytes are the name
static char sArcFileName[12] = "arc_tool.c";
static char sAssertNameAndFormat[48] = "\0\0\0\0ArchiveMoveImageTop fileCnt=%d, datID=%d";

void GFL_ArcSysInit(const char **paths, u32 count) {
    sArcSys.table.paths = paths;
    sArcSys.count = count;
    sArcSys.inMemory = FALSE;
}

static void GFL_ArcSysReadImpl(void *dest, u32 arcId, u32 fileId, u32 offset, u32 size) {
    FSFile file;

    GFL_ArcSysInitArcHandle(&file, arcId);
    size = ArchiveMoveImageTop(&file, fileId, offset, size);
    FS_ReadFile(&file, dest, size);
    FS_CloseFile(&file);
}

static void *GFL_ArcSysReadHeapNewImpl(u32 arcId, u32 fileId, HeapID heapId, u32 offset, u32 size) {
    FSFile file;
    u32 dataSize;
    void *data;

    GFL_ArcSysInitArcHandle(&file, arcId);
    dataSize = ArchiveMoveImageTop(&file, fileId, offset, size);
    data = GFL_HeapAllocate(heapId, dataSize, FALSE, sArcFileName, 187);
    FS_ReadFile(&file, data, dataSize);
    FS_CloseFile(&file);
    return data;
}

static void GFL_ArcSysInitArcHandle(FSFile *file, u32 arcId) {
    FS_InitFile(file);
    if (!sArcSys.inMemory) {
        FS_OpenFile(file, sArcSys.table.paths[arcId]);
    } else {
        u32 size = sArcSys.table.ranges[arcId].end;

        size -= sArcSys.table.ranges[arcId].start;
        FS_CreateFileFromMemory(file, (void *)sArcSys.table.ranges[arcId].start, size);
    }
}

#ifdef PLATFORM_PC
// The host keeps the equivalent C used before upstream's MWCC-only assembly exception. The dead stack store that
// forced that exception affects neither archive offsets nor data; host compilers cannot assemble its ARM code.
static u32 ArchiveMoveImageTop(FSFile *file, u32 fileId, u32 offset, u32 size) {
    NarcFATBEntry entry;
    NarcFATBHeader fatb;
    u32 value = 0;
    u32 fntbOffset;
    u32 fimgOffset;
    u32 fatbOffset;

    FS_SeekFile(file, NARC_HEADER_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(file, &value, 2);
    fatbOffset = value;
    FS_SeekFile(file, fatbOffset + NARC_BLOCK_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(file, &fatb, 6);
    if (!(fatb.fileCount > fileId)) {
        GFL_DebugAssertFailEx(sAssertNameAndFormat, 0, sAssertNameAndFormat + 4, fatb.fileCount, fileId);
    }
    fntbOffset = fatbOffset + fatb.size;
    FS_SeekFile(file, fntbOffset + NARC_BLOCK_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(file, &value, 4);
    fimgOffset = fntbOffset + value;
    FS_SeekFile(file, fatbOffset + NARC_FATB_ENTRIES_OFFSET + fileId * sizeof(NarcFATBEntry), FS_SEEK_SET);
    FS_ReadFile(file, &entry, sizeof(NarcFATBEntry));
    fimgOffset += NARC_FIMG_DATA_OFFSET + entry.start;
    FS_SeekFile(file, offset + fimgOffset, FS_SEEK_SET);
    if (size == 0) {
        size = entry.end - entry.start;
    }
    return size;
}
#else
// Seeks to the data of a file of an archive, at offset into it, and returns its size, or size if it is not 0.
// Hand-written in assembly: the original build's compiler kept the dead store of fimgOffset + 8 back to its stack
// slot, which no installed compiler produces from the C, so the maintainer took the assembly exception for this, the
// last non-matching function. The pool words below are the assert's two strings, sAssertNameAndFormat and its +4.
// clang-format off
// style-ok: asm the maintainer's exception for the last non-matching function, which no compiler reproduces from C
static asm u32 ArchiveMoveImageTop(FSFile *file, u32 fileId, u32 offset, u32 size) {
    push {r4, r5, r6, r7, lr}
    sub sp, #0x24
    mov r6, r1
    mov r1, #0x0
    str r2, [sp, #0x4]
    str r1, [sp, #0x10]
    mov r5, r0
    mov r1, #0xc
    mov r2, #0x0
    mov r7, r3
    bl FS_SeekFile
    mov r0, r5
    add r1, sp, #0x10
    mov r2, #0x2
    bl FS_ReadFile
    ldr r4, [sp, #0x10]
    mov r0, r5
    add r1, r4, #0x4
    mov r2, #0x0
    bl FS_SeekFile
    mov r0, r5
    add r1, sp, #0x14
    mov r2, #0x6
    bl FS_ReadFile
    add r0, sp, #0x10
    ldrh r3, [r0, #0x8]
    cmp r3, r6
    bhi AssertDone
    ldr r0, [pc, #0x6c]
    ldr r2, [pc, #0x70]
    mov r1, #0x0
    str r6, [sp, #0x0]
    bl GFL_DebugAssertFailEx
AssertDone:
    ldr r0, [sp, #0x14]
    mov r2, #0x0
    add r0, r4, r0
    str r0, [sp, #0xc]
    ldr r1, [sp, #0xc]
    mov r0, r5
    add r1, r1, #0x4
    bl FS_SeekFile
    mov r0, r5
    add r1, sp, #0x10
    mov r2, #0x4
    bl FS_ReadFile
    ldr r1, [sp, #0x10]
    ldr r0, [sp, #0xc]
    add r4, #0xc
    add r0, r0, r1
    lsl r1, r6, #0x3
    str r0, [sp, #0x8]
    mov r0, r5
    add r1, r4, r1
    mov r2, #0x0
    bl FS_SeekFile
    mov r0, r5
    add r1, sp, #0x1c
    mov r2, #0x8
    bl FS_ReadFile
    ldr r1, [sp, #0x8]
    ldr r2, [sp, #0x1c]
    add r1, #0x8
    str r1, [sp, #0x8]
    add r2, r1, r2
    ldr r1, [sp, #0x4]
    mov r0, r5
    add r1, r1, r2
    mov r2, #0x0
    bl FS_SeekFile
    cmp r7, #0x0
    bne SizeDone
    ldr r1, [sp, #0x20]
    ldr r0, [sp, #0x1c]
    sub r7, r1, r0
SizeDone:
    mov r0, r7
    add sp, #0x24
    pop {r4, r5, r6, r7, pc}
    mov r8, r8
    dcd sAssertNameAndFormat
    dcd sAssertNameAndFormat+4
}

// clang-format on
#endif

void GFL_ArcSysRead(void *dest, u32 arcId, u32 fileId) {
    GFL_ArcSysReadImpl(dest, arcId, fileId, 0, 0);
}

void *GFL_ArcSysReadHeapNew(u32 arcId, u32 fileId, HeapID heapId) {
    return GFL_ArcSysReadHeapNewImpl(arcId, fileId, heapId, 0, 0);
}

void GFL_ArcSysReadRange(void *dest, u32 arcId, u32 fileId, u32 offset, u32 size) {
    GFL_ArcSysReadImpl(dest, arcId, fileId, offset, size);
}

void *GFL_ArcSysReadHeapNewRange(u32 arcId, u32 fileId, HeapID heapId, u32 offset, u32 size) {
    return GFL_ArcSysReadHeapNewImpl(arcId, fileId, heapId, offset, size);
}

void *GFL_ArcSysReadHeapNewDirect(const char *path, u32 fileId, HeapID heapId) {
    FSFile file;
    u32 size;
    void *data;

    FS_InitFile(&file);
    FS_OpenFile(&file, path);
    size = ArchiveMoveImageTop(&file, fileId, 0, 0);
    data = GFL_HeapAllocate(heapId, size, FALSE, sArcFileName, 386);
    FS_ReadFile(&file, data, size);
    FS_CloseFile(&file);
    return data;
}

u32 GFL_ArcSysGetDataMax(u32 arcId) {
    FSFile file;
    u32 value = 0;
    NarcFATBHeader fatb;

    GFL_ArcSysInitArcHandle(&file, arcId);
    FS_SeekFile(&file, NARC_HEADER_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(&file, &value, 2);
    FS_SeekFile(&file, value + NARC_BLOCK_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(&file, &fatb, 6);
    FS_CloseFile(&file);
    return fatb.fileCount;
}

u32 GFL_ArcSysGetDataLength(u32 arcId, u32 fileId) {
    FSFile file;
    u32 size;

    GFL_ArcSysInitArcHandle(&file, arcId);
    size = ArchiveMoveImageTop(&file, fileId, 0, 0);
    FS_CloseFile(&file);
    return size;
}

ArcTool *GFL_ArcSysCreateFileHandle(u32 arcId, HeapID heapId) {
    ArcTool *handle = GFL_HeapAllocate(heapId, sizeof(ArcTool), FALSE, sArcFileName, 521);

    GFL_ArcSysInitArcHandle(&handle->file, arcId);
    GFL_ArcToolInit(handle);
    return handle;
}

ArcTool *GFL_ArcSysCreateMemoryHandle(void *data, u32 size, HeapID heapId) {
    ArcTool *handle = GFL_HeapAllocate(heapId, sizeof(ArcTool), FALSE, sArcFileName, 543);

    FS_InitFile(&handle->file);
    FS_CreateFileFromMemory(&handle->file, data, size);
    GFL_ArcToolInit(handle);
    return handle;
}

static void GFL_ArcToolInit(ArcTool *handle) {
    u32 fntbSize;
    NarcFATBHeader fatb;
    u32 fntbOffset;

    handle->fatbOffset = 0;
    FS_SeekFile(&handle->file, NARC_HEADER_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(&handle->file, &handle->fatbOffset, 2);
    FS_SeekFile(&handle->file, handle->fatbOffset + NARC_BLOCK_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(&handle->file, &fatb, 6);
    handle->fileCount = fatb.fileCount;
    fntbOffset = handle->fatbOffset + fatb.size;
    FS_SeekFile(&handle->file, fntbOffset + NARC_BLOCK_SIZE_OFFSET, FS_SEEK_SET);
    FS_ReadFile(&handle->file, &fntbSize, 4);
    handle->fimgOffset = fntbOffset + fntbSize;
}

void GFL_ArcToolFree(ArcTool *handle) {
    FS_CloseFile(&handle->file);
    GFL_HeapFree(handle);
}

void *GFL_ArcToolReadHeapNew(ArcTool *handle, u32 fileId, HeapID heapId) {
    NarcFATBEntry entry;
    void *data;

    if (!(handle->fileCount > fileId)) {
        GFL_DebugAssertFailEx(sAssertNameAndFormat, 0, "DatCount=%d, DatID=%d", handle->fileCount, fileId);
    }
    FS_SeekFile(&handle->file, handle->fatbOffset + NARC_FATB_ENTRIES_OFFSET + fileId * sizeof(NarcFATBEntry),
                FS_SEEK_SET);
    FS_ReadFile(&handle->file, &entry, sizeof(NarcFATBEntry));
    FS_SeekFile(&handle->file, handle->fimgOffset + NARC_FIMG_DATA_OFFSET + entry.start, FS_SEEK_SET);
    data = GFL_HeapAllocate(heapId, entry.end - entry.start, FALSE, sArcFileName, 619);
    if (data != NULL) {
        FS_ReadFile(&handle->file, data, entry.end - entry.start);
    }
    return data;
}

void GFL_ArcToolRead(ArcTool *handle, u32 fileId, void *dest) {
    NarcFATBEntry entry;

    if (!(handle->fileCount > fileId)) {
        GFL_DebugAssertFailEx(sAssertNameAndFormat, 0, "DatCount=%d, DatID=%d", handle->fileCount, fileId);
    }
    FS_SeekFile(&handle->file, handle->fatbOffset + NARC_FATB_ENTRIES_OFFSET + fileId * sizeof(NarcFATBEntry),
                FS_SEEK_SET);
    FS_ReadFile(&handle->file, &entry, sizeof(NarcFATBEntry));
    FS_SeekFile(&handle->file, handle->fimgOffset + NARC_FIMG_DATA_OFFSET + entry.start, FS_SEEK_SET);
    FS_ReadFile(&handle->file, dest, entry.end - entry.start);
}

u32 GFL_ArcToolGetDataLength(ArcTool *handle, u32 fileId) {
    NarcFATBEntry entry;

    if (!(handle->fileCount > fileId)) {
        GFL_DebugAssertFailEx(sAssertNameAndFormat, 0, "DatCount=%d, DatID=%d", handle->fileCount, fileId);
    }
    FS_SeekFile(&handle->file, handle->fatbOffset + NARC_FATB_ENTRIES_OFFSET + fileId * sizeof(NarcFATBEntry),
                FS_SEEK_SET);
    FS_ReadFile(&handle->file, &entry, sizeof(NarcFATBEntry));
    return entry.end - entry.start;
}

void GFL_ArcToolReadRange(ArcTool *handle, u32 fileId, u32 offset, u32 size, void *dest) {
    u32 start;

    if (!(handle->fileCount > fileId)) {
        GFL_DebugAssertFailEx(sAssertNameAndFormat, 0, "DatCount=%d, DatID=%d", handle->fileCount, fileId);
    }
    FS_SeekFile(&handle->file, handle->fatbOffset + NARC_FATB_ENTRIES_OFFSET + fileId * sizeof(NarcFATBEntry),
                FS_SEEK_SET);
    FS_ReadFile(&handle->file, &start, 4);
    FS_SeekFile(&handle->file, offset + (handle->fimgOffset + NARC_FIMG_DATA_OFFSET + start), FS_SEEK_SET);
    FS_ReadFile(&handle->file, dest, size);
}

void GFL_ArcToolCopyDataOfs(ArcTool *handle, u32 fileId, u32 *offset) {
    u32 start;

    if (!(handle->fileCount > fileId)) {
        GFL_DebugAssertFailEx(sAssertNameAndFormat, 0, "DatCount=%d, DatID=%d", handle->fileCount, fileId);
    }
    FS_SeekFile(&handle->file, handle->fatbOffset + NARC_FATB_ENTRIES_OFFSET + fileId * sizeof(NarcFATBEntry),
                FS_SEEK_SET);
    FS_ReadFile(&handle->file, &start, 4);
    *offset = handle->fimgOffset + NARC_FIMG_DATA_OFFSET + start;
}

u32 GFL_ArcToolGetDataOfs(ArcTool *handle, u32 fileId) {
    u32 start;

    if (!(handle->fileCount > fileId)) {
        GFL_DebugAssertFailEx(sAssertNameAndFormat, 0, "DatCount=%d, DatID=%d", handle->fileCount, fileId);
    }
    FS_SeekFile(&handle->file, handle->fatbOffset + NARC_FATB_ENTRIES_OFFSET + fileId * sizeof(NarcFATBEntry),
                FS_SEEK_SET);
    FS_ReadFile(&handle->file, &start, 4);
    return handle->fimgOffset + NARC_FIMG_DATA_OFFSET + start;
}

void GFL_ArcToolReadRaw(ArcTool *handle, u32 size, void *dest) {
    FS_ReadFile(&handle->file, dest, size);
}

void GFL_ArcToolSeek(ArcTool *handle, u32 offset) {
    FS_SeekFile(&handle->file, offset, FS_SEEK_SET);
}

u32 GFL_ArcToolGetDataMax(ArcTool *handle) {
    return handle->fileCount;
}
