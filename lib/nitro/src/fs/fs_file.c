// NitroSDK's file system (libfs): the functions of a file, which send its commands to its archive. The file name is a
// guess from NitroSDK's own layout (fs_file.c); no string in the ROM names it. The functions are under swan's names,
// with NitroSDK's in their comments.

#include "nitro/fs.h"
#include "nitro/os.h"

// The buffer that a path is normalized into, in the functions that look a file up and that open one. Their frames have
// 0x14 bytes after it that nothing touches, so it is declared that much larger than the normalizer's limit
#define FS_FILE_PATH_BUFFER_SIZE (FS_PATH_BUFFER_SIZE + 0x14)

// NitroSDK's FS_InitFile
void finit(FSFile *file) {
    file->arc = NULL;
    file->prop = NULL;
    file->next = NULL;
    OS_InitThreadQueue(&file->queue);
    file->stat = 0;
    file->stat |= FS_COMMAND_INVALID << FS_FILE_STATUS_COMMAND_SHIFT;
    file->argument = NULL;
    file->error = FS_RESULT_SUCCESS;
}

// NitroSDK's FS_CancelFile: asks the file's archive to stop the command running on it
void FS_CancelFile(FSFile *file) {
    u32 interrupts = CPU_IRQDisable();

    if (FS_IsBusy(file)) {
        file->stat |= FS_FILE_STATUS_CANCEL;
        file->arc->flag |= FS_ARCHIVE_FLAG_CANCELING;
    }
    CPU_SetIRQMask(interrupts);
}

// NitroSDK's FS_ConvertPathToFileID: finds the archive and number of the file that a path names
BOOL fs_resolve_file(FSFileID *fileId, const char *path) {
    BOOL result = FALSE;
    u32 baseDirId = 0;
    char normalized[FS_FILE_PATH_BUFFER_SIZE];
    FSArchive *arc = fs_normalize_path(path, &baseDirId, normalized);

    if (arc != NULL) {
        FSFile file;
        FSFindPathArgs args;

        finit(&file);
        file.argument = &args;
        args.baseDirId = baseDirId;
        args.path = normalized;
        file.arc = arc;
        args.isDirectory = FALSE;
        if (fs_call_syscmd(&file, FS_COMMAND_FINDPATH, TRUE)) {
            fileId->arc = arc;
            fileId->file_id = args.targetId;
            result = TRUE;
        }
    }
    return result;
}

// NitroSDK's FS_OpenFileDirect: opens a file that is a range of an archive's data
BOOL fopen_raw(FSFile *file, FSArchive *archive, u32 start, u32 end, int fileId) {
    FSOpenFileDirectArgs args;

    file->arc = archive;
    file->argument = &args;
    args.fileId = fileId;
    args.top = start;
    args.bottom = end;
    args.unk_0c = 0;
    return fs_call_syscmd(file, FS_COMMAND_OPENFILEDIRECT, TRUE);
}

// NitroSDK's FS_OpenFileFast: opens a file by archive and number
BOOL romfs_fopen_id(FSFile *file, FSFileID fileId) {
    BOOL result = FALSE;

    if (fileId.arc != NULL) {
        FSOpenFileFastArgs args;

        file->arc = fileId.arc;
        file->argument = &args;
        args.fileId = fileId.file_id;
        args.unk_04 = 0;
        result = fs_call_syscmd(file, FS_COMMAND_OPENFILEFAST, TRUE);
    }
    return result;
}

// NitroSDK's FS_OpenFileEx: opens the file that a path names, with a mode (FS_FILEMODE_*)
BOOL romfs_fopen_core(FSFile *file, const char *path, u32 mode) {
    BOOL result = FALSE;
    u32 baseDirId = 0;
    char normalized[FS_FILE_PATH_BUFFER_SIZE];
    FSArchive *arc = fs_normalize_path(path, &baseDirId, normalized);

    if (arc != NULL) {
        FSOpenFileArgs args;

        finit(file);
        file->arc = arc;
        file->argument = &args;
        args.baseDirId = baseDirId;
        args.mode = mode;
        args.path = normalized;
        if (fs_call_syscmd(file, FS_COMMAND_OPENFILE, TRUE)) {
            result = TRUE;
        } else {
            file->arc = NULL;
        }
    }
    return result;
}

// NitroSDK's FS_CloseFile
BOOL romfs_fclose(FSFile *file) {
    return fs_call_syscmd(file, FS_COMMAND_CLOSEFILE, TRUE);
}

// NitroSDK's FS_GetFileLength
u32 romfs_fgetsize(FSFile *file) {
    u32 length = 0;

    if (!func_02071e74(file, &length)) {
        FSGetFileValueArgs args;

        file->argument = &args;
        args.value = 0;
        if (fs_call_syscmd(file, FS_COMMAND_GETFILELENGTH, TRUE)) {
            length = args.value;
        }
    }
    return length;
}

// NitroSDK's FS_GetFilePosition
u32 romfs_ftell(FSFile *file) {
    u32 position = 0;

    if (!func_02071e9c(file, &position)) {
        FSGetFileValueArgs args;

        file->argument = &args;
        args.value = 0;
        if (fs_call_syscmd(file, FS_COMMAND_GETFILEPOSITION, TRUE)) {
            position = args.value;
        }
    }
    return position;
}

// NitroSDK's FS_SeekFile
BOOL romfs_fseek(FSFile *file, s32 offset, int whence) {
    FSSeekFileArgs args;

    file->argument = &args;
    args.offset = offset;
    args.whence = whence;
    return fs_call_syscmd(file, FS_COMMAND_SEEKFILE, TRUE);
}

// NitroSDK's FS_ReadFile: reads up to size bytes, and returns how many it read, or -1 if the command's parameters were
// invalid
s32 romfs_fread(FSFile *file, void *dest, s32 size) {
    FSReadFileArgs args;
    s32 result = -1;

    file->argument = &args;
    args.buffer = dest;
    args.length = size;
    if (fs_call_syscmd(file, FS_COMMAND_READFILE, TRUE) || FS_GetResultCode(file) != FS_RESULT_INVALID_PARAMETER) {
        result = args.length;
    }
    return result;
}

// NitroSDK's FS_WriteFile
s32 func_02070e70(FSFile *file, const void *src, s32 size) {
    FSWriteFileArgs args;
    s32 result = -1;

    file->argument = &args;
    args.buffer = src;
    args.length = size;
    if (fs_call_syscmd(file, FS_COMMAND_WRITEFILE, TRUE) || FS_GetResultCode(file) != FS_RESULT_INVALID_PARAMETER) {
        result = args.length;
    }
    return result;
}

// NitroSDK's FS_OpenFile
BOOL romfs_fopen(FSFile *file, const char *path) {
    return romfs_fopen_core(file, path, FS_FILEMODE_R);
}

// NitroSDK's FS_GetLength
u32 GetFileSize(FSFile *file) {
    return romfs_fgetsize(file);
}

// Makes a path's archive and directory the current ones: a thunk to NitroSDK's FS_ChangeDir (func_02070680), under a
// name that is not known
BOOL func_02070eb4(const char *path) {
    return func_02070680(path);
}
