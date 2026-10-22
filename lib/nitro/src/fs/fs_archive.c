// NitroSDK's file system (libfs): the archives, the queue of the commands that their files send, and the paths that
// name a file in an archive ("rom:/dir/file"). The file name is a guess from NitroSDK's own layout (fs_archive.c); no
// string in the ROM names it. The functions have NitroSDK's names where they are known.

#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/std.h"

// Where the archives are registered, and the archive and directory that a relative path starts in
static struct {
    FSArchive *list;
    FSDirPos current;
} sArchives;

// The names of more than 3 characters, one slot each. A slot is free when it starts with a NUL
#define FS_ARCHIVE_NAME_SLOT_COUNT 16
static char sArchiveNames[FS_ARCHIVE_NAME_SLOT_COUNT][FS_ARCHIVE_NAME_LEN_MAX + 1];

// The path of the current directory inside the current archive
static char sCurrentPath[FS_PATH_BUFFER_SIZE];

static int FSi_DecrementSjisPositionToSlash(const char *path, int position);
static int FSi_TrimSjisTrailingSlash(char *path);

// The command of a file
static inline FSCommandType FSi_GetCommand(const FSFile *file) {
    return (FSCommandType)(u8)(file->stat >> FS_FILE_STATUS_COMMAND_SHIFT);
}

// The bit of a command in the mask of the events, counted from the first of them
#define FSi_EVENT_BIT(command) (1 << ((command) - FS_COMMAND_ACTIVATE))

// NitroSDK's FSi_IsEventCommand: whether a command is an event
static BOOL FSi_IsEventCommand(u32 command) {
    BOOL isEvent = FALSE;
    u32 index = command - FS_COMMAND_ACTIVATE;

    if (index <= FS_COMMAND_INVALID - FS_COMMAND_ACTIVATE) {
        u32 events = FSi_EVENT_BIT(FS_COMMAND_ACTIVATE) | FSi_EVENT_BIT(FS_COMMAND_IDLE) |
                     FSi_EVENT_BIT(FS_COMMAND_SUSPEND) | FSi_EVENT_BIT(FS_COMMAND_RESUME) |
                     FSi_EVENT_BIT(FS_COMMAND_MOUNT) | FSi_EVENT_BIT(FS_COMMAND_UNMOUNT) |
                     FSi_EVENT_BIT(FS_COMMAND_INVALID);

        if (events & (1 << index)) {
            isEvent = TRUE;
        }
    }
    return isEvent;
}

// NitroSDK's FSi_EndCommand: ends a file's command with a result, taking the file out of its archive's queue and waking
// the threads that wait for it
static void FSi_EndCommand(FSFile *file, FSResult result) {
    FSArchive *arc;
    FSCommandType command;
    OSIntrMode interrupts = OS_DisableInterrupts();

    arc = file->arc;
    if (arc != NULL) {
        FSFile *current;
        FSFile **link = &arc->list;

        for (current = arc->list; current != NULL; current = current->next) {
            if (current == file) {
                *link = file->next;
                break;
            }
            link = &current->next;
        }
        file->next = NULL;
    }
    command = FSi_GetCommand(file);
    if (!FSi_IsEventCommand(command) && arc != NULL) {
        arc->command = command;
        arc->result = result;
    }
    file->error = result;
    file->stat &= ~FS_FILE_STATUS_PROGRESS_MASK;
    OS_WakeupThread(&file->queue);
    OS_RestoreInterrupts(interrupts);
}

// NitroSDK's FSi_WaitForArchiveCompletion: if a command went on apart from the thread (FS_RESULT_PROC_ASYNC), waits
// until the archive ends it, and returns its result
FSResult FSi_WaitForArchiveCompletion(FSFile *file, FSResult result) {
    if (result == FS_RESULT_PROC_ASYNC) {
        OSIntrMode interrupts = OS_DisableInterrupts();

        while (!(file->stat & FS_FILE_STATUS_ENDED)) {
            OS_SleepThread(&file->queue);
        }
        OS_RestoreInterrupts(interrupts);
        file->stat &= ~FS_FILE_STATUS_ENDED;
        result = file->error;
    }
    return result;
}

