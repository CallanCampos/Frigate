// The actual NitroSDK fs_archive.c/fs_file.c/default commands on synthetic archives: SystemInit's and DWC utility's
// FAT/FNT cache must retain caller-native pointers and avoid storage reads, while payload files still use storage.
#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/std.h"
#include "port/hal/fs/port_fs_tables.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(value)                                                                                                   \
    do {                                                                                                               \
        if (!(value)) {                                                                                                \
            fprintf(stderr, "cache fixture line %d: %s\n", __LINE__, #value);                                          \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)

typedef struct Fixture {
    FSArchive arc;
    u8 storage[320];
    u32 metadataReads;
    u32 payloadReads;
    FSResult failRead;
} Fixture;
static Fixture sFixtures[2];

// Only synchronous storage is exercised; unexpected thread waits are failures, not emulated scheduler behavior.
OSIntrMode OS_DisableInterrupts(void) {
    return 0;
}
OSIntrMode OS_RestoreInterrupts(OSIntrMode mode) {
    return mode;
}
void OS_WakeupThread(OSThreadQueue *queue) {
    (void)queue;
}
void OS_SleepThread(OSThreadQueue *queue) {
    (void)queue;
    CHECK(FALSE);
}
void OS_Terminate(void) {
    CHECK(FALSE);
}
void MI_CpuCopy8(const void *source, void *dest, u32 size) {
    memcpy(dest, source, size);
}
void MI_CpuFill8(void *dest, u8 value, u32 size) {
    memset(dest, value, size);
}
// Synthetic archives never occupy the DSi-only portion of the ROM.
BOOL FSi_IsDsiOnlyRomOffset(const FSArchive *arc, u32 position) {
    (void)arc;
    (void)position;
    return FALSE;
}
void MIi_CpuClear32(u32 value, void *dest, u32 size) {
    u32 i;
    for (i = 0; i < size / 4; ++i)
        ((u32 *)dest)[i] = value;
}
const u8 *CARD_GetRomHeader(void) {
    static CARDRomHeader header;
    return (const u8 *)&header;
}

static void Write32(u8 *dest, u32 value) {
    unsigned i;
    for (i = 0; i < 4; ++i)
        dest[i] = (u8)(value >> (8 * i));
}
static FSResult Read(FSArchive *arc, void *dest, u32 position, u32 length) {
    Fixture *fixture = arc == &sFixtures[0].arc ? &sFixtures[0] : &sFixtures[1];
    CHECK(position <= sizeof(fixture->storage) && length <= sizeof(fixture->storage) - position);
    if (position < 128)
        ++fixture->metadataReads;
    else
        ++fixture->payloadReads;
    if (fixture->failRead)
        return fixture->failRead;
    memcpy(dest, fixture->storage + position, length);
    return FS_RESULT_SUCCESS;
}
static void Init(Fixture *fixture, const char *name, u32 fat, u32 fnt) {
    memset(fixture, 0, sizeof(*fixture));
    Write32(fixture->storage + fat, 256);
    Write32(fixture->storage + fat + 4, 260);
    Write32(fixture->storage + fat + 8, 260);
    Write32(fixture->storage + fat + 12, 264);
    Write32(fixture->storage + fnt, 8);
    fixture->storage[fnt + 6] = 1;
    memcpy(fixture->storage + fnt + 8, "\003one\003two\0", 9);
    memcpy(fixture->storage + 256, "abcdEFGH", 8);
    FS_InitArchive(&fixture->arc);
    CHECK(FS_RegisterArchiveName(&fixture->arc, name, 3));
    CHECK(FS_LoadArchive(&fixture->arc, 0, fat, 16, fnt, 32, Read, NULL));
}
static void ReadFile(Fixture *fixture, const char *path, const char *expected) {
    FSFile file;
    char data[4];
    FSFileID id;
    CHECK(FS_ConvertPathToFileID(&id, path));
    CHECK(id.arc == &fixture->arc && id.file_id == 0);
    FS_InitFile(&file);
    CHECK(FS_OpenFileFast(&file, id));
    CHECK(FS_ReadFile(&file, data, 4) == 4);
    CHECK(memcmp(data, expected, 4) == 0);
    CHECK(FS_CloseFile(&file));
    CHECK(FS_OpenFile(&file, path));
    CHECK(FS_GetFileLength(&file) == 4);
    CHECK(FS_CloseFile(&file));
}
int main(void) {
    Fixture *a = &sFixtures[0], *b = &sFixtures[1];
    u8 *owner = malloc(256), *secondOwner = malloc(256), *otherOwner = malloc(256);
    u8 *memory, *second;
    u32 size, reads;
    u8 byte = 0xaa;
    CHECK(owner != NULL && secondOwner != NULL && otherOwner != NULL);
    memory = owner + 1;
    second = secondOwner + 3;
    // 64-bit supported hosts must actually exercise a pointer that cannot fit in a DS word.
    CHECK(sizeof(void *) <= 4 || (uintptr_t)owner > UINT32_MAX);
    Init(a, "aaa", 64, 96);
    Init(b, "bbb", 0, 16);
    ReadFile(a, "aaa:/one", "abcd");
    CHECK(a->metadataReads > 0 && a->payloadReads == 1);
    size = FS_LoadArchiveTables(&a->arc, NULL, 0);
    CHECK(size == 96 && a->arc.tables.load_mem == NULL);
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size - 1) == size);
    CHECK(a->arc.tables.load_mem == NULL);
    CHECK(FS_LoadArchiveTables(&a->arc, NULL, size) == size && a->arc.tables.load_mem == NULL);
    reads = a->metadataReads;
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size) == size);
    CHECK(a->metadataReads == reads + 2);
    CHECK(a->arc.tables.load_mem == memory && (a->arc.flag & FS_ARCHIVE_FLAG_TABLE_LOAD));
    CHECK(a->arc.tables.fat == 64 && a->arc.tables.fnt == 96);
    CHECK(memcmp((void *)(((uintptr_t)memory + 31) & ~(uintptr_t)31), a->storage + 64, 16) == 0);
    reads = a->metadataReads;
    ReadFile(a, "aaa:/one", "abcd");
    CHECK(a->metadataReads == reads && a->payloadReads == 2);
    CHECK(FS_LoadArchiveTables(&a->arc, NULL, 0) == size && a->arc.tables.load_mem == memory);
    CHECK(FS_LoadArchiveTables(&a->arc, NULL, size) == size && a->arc.tables.load_mem == memory);
    CHECK(FS_LoadArchiveTables(&b->arc, otherOwner, size) == size);
    ReadFile(b, "bbb:/one", "abcd");
    CHECK(b->metadataReads == 2 && a->metadataReads == reads);
    CHECK(FSi_HostReadArchiveTable(&b->arc, &byte, 0, 0) == FS_RESULT_SUCCESS);
    CHECK(FSi_HostReadArchiveTable(&b->arc, &byte, 16, 0) == FS_RESULT_SUCCESS); // FAT end == FNT start.
    CHECK(FSi_HostReadArchiveTable(&b->arc, &byte, 48, 0) == FS_RESULT_SUCCESS);
    CHECK(byte == 0xaa && b->metadataReads == 2);
    CHECK(FSi_HostReadArchiveTable(&b->arc, &byte, 49, 0) == FS_RESULT_FAILURE);
    // Backing metadata changes must not leak into the loaded snapshot.
    Write32(a->storage + 64, 260);
    Write32(a->storage + 68, 264);
    a->storage[105] = 'x';
    ReadFile(a, "aaa:/one", "abcd");
    CHECK(a->metadataReads == reads);
    CHECK(FSi_HostReadArchiveTable(&a->arc, &byte, 79, 2) == FS_RESULT_FAILURE && byte == 0xaa);
    CHECK(FSi_HostReadArchiveTable(&a->arc, &byte, UINT32_MAX, 2) == FS_RESULT_FAILURE);
    CHECK(FS_UnloadArchiveTables(&a->arc) == memory);
    CHECK(a->arc.tables.load_mem == NULL && !(a->arc.flag & FS_ARCHIVE_FLAG_TABLE_LOAD));
    CHECK(a->arc.tables.fat == 64 && a->arc.tables.fnt == 96);
    ReadFile(a, "aaa:/xne", "EFGH");
    CHECK(a->metadataReads > reads);
    CHECK(FSi_HostReadArchiveTable(&a->arc, &byte, 64, 1) == FS_RESULT_FAILURE);
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size) == size);
    CHECK(FS_LoadArchiveTables(&a->arc, second, size - 1) == size && a->arc.tables.load_mem == NULL);
    CHECK(FS_UnloadArchiveTables(&a->arc) == NULL);
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size) == size);
    CHECK(FS_LoadArchiveTables(&a->arc, second, size) == size && a->arc.tables.load_mem == second);
    CHECK(FS_UnloadArchiveTables(&a->arc) == second);
    CHECK(FS_UnloadArchiveTables(&b->arc) == otherOwner);
    b->arc.tables.fat_size = 0;
    CHECK(FS_LoadArchiveTables(&b->arc, otherOwner, size) == 64);
    CHECK(FSi_HostReadArchiveTable(&b->arc, &byte, 0, 0) == FS_RESULT_SUCCESS); // Empty FAT.
    CHECK(FSi_HostReadArchiveTable(&b->arc, &byte, 16, 0) == FS_RESULT_SUCCESS);
    CHECK(FSi_HostReadArchiveTable(&b->arc, &byte, 0, 1) == FS_RESULT_FAILURE);
    CHECK(FS_UnloadArchiveTables(&b->arc) == otherOwner);
    // Overlap and wrap cannot self-certify a cache; malformed reads cannot fall back to storage.
    a->arc.tables.fnt = a->arc.tables.fnt_bak = 72;
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size) == size && a->arc.tables.load_mem == NULL);
    a->arc.tables.load_mem = memory;
    CHECK(FSi_HostReadArchiveTable(&a->arc, &byte, 73, 1) == FS_RESULT_FAILURE);
    a->arc.tables.load_mem = NULL;
    a->arc.tables.fnt = a->arc.tables.fnt_bak = UINT32_MAX - 1;
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size) == size && a->arc.tables.load_mem == NULL);
    a->arc.tables.fnt = a->arc.tables.fnt_bak = 96;
    a->failRead = FS_RESULT_INVALID_PARAMETER;
    memset(memory, 0xaa, size);
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size) == size);
    CHECK(FSi_HostReadArchiveTable(&a->arc, &byte, 64, 1) == FS_RESULT_SUCCESS && byte == 0);
    CHECK(FS_UnloadArchiveTables(&a->arc) == memory);
    // FS_ReadFile only returns -1 for INVALID_PARAMETER. Other failure codes retain the requested length, so the
    // SDK loader does not zero that table. Preserve that original behavior rather than inventing retry semantics.
    a->failRead = FS_RESULT_FAILURE;
    memset(memory, 0x5a, size);
    CHECK(FS_LoadArchiveTables(&a->arc, memory, size) == size);
    CHECK(FSi_HostReadArchiveTable(&a->arc, &byte, 64, 1) == FS_RESULT_SUCCESS && byte == 0x5a);
    CHECK(FS_UnloadArchiveTables(&a->arc) == memory);
    free(otherOwner);
    free(owner);
    free(secondOwner);
    CHECK(FS_UnloadArchive(&a->arc));
    CHECK(FS_UnloadArchive(&b->arc));
    puts("archive table cache: all checks passed");
    return 0;
}
