// NitroSDK's file system (libfs): an archive that has a file allocation table and a name table, such as the ROM's. The
// commands that it runs itself when its hook leaves them to it (the default commands, on the tables), the functions
// of its interface that run them, and the functions that set such an archive up and load its tables into memory. The
// file name is a guess from NitroSDK's own layout (fs_command.c and fs_rom.c); no string in the ROM names it. The
// functions are under swan's names, with NitroSDK's in their comments.

#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// Where the next bytes of the archive's tables are read from
typedef struct FSTableReader {
    FSArchive *arc;
    u32 position;
} FSTableReader;

// An entry of the file allocation table: where a file starts and ends in the archive
typedef struct FSFatEntry {
    u32 top;
    u32 bottom;
} FSFatEntry;

// An entry of the name table's main table, one for each directory: where its entries start in the name table, the
// number of its first file, and its parent (of the root: the number of directories)
typedef struct FSDirTableEntry {
    u32 entryOffset;
    u16 firstFileId;
    u16 parentId;
} FSDirTableEntry;

// The number that says there is no file, or no directory
#define FS_NO_ID 0x10000

static FSResult fs_call_filecmd(FSFile *file, u32 command, BOOL blocking);
static void *func_02071ff8(const FSArchive *arc, u32 offset);
static BOOL func_02072000(const FSArchive *arc);

// Reads length bytes of the archive's tables at the reader's position, and moves the position on. The tables are read
// through the archive's read function, or copied from memory once they were loaded into it
static FSResult func_0207130c(FSTableReader *reader, void *buffer, u32 length) {
    FSArchive *arc = reader->arc;
    FSArchiveTables *tables = arc->userdata;
    FSResult result;

    if (tables->loadedMemory != NULL) {
        sys_memcpy((void *)reader->position, buffer, length);
        result = FS_RESULT_SUCCESS;
    } else {
        result = tables->readFunc(arc, buffer, reader->position, length);
        result = func_02070130(arc->list, result);
    }
    reader->position += length;
    return result;
}

// Sets the directory that a command of the file works on: the one with the number dirId, at its start
static FSResult func_02071340(FSFile *file, u32 dirId) {
    file->arg.seekDirectory.arc = file->arc;
    file->arg.seekDirectory.dirId = dirId;
    file->arg.seekDirectory.fileId = 0;
    file->arg.seekDirectory.position = 0;
    return fs_call_filecmd(file, FS_COMMAND_SEEKDIR, TRUE);
}

// The default command FS_COMMAND_SEEKDIR: puts the file in the directory at the position it holds. A position that is
// the start of the directory takes its first file and its entries from the directory's main table entry
static FSResult func_02071358(FSFile *file) {
    FSArchive *arc = file->arc;
    FSArchiveTables *tables = arc->userdata;
    FSDirPos *pos = &file->arg.seekDirectory;
    FSDirTableEntry entry;
    FSTableReader reader;
    FSResult result;

    reader.arc = arc;
    reader.position = tables->fnt + pos->dirId * sizeof(FSDirTableEntry);
    result = func_0207130c(&reader, &entry, sizeof(entry));
    if (result == FS_RESULT_SUCCESS) {
        file->body.dir.pos = *pos;
        if (pos->fileId == 0 && pos->position == 0) {
            file->body.dir.pos.fileId = entry.firstFileId;
            file->body.dir.pos.position = tables->fnt + entry.entryOffset;
        }
        file->body.dir.parentId = entry.parentId & 0xfff;
    }
    return result;
}

// The default command FS_COMMAND_READDIR: reads the next entry of the file's directory into the entry that the
// command holds, with its name unless the command skips it. A length of 0 ends the directory
static FSResult func_020713bc(FSFile *file) {
    FSDirEntry *entry = file->arg.readDirectory.entry;
    FSTableReader reader;
    u8 info;
    u16 dirId;
    FSResult result;

    reader.arc = file->arc;
    reader.position = file->body.dir.pos.position;
    result = func_0207130c(&reader, &info, 1);
    if (result == FS_RESULT_SUCCESS) {
        entry->nameLength = info & 0x7f;
        entry->isDirectory = (info >> 7) & 1;
        if (entry->nameLength == 0) {
            return FS_RESULT_FAILURE;
        }
        if (file->arg.readDirectory.skipName == FALSE) {
            result = func_0207130c(&reader, entry->name, entry->nameLength);
            if (result != FS_RESULT_SUCCESS) {
                return result;
            }
            entry->name[entry->nameLength] = '\0';
        } else {
            reader.position += entry->nameLength;
        }
        if (entry->isDirectory == FALSE) {
            entry->id.file.arc = file->arc;
            entry->id.file.file_id = file->body.dir.pos.fileId;
            file->body.dir.pos.fileId++;
        } else {
            result = func_0207130c(&reader, &dirId, 2);
            if (result == FS_RESULT_SUCCESS) {
                entry->id.dir.arc = file->arc;
                entry->id.dir.dirId = dirId & 0xfff;
                entry->id.dir.fileId = 0;
                entry->id.dir.position = 0;
            }
        }
        if (result == FS_RESULT_SUCCESS) {
            file->body.dir.pos.position = reader.position;
        }
    }
    return result;
}

