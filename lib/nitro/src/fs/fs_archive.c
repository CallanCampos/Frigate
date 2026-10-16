// NitroSDK's file system (libfs): the archives, the queue of the commands that their files send, and the paths that
// name a file in an archive ("rom:/dir/file"). The file name is a guess from NitroSDK's own layout (fs_archive.c); no
// string in the ROM names it. The functions are under swan's names, with NitroSDK's in their comments.

#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// Where the archives are registered, and the archive and directory that a relative path starts in
static struct {
    FSArchive *list;
    FSArchive *current;
    u16 directoryId;
    u16 unk_0a;
    u32 unk_0c;
} sArchives;

// The names of more than 3 characters, one slot each. A slot is free when it starts with a NUL
#define FS_ARCHIVE_NAME_SLOT_COUNT 16
static char sArchiveNames[FS_ARCHIVE_NAME_SLOT_COUNT][FS_ARCHIVE_NAME_LEN_MAX + 1];

// The path of the current directory inside the current archive
static char sCurrentPath[FS_PATH_BUFFER_SIZE];

static int func_02070c34(const char *path, int position);
static int func_02070c50(char *path);

// The command of a file
static inline FSCommandType FSi_GetCommand(const FSFile *file) {
    return (FSCommandType)(u8)(file->stat >> FS_FILE_STATUS_COMMAND_SHIFT);
}

// The tests of the state of a file and of an archive. They return u32 and not BOOL, with which MWCC keeps the 0 or 1 as
// a value to compare, as the ROM's code does, and not folded into the branch
static inline u32 FSi_IsFileCanceled(const FSFile *file) {
    return (file->stat & FS_FILE_STATUS_CANCEL) != 0;
}

static inline u32 FSi_IsFileBusy(const FSFile *file) {
    return (file->stat & FS_FILE_STATUS_BUSY) != 0;
}

static inline u32 FSi_IsArchiveMounted(const FSArchive *arc) {
    return (arc->flag & FS_ARCHIVE_FLAG_MOUNTED) != 0;
}

static inline u32 FSi_IsArchiveSuspended(const FSArchive *arc) {
    return (arc->flag & FS_ARCHIVE_FLAG_SUSPENDED) != 0;
}

// The bit of a command in the mask of the events, counted from the first of them
#define FSi_EVENT_BIT(command) (1 << ((command) - FS_COMMAND_ACTIVATE))