// NitroSDK's FSi_InvokeCommand: runs a file's command through its archive's interface. The events return at once; the
// other commands end here, or wait for the archive to end them if a thread is blocked on the file
static FSResult FSi_InvokeCommand(FSFile *file, u32 command) {
    FSArchive *arc = file->arc;
    const FSArchiveInterface *vtbl = arc->vtbl;
    FSResult result;

    if (command >= FS_COMMAND_INVALID) {
        result = FS_RESULT_UNSUPPORTED;
    } else if (vtbl->command[command] == NULL) {
        result = FS_RESULT_UNSUPPORTED;
    } else {
        switch (command) {
        case FS_COMMAND_READFILE: {
            FSReadFileArgs *args = file->argument;

            result = vtbl->readFile(arc, file, args->buffer, &args->length);
            break;
        }
        case FS_COMMAND_WRITEFILE: {
            FSWriteFileArgs *args = file->argument;

            result = vtbl->writeFile(arc, file, args->buffer, &args->length);
            break;
        }
        case FS_COMMAND_SEEKDIR: {
            u32 *args = file->argument;

            result = vtbl->seekDirectory(arc, file, args[0], args[1]);
            break;
        }
        case FS_COMMAND_READDIR: {
            void **args = file->argument;

            result = vtbl->readDirectory(arc, file, args[0]);
            break;
        }
        case FS_COMMAND_FINDPATH: {
            FSFindPathArgs *args = file->argument;

            result = vtbl->findPath(arc, args->baseDirId, args->path, &args->targetId, args->isDirectory);
            break;
        }
        case FS_COMMAND_GETPATH: {
            FSGetPathArgs *args = file->argument;

            result = vtbl->getPath(arc, file, args->unk_00, args->buffer, &args->length);
            break;
        }
        case FS_COMMAND_OPENFILEFAST: {
            FSOpenFileFastArgs *args = file->argument;

            result = vtbl->openFileFast(arc, file, args->fileId, args->unk_04);
            break;
        }
        case FS_COMMAND_OPENFILEDIRECT: {
            FSOpenFileDirectArgs *args = file->argument;

            result = vtbl->openFileDirect(arc, file, args->top, args->bottom, &args->fileId);
            break;
        }
        case FS_COMMAND_CLOSEFILE:
            result = vtbl->closeFile(arc, file);
            break;
        case FS_COMMAND_ACTIVATE:
            vtbl->activate(arc);
            return FS_RESULT_SUCCESS;
        case FS_COMMAND_IDLE:
            vtbl->idle(arc);
            return FS_RESULT_SUCCESS;
        case FS_COMMAND_SUSPEND:
            vtbl->suspend(arc);
            return FS_RESULT_SUCCESS;
        case FS_COMMAND_RESUME:
            vtbl->resume(arc);
            return FS_RESULT_SUCCESS;
        case FS_COMMAND_OPENFILE: {
            FSOpenFileArgs *args = file->argument;

            result = vtbl->openFile(arc, file, args->baseDirId, args->path, args->mode);
            break;
        }
        case FS_COMMAND_SEEKFILE: {
            FSSeekFileArgs *args = file->argument;

            result = vtbl->seekFile(arc, file, &args->offset, args->whence);
            break;
        }
        case FS_COMMAND_GETFILELENGTH: {
            FSGetFileValueArgs *args = file->argument;

            result = vtbl->getFileLength(arc, file, &args->value);
            break;
        }
        case FS_COMMAND_GETFILEPOSITION: {
            FSGetFileValueArgs *args = file->argument;

            result = vtbl->getFilePosition(arc, file, &args->value);
            break;
        }
        case FS_COMMAND_MOUNT:
            vtbl->mount(arc);
            return FS_RESULT_SUCCESS;
        case FS_COMMAND_UNMOUNT:
            vtbl->unmount(arc);
            return FS_RESULT_SUCCESS;
        case FS_COMMAND_GETARCHIVECAPS:
            result = vtbl->getArchiveCaps(arc, file->argument);
            break;
        case FS_COMMAND_CREATEFILE: {
            u32 *args = file->argument;

            result = vtbl->createFile(arc, args[0], args[1], args[2]);
            break;
        }
        case FS_COMMAND_DELETEFILE: {
            u32 *args = file->argument;

            result = vtbl->deleteFile(arc, args[0], args[1]);
            break;
        }
        case FS_COMMAND_RENAMEFILE: {
            u32 *args = file->argument;

            result = vtbl->renameFile(arc, args[0], args[1], args[2], args[3]);
            break;
        }
        case FS_COMMAND_GETPATHINFO: {
            FSGetPathInfoArgs *args = file->argument;

            result = vtbl->getPathInfo(arc, args->baseDirId, args->path, args->info);
            break;
        }
        case FS_COMMAND_SETPATHINFO: {
            u32 *args = file->argument;

            result = vtbl->setPathInfo(arc, args[0], args[1], args[2]);
            break;
        }
        case FS_COMMAND_CREATEDIRECTORY: {
            u32 *args = file->argument;

            result = vtbl->createDirectory(arc, args[0], args[1], args[2]);
            break;
        }
        case FS_COMMAND_DELETEDIRECTORY: {
            u32 *args = file->argument;

            result = vtbl->deleteDirectory(arc, args[0], args[1]);
            break;
        }
        case FS_COMMAND_RENAMEDIRECTORY: {
            u32 *args = file->argument;

            result = vtbl->renameDirectory(arc, args[0], args[1], args[2], args[3]);
            break;
        }
        case FS_COMMAND_GETARCHIVERESOURCE: {
            FSGetArchiveResourceArgs *args = file->argument;

            result = vtbl->getArchiveResource(arc, args->resource);
            break;
        }
        case FS_COMMAND_FLUSHFILE:
            result = vtbl->flushFile(arc, file);
            break;
        case FS_COMMAND_SETFILELENGTH: {
            u32 *args = file->argument;

            result = vtbl->setFileLength(arc, file, args[0]);
            break;
        }
        case FS_COMMAND_OPENDIRECTORY: {
            FSOpenDirectoryArgs *args = file->argument;

            result = vtbl->openDirectory(arc, file, args->baseDirId, args->path, args->unk_08);
            break;
        }
        case FS_COMMAND_CLOSEDIRECTORY:
            result = vtbl->closeDirectory(arc, file);
            break;
        case FS_COMMAND_SETSEEKCACHE: {
            u32 *args = file->argument;

            result = vtbl->setSeekCache(arc, file, args[0], args[1]);
            break;
        }
        default:
            result = FS_RESULT_UNSUPPORTED;
            break;
        }
    }
    if (!FSi_IsEventCommand(command)) {
        if (file->stat & FS_FILE_STATUS_BLOCKING) {
            result = FSi_WaitForArchiveCompletion(file, result);
        } else if (result != FS_RESULT_PROC_ASYNC) {
            FSi_EndCommand(file, result);
        }
    }
    return result;
}

