// The actual arc_tool.c archive seeker on a fabricated NARC, with bounded file-system callbacks. Checks its FATB,
// FNTB and FIMG offsets, whole-file/explicit-size reads and assertion path without a ROM or game initialization.
#include "src/gfl/arc_tool.c"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char sArchive[80];
static const unsigned char *sData;
static size_t sSize;
static size_t sPosition;

static void Check(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "archive fixture: %s\n", message);
        exit(1);
    }
}

void FS_InitFile(FSFile *file) {
    memset(file, 0, sizeof(*file));
}
BOOL FS_OpenFile(FSFile *file, const char *path) {
    (void)file;
    (void)path;
    sPosition = 0;
    return TRUE;
}
BOOL FS_CloseFile(FSFile *file) {
    (void)file;
    return TRUE;
}
BOOL FS_CreateFileFromMemory(FSFile *file, void *data, u32 size) {
    (void)file;
    sData = data;
    sSize = size;
    sPosition = 0;
    return TRUE;
}
BOOL FS_SeekFile(FSFile *file, s32 offset, FSSeekFileMode mode) {
    (void)file;
    Check(mode == FS_SEEK_SET && offset >= 0 && (size_t)offset <= sSize, "seek outside fixture");
    sPosition = (size_t)offset;
    return TRUE;
}
s32 FS_ReadFile(FSFile *file, void *dest, s32 size) {
    (void)file;
    Check(size >= 0 && sPosition + (size_t)size <= sSize, "read outside fixture");
    memcpy(dest, sData + sPosition, (size_t)size);
    sPosition += (size_t)size;
    return size;
}
void *GFL_HeapAllocate(HeapID heap, u32 size, BOOL clear, const char *file, u16 line) {
    (void)heap;
    (void)file;
    (void)line;
    return clear ? calloc(1, size) : malloc(size);
}
void GFL_HeapFree(void *value) {
    free(value);
}
void GFL_DebugAssertFailEx(const char *file, u32 line, const char *format, ...) {
    va_list args;
    (void)file;
    (void)line;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    exit(70);
}
static void Write32(size_t offset, u32 value) {
    unsigned i;
    for (i = 0; i < 4; i++)
        sArchive[offset + i] = (unsigned char)(value >> (8 * i));
}
int main(int argc, char **argv) {
    FSFile file;
    char bytes[6] = { 0 };
    (void)argv;
    Write32(0xc, 0x10); // NARC header size; the seeker reads its low halfword.
    Write32(0x14, 28);  // FATB size: header and two entries.
    Write32(0x18, 2);   // File count, also read as a halfword.
    Write32(0x1c, 0);
    Write32(0x20, 4);
    Write32(0x24, 4);
    Write32(0x28, 9);
    Write32(0x30, 16); // FNTB size; FIMG payload begins at 0x44.
    memcpy(sArchive + 0x44, "abcdefghi", 9);
    FS_InitFile(&file);
    FS_CreateFileFromMemory(&file, sArchive, sizeof(sArchive));
    if (argc > 1) {
        ArchiveMoveImageTop(&file, 2, 0, 0);
        Check(0, "invalid file ID did not assert");
    }
    Check(ArchiveMoveImageTop(&file, 0, 0, 0) == 4 && sPosition == 0x44, "first file length/offset");
    FS_ReadFile(&file, bytes, 4);
    Check(memcmp(bytes, "abcd", 4) == 0, "first file bytes");
    Check(ArchiveMoveImageTop(&file, 1, 1, 3) == 3 && sPosition == 0x49, "range length/offset");
    FS_ReadFile(&file, bytes, 3);
    Check(memcmp(bytes, "fgh", 3) == 0, "range bytes");
    Check(ArchiveMoveImageTop(&file, 1, 2, 0) == 5 && sPosition == 0x4a, "offset keeps original whole length");
    Write32(0x28, 4);
    Check(ArchiveMoveImageTop(&file, 1, 0, 0) == 0 && sPosition == 0x48, "empty file");
    puts("archive seek: every check passed");
    return 0;
}