// Compares two names of the same length, giving the difference of the first letters that differ. The case of a
// letter doesn't count
static inline int FSi_CompareNames(const char *a, const char *b, u32 length) {
    int diff = 0;
    u32 i;

    for (i = 0; i < length; i++) {
        u32 ca = (u8)(a[i] - 'A');
        u32 cb = (u8)(b[i] - 'A');

        if (ca <= 'Z' - 'A') {
            ca += 'a' - 'A';
        }
        if (cb <= 'Z' - 'A') {
            cb += 'a' - 'A';
        }
        diff = ca - cb;
        if (diff != 0) {
            break;
        }
    }
    return diff;
}

// The default command FS_COMMAND_FINDPATH: follows the path from the directory that the command holds, entry by
// entry of each directory, and gives the position of the directory or the number of the file that it names. "." and
// ".." are followed, and a "*" ends the path
static FSResult func_02071468(FSFile *file) {
    const char *path = file->arg.findPath.path;
    BOOL isDirectory = file->arg.findPath.isDirectory;
    FSDirEntry entry;
    BOOL wantDirectory;
    int length;

    fs_call_filecmd(file, FS_COMMAND_SEEKDIR, TRUE);
    for (; *path != '\0'; path += (*path != '\0') ? 1 : 0) {
        char c;

        length = func_02070bfc(path, 0);
        wantDirectory = TRUE;
        if (path[length] == '\0' && isDirectory == FALSE) {
            wantDirectory = FALSE;
        }
        if (length == 0) {
            return FS_RESULT_INVALID_PARAMETER;
        }
        c = path[0];
        if (c == '.') {
            if (length == 1) {
                path += 1;
                continue;
            } else if (length == 2 && path[1] == '.') {
                if (file->body.dir.pos.dirId != 0) {
                    func_02071340(file, file->body.dir.parentId);
                }
                path += 2;
                continue;
            }
        } else if (c == '*') {
            break;
        }
        if (length > FS_ENTRY_NAME_LEN_MAX) {
            return FS_RESULT_NO_ENTRY;
        }
        file->arg.readDirectory.entry = &entry;
        file->arg.readDirectory.skipName = FALSE;
        for (;;) {
            if (fs_call_filecmd(file, FS_COMMAND_READDIR, TRUE) != FS_RESULT_SUCCESS) {
                return FS_RESULT_NO_ENTRY;
            }
            if (wantDirectory != entry.isDirectory || length != entry.nameLength) {
                continue;
            }
            if (FSi_CompareNames(path, entry.name, length) == 0) {
                break;
            }
        }
        if (wantDirectory) {
            file->arg.seekDirectory = entry.id.dir;
            path += length;
            fs_call_filecmd(file, FS_COMMAND_SEEKDIR, TRUE);
        } else {
            if (isDirectory) {
                return FS_RESULT_NO_ENTRY;
            }
            file->arg.findPath.result->file = entry.id.file;
            return FS_RESULT_SUCCESS;
        }
    }
    if (isDirectory == FALSE) {
        return FS_RESULT_NO_ENTRY;
    }
    file->arg.findPath.result->dir = file->body.dir.pos;
    return FS_RESULT_SUCCESS;
}