// NitroSDK's FSi_NextCommand: ends the commands that were canceled, activates the archive if it is to start one, and
// returns the file whose command the caller has to run. A blocked thread runs its own, so none is returned for it
static FSFile *FSi_NextCommand(FSArchive *arc, BOOL start) {
    FSFile *next = NULL;
    OSIntrMode interrupts;
    OSIntrMode activateInterrupts;
    OSIntrMode runInterrupts;
    BOOL activate;

    interrupts = OS_DisableInterrupts();
    if (arc->flag & FS_ARCHIVE_FLAG_CANCELING) {
        FSFile *file = arc->list;

        arc->flag &= ~FS_ARCHIVE_FLAG_CANCELING;
        while (file != NULL) {
            FSFile *following = file->next;

            if (FS_IsCanceling(file)) {
                FSi_EndCommand(file, FS_RESULT_CANCELED);
                if (following == NULL) {
                    following = arc->list;
                }
            }
            file = following;
        }
    }
    OS_RestoreInterrupts(interrupts);

    activateInterrupts = OS_DisableInterrupts();
    if (!(arc->flag & FS_ARCHIVE_FLAG_SUSPENDING) && !(arc->flag & FS_ARCHIVE_FLAG_SUSPEND) && arc->list != NULL) {
        activate = start && !(arc->flag & FS_ARCHIVE_FLAG_RUNNING);
        if (activate) {
            arc->flag |= FS_ARCHIVE_FLAG_RUNNING;
        }
        OS_RestoreInterrupts(activateInterrupts);
        if (activate) {
            FSi_InvokeCommand(arc->list, FS_COMMAND_ACTIVATE);
        }
        runInterrupts = OS_DisableInterrupts();
        if (start || activate) {
            next = arc->list;
            next->stat |= FS_FILE_STATUS_OPERATING;
        }
        if (start && (next->stat & FS_FILE_STATUS_BLOCKING)) {
            OS_WakeupThread(&next->queue);
            next = NULL;
        }
        OS_RestoreInterrupts(runInterrupts);
    } else {
        if (start) {
            if (arc->flag & FS_ARCHIVE_FLAG_RUNNING) {
                FSFile file;

                FS_InitFile(&file);
                file.arc = arc;
                arc->flag &= ~FS_ARCHIVE_FLAG_RUNNING;
                FSi_InvokeCommand(&file, FS_COMMAND_IDLE);
            }
            if (arc->flag & FS_ARCHIVE_FLAG_SUSPENDING) {
                arc->flag = (arc->flag & ~FS_ARCHIVE_FLAG_SUSPENDING) | FS_ARCHIVE_FLAG_SUSPEND;
                OS_WakeupThread(&arc->queue);
            }
        }
        OS_RestoreInterrupts(activateInterrupts);
    }
    return next;
}