// NitroSDK's FSi_IsEventCommand: whether a command is an event
static BOOL func_020700a8(u32 command) {
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
static void func_020700c8(FSFile *file, FSResult result) {
    FSArchive *arc;
    FSCommandType command;
    u32 interrupts = CPU_IRQDisable();

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
    if (!func_020700a8(command) && arc != NULL) {
        arc->command = command;
        arc->result = result;
    }
    file->error = result;
    file->stat &= ~FS_FILE_STATUS_PROGRESS_MASK;
    OS_WakeupThread(&file->queue);
    CPU_SetIRQMask(interrupts);
}

// NitroSDK's FSi_WaitForArchiveCompletion: if a command went on apart from the thread (FS_RESULT_PROC_ASYNC), waits
// until the archive ends it, and returns its result
FSResult func_02070130(FSFile *file, FSResult result) {
    if (result == FS_RESULT_PROC_ASYNC) {
        u32 interrupts = CPU_IRQDisable();

        if (!(file->stat & FS_FILE_STATUS_ENDED)) {
            do {
                scheduler_yield(&file->queue);
            } while (!(file->stat & FS_FILE_STATUS_ENDED));
        }
        CPU_SetIRQMask(interrupts);
        file->stat &= ~FS_FILE_STATUS_ENDED;
        result = file->error;
    }
    return result;
}

// NitroSDK's FSi_InvokeCommand: runs a file's command through its archive's interface. The events return at once; the
// other commands end here, or wait for the archive to end them if a thread is blocked on the file
static FSResult func_02070170(FSFile *file, u32 command) {
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
        case FS_COMMAND_19:
            result = vtbl->command19(arc, file->argument);
            break;
        case FS_COMMAND_20: {
            u32 *args = file->argument;

            result = vtbl->command20(arc, args[0], args[1], args[2]);
            break;
        }
        case FS_COMMAND_21: {
            u32 *args = file->argument;

            result = vtbl->command21(arc, args[0], args[1]);
            break;
        }
        case FS_COMMAND_22: {
            u32 *args = file->argument;

            result = vtbl->command22(arc, args[0], args[1], args[2], args[3]);
            break;
        }
        case FS_COMMAND_GETPATHINFO: {
            FSGetPathInfoArgs *args = file->argument;

            result = vtbl->getPathInfo(arc, args->baseDirId, args->path, args->info);
            break;
        }
        case FS_COMMAND_24: {
            u32 *args = file->argument;

            result = vtbl->command24(arc, args[0], args[1], args[2]);
            break;
        }
        case FS_COMMAND_25: {
            u32 *args = file->argument;

            result = vtbl->command25(arc, args[0], args[1], args[2]);
            break;
        }
        case FS_COMMAND_26: {
            u32 *args = file->argument;

            result = vtbl->command26(arc, args[0], args[1]);
            break;
        }
        case FS_COMMAND_27: {
            u32 *args = file->argument;

            result = vtbl->command27(arc, args[0], args[1], args[2], args[3]);
            break;
        }
        case FS_COMMAND_GETARCHIVERESOURCE: {
            FSGetArchiveResourceArgs *args = file->argument;

            result = vtbl->getArchiveResource(arc, args->resource);
            break;
        }
        case FS_COMMAND_30:
            result = vtbl->command30(arc, file);
            break;
        case FS_COMMAND_31: {
            u32 *args = file->argument;

            result = vtbl->command31(arc, file, args[0]);
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
        case FS_COMMAND_34: {
            u32 *args = file->argument;

            result = vtbl->command34(arc, file, args[0], args[1]);
            break;
        }
        default:
            result = FS_RESULT_UNSUPPORTED;
            break;
        }
    }
    if (!func_020700a8(command)) {
        if (file->stat & FS_FILE_STATUS_BLOCKING) {
            result = func_02070130(file, result);
        } else if (result != FS_RESULT_PROC_ASYNC) {
            func_020700c8(file, result);
        }
    }
    return result;
}

// NitroSDK's FSi_NextCommand: ends the commands that were canceled, activates the archive if it is to start one, and
// returns the file whose command the caller has to run. A blocked thread runs its own, so none is returned for it
static FSFile *func_0207038c(FSArchive *arc, BOOL start) {
    FSFile *next = NULL;
    u32 interrupts;
    u32 activateInterrupts;
    u32 runInterrupts;
    BOOL activate;

    interrupts = CPU_IRQDisable();
    if (arc->flag & FS_ARCHIVE_FLAG_CANCELING) {
        FSFile *file = arc->list;

        arc->flag &= ~FS_ARCHIVE_FLAG_CANCELING;
        while (file != NULL) {
            FSFile *following = file->next;

            if (FSi_IsFileCanceled(file)) {
                func_020700c8(file, FS_RESULT_CANCELED);
                if (following == NULL) {
                    following = arc->list;
                }
            }
            file = following;
        }
    }
    CPU_SetIRQMask(interrupts);

    activateInterrupts = CPU_IRQDisable();
    if (!(arc->flag & FS_ARCHIVE_FLAG_SUSPENDING) && !(arc->flag & FS_ARCHIVE_FLAG_SUSPENDED) && arc->list != NULL) {
        activate = start && !(arc->flag & FS_ARCHIVE_FLAG_ACTIVE);
        if (activate) {
            arc->flag |= FS_ARCHIVE_FLAG_ACTIVE;
        }
        CPU_SetIRQMask(activateInterrupts);
        if (activate) {
            func_02070170(arc->list, FS_COMMAND_ACTIVATE);
        }
        runInterrupts = CPU_IRQDisable();
        if (start || activate) {
            next = arc->list;
            next->stat |= FS_FILE_STATUS_RUNNING;
        }
        if (start && (next->stat & FS_FILE_STATUS_BLOCKING)) {
            OS_WakeupThread(&next->queue);
            next = NULL;
        }
        CPU_SetIRQMask(runInterrupts);
    } else {
        if (start) {
            if (arc->flag & FS_ARCHIVE_FLAG_ACTIVE) {
                FSFile file;

                finit(&file);
                file.arc = arc;
                arc->flag &= ~FS_ARCHIVE_FLAG_ACTIVE;
                func_02070170(&file, FS_COMMAND_IDLE);
            }
            if (arc->flag & FS_ARCHIVE_FLAG_SUSPENDING) {
                arc->flag = (arc->flag & ~FS_ARCHIVE_FLAG_SUSPENDING) | FS_ARCHIVE_FLAG_SUSPENDED;
                OS_WakeupThread(&arc->queue);
            }
        }
        CPU_SetIRQMask(activateInterrupts);
    }
    return next;
}

