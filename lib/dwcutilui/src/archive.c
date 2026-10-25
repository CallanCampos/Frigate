#include "archive.h"
#include "deque.h"
#include "dwc/ext.h"
#include "dwc_util_heap.h"
#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"

typedef struct DWCUtilResource {
    void *tables;
    // One of these is popped for each file read and pushed back when the file's data is freed
    void *openFiles[32];
    DWCUtilStack *openFileStack;
    FSArchive archive;
    u16 lockId;
} DWCUtilResource;

static const char sArchiveName[] = "dwc";
static DWCUtilResource *sResource;

static FSResult DWCUtilRomResource_ArchiveProc(FSFile *file, FSCommandType command);
static FSResult DWCUtilRomResource_ReadArchive(FSArchive *arc, void *buffer, u32 position, u32 length);
static void cbReadRom(void *arc);
static FSResult DWCUtilRomResource_WriteArchive(FSArchive *arc, const void *buffer, u32 position, u32 length);

void DWCUtilRomResource_Init(void) {
    FSFile file;
    char path[0x80];
    u32 fnt[2];
    u32 fat[2];
    u32 base;
    u32 size;

    sResource = DWCi_HEAPlAllocEx(sizeof(DWCUtilResource), 4);
    FS_InitFile(&file);
    if (!FS_OpenFile(&file, "rom:/dwc/utility.bin")) {
        OS_Terminate();
    }
    sResource->lockId = OS_GetLockID();
    base = FS_GetFileImageTop(&file);
    FS_ReadFile(&file, fnt, sizeof(fnt));
    FS_ReadFile(&file, fat, sizeof(fat));
    FS_CloseFile(&file);
    FS_InitArchive(&sResource->archive);
    if (!FS_RegisterArchiveName(&sResource->archive, sArchiveName, 3)) {
        OS_Terminate();
    }
    FS_SetArchiveProc(&sResource->archive, DWCUtilRomResource_ArchiveProc,
                      (1 << FS_COMMAND_WRITEFILE) | (1 << FS_COMMAND_ACTIVATE) | (1 << FS_COMMAND_IDLE));
    if (!FS_LoadArchive(&sResource->archive, base, fat[0], fat[1], fnt[0], fnt[1], DWCUtilRomResource_ReadArchive,
                        DWCUtilRomResource_WriteArchive)) {
        OS_Terminate();
    }
    size = FS_LoadArchiveTables(&sResource->archive, NULL, 0);
    sResource->tables = DWCi_HEAPlAllocEx(size, 4);
    FS_LoadArchiveTables(&sResource->archive, sResource->tables, size);
    sResource->openFileStack = DWCi_QUElFormEx(32, (u8 *)sResource->openFiles, sizeof(void *));
    OS_SPrintf(path, "%s:/", sArchiveName);
    FS_ChangeDir(path);
}

void DWCi_ARClEnd(void) {
    FS_ChangeDir("rom:/");
    FS_UnloadArchiveTables(&sResource->archive);
    FS_UnloadArchive(&sResource->archive);
    FS_ReleaseArchiveName(&sResource->archive);
    OS_ReleaseLockID(sResource->lockId);
    sResource->lockId = 0;
    DWCi_HEAPlFree_(&sResource->tables);
    sResource->tables = NULL;
    DWCi_HEAPlFree_(&sResource);
}

// The archive's hook: the card is locked while the archive is active, and it can't be written
static FSResult DWCUtilRomResource_ArchiveProc(FSFile *file, FSCommandType command) {
    switch (command) {
    case FS_COMMAND_ACTIVATE:
        CARD_LockRom(sResource->lockId);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_IDLE:
        CARD_UnlockRom(sResource->lockId);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_WRITEFILE:
        return FS_RESULT_UNSUPPORTED;
    default:
        return FS_RESULT_PROC_DEFAULT;
    }
}

static FSResult DWCUtilRomResource_ReadArchive(FSArchive *arc, void *buffer, u32 position, u32 length) {
    CARD_ReadRomAsync((u32)-1, (const void *)(position + FS_GetArchiveBase(arc)), buffer, length, cbReadRom, arc);
    return FS_RESULT_PROC_ASYNC;
}

static void cbReadRom(void *arc) {
    FS_NotifyArchiveAsyncEnd(arc, FS_RESULT_SUCCESS);
}

static FSResult DWCUtilRomResource_WriteArchive(FSArchive *arc, const void *buffer, u32 position, u32 length) {
    return FS_RESULT_FAILURE;
}

void *DWCUtilRomResource_LoadFile(const char *path, u32 *size, int align) {
    FSFile file;
    void *data;
    u32 length;
    int alignment;

    DWCi_QUElPopFront(sResource->openFileStack);
    FS_InitFile(&file);
    if (!FS_OpenFile(&file, path)) {
        OS_Terminate();
    }
    length = FS_GetLength(&file);
    if (size != NULL) {
        *size = length;
    }
    // A compressed file is read to the end of the heap, as it is freed once it is decompressed
    if (cmpBack(path, ".l", 2)) {
        alignment = -4;
    } else {
        alignment = align;
    }
    data = DWCi_HEAPlAllocEx(length, alignment);
    FS_ReadFile(&file, data, length);
    FS_CloseFile(&file);
    if (alignment > 0) {
        return data;
    }
    {
        void *out;

        length = *(u32 *)data >> 8;
        if (size != NULL) {
            *size = length;
        }
        out = DWCi_HEAPlAllocEx(length, align);
        MI_UncompressLZ8(data, out);
        DWCi_HEAPlFree_(&data);
        return out;
    }
}

void DWCi_ARClRelease(void *data) {
    DWCi_HEAPlFree_(&data);
    DWCi_QUElPushBack(sResource->openFileStack, data);
}

BOOL cmpBack(const char *str, const char *suffix, int length) {
    int strLength = strlen(str);
    int suffixLength = strlen(suffix);

    if (strLength < length || suffixLength < length) {
        return FALSE;
    }
    if (memcmp(str + (strLength - length), suffix + (suffixLength - length), length) == 0) {
        return TRUE;
    }
    return FALSE;
}