// NitroSDK's FSi_ExecuteAsyncCommand: runs the commands of an archive from the one of a file, until one goes on apart
// from the thread
static void FSi_ExecuteAsyncCommand(FSFile *file) {
    FSArchive *arc = file->arc;

    while (file != NULL) {
        OSIntrMode interrupts = OS_DisableInterrupts();

        file->stat |= FS_FILE_STATUS_OPERATING;
        if (file->stat & FS_FILE_STATUS_BLOCKING) {
            OS_WakeupThread(&file->queue);
            file = NULL;
        }
        OS_RestoreInterrupts(interrupts);
        if (file == NULL) {
            break;
        }
        if (FSi_InvokeCommand(file, FSi_GetCommand(file)) == FS_RESULT_PROC_ASYNC) {
            break;
        }
        file = FSi_NextCommand(arc, TRUE);
    }
}

// NitroSDK's FSi_ExecuteSyncCommand: waits for a file's turn, runs its command in the calling thread, and goes on with
// the next ones
static void FSi_ExecuteSyncCommand(FSFile *file) {
    OSIntrMode interrupts = OS_DisableInterrupts();

    while (!(file->stat & FS_FILE_STATUS_OPERATING) && (file->stat & FS_FILE_STATUS_BUSY)) {
        OS_SleepThread(&file->queue);
    }
    OS_RestoreInterrupts(interrupts);
    if (file->stat & FS_FILE_STATUS_OPERATING) {
        FSArchive *arc = file->arc;
        FSFile *next;

        FSi_EndCommand(file, FSi_InvokeCommand(file, FSi_GetCommand(file)));
        next = FSi_NextCommand(arc, TRUE);
        if (next != NULL) {
            FSi_ExecuteAsyncCommand(next);
        }
    }
}