// NitroSDK's FSi_ExecuteAsyncCommand: runs the commands of an archive from the one of a file, until one goes on apart
// from the thread
static void func_020704a0(FSFile *file) {
    FSArchive *arc = file->arc;

    while (file != NULL) {
        u32 interrupts = CPU_IRQDisable();

        file->stat |= FS_FILE_STATUS_RUNNING;
        if (file->stat & FS_FILE_STATUS_BLOCKING) {
            OS_WakeupThread(&file->queue);
            file = NULL;
        }
        CPU_SetIRQMask(interrupts);
        if (file == NULL) {
            break;
        }
        if (func_02070170(file, FSi_GetCommand(file)) == FS_RESULT_PROC_ASYNC) {
            break;
        }
        file = func_0207038c(arc, TRUE);
    }
}

// NitroSDK's FSi_ExecuteSyncCommand: waits for a file's turn, runs its command in the calling thread, and goes on with
// the next ones
static void func_020704f4(FSFile *file) {
    u32 interrupts = CPU_IRQDisable();

    while (!(file->stat & FS_FILE_STATUS_RUNNING) && (file->stat & FS_FILE_STATUS_BUSY)) {
        scheduler_yield(&file->queue);
    }
    CPU_SetIRQMask(interrupts);
    if (file->stat & FS_FILE_STATUS_RUNNING) {
        FSArchive *arc = file->arc;
        FSFile *next;

        func_020700c8(file, func_02070170(file, FSi_GetCommand(file)));
        next = func_0207038c(arc, TRUE);
        if (next != NULL) {
            func_020704a0(next);
        }
    }
}

// NitroSDK's FSi_SendCommand: queues a command on a file's archive, and runs it when it is the next. With blocking set
// it returns when the command has ended, TRUE if it succeeded
BOOL fs_call_syscmd(FSFile *file, FSCommandType command, BOOL blocking) {
    BOOL result = FALSE;
    FSArchive *arc = file->arc;
    u32 interrupts;
    BOOL start;
    FSFile *next;

    // A file takes one command at a time
    if (FSi_IsFileBusy(file)) {
        sys_exit();
    }
    if (arc == NULL) {
        file->error = FS_RESULT_INVALID_PARAMETER;
        return FALSE;
    }
    file->error = FS_RESULT_BUSY;
    {
        u32 *stat = &file->stat;

        *stat = (*stat & ~(0xff << FS_FILE_STATUS_COMMAND_SHIFT)) | (command << FS_FILE_STATUS_COMMAND_SHIFT) |
                FS_FILE_STATUS_BUSY;
        file->next = NULL;
        if (blocking) {
            *stat |= FS_FILE_STATUS_BLOCKING;
        }
    }
    interrupts = CPU_IRQDisable();
    if (arc->flag & FS_ARCHIVE_FLAG_UNMOUNTING) {
        func_020700c8(file, FS_RESULT_CANCELED);
    } else {
        FSFile **link = &arc->list;
        FSFile *last = arc->list;

        while (last != NULL) {
            link = &last->next;
            last = last->next;
        }
        *link = file;
    }
    start = FALSE;
    if (arc->list == file && !(arc->flag & FS_ARCHIVE_FLAG_ACTIVE)) {
        start = TRUE;
    }
    CPU_SetIRQMask(interrupts);
    if (file->error != FS_RESULT_CANCELED) {
        next = func_0207038c(arc, start);
        if (blocking) {
            func_020704f4(file);
            result = TRUE;
            if (file->error != FS_RESULT_SUCCESS) {
                result = FALSE;
            }
        } else {
            if (next != NULL) {
                func_020704a0(next);
            }
            result = TRUE;
        }
    }
    return result;
}