// The default command FS_COMMAND_GETPATH: writes the path of the file or directory to the command's buffer, from the
// archive's name, if it fits there, and gives its length and its directory
static FSResult func_020715d0(FSFile *file) {
    const char *name;
    FSArchive *arc = file->arc;
    FSPathBuffer *args = &file->arg.getPath;
    u32 i;
    u32 dirCount;
    u32 length;
    char *buffer;
    u32 pathLength;
    u32 dirId;
    u32 fileId;
    u32 child;
    FSDirEntry entry;
    FSFile dir;

    finit(&dir);
    dir.arc = arc;
    if (FS_IsDir(file)) {
        dirId = file->body.dir.pos.dirId;
        fileId = FS_NO_ID;
    } else {
        fileId = file->body.file.fileId;
        i = 0;
        dirCount = 0;
        dirId = FS_NO_ID;
        do {
            func_02071340(&dir, i);
            if (i == 0) {
                dirCount = dir.body.dir.parentId;
            }
            dir.arg.readDirectory.entry = &entry;
            dir.arg.readDirectory.skipName = TRUE;
            if (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                do {
                    if (entry.isDirectory == FALSE && entry.id.file.file_id == fileId) {
                        dirId = dir.body.dir.pos.dirId;
                        break;
                    }
                } while (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
            }
            if (dirId != FS_NO_ID) {
                break;
            }
            i++;
        } while (i < dirCount);
    }
    if (dirId == FS_NO_ID) {
        args->pathLength = 0;
        return FS_RESULT_NO_ENTRY;
    }
    child = dirId;
    length = NNS_STD_StrLen(func_02070a24(arc)) + 2;
    func_02071340(&dir, dirId);
    if (fileId != FS_NO_ID) {
        length += entry.nameLength;
    }
    if (dirId != 0) {
        do {
            func_02071340(&dir, dir.body.dir.parentId);
            dir.arg.readDirectory.entry = &entry;
            dir.arg.readDirectory.skipName = TRUE;
            if (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                do {
                    if (entry.isDirectory && entry.id.dir.dirId == child) {
                        length += entry.nameLength + 1;
                        break;
                    }
                } while (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
            }
            child = dir.body.dir.pos.dirId;
        } while (child != 0);
    }
    args->pathLength = length + 1;
    args->dirId = dirId;
    buffer = args->buffer;
    if (buffer != NULL) {
        pathLength = args->pathLength;
        if (args->length >= pathLength) {
            int pos = 0;

            name = func_02070a24(arc);
            length = NNS_STD_StrLen(name);
            sys_memcpy(name, buffer + pos, length);
            pos += length;
            sys_memcpy(":/", buffer + pos, 2);
            func_02071340(&dir, dirId);
            if (fileId != FS_NO_ID) {
                dir.arg.readDirectory.entry = &entry;
                dir.arg.readDirectory.skipName = FALSE;
                if (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                    do {
                        if (entry.isDirectory == FALSE && entry.id.file.file_id == fileId) {
                            break;
                        }
                    } while (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
                }
                // fileId has done its work, and holds the size of the name
                fileId = entry.nameLength + 1;
                sys_memcpy(entry.name, buffer + pathLength - fileId, fileId);
                pathLength -= fileId;
            } else {
                *(buffer + pathLength - 1) = '\0';
                pathLength--;
            }
            if (dirId != 0) {
                do {
                    func_02071340(&dir, dir.body.dir.parentId);
                    dir.arg.readDirectory.skipName = FALSE;
                    dir.arg.readDirectory.entry = &entry;
                    *(buffer + pathLength - 1) = '/';
                    pathLength--;
                    if (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                        do {
                            if (entry.isDirectory && entry.id.dir.dirId == dirId) {
                                // dirId is set again below, and holds the length of the name until then
                                dirId = entry.nameLength;
                                sys_memcpy(entry.name, buffer + pathLength - dirId, dirId);
                                pathLength -= dirId;
                                break;
                            }
                        } while (fs_call_filecmd(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
                    }
                    dirId = dir.body.dir.pos.dirId;
                } while (dirId != 0);
            }
        }
    }
    return FS_RESULT_SUCCESS;
}

// The default command FS_COMMAND_OPENFILEFAST: reads the file's entry of the allocation table and opens it by its range
static FSResult func_0207181c(FSFile *file) {
    FSArchive *arc = file->arc;
    u32 fileId = file->arg.openFileFast.file_id;
    FSArchiveTables *tables = arc->userdata;
    u32 offset = fileId * sizeof(FSFatEntry);
    FSFatEntry entry;
    FSTableReader reader;
    FSResult result;

    if (offset >= tables->fatSize) {
        return FS_RESULT_NO_ENTRY;
    }
    reader.arc = arc;
    reader.position = tables->fat + offset;
    result = func_0207130c(&reader, &entry, sizeof(entry));
    if (result == FS_RESULT_SUCCESS) {
        file->arg.openFileDirect.top = entry.top;
        file->arg.openFileDirect.bottom = entry.bottom;
        file->arg.openFileDirect.fileId = fileId;
        result = fs_call_filecmd(file, FS_COMMAND_OPENFILEDIRECT, TRUE);
    }
    return result;
}

// The default command FS_COMMAND_OPENFILEDIRECT: makes the file a range of the archive, with its position at the start
static FSResult func_02071864(FSFile *file) {
    file->body.file.top = file->arg.openFileDirect.top;
    file->body.file.position = file->arg.openFileDirect.top;
    file->body.file.bottom = file->arg.openFileDirect.bottom;
    file->body.file.fileId = file->arg.openFileDirect.fileId;
    return FS_RESULT_SUCCESS;
}

// The default command FS_COMMAND_READFILE: reads from the archive at the file's position, and moves it on
static FSResult func_02071878(FSFile *file) {
    FSArchive *arc = file->arc;
    u32 position = file->body.file.position;
    u32 length = file->arg.readFile.length;
    FSArchiveTables *tables = arc->userdata;
    void *buffer = file->arg.readFile.buffer;

    file->body.file.position = position + length;
    return tables->readFunc(arc, buffer, position, length);
}

// The default command FS_COMMAND_WRITEFILE: the same, to the archive
static FSResult func_02071890(FSFile *file) {
    FSArchive *arc = file->arc;
    u32 position = file->body.file.position;
    u32 length = file->arg.writeFile.length;
    FSArchiveTables *tables = arc->userdata;
    const void *buffer = file->arg.writeFile.buffer;

    file->body.file.position = position + length;
    return tables->writeFunc(arc, buffer, position, length);
}

// The default command for the events and for FS_COMMAND_CLOSEFILE, which have nothing to do
static FSResult func_020718a8(FSFile *file) {
    return FS_RESULT_SUCCESS;
}

// The default commands, by command number
static FSResult (*const FS_COMMAND_DEFAULT[13])(FSFile *file) = {
    func_02071878, func_02071890, func_02071358, func_020713bc, func_02071468, func_020715d0, func_0207181c,
    func_02071864, func_020718a8, func_020718a8, func_020718a8, func_020718a8, func_020718a8,
};

// Runs a command of a file, through the archive's hook if it has one for the command, and otherwise (or if the hook
// leaves it) through the default commands. With blocking set, it waits for a command that went on apart from the
// thread to end
static FSResult fs_call_filecmd(FSFile *file, u32 command, BOOL blocking) {
    FSArchiveTables *tables = file->arc->userdata;
    FSResult result = FS_RESULT_PROC_UNKNOWN;

    if (tables->procMask & (1 << command)) {
        result = tables->proc(file, command);
        switch (result) {
        case FS_RESULT_SUCCESS:
        case FS_RESULT_FAILURE:
        case FS_RESULT_UNSUPPORTED:
        case FS_RESULT_PROC_ASYNC:
            break;
        case FS_RESULT_PROC_DEFAULT:
            tables->procMask &= ~(1 << command);
            result = FS_RESULT_PROC_UNKNOWN;
            break;
        }
    }
    if (result == FS_RESULT_PROC_UNKNOWN) {
        if (command < 13) {
            result = FS_COMMAND_DEFAULT[command](file);
        } else {
            result = FS_RESULT_UNSUPPORTED;
        }
    }
    if (blocking) {
        result = func_02070130(file, result);
    }
    return result;
}

// FS_COMMAND_READFILE of the archive's interface: asks the archive to read from the file, as much as it has left
static FSResult func_02071940(FSArchive *arc, FSFile *file, void *buffer, u32 *length) {
    FSFileRange *range = &file->prop->file;
    u32 left = range->bottom - range->position;
    u32 requested = *length;

    if (requested > left) {
        *length = left;
    }
    file->arg.readFile.buffer = buffer;
    file->arg.readFile.requested = requested;
    file->arg.readFile.length = *length;
    return fs_call_filecmd(file, FS_COMMAND_READFILE, FALSE);
}

// FS_COMMAND_WRITEFILE of the interface
static FSResult func_02071968(FSArchive *arc, FSFile *file, const void *buffer, u32 *length) {
    FSFileRange *range = &file->prop->file;
    u32 left = range->bottom - range->position;
    u32 requested = *length;

    if (requested > left) {
        *length = left;
    }
    file->arg.writeFile.buffer = buffer;
    file->arg.writeFile.requested = requested;
    file->arg.writeFile.length = *length;
    return fs_call_filecmd(file, FS_COMMAND_WRITEFILE, FALSE);
}

// FS_COMMAND_SEEKDIR of the interface: opens the directory with the number that the low half of id says
// (the high half is the number of the file that comes next), at the position
static FSResult func_02071990(FSArchive *arc, FSFile *file, u32 id, u32 position) {
    FSDirPos *pos = &file->arg.seekDirectory;
    FSResult result;

    pos->dirId = id;
    pos->fileId = id >> 16;
    pos->position = position;
    file->arc = arc;
    pos->arc = arc;
    result = fs_call_filecmd(file, FS_COMMAND_SEEKDIR, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        file->stat |= FS_FILE_STATUS_IS_DIR;
        file->stat &= ~FS_FILE_STATUS_IS_FILE;
        file->arc = arc;
        file->prop = &file->body;
    }
    return result;
}

// FS_COMMAND_READDIR of the interface
static FSResult func_020719c8(FSArchive *arc, FSFile *file, FSDirectoryEntryInfo *info) {
    FSDirReadArgs *args = &file->arg.readDirectory;
    FSDirEntry entry;
    FSResult result;

    args->entry = &entry;
    args->skipName = FALSE;
    result = fs_call_filecmd(file, FS_COMMAND_READDIR, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        info->shortnameLength = 0;
        info->longnameLength = entry.nameLength;
        sys_memcpy(entry.name, info->longname, info->longnameLength);
        info->longname[info->longnameLength] = '\0';
        if (entry.isDirectory) {
            info->attributes = FS_ATTRIBUTE_IS_DIRECTORY;
            info->id = entry.id.dir.dirId | entry.id.dir.fileId << 16;
            info->filesize = 0;
        } else {
            FSArchiveTables *tables;
            u32 offset;

            info->attributes = 0;
            info->id = entry.id.file.file_id;
            info->filesize = 0;
            tables = arc->userdata;
            offset = info->id * sizeof(FSFatEntry);
            if (offset < tables->fatSize) {
                FSFatEntry fat;
                FSTableReader reader;

                reader.arc = arc;
                reader.position = tables->fat + offset;
                if (func_0207130c(&reader, &fat, sizeof(fat)) == FS_RESULT_SUCCESS) {
                    info->filesize = fat.bottom - fat.top;
                    if (func_02072130(arc, fat.top)) {
                        info->attributes |= FS_ATTRIBUTE_IS_PROTECTED;
                    }
                }
            }
        }
        info->mtime.year = 0;
        info->mtime.month = 0;
        info->mtime.day = 0;
        info->mtime.hour = 0;
        info->mtime.minute = 0;
        info->mtime.second = 0;
    }
    return result;
}

// FS_COMMAND_FINDPATH of the interface: finds the directory or file that a path names from a directory, and gives its
// number (of a directory: the low half of the word that FS_COMMAND_SEEKDIR takes)
static FSResult func_02071aac(FSArchive *arc, u32 baseDirId, const char *path, u32 *targetId, BOOL isDirectory) {
    FSFile file;
    FSEntryId id;
    FSResult result;

    finit(&file);
    file.arg.findPath.start.dirId = baseDirId;
    file.arg.findPath.start.fileId = 0;
    file.arg.findPath.start.position = 0;
    file.arg.findPath.isDirectory = isDirectory;
    file.arg.findPath.result = &id;
    file.arc = arc;
    file.arg.findPath.start.arc = arc;
    file.arg.findPath.path = path;
    result = fs_call_filecmd(&file, FS_COMMAND_FINDPATH, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        if (isDirectory) {
            *targetId = id.dir.dirId;
        } else {
            *targetId = id.file.file_id;
        }
    }
    return result;
}

// FS_COMMAND_GETPATH of the interface. The command leaves the length of the path in pathLength, and what this gives
// back to the caller is the size of the buffer that it started with
static FSResult func_02071afc(FSArchive *arc, FSFile *file, u32 arg0, char *buffer, u32 *length) {
    FSPathBuffer *args = &file->arg.getPath;
    FSResult result;

    args->buffer = buffer;
    args->pathLength = 0;
    args->dirId = 0;
    args->length = *length;
    result = fs_call_filecmd(file, FS_COMMAND_GETPATH, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        *length = args->length;
    }
    return result;
}

// FS_COMMAND_OPENFILEFAST of the interface
static FSResult func_02071b24(FSArchive *arc, FSFile *file, u32 fileId, u32 arg1) {
    FSFileID *args = &file->arg.openFileFast;
    FSResult result;

    args->file_id = fileId;
    args->arc = arc;
    result = fs_call_filecmd(file, FS_COMMAND_OPENFILEFAST, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        file->stat |= FS_FILE_STATUS_IS_FILE;
        file->stat &= ~FS_FILE_STATUS_IS_DIR;
        file->arc = arc;
        file->prop = &file->body;
    }
    return result;
}

// FS_COMMAND_OPENFILEDIRECT of the interface
static FSResult func_02071b58(FSArchive *arc, FSFile *file, u32 top, u32 bottom, u32 *fileId) {
    FSFileRangeArgs *args = &file->arg.openFileDirect;
    FSResult result;

    args->top = top;
    args->bottom = bottom;
    args->fileId = *fileId;
    result = fs_call_filecmd(file, FS_COMMAND_OPENFILEDIRECT, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        file->stat |= FS_FILE_STATUS_IS_FILE;
        file->stat &= ~FS_FILE_STATUS_IS_DIR;
        file->arc = arc;
        file->prop = &file->body;
    }
    return result;
}

// FS_COMMAND_CLOSEFILE of the interface
static FSResult func_02071b90(FSArchive *arc, FSFile *file) {
    FSResult result = fs_call_filecmd(file, FS_COMMAND_CLOSEFILE, TRUE);

    file->prop = NULL;
    file->stat &= ~(FS_FILE_STATUS_IS_FILE | FS_FILE_STATUS_IS_DIR);
    return result;
}

// The events of the interface, which run FS_COMMAND_ACTIVATE, FS_COMMAND_IDLE, FS_COMMAND_SUSPEND and
// FS_COMMAND_RESUME through the archive's hook, on a file of no use but to carry the archive
static void func_02071bac(FSArchive *arc) {
    FSFile file;

    finit(&file);
    file.arc = arc;
    fs_call_filecmd(&file, FS_COMMAND_ACTIVATE, FALSE);
}

static void func_02071bcc(FSArchive *arc) {
    FSFile file;

    finit(&file);
    file.arc = arc;
    fs_call_filecmd(&file, FS_COMMAND_IDLE, FALSE);
}

static void func_02071bec(FSArchive *arc) {
    FSFile file;

    finit(&file);
    file.arc = arc;
    fs_call_filecmd(&file, FS_COMMAND_SUSPEND, FALSE);
}

static void func_02071c0c(FSArchive *arc) {
    FSFile file;

    finit(&file);
    file.arc = arc;
    fs_call_filecmd(&file, FS_COMMAND_RESUME, FALSE);
}

// FS_COMMAND_OPENFILE of the interface: finds the file that a path names, and opens it
static FSResult func_02071c2c(FSArchive *arc, FSFile *file, u32 baseDirId, const char *path, u32 mode) {
    u32 fileId;
    FSResult result = func_02071aac(arc, baseDirId, path, &fileId, FALSE);

    if (result == FS_RESULT_SUCCESS) {
        result = func_02071b24(arc, file, fileId, mode);
    }
    return result;
}

// FS_COMMAND_SEEKFILE of the interface: moves the position of the file, from its start, its position or its end, and
// gives the position
static FSResult func_02071c58(FSArchive *arc, FSFile *file, s32 *offset, int whence) {
    FSFileRange *range = &file->prop->file;
    s32 position = *offset;

    switch (whence) {
    case 0:
        position += range->top;
        break;
    case 1:
    default:
        position += range->position;
        break;
    case 2:
        position += range->bottom;
        break;
    }
    if (position < range->top || position > range->bottom) {
        return FS_RESULT_INVALID_PARAMETER;
    }
    range->position = position;
    *offset = position;
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_GETFILELENGTH of the interface
static FSResult func_02071c90(FSArchive *arc, FSFile *file, u32 *length) {
    FSFileRange *range = &file->prop->file;

    *length = range->bottom - range->top;
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_GETFILEPOSITION of the interface
static FSResult func_02071ca0(FSArchive *arc, FSFile *file, u32 *position) {
    FSFileRange *range = &file->prop->file;

    *position = range->position - range->top;
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_UNMOUNT of the interface: forgets the tables. It also asks whether they were loaded, and does nothing
// with the answer
static void func_02071cb0(FSArchive *arc) {
    FSArchiveTables *tables = arc->userdata;

    func_02072000(arc);
    tables->base = 0;
    tables->fat = 0;
    tables->fatSize = 0;
    tables->fnt = 0;
    tables->fntSize = 0;
    tables->fatBackup = 0;
    tables->fntBackup = 0;
}

// FS_COMMAND_19 of the interface: gives 0 (the game never sends it)
static FSResult func_02071ccc(FSArchive *arc, u32 *value) {
    *value = 0;
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_OPENDIRECTORY of the interface: finds the directory that a path names, and opens it
static FSResult func_02071cd4(FSArchive *arc, FSFile *file, u32 baseDirId, const char *path, u32 arg2) {
    u32 dirId = 0;
    FSResult result = func_02071aac(arc, baseDirId, path, &dirId, TRUE);

    if (result == FS_RESULT_SUCCESS) {
        result = func_02071990(arc, file, dirId, 0);
    }
    return result;
}

// FS_COMMAND_CLOSEDIRECTORY of the interface
static FSResult func_02071d04(FSArchive *arc, FSFile *file) {
    file->prop = NULL;
    file->stat &= ~(FS_FILE_STATUS_IS_FILE | FS_FILE_STATUS_IS_DIR);
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_GETPATHINFO of the interface
static FSResult func_02071d14(FSArchive *arc, u32 baseDirId, const char *path, FSPathInfo *info) {
    u32 id = 0;
    FSResult result = FS_RESULT_ERROR;

    sys_memset(info, 0, sizeof(*info));
    if (func_02071aac(arc, baseDirId, path, &id, TRUE) == FS_RESULT_SUCCESS) {
        info->attributes = FS_ATTRIBUTE_IS_DIRECTORY;
        info->id = id;
        result = FS_RESULT_SUCCESS;
    } else if (func_02071aac(arc, baseDirId, path, &id, FALSE) == FS_RESULT_SUCCESS) {
        FSArchiveTables *tables;
        u32 offset;

        info->attributes = 0;
        info->id = id;
        info->filesize = 0;
        tables = arc->userdata;
        offset = id * sizeof(FSFatEntry);
        if (offset < tables->fatSize) {
            FSFatEntry fat;
            FSTableReader reader;

            reader.arc = arc;
            reader.position = tables->fat + offset;
            if (func_0207130c(&reader, &fat, sizeof(fat)) == FS_RESULT_SUCCESS) {
                info->filesize = fat.bottom - fat.top;
                if (func_02072130(arc, fat.top)) {
                    info->attributes |= FS_ATTRIBUTE_IS_PROTECTED;
                }
            }
        }
        result = FS_RESULT_SUCCESS;
    }
    info->attributes |= FS_ATTRIBUTE_IS_ARCHIVE_ENTRY;
    return result;
}

// FS_COMMAND_GETARCHIVERESOURCE of the interface: the archive is the ROM, as big as the ROM header says
static FSResult func_02071dc0(FSArchive *arc, FSArchiveResource *resource) {
    const ROMHeader *header = func_0206eed8();

    resource->unk_20[0] = 0;
    resource->unk_20[1] = 0;
    resource->unk_20[2] = 0;
    resource->unk_20[3] = 0;
    resource->totalSize = header->romSize;
    resource->availableSize = 0;
    resource->maxFileSize = 0x7fffffff;
    resource->maxDirectorySize = 0x7fffffff;
    return FS_RESULT_SUCCESS;
}

// The interface of an archive that has tables
static const FSArchiveInterface FS_VTABLE_BASE = {
    .readFile = func_02071940,
    .writeFile = func_02071968,
    .seekDirectory = func_02071990,
    .readDirectory = func_020719c8,
    .findPath = func_02071aac,
    .getPath = func_02071afc,
    .openFileFast = func_02071b24,
    .openFileDirect = func_02071b58,
    .closeFile = func_02071b90,
    .activate = func_02071bac,
    .idle = func_02071bcc,
    .suspend = func_02071bec,
    .resume = func_02071c0c,
    .openFile = func_02071c2c,
    .seekFile = func_02071c58,
    .getFileLength = func_02071c90,
    .getFilePosition = func_02071ca0,
    .unmount = func_02071cb0,
    .command19 = func_02071ccc,
    .getPathInfo = func_02071d14,
    .getArchiveResource = func_02071dc0,
    .openDirectory = func_02071cd4,
    .closeDirectory = func_02071d04,
};

// The default functions to read and write an archive: it is in memory, at the base
static FSResult func_02071df0(FSArchive *arc, void *buffer, u32 position, u32 length) {
    sys_memcpy(func_02071ff8(arc, position), buffer, length);
    return FS_RESULT_SUCCESS;
}

static FSResult func_02071e08(FSArchive *arc, const void *buffer, u32 position, u32 length) {
    sys_memcpy(buffer, func_02071ff8(arc, position), length);
    return FS_RESULT_SUCCESS;
}

// NitroSDK's FS_LoadArchive: gives an archive its tables, in the storage at the base or in memory, and the functions
// to read and write it (the defaults for NULL), and mounts it
BOOL fs_bind(FSArchive *arc, u32 base, u32 fat, u32 fatSize, u32 fnt, u32 fntSize, FSReadFunc readFunc,
             FSWriteFunc writeFunc) {
    FSArchiveTables *tables = &arc->tables;

    tables->base = base;
    tables->fatSize = fatSize;
    tables->fntSize = fntSize;
    tables->fat = fat;
    tables->fnt = fnt;
    tables->fntBackup = fnt;
    tables->fatBackup = fat;
    tables->readFunc = readFunc != NULL ? readFunc : func_02071df0;
    tables->writeFunc = writeFunc != NULL ? writeFunc : func_02071e08;
    tables->loadedMemory = NULL;
    return func_02070a30(arc, tables, &FS_VTABLE_BASE);
}

// NitroSDK's FS_UnloadArchive: unmounts the archive
BOOL func_02071e6c(FSArchive *arc) {
    return func_02070a5c(arc);
}

// The length of a file whose archive has this interface, without sending a command to the archive. Returns whether
// it could
BOOL func_02071e74(FSFile *file, u32 *length) {
    BOOL handled = FALSE;

    if (file->arc->vtbl == &FS_VTABLE_BASE) {
        if (func_02071c90(file->arc, file, length) == FS_RESULT_SUCCESS) {
            handled = TRUE;
        }
    }
    return handled;
}

// and the position in it
BOOL func_02071e9c(FSFile *file, u32 *position) {
    BOOL handled = FALSE;

    if (file->arc->vtbl == &FS_VTABLE_BASE) {
        if (func_02071ca0(file->arc, file, position) == FS_RESULT_SUCCESS) {
            handled = TRUE;
        }
    }
    return handled;
}

// NitroSDK's FS_SetArchiveProc: sets the hook, and the commands that it is run for; neither goes without the other
void func_02071ec4(FSArchive *arc, FSProc proc, u32 procMask) {
    FSArchiveTables *tables = &arc->tables;

    if (procMask == 0) {
        proc = NULL;
    } else if (proc == NULL) {
        procMask = 0;
    }
    tables->proc = proc;
    tables->procMask = procMask;
}

// NitroSDK's FS_LoadArchiveTables: copies the tables into the memory (aligned up to 32 bytes) and keeps reading them
// from there. Returns the size of memory they need, and loads them only if the memory has that much
u32 func_02071edc(FSArchive *arc, void *memory, u32 memorySize) {
    FSArchiveTables *tables;
    u32 size;
    u32 aligned;
    FSFile file;

    if (memory != NULL && func_02072000(arc)) {
        func_02071fa0(arc);
    }
    tables = arc->userdata;
    size = (tables->fatSize + tables->fntSize + 0x3f) & ~0x1f;
    if (size <= memorySize) {
        aligned = ((u32)memory + 0x1f) & ~0x1f;
        finit(&file);
        if (fopen_raw(&file, arc, tables->fat, tables->fat + tables->fatSize, -1)) {
            if (romfs_fread(&file, (void *)aligned, tables->fatSize) < 0) {
                sys_memset((void *)aligned, 0, tables->fatSize);
            }
            romfs_fclose(&file);
        }
        tables->fat = aligned;
        aligned += tables->fatSize;
        if (fopen_raw(&file, arc, tables->fnt, tables->fnt + tables->fntSize, -1)) {
            if (romfs_fread(&file, (void *)aligned, tables->fntSize) < 0) {
                sys_memset((void *)aligned, 0, tables->fntSize);
            }
            romfs_fclose(&file);
        }
        tables->fnt = aligned;
        tables->loadedMemory = memory;
        arc->flag |= FS_ARCHIVE_FLAG_TABLE_LOADED;
    }
    return size;
}

// NitroSDK's FS_UnloadArchiveTables: reads the tables from the storage again, and returns the memory they were in
void *func_02071fa0(FSArchive *arc) {
    void *memory = NULL;

    if (FS_IsArchiveMounted(arc)) {
        FSArchiveTables *tables = arc->userdata;
        BOOL suspended = func_02070ad4(arc);

        if (func_02072000(arc)) {
            arc->flag &= ~FS_ARCHIVE_FLAG_TABLE_LOADED;
            memory = tables->loadedMemory;
            tables->fat = tables->fatBackup;
            tables->fnt = tables->fntBackup;
            tables->loadedMemory = NULL;
        }
        if (suspended) {
            func_02070b44(arc);
        }
    }
    return memory;
}

// NitroSDK's FS_GetArchiveBase
u32 func_02071ff0(const FSArchive *arc) {
    FSArchiveTables *tables = arc->userdata;

    return tables->base;
}

// The memory that an offset of the archive is at, when the archive is in memory at its base
static void *func_02071ff8(const FSArchive *arc, u32 offset) {
    FSArchiveTables *tables = arc->userdata;

    return (void *)(tables->base + offset);
}

// NitroSDK's FS_IsArchiveTableLoaded
static BOOL func_02072000(const FSArchive *arc) {
    return (arc->flag & FS_ARCHIVE_FLAG_TABLE_LOADED) != 0;
}

u32 func_02072010(const FSFile *file) {
    return file->body.file.top;
}