// NitroSDK's FSi_SendCommand: queues a command on a file's archive, and runs it when it is the next. With blocking set
// it returns when the command has ended, TRUE if it succeeded
BOOL FSi_SendCommand(FSFile *file, FSCommandType command, BOOL blocking) {
    BOOL result = FALSE;
    FSArchive *arc = file->arc;
    OSIntrMode interrupts;
    BOOL start;
    FSFile *next;

    // A file takes one command at a time
    if (FS_IsBusy(file)) {
        OS_Terminate();
    }
    if (arc == NULL) {
        file->error = FS_RESULT_INVALID_PARAMETER;
        return FALSE;
    }
    file->error = FS_RESULT_BUSY;
    file->stat &= ~(0xff << FS_FILE_STATUS_COMMAND_SHIFT);
    file->stat |= command << FS_FILE_STATUS_COMMAND_SHIFT;
    file->stat |= FS_FILE_STATUS_BUSY;
    file->next = NULL;
    if (blocking) {
        file->stat |= FS_FILE_STATUS_BLOCKING;
    }
    interrupts = OS_DisableInterrupts();
    if (arc->flag & FS_ARCHIVE_FLAG_UNLOADING) {
        FSi_EndCommand(file, FS_RESULT_CANCELED);
    } else {
        FSFile **link = &arc->list;
        FSFile *last = arc->list;

        while (last != NULL) {
            link = &last->next;
            last = last->next;
        }
        *link = file;
    }
    start = arc->list == file && !(arc->flag & FS_ARCHIVE_FLAG_RUNNING);
    OS_RestoreInterrupts(interrupts);
    if (file->error != FS_RESULT_CANCELED) {
        next = FSi_NextCommand(arc, start);
        if (blocking) {
            FSi_ExecuteSyncCommand(file);
            result = file->error == FS_RESULT_SUCCESS;
        } else {
            if (next != NULL) {
                FSi_ExecuteAsyncCommand(next);
            }
            result = TRUE;
        }
    }
    return result;
}

// NitroSDK's FS_FindArchive: finds the mounted archive of a name with the given number of characters
FSArchive *FS_FindArchive(const char *name, int nameLength) {
    FSArchive *arc;
    OSIntrMode interrupts = OS_DisableInterrupts();

    for (arc = sArchives.list; arc != NULL; arc = arc->next) {
        if (FS_IsArchiveLoaded(arc)) {
            const char *arcName = FS_GetArchiveName(arc);

            if (STD_CompareNString(arcName, name, nameLength) == 0 && arcName[nameLength] == '\0') {
                break;
            }
        }
    }
    OS_RestoreInterrupts(interrupts);
    return arc;
}

// TwlSDK's FS_SetCurrentDirectory: makes a path's archive and directory the current ones
BOOL FS_SetCurrentDirectory(const char *path) {
    u32 baseDirId = 0;
    BOOL result = FALSE;
    char normalized[FS_PATH_BUFFER_SIZE];
    FSArchive *arc = FSi_NormalizePath(path, &baseDirId, normalized);

    if (arc != NULL) {
        sArchives.current.arc = arc;
        sArchives.current.own_id = 0;
        sArchives.current.index = 0;
        sArchives.current.pos = 0;
        STD_CopyLString(sCurrentPath, normalized, FS_PATH_BUFFER_SIZE);
        if (arc->vtbl->findPath != NULL) {
            FSFile file;
            FSFindPathArgs args;

            FS_InitFile(&file);
            file.arc = arc;
            file.argument = &args;
            args.baseDirId = baseDirId;
            args.path = normalized;
            args.isDirectory = TRUE;
            if (FSi_SendCommand(&file, FS_COMMAND_FINDPATH, TRUE)) {
                sArchives.current.own_id = args.targetId;
                STD_CopyLString(sCurrentPath, normalized, FS_PATH_BUFFER_SIZE);
            }
        }
        result = TRUE;
    }
    return result;
}

// Copies at most length characters of a string, no more than fit in a buffer with its NUL, and sets *overflow when
// there were more. Returns the number of characters copied. NitroSDK's name is not known
static int FSi_CopyPathPart(char *dest, int destSize, const char *src, int length, BOOL *overflow) {
    int i;
    char c;
    int count = destSize - 1;

    if (count >= length) {
        count = length;
    }
    for (i = 0; i < count && (c = src[i]) != '\0'; i++) {
        dest[i] = c;
    }
    if (i < length && src[i] != '\0') {
        *overflow = TRUE;
    }
    dest[i] = '\0';
    return i;
}