// NitroSDK's FS_FindArchive: finds the mounted archive of a name with the given number of characters
FSArchive *func_0207062c(const char *name, int nameLength) {
    FSArchive *arc;
    u32 interrupts = CPU_IRQDisable();

    for (arc = sArchives.list; arc != NULL; arc = arc->next) {
        if (FSi_IsArchiveMounted(arc)) {
            const char *arcName = func_02070a24(arc);

            if (func_0207f9c8(arcName, name, nameLength) == 0 && arcName[nameLength] == '\0') {
                break;
            }
        }
    }
    CPU_SetIRQMask(interrupts);
    return arc;
}

// NitroSDK's FS_ChangeDir: makes a path's archive and directory the current ones
BOOL func_02070680(const char *path) {
    u32 baseDirId = 0;
    BOOL result = FALSE;
    char normalized[FS_PATH_BUFFER_SIZE];
    FSArchive *arc = fs_normalize_path(path, &baseDirId, normalized);

    if (arc != NULL) {
        sArchives.current = arc;
        sArchives.directoryId = 0;
        sArchives.unk_0a = 0;
        sArchives.unk_0c = 0;
        func_0207f7cc(sCurrentPath, normalized, FS_PATH_BUFFER_SIZE);
        if (arc->vtbl->findPath != NULL) {
            FSFile file;
            FSFindPathArgs args;

            finit(&file);
            file.arc = arc;
            file.argument = &args;
            args.baseDirId = baseDirId;
            args.path = normalized;
            args.isDirectory = TRUE;
            if (fs_call_syscmd(&file, FS_COMMAND_FINDPATH, TRUE)) {
                sArchives.directoryId = args.targetId;
                func_0207f7cc(sCurrentPath, normalized, FS_PATH_BUFFER_SIZE);
            }
        }
        result = TRUE;
    }
    return result;
}

