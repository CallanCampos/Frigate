#ifndef POKEBW2_NITRO_FS_H
#define POKEBW2_NITRO_FS_H

#include "types.h"
#include "nitro/mi.h"

// NitroSDK's file system, under swan's names: FS_InitFile, FS_OpenFile, FS_GetLength, FS_ReadFile, FS_CloseFile,
// FS_SeekFile, and FS_CreateFileFromMemory, which opens a file over memory

// A file system's archive, such as "rom". Only its name, the first field, is known
typedef struct FSArchive {
    char name[4];
} FSArchive;

typedef struct {
    u8 unk_00[8];
    FSArchive *arc;
    u8 unk_0c[0x3c];
} FSFile;

void finit(FSFile *file);
BOOL romfs_fopen(FSFile *file, const char *path);
u32 GetFileSize(FSFile *file);
s32 romfs_fread(FSFile *file, void *dest, s32 size);
BOOL romfs_fclose(FSFile *file);
BOOL romfs_fseek(FSFile *file, s32 offset, int whence);
BOOL extfs_fopen(FSFile *file, const void *data, u32 size);
// The position in the file
u32 romfs_ftell(FSFile *file);
// Opens a file that is a range of an archive's data, from start to end
BOOL fopen_raw(FSFile *file, FSArchive *archive, u32 start, u32 end, int fileId);
// The archive of a name of the given length, or NULL
FSArchive *func_0207062c(const char *name, int nameLength);
// A field of the file: where its data starts in its archive
u32 func_02072010(const FSFile *file);

#define FS_SEEK_SET 0

// A file by archive and number: NitroSDK's FS_ConvertPathToFileID and FS_OpenFileFast, and FS_OpenFileEx (which
// opens with a mode), under swan's names

typedef struct FSFileID {
    FSArchive *arc;
    u32 file_id;
} FSFileID;

#define FS_FILEMODE_R 1

BOOL fs_resolve_file(FSFileID *fileId, const char *path);
BOOL romfs_fopen_id(FSFile *file, FSFileID fileId);
BOOL romfs_fopen_core(FSFile *file, const char *path, u32 mode);
// NitroSDK's FS_CancelFile: asks the file's archive to stop the command running on it
void FS_CancelFile(FSFile *file);

// Overlays: NitroSDK's FS_LoadOverlayInfo, FS_LoadOverlay, FS_UnloadOverlay and FS_SetDefaultDMA, under swan's names.
// The target is the processor, MI_PROCESSOR_ARM9 (nitro/mi.h)
#define FS_DMA_NOT_USE 0xffffffff

typedef struct {
    u32 id;
    u8 *ramAddress;
    u32 ramSize;
    u32 bssSize;
    u8 rest[0x1c];
} FSOverlayInfo;

BOOL sys_read_overlay_header(FSOverlayInfo *info, int target, u32 id);
BOOL sys_load_overlay(int target, u32 id);
BOOL sys_unload_overlay(int target, u32 id);
u32 fs_set_dma_id(u32 dma);

#endif // POKEBW2_NITRO_FS_H