static inline BOOL FSi_IsSlash(u8 c) {
    return c == '/' || c == '\\';
}

// NitroSDK's FSi_NormalizePath: splits a path into its archive, the directory it starts in and the path inside the
// archive, written without "." and ".." and a trailing slash. Returns the archive, or NULL when the archive is
// unknown or the path does not fit in the buffer
FSArchive *FSi_NormalizePath(const char *path, u32 *baseDirId, char *normalized) {
    FSArchive *arc;
    BOOL overflow = FALSE;
    int length = 0;
    int i;

    if (sArchives.current.arc == NULL) {
        sArchives.current.arc = sArchives.list;
        sArchives.current.own_id = 0;
        sArchives.current.pos = 0;
        sArchives.current.index = 0;
        sCurrentPath[0] = '\0';
    }
    if (FSi_IsSlash(path[0])) {
        path++;
        arc = sArchives.current.arc;
        if (baseDirId != NULL) {
            *baseDirId = 0;
        }
    } else {
        for (i = 0;; i = i + 1 + STD_IsSjisLeadByte(path[i])) {
            u8 c = path[i];

            if (c == '\0' || FSi_IsSlash(c)) {
                arc = sArchives.current.arc;
                if (baseDirId != NULL) {
                    *baseDirId = sArchives.current.own_id;
                }
                if (normalized != NULL && sArchives.current.own_id == 0 && sCurrentPath[0] != '\0') {
                    length +=
                        FSi_CopyPathPart(normalized, FS_PATH_BUFFER_SIZE, sCurrentPath, FS_PATH_BUFFER_SIZE, &overflow);
                    length += FSi_CopyPathPart(normalized + length, FS_PATH_BUFFER_SIZE - length, "/", 1, &overflow);
                }
                break;
            }
            if (c == ':') {
                arc = FS_FindArchive(path, i);
                path += i + 1;
                if (FSi_IsSlash(path[0])) {
                    path++;
                }
                if (baseDirId != NULL) {
                    *baseDirId = 0;
                }
                break;
            }
        }
    }
    if (normalized != NULL) {
        i = 0;
        while (!overflow) {
            char c = path[i];

            if (c != '\0' && !FSi_IsSlash(c)) {
                i += STD_IsSjisCharacter(path + i) ? 2 : 1;
                continue;
            }
            if (i == 0 || (i == 1 && path[0] == '.')) {
                // The directory itself
            } else if (i == 2 && path[0] == '.' && path[1] == '.') {
                if (length > 0) {
                    length--;
                }
                length = FSi_DecrementSjisPositionToSlash(normalized, length) + 1;
            } else {
                length += FSi_CopyPathPart(normalized + length, FS_PATH_BUFFER_SIZE - length, path, i, &overflow);
                if (c != '\0') {
                    length += FSi_CopyPathPart(normalized + length, FS_PATH_BUFFER_SIZE - length, "/", 1, &overflow);
                }
            }
            if (c == '\0') {
                break;
            }
            path += i + 1;
            i = 0;
        }
        normalized[length] = '\0';
        FSi_TrimSjisTrailingSlash(normalized);
    }
    if (overflow) {
        arc = NULL;
    }
    return arc;
}

// NitroSDK's FS_InitArchive: clears an archive and its queue
void FS_InitArchive(FSArchive *arc) {
    MI_CpuFill8(arc, 0, sizeof(FSArchive));
    OS_InitThreadQueue(&arc->queue);
}