// Copies at most length characters of a string, no more than fit in a buffer with its NUL, and sets *overflow when
// there were more. Returns the number of characters copied. NitroSDK's name is not known
static int func_020706fc(char *dest, int destSize, const char *src, int length, BOOL *overflow) {
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

// Whether a byte starts a two-byte Shift_JIS character
static inline BOOL FSi_IsSjisLead(u8 c) {
    return (u32)((c ^ 0x20) - 0xa1) < 0x3c;
}

// Whether the bytes at a string's start are a two-byte Shift_JIS character
static inline BOOL FSi_IsSjisCharacter(const char *s) {
    BOOL result = FALSE;

    if (FSi_IsSjisLead(s[0]) && s[1] != 0x7f && (u8)(s[1] - 0x40) <= 0xbc) {
        result = TRUE;
    }
    return result;
}

static inline BOOL FSi_IsSlash(u8 c) {
    return c == '/' || c == '\\';
}

// NitroSDK's FSi_NormalizePath: splits a path into its archive, the directory it starts in and the path inside the
// archive, written without "." and ".." and a trailing slash. Returns the archive, or NULL when the archive is
// unknown or the path does not fit in the buffer
FSArchive *fs_normalize_path(const char *path, u32 *baseDirId, char *normalized) {
    FSArchive *arc;
    BOOL overflow = FALSE;
    int length = 0;
    int i;

    if (sArchives.current == NULL) {
        sArchives.current = sArchives.list;
        sArchives.directoryId = 0;
        sArchives.unk_0c = 0;
        sArchives.unk_0a = 0;
        sCurrentPath[0] = '\0';
    }
    if (FSi_IsSlash(path[0])) {
        path++;
        arc = sArchives.current;
        if (baseDirId != NULL) {
            *baseDirId = 0;
        }
    } else {
        for (i = 0;; i = i + 1 + FSi_IsSjisLead(path[i])) {
            u8 c = path[i];

            if (c == '\0' || FSi_IsSlash(c)) {
                arc = sArchives.current;
                if (baseDirId != NULL) {
                    *baseDirId = sArchives.directoryId;
                }
                if (normalized != NULL && sArchives.directoryId == 0 && sCurrentPath[0] != '\0') {
                    length +=
                        func_020706fc(normalized, FS_PATH_BUFFER_SIZE, sCurrentPath, FS_PATH_BUFFER_SIZE, &overflow);
                    length += func_020706fc(normalized + length, FS_PATH_BUFFER_SIZE - length, "/", 1, &overflow);
                }
                break;
            }
            if (c == ':') {
                arc = func_0207062c(path, i);
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
                i += FSi_IsSjisCharacter(path + i) ? 2 : 1;
                continue;
            }
            if (i == 0 || (i == 1 && path[0] == '.')) {
                // The directory itself
            } else if (i == 2 && path[0] == '.' && path[1] == '.') {
                if (length > 0) {
                    length--;
                }
                length = func_02070c34(normalized, length) + 1;
            } else {
                length += func_020706fc(normalized + length, FS_PATH_BUFFER_SIZE - length, path, i, &overflow);
                if (c != '\0') {
                    length += func_020706fc(normalized + length, FS_PATH_BUFFER_SIZE - length, "/", 1, &overflow);
                }
            }
            if (c == '\0') {
                break;
            }
            path += i + 1;
            i = 0;
        }
        normalized[length] = '\0';
        func_02070c50(normalized);
    }
    if (overflow) {
        arc = NULL;
    }
    return arc;
}

// NitroSDK's FS_InitArchive: clears an archive and its queue
void fs_create(FSArchive *arc) {
    sys_memset(arc, 0, sizeof(FSArchive));
    OS_InitThreadQueue(&arc->queue);
}

// NitroSDK's FS_RegisterArchiveName: registers an archive under a name of at most 15 characters, unless the name is in
// use
BOOL fs_open(FSArchive *arc, const char *name, u32 nameLength) {
    BOOL result = FALSE;
    u32 interrupts = CPU_IRQDisable();

    if (func_0207062c(name, nameLength) == NULL) {
        FSArchive **link = &sArchives.list;
        FSArchive *last = sArchives.list;

        while (last != NULL) {
            link = &last->next;
            last = last->next;
        }
        *link = arc;
        if (nameLength <= FS_ARCHIVE_SHORT_NAME_LEN_MAX) {
            arc->longName = NULL;
            func_0207f7cc(arc->name, name, nameLength + 1);
        } else if (nameLength <= FS_ARCHIVE_NAME_LEN_MAX) {
            int i;

            for (i = 0;; i++) {
                if (i >= FS_ARCHIVE_NAME_SLOT_COUNT) {
                    sys_exit();
                } else if (sArchiveNames[i][0] == '\0') {
                    func_0207f7cc(sArchiveNames[i], name, nameLength + 1);
                    arc->longName = sArchiveNames[i];
                    break;
                }
            }
        } else {
            sys_exit();
        }
        arc->flag |= FS_ARCHIVE_FLAG_REGISTERED;
        result = TRUE;
    }
    CPU_SetIRQMask(interrupts);
    return result;
}

// NitroSDK's FS_ReleaseArchiveName: takes an archive's name out of the list of archives (not the first)
void func_020709b8(FSArchive *arc) {
    if (arc == sArchives.list) {
        sys_exit();
    }
    if (arc->longName != NULL) {
        u32 interrupts = CPU_IRQDisable();
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
        arc->flag &= ~FS_ARCHIVE_FLAG_REGISTERED;
        if (sArchives.current == arc) {
            sArchives.current = NULL;
        }
        CPU_SetIRQMask(interrupts);
    }
}

// NitroSDK's FS_GetArchiveName: the name of an archive
const char *func_02070a24(const FSArchive *arc) {
    if (arc->name[FS_ARCHIVE_SHORT_NAME_LEN_MAX] != '\0') {
        return arc->longName;
    }
    return arc->name;
}

// NitroSDK's FS_MountArchive: sets an archive's interface and mounts it
BOOL func_02070a30(FSArchive *arc, void *userdata, const FSArchiveInterface *vtbl) {
    FSFile file;

    arc->userdata = userdata;
    arc->vtbl = vtbl;
    finit(&file);
    file.arc = arc;
    func_02070170(&file, FS_COMMAND_MOUNT);
    arc->flag |= FS_ARCHIVE_FLAG_MOUNTED;
    return TRUE;
}

// NitroSDK's FS_UnmountArchive: cancels the commands of an archive, and runs FS_COMMAND_UNMOUNT
BOOL func_02070a5c(FSArchive *arc) {
    u32 interrupts = CPU_IRQDisable();

    if (FSi_IsArchiveMounted(arc)) {
        BOOL suspended = func_02070ad4(arc);
        FSFile *file = arc->list;
        FSFile unmountFile;

        arc->flag |= FS_ARCHIVE_FLAG_UNMOUNTING;
        while (file != NULL) {
            FSFile *following = file->next;

            func_020700c8(file, FS_RESULT_CANCELED);
            file = following;
        }
        arc->list = NULL;
        if (suspended) {
            func_02070b44(arc);
        }
        finit(&unmountFile);
        unmountFile.arc = arc;
        func_02070170(&unmountFile, FS_COMMAND_UNMOUNT);
        arc->flag &= ~(FS_ARCHIVE_FLAG_UNMOUNTING | FS_ARCHIVE_FLAG_CANCELING | FS_ARCHIVE_FLAG_MOUNTED);
    }
    CPU_SetIRQMask(interrupts);
    return TRUE;
}

// NitroSDK's FS_SuspendArchive: stops an archive from starting commands, waiting for the one it runs to end. Returns
// FALSE if it was suspended already
BOOL func_02070ad4(FSArchive *arc) {
    u32 interrupts = CPU_IRQDisable();
    BOOL result = !FSi_IsArchiveSuspended(arc);

    if (result) {
        if (!(arc->flag & FS_ARCHIVE_FLAG_ACTIVE)) {
            arc->flag |= FS_ARCHIVE_FLAG_SUSPENDED;
        } else {
            u32 waitInterrupts;

            arc->flag |= FS_ARCHIVE_FLAG_SUSPENDING;
            waitInterrupts = CPU_IRQDisable();
            while (arc->flag & FS_ARCHIVE_FLAG_SUSPENDING) {
                scheduler_yield(&arc->queue);
            }
            CPU_SetIRQMask(waitInterrupts);
        }
    }
    CPU_SetIRQMask(interrupts);
    return result;
}

// NitroSDK's FS_ResumeArchive: lets a suspended archive start commands again. Returns TRUE if it was not suspended
BOOL func_02070b44(FSArchive *arc) {
    u32 interrupts = CPU_IRQDisable();
    BOOL result = !FSi_IsArchiveSuspended(arc);
    FSFile *file;

    if (!result) {
        arc->flag &= ~FS_ARCHIVE_FLAG_SUSPENDED;
    }
    CPU_SetIRQMask(interrupts);
    file = func_0207038c(arc, TRUE);
    if (file != NULL) {
        func_020704a0(file);
    }
    return result;
}

// NitroSDK's FS_NotifyArchiveAsyncEnd: ends the command that an archive ran apart from the thread of the caller, with
// its result
void func_02070b88(FSArchive *arc, FSResult result) {
    FSFile *file = arc->list;

    if (file->stat & FS_FILE_STATUS_BLOCKING) {
        u32 interrupts = CPU_IRQDisable();

        file->stat |= FS_FILE_STATUS_ENDED;
        file->error = result;
        OS_WakeupThread(&file->queue);
        CPU_SetIRQMask(interrupts);
    } else {
        FSFile *next;

        func_020700c8(file, result);
        next = func_0207038c(arc, TRUE);
        if (next != NULL) {
            func_020704a0(next);
        }
    }
}

// NitroSDK's FSi_DecrementSjisPosition: the start of the character before a position in a Shift_JIS string
static int func_02070bd0(const char *path, int position) {
    int start = position - 1;

    while (start > 0 && FSi_IsSjisLead(path[start - 1])) {
        start--;
    }
    return (position - 1) - (((position - 1) - start) & 1);
}

// NitroSDK's FSi_IncrementSjisPositionToSlash: the position of the next slash or the end of a string, from a position
int func_02070bfc(const char *path, int position) {
    char c;

    while ((c = path[position]) != '\0' && (c & 0xff) != '/' && (c & 0xff) != '\\') {
        position = position + 1 + FSi_IsSjisLead(c);
    }
    return position;
}

// NitroSDK's FSi_DecrementSjisPositionToSlash: the position of the last slash before a position, or -1
static int func_02070c34(const char *path, int position) {
    do {
        position = func_02070bd0(path, position);
    } while (position >= 0 && !FSi_IsSlash(path[position]));
    return position;
}

// NitroSDK's FSi_TrimSjisTrailingSlash: cuts a trailing slash off a path. Returns the new length
static int func_02070c50(char *path) {
    int length = NNS_STD_StrLen(path);
    int position = func_02070bd0(path, length);

    if (position >= 0 && FSi_IsSlash(path[position])) {
        length = position;
        path[position] = '\0';
    }
    return length;
}
