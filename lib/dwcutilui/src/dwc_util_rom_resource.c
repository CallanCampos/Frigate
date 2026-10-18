#include "dwc_util_rom_resource.h"
#include "dwc/ext.h"
#include "dwc_util_cell.h"
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

static FSResult func_ov182_021bb2f8(FSFile *file, FSCommandType command);
static FSResult func_ov182_021bb338(FSArchive *arc, void *buffer, u32 position, u32 length);
static void func_ov182_021bb36c(void *arc);
static FSResult func_ov182_021bb378(FSArchive *arc, const void *buffer, u32 position, u32 length);

void func_ov182_021bb18c(void) {
    FSFile file;
    char path[0x80];
    u32 fnt[2];
    u32 fat[2];
    u32 base;
    u32 size;

    sResource = func_ov182_021bc4d8(sizeof(DWCUtilResource), 4);
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
    FS_SetArchiveProc(&sResource->archive, func_ov182_021bb2f8,
                      (1 << FS_COMMAND_WRITEFILE) | (1 << FS_COMMAND_ACTIVATE) | (1 << FS_COMMAND_IDLE));
    if (!FS_LoadArchive(&sResource->archive, base, fat[0], fat[1], fnt[0], fnt[1], func_ov182_021bb338,
                        func_ov182_021bb378)) {
        OS_Terminate();
    }
    size = FS_LoadArchiveTables(&sResource->archive, NULL, 0);
    sResource->tables = func_ov182_021bc4d8(size, 4);
    FS_LoadArchiveTables(&sResource->archive, sResource->tables, size);
    sResource->openFileStack = func_ov182_021bb9c8(32, (u8 *)sResource->openFiles, sizeof(void *));
    OS_SPrintf(path, "%s:/", sArchiveName);
    FS_ChangeDir(path);
}

void func_ov182_021bb2a4(void) {
    FS_ChangeDir("rom:/");
    FS_UnloadArchiveTables(&sResource->archive);
    FS_UnloadArchive(&sResource->archive);
    FS_ReleaseArchiveName(&sResource->archive);
    OS_ReleaseLockID(sResource->lockId);
    sResource->lockId = 0;
    func_ov182_021bc4f0(&sResource->tables);
    sResource->tables = NULL;
    func_ov182_021bc4f0(&sResource);
}

// The archive's hook: the card is locked while the archive is active, and it can't be written
static FSResult func_ov182_021bb2f8(FSFile *file, FSCommandType command) {
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

static FSResult func_ov182_021bb338(FSArchive *arc, void *buffer, u32 position, u32 length) {
    CARDi_ReadRom((u32)-1, position + FS_GetArchiveBase(arc), buffer, length, func_ov182_021bb36c, arc, TRUE);
    return FS_RESULT_PROC_ASYNC;
}

static void func_ov182_021bb36c(void *arc) {
    FS_NotifyArchiveAsyncEnd(arc, FS_RESULT_SUCCESS);
}

static FSResult func_ov182_021bb378(FSArchive *arc, const void *buffer, u32 position, u32 length) {
    return FS_RESULT_FAILURE;
}

void *func_ov182_021bb37c(const char *path, u32 *size, int align) {
    FSFile file;
    void *data;
    u32 length;
    int alignment;

    func_ov182_021bba38(sResource->openFileStack);
    FS_InitFile(&file);
    if (!FS_OpenFile(&file, path)) {
        OS_Terminate();
    }
    length = FS_GetLength(&file);
    if (size != NULL) {
        *size = length;
    }
    // A compressed file is read to the end of the heap, as it is freed once it is decompressed
    if (func_ov182_021bb444(path, ".l", 2)) {
        alignment = -4;
    } else {
        alignment = align;
    }
    data = func_ov182_021bc4d8(length, alignment);
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
        out = func_ov182_021bc4d8(length, align);
        MI_UncompressLZ8(data, out);
        func_ov182_021bc4f0(&data);
        return out;
    }
}

void func_ov182_021bb420(void *data) {
    func_ov182_021bc4f0(&data);
    func_ov182_021bba00(sResource->openFileStack, data);
}

BOOL func_ov182_021bb444(const char *str, const char *suffix, int length) {
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