// NitroSDK's FS_RegisterArchiveName: registers an archive under a name of at most 15 characters, unless the name is in
// use
BOOL FS_RegisterArchiveName(FSArchive *arc, const char *name, u32 nameLength) {
    BOOL result = FALSE;
    OSIntrMode interrupts = OS_DisableInterrupts();

    if (FS_FindArchive(name, nameLength) == NULL) {
        FSArchive **link = &sArchives.list;
        FSArchive *last = sArchives.list;

        while (last != NULL) {
            link = &last->next;
            last = last->next;
        }
        *link = arc;
        if (nameLength <= FS_ARCHIVE_SHORT_NAME_LEN_MAX) {
            arc->longName = NULL;
            STD_CopyLString(arc->name, name, nameLength + 1);
        } else if (nameLength <= FS_ARCHIVE_NAME_LEN_MAX) {
            int i;

            for (i = 0;; i++) {
                if (i >= FS_ARCHIVE_NAME_SLOT_COUNT) {
                    OS_Terminate();
                } else if (sArchiveNames[i][0] == '\0') {
                    STD_CopyLString(sArchiveNames[i], name, nameLength + 1);
                    arc->longName = sArchiveNames[i];
                    break;
                }
            }
        } else {
            OS_Terminate();
        }
        arc->flag |= FS_ARCHIVE_FLAG_REGISTER;
        result = TRUE;
    }
    OS_RestoreInterrupts(interrupts);
    return result;
}

// NitroSDK's FS_ReleaseArchiveName: takes an archive's name out of the list of archives (not the first)
void FS_ReleaseArchiveName(FSArchive *arc) {
    if (arc == sArchives.list) {
        OS_Terminate();
    }
    if (arc->longName != NULL) {
        OSIntrMode interrupts = OS_DisableInterrupts();
        FSArchive *current = sArchives.list;
        FSArchive **link = &sArchives.list;

        while (current != NULL) {
            if (current == arc) {
                *link = current->next;
                break;
            }
            link = &current->next;
            current = current->next;
        }
        if (arc->name[FS_ARCHIVE_SHORT_NAME_LEN_MAX] != '\0') {
            arc->longName[0] = '\0';
        }
        arc->longName = NULL;
        arc->next = NULL;
        arc->flag &= ~FS_ARCHIVE_FLAG_REGISTER;
        if (sArchives.current.arc == arc) {
            sArchives.current.arc = NULL;
        }
        OS_RestoreInterrupts(interrupts);
    }
}

// NitroSDK's FS_GetArchiveName: the name of an archive
const char *FS_GetArchiveName(const FSArchive *arc) {
    if (arc->name[FS_ARCHIVE_SHORT_NAME_LEN_MAX] != '\0') {
        return arc->longName;
    }
    return arc->name;
}

// NitroSDK's FS_MountArchive: sets an archive's interface and mounts it. reserved is unused
BOOL FS_MountArchive(FSArchive *arc, void *userdata, const FSArchiveInterface *vtbl, u32 reserved) {
    FSFile file;

    arc->userdata = userdata;
    arc->vtbl = vtbl;
    FS_InitFile(&file);
    file.arc = arc;
    FSi_InvokeCommand(&file, FS_COMMAND_MOUNT);
    arc->flag |= FS_ARCHIVE_FLAG_LOADED;
    return TRUE;
}

// NitroSDK's FS_UnmountArchive: cancels the commands of an archive, and runs FS_COMMAND_UNMOUNT
BOOL FS_UnmountArchive(FSArchive *arc) {
    OSIntrMode interrupts = OS_DisableInterrupts();

    if (FS_IsArchiveLoaded(arc)) {
        BOOL suspended = FS_SuspendArchive(arc);
        FSFile *file = arc->list;
        FSFile unmountFile;

        arc->flag |= FS_ARCHIVE_FLAG_UNLOADING;
        while (file != NULL) {
            FSFile *following = file->next;

            FSi_EndCommand(file, FS_RESULT_CANCELED);
            file = following;
        }
        arc->list = NULL;
        if (suspended) {
            FS_ResumeArchive(arc);
        }
        FS_InitFile(&unmountFile);
        unmountFile.arc = arc;
        FSi_InvokeCommand(&unmountFile, FS_COMMAND_UNMOUNT);
        arc->flag &= ~(FS_ARCHIVE_FLAG_UNLOADING | FS_ARCHIVE_FLAG_CANCELING | FS_ARCHIVE_FLAG_LOADED);
    }
    OS_RestoreInterrupts(interrupts);
    return TRUE;
}

