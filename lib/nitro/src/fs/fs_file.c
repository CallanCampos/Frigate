// NitroSDK's file system (libfs): the functions of a file, which send its commands to its archive. The file name is a
// guess from NitroSDK's own layout (fs_file.c); no string in the ROM names it. The functions have NitroSDK's names
// where they are known.

#include "nitro/fs.h"
#include "nitro/os.h"

// The buffer that a path is normalized into, in the functions that look a file up and that open one. Their frames have
// 0x14 bytes after it that nothing touches, so it is declared that much larger than the normalizer's limit
#define FS_FILE_PATH_BUFFER_SIZE (FS_PATH_BUFFER_SIZE + 0x14)

void FS_InitFile(FSFile *file) {
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
    u32 interrupts = OS_DisableInterrupts();

    if (FS_IsBusy(file)) {
        file->stat |= FS_FILE_STATUS_CANCEL;
        file->arc->flag |= FS_ARCHIVE_FLAG_CANCELING;
    }
    OS_RestoreInterrupts(interrupts);
}

// NitroSDK's FS_ConvertPathToFileID: finds the archive and number of the file that a path names
BOOL FS_ConvertPathToFileID(FSFileID *fileId, const char *path) {
    BOOL result = FALSE;
    u32 baseDirId = 0;
    char normalized[FS_FILE_PATH_BUFFER_SIZE];
    FSArchive *arc = FSi_NormalizePath(path, &baseDirId, normalized);

    if (arc != NULL) {
        FSFile file;
        FSFindPathArgs args;

        FS_InitFile(&file);
        file.argument = &args;
        args.baseDirId = baseDirId;
        args.path = normalized;
        file.arc = arc;
        args.isDirectory = FALSE;
        if (FSi_SendCommand(&file, FS_COMMAND_FINDPATH, TRUE)) {
            fileId->arc = arc;
            fileId->file_id = args.targetId;
            result = TRUE;
        }
    }
    return result;
}

// NitroSDK's FS_OpenFileDirect: opens a file that is a range of an archive's data
BOOL FS_OpenFileDirect(FSFile *file, FSArchive *archive, u32 start, u32 end, int fileId) {
    FSOpenFileDirectArgs args;

    file->arc = archive;
    file->argument = &args;
    args.fileId = fileId;
    args.top = start;
    args.bottom = end;
    args.unk_0c = 0;
    return FSi_SendCommand(file, FS_COMMAND_OPENFILEDIRECT, TRUE);
}

// NitroSDK's FS_OpenFileFast: opens a file by archive and number
BOOL FS_OpenFileFast(FSFile *file, FSFileID fileId) {
    BOOL result = FALSE;

    if (fileId.arc != NULL) {
        FSOpenFileFastArgs args;

        file->arc = fileId.arc;
        file->argument = &args;
        args.fileId = fileId.file_id;
        args.unk_04 = 0;
        result = FSi_SendCommand(file, FS_COMMAND_OPENFILEFAST, TRUE);
    }
    return result;
}

// NitroSDK's FS_OpenFileEx: opens the file that a path names, with a mode (FS_FILEMODE_*)
BOOL FS_OpenFileEx(FSFile *file, const char *path, u32 mode) {
    BOOL result = FALSE;
    u32 baseDirId = 0;
    char normalized[FS_FILE_PATH_BUFFER_SIZE];
    FSArchive *arc = FSi_NormalizePath(path, &baseDirId, normalized);

    if (arc != NULL) {
        FSOpenFileArgs args;

        FS_InitFile(file);
        file->arc = arc;
        file->argument = &args;
        args.baseDirId = baseDirId;
        args.mode = mode;
        args.path = normalized;
        if (FSi_SendCommand(file, FS_COMMAND_OPENFILE, TRUE)) {
            result = TRUE;
        } else {
            file->arc = NULL;
        }
    }
    return result;
}

BOOL FS_CloseFile(FSFile *file) {
    return FSi_SendCommand(file, FS_COMMAND_CLOSEFILE, TRUE);
}

u32 FS_GetFileLength(FSFile *file) {
    u32 length = 0;

    if (!func_02071e74(file, &length)) {
        FSGetFileValueArgs args;

        file->argument = &args;
        args.value = 0;
        if (FSi_SendCommand(file, FS_COMMAND_GETFILELENGTH, TRUE)) {
            length = args.value;
        }
    }
    return length;
}

u32 FS_GetFilePosition(FSFile *file) {
    u32 position = 0;

    if (!func_02071e9c(file, &position)) {
        FSGetFileValueArgs args;

        file->argument = &args;
        args.value = 0;
        if (FSi_SendCommand(file, FS_COMMAND_GETFILEPOSITION, TRUE)) {
            position = args.value;
        }
    }
    return position;
}

BOOL FS_SeekFile(FSFile *file, s32 offset, int whence) {
    FSSeekFileArgs args;

    file->argument = &args;
    args.offset = offset;
    args.whence = whence;
    return FSi_SendCommand(file, FS_COMMAND_SEEKFILE, TRUE);
}

// NitroSDK's FS_ReadFile: reads up to size bytes, and returns how many it read, or -1 if the command's parameters were
// invalid
s32 FS_ReadFile(FSFile *file, void *dest, s32 size) {
    FSReadFileArgs args;
    s32 result = -1;

    file->argument = &args;
    args.buffer = dest;
    args.length = size;
    if (FSi_SendCommand(file, FS_COMMAND_READFILE, TRUE) || FS_GetResultCode(file) != FS_RESULT_INVALID_PARAMETER) {
        result = args.length;
    }
    return result;
}

s32 FS_WriteFile(FSFile *file, const void *src, s32 size) {
    FSWriteFileArgs args;
    s32 result = -1;

    file->argument = &args;
    args.buffer = src;
    args.length = size;
    if (FSi_SendCommand(file, FS_COMMAND_WRITEFILE, TRUE) || FS_GetResultCode(file) != FS_RESULT_INVALID_PARAMETER) {
        result = args.length;
    }
    return result;
}

BOOL FS_OpenFile(FSFile *file, const char *path) {
    return FS_OpenFileEx(file, path, FS_FILEMODE_R);
}

u32 FS_GetLength(FSFile *file) {
    return FS_GetFileLength(file);
}

// Makes a path's archive and directory the current ones: a thunk to NitroSDK's FS_ChangeDir, under a
// name that is not known
BOOL func_02070eb4(const char *path) {
    return FS_ChangeDir(path);
}