// NitroSDK's FS_SuspendArchive: stops an archive from starting commands, waiting for the one it runs to end. Returns
// FALSE if it was suspended already
BOOL FS_SuspendArchive(FSArchive *arc) {
    OSIntrMode interrupts = OS_DisableInterrupts();
    BOOL result = !FS_IsArchiveSuspended(arc);

    if (result) {
        if (!(arc->flag & FS_ARCHIVE_FLAG_RUNNING)) {
            arc->flag |= FS_ARCHIVE_FLAG_SUSPEND;
        } else {
            OSIntrMode waitInterrupts;

            arc->flag |= FS_ARCHIVE_FLAG_SUSPENDING;
            waitInterrupts = OS_DisableInterrupts();
            while (arc->flag & FS_ARCHIVE_FLAG_SUSPENDING) {
                OS_SleepThread(&arc->queue);
            }
            OS_RestoreInterrupts(waitInterrupts);
        }
    }
    OS_RestoreInterrupts(interrupts);
    return result;
}

// NitroSDK's FS_ResumeArchive: lets a suspended archive start commands again. Returns TRUE if it was not suspended
BOOL FS_ResumeArchive(FSArchive *arc) {
    OSIntrMode interrupts = OS_DisableInterrupts();
    BOOL result = !FS_IsArchiveSuspended(arc);
    FSFile *file;

    if (!result) {
        arc->flag &= ~FS_ARCHIVE_FLAG_SUSPEND;
    }
    OS_RestoreInterrupts(interrupts);
    file = FSi_NextCommand(arc, TRUE);
    if (file != NULL) {
        FSi_ExecuteAsyncCommand(file);
    }
    return result;
}

// NitroSDK's FS_NotifyArchiveAsyncEnd: ends the command that an archive ran apart from the thread of the caller, with
// its result
void FS_NotifyArchiveAsyncEnd(FSArchive *arc, FSResult result) {
    FSFile *file = arc->list;

    if (file->stat & FS_FILE_STATUS_BLOCKING) {
        OSIntrMode interrupts = OS_DisableInterrupts();

        file->stat |= FS_FILE_STATUS_ENDED;
        file->error = result;
        OS_WakeupThread(&file->queue);
        OS_RestoreInterrupts(interrupts);
    } else {
        FSFile *next;

        FSi_EndCommand(file, result);
        next = FSi_NextCommand(arc, TRUE);
        if (next != NULL) {
            FSi_ExecuteAsyncCommand(next);
        }
    }
}

// NitroSDK's FSi_DecrementSjisPosition: the start of the character before a position in a Shift_JIS string
static int FSi_DecrementSjisPosition(const char *path, int position) {
    int start = position - 1;

    while (start > 0 && STD_IsSjisLeadByte(path[start - 1])) {
        start--;
    }
    return (position - 1) - (((position - 1) - start) & 1);
}

// NitroSDK's FSi_IncrementSjisPositionToSlash: the position of the next slash or the end of a string, from a position
int FSi_IncrementSjisPositionToSlash(const char *path, int position) {
    char c;

    while ((c = path[position]) != '\0' && (c & 0xff) != '/' && (c & 0xff) != '\\') {
        position = position + 1 + STD_IsSjisLeadByte(c);
    }
    return position;
}

// NitroSDK's FSi_DecrementSjisPositionToSlash: the position of the last slash before a position, or -1
static int FSi_DecrementSjisPositionToSlash(const char *path, int position) {
    do {
        position = FSi_DecrementSjisPosition(path, position);
    } while (position >= 0 && !FSi_IsSlash(path[position]));
    return position;
}

// NitroSDK's FSi_TrimSjisTrailingSlash: cuts a trailing slash off a path. Returns the new length
static int FSi_TrimSjisTrailingSlash(char *path) {
    int length = STD_GetStringLength(path);
    int position = FSi_DecrementSjisPosition(path, length);

    if (position >= 0 && FSi_IsSlash(path[position])) {
        length = position;
        path[position] = '\0';
    }
    return length;
}
