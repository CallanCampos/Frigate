// NitroSDK's file system (libfs): an archive that has a file allocation table and a name table, such as the ROM's. The
// commands that it runs itself when its hook leaves them to it (the default commands, on the tables), the functions
// of its interface that run them, and the functions that set such an archive up and load its tables into memory. The
// file name is NitroSDK's (fs_command_default.c, as ntrtwl's reconstruction of NitroSDK has it); no string in the ROM
// names it. The default commands (FSi_*Command), their table fsi_default_command and FSi_TranslateCommand have
// NitroSDK's names, as that reconstruction has them; the interface's FSi_ROMFAT_* functions have TwlSDK's names as they
// are recalled from the SDK, which the reference, older than TwlSDK, can't confirm; FS_GetArchiveOffset is ours.

#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/std.h"
#ifdef PLATFORM_PC
#include "port/hal/fs/port_fs_tables.h"
#endif

// Where the next bytes of the archive's tables are read from
typedef struct FSiSyncReadParam {
    FSArchive *arc;
    u32 pos;
} FSiSyncReadParam;

// An entry of the file allocation table: where a file starts and ends in the archive
typedef struct FSArchiveFAT {
    u32 top;
    u32 bottom;
} FSArchiveFAT;

// An entry of the name table's main table, one for each directory: where its entries start in the name table, the
// number of its first file, and its parent (of the root: the number of directories)
typedef struct FSArchiveFNT {
    u32 start;
    u16 index;
    u16 parent;
} FSArchiveFNT;

// The number that says there is no file, or no directory
#define FS_NO_ID 0x10000

static FSResult FSi_TranslateCommand(FSFile *file, FSCommandType command, BOOL blocking); // swan's fs_call_filecmd
static void *FS_GetArchiveOffset(const FSArchive *arc, u32 offset);
static BOOL FS_IsArchiveTableLoaded(const FSArchive *arc);

// Reads length bytes of the archive's tables at the reader's position, and moves the position on. The tables are read
// through the archive's read function, or copied from memory once they were loaded into it
static FSResult FSi_ReadTable(FSiSyncReadParam *reader, void *buffer, u32 length) {
    FSArchive *arc = reader->arc;
    FSArchiveTables *tables = arc->userdata;
    FSResult result;

    if (tables->load_mem != NULL) {
#ifdef PLATFORM_PC
        // Cached positions stay archive offsets; the host resolves them through the full native cache pointer.
        result = FSi_HostReadArchiveTable(arc, buffer, reader->pos, length);
#else
        MI_CpuCopy8((void *)reader->pos, buffer, length);
        result = FS_RESULT_SUCCESS;
#endif
    } else {
        result = tables->read_func(arc, buffer, reader->pos, length);
        result = FSi_WaitForArchiveCompletion(arc->list, result);
    }
    reader->pos += length;
    return result;
}

// Sets the directory that a command of the file works on: the one with the number dirId, at its start
static FSResult FSi_SeekDirDirect(FSFile *file, u32 dirId) {
    file->arg.seekDirectory.arc = file->arc;
    file->arg.seekDirectory.own_id = dirId;
    file->arg.seekDirectory.index = 0;
    file->arg.seekDirectory.pos = 0;
    return FSi_TranslateCommand(file, FS_COMMAND_SEEKDIR, TRUE);
}

// The default command FS_COMMAND_SEEKDIR: puts the file in the directory at the position it holds. A position that is
// the start of the directory takes its first file and its entries from the directory's main table entry
static FSResult FSi_SeekDirCommand(FSFile *file) {
    FSArchive *arc = file->arc;
    FSArchiveTables *tables = arc->userdata;
    FSDirPos *pos = &file->arg.seekDirectory;
    FSArchiveFNT entry;
    FSiSyncReadParam reader;
    FSResult result;

    reader.arc = arc;
    reader.pos = tables->fnt + pos->own_id * sizeof(FSArchiveFNT);
    result = FSi_ReadTable(&reader, &entry, sizeof(entry));
    if (result == FS_RESULT_SUCCESS) {
        file->body.dir.pos = *pos;
        if (pos->index == 0 && pos->pos == 0) {
            file->body.dir.pos.index = entry.index;
            file->body.dir.pos.pos = tables->fnt + entry.start;
        }
        file->body.dir.parentId = entry.parent & 0xfff;
    }
    return result;
}

// The default command FS_COMMAND_READDIR: reads the next entry of the file's directory into the entry that the
// command holds, with its name unless the command skips it. A length of 0 ends the directory
static FSResult FSi_ReadDirCommand(FSFile *file) {
    FSDirEntry *entry = file->arg.readDirectory.entry;
    FSiSyncReadParam reader;
    u8 info;
    u16 dirId;
    FSResult result;

    reader.arc = file->arc;
    reader.pos = file->body.dir.pos.pos;
    result = FSi_ReadTable(&reader, &info, 1);
    if (result == FS_RESULT_SUCCESS) {
        entry->name_len = info & 0x7f;
        entry->is_directory = (info >> 7) & 1;
        if (entry->name_len == 0) {
            return FS_RESULT_FAILURE;
        }
        if (file->arg.readDirectory.skipName == FALSE) {
            result = FSi_ReadTable(&reader, entry->name, entry->name_len);
            if (result != FS_RESULT_SUCCESS) {
                return result;
            }
            entry->name[entry->name_len] = '\0';
        } else {
            reader.pos += entry->name_len;
        }
        if (entry->is_directory == FALSE) {
            entry->id.file.arc = file->arc;
            entry->id.file.file_id = file->body.dir.pos.index;
            file->body.dir.pos.index++;
        } else {
            result = FSi_ReadTable(&reader, &dirId, 2);
            if (result == FS_RESULT_SUCCESS) {
                entry->id.dir.arc = file->arc;
                entry->id.dir.own_id = dirId & 0xfff;
                entry->id.dir.index = 0;
                entry->id.dir.pos = 0;
            }
        }
        if (result == FS_RESULT_SUCCESS) {
            file->body.dir.pos.pos = reader.pos;
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
static FSResult FSi_FindPathCommand(FSFile *file) {
    const char *path = file->arg.findPath.path;
    BOOL isDirectory = file->arg.findPath.isDirectory;
    FSDirEntry entry;
    BOOL wantDirectory;
    int length;

    FSi_TranslateCommand(file, FS_COMMAND_SEEKDIR, TRUE);
    for (; *path != '\0'; path += (*path != '\0') ? 1 : 0) {
        char c;

        length = FSi_IncrementSjisPositionToSlash(path, 0);
        wantDirectory = path[length] != '\0' || isDirectory;
        if (length == 0) {
            return FS_RESULT_INVALID_PARAMETER;
        }
        c = path[0];
        if (c == '.') {
            if (length == 1) {
                path += 1;
                continue;
            } else if (length == 2 && path[1] == '.') {
                if (file->body.dir.pos.own_id != 0) {
                    FSi_SeekDirDirect(file, file->body.dir.parentId);
                }
                path += 2;
                continue;
            }
        } else if (c == '*') {
            break;
        }
        if (length > FS_FILE_NAME_MAX) {
            return FS_RESULT_NO_ENTRY;
        }
        file->arg.readDirectory.entry = &entry;
        file->arg.readDirectory.skipName = FALSE;
        for (;;) {
            if (FSi_TranslateCommand(file, FS_COMMAND_READDIR, TRUE) != FS_RESULT_SUCCESS) {
                return FS_RESULT_NO_ENTRY;
            }
            if (wantDirectory != entry.is_directory || length != entry.name_len) {
                continue;
            }
            if (FSi_CompareNames(path, entry.name, length) == 0) {
                break;
            }
        }
        if (wantDirectory) {
            file->arg.seekDirectory = entry.id.dir;
            path += length;
            FSi_TranslateCommand(file, FS_COMMAND_SEEKDIR, TRUE);
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
static FSResult FSi_GetPathCommand(FSFile *file) {
    const char *name;
    FSArchive *arc = file->arc;
    FSPathBuffer *args = &file->arg.getPath;
    u32 i;
    u32 dirCount;
    u32 length;
    char *buffer;
    u32 pathLength;
    // The order of these declarations sets the locals' registers
    u32 fileId;
    u32 dirId;
    u32 child;
    FSDirEntry entry;
    FSFile dir;

    FS_InitFile(&dir);
    dir.arc = arc;
    if (FS_IsDir(file)) {
        dirId = file->body.dir.pos.own_id;
        fileId = FS_NO_ID;
    } else {
        fileId = file->body.file.fileId;
        i = 0;
        dirCount = 0;
        dirId = FS_NO_ID;
        do {
            FSi_SeekDirDirect(&dir, i);
            if (i == 0) {
                dirCount = dir.body.dir.parentId;
            }
            dir.arg.readDirectory.entry = &entry;
            dir.arg.readDirectory.skipName = TRUE;
            if (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                do {
                    if (entry.is_directory == FALSE && entry.id.file.file_id == fileId) {
                        dirId = dir.body.dir.pos.own_id;
                        break;
                    }
                } while (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
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
    length = STD_GetStringLength(FS_GetArchiveName(arc)) + 2;
    FSi_SeekDirDirect(&dir, dirId);
    if (fileId != FS_NO_ID) {
        length += entry.name_len;
    }
    if (dirId != 0) {
        do {
            FSi_SeekDirDirect(&dir, dir.body.dir.parentId);
            dir.arg.readDirectory.entry = &entry;
            dir.arg.readDirectory.skipName = TRUE;
            if (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                do {
                    if (entry.is_directory && entry.id.dir.own_id == child) {
                        length += entry.name_len + 1;
                        break;
                    }
                } while (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
            }
            child = dir.body.dir.pos.own_id;
        } while (child != 0);
    }
    args->pathLength = length + 1;
    args->dirId = dirId;
    buffer = args->buffer;
    if (buffer != NULL) {
        pathLength = args->pathLength;
        if (args->length >= pathLength) {
            int pos = 0;

            name = FS_GetArchiveName(arc);
            length = STD_GetStringLength(name);
            MI_CpuCopy8(name, buffer + pos, length);
            pos += length;
            MI_CpuCopy8(":/", buffer + pos, 2);
            FSi_SeekDirDirect(&dir, dirId);
            if (fileId != FS_NO_ID) {
                dir.arg.readDirectory.entry = &entry;
                dir.arg.readDirectory.skipName = FALSE;
                if (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                    do {
                        if (entry.is_directory == FALSE && entry.id.file.file_id == fileId) {
                            break;
                        }
                    } while (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
                }
                {
                    u32 nameLength = entry.name_len + 1;

                    MI_CpuCopy8(entry.name, buffer + pathLength - nameLength, nameLength);
                    pathLength -= nameLength;
                }
            } else {
                *(buffer + pathLength - 1) = '\0';
                pathLength--;
            }
            if (dirId != 0) {
                do {
                    FSi_SeekDirDirect(&dir, dir.body.dir.parentId);
                    dir.arg.readDirectory.skipName = FALSE;
                    dir.arg.readDirectory.entry = &entry;
                    *(buffer + pathLength - 1) = '/';
                    pathLength--;
                    if (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS) {
                        do {
                            if (entry.is_directory && entry.id.dir.own_id == dirId) {
                                u32 nameLength = entry.name_len;

                                MI_CpuCopy8(entry.name, buffer + pathLength - nameLength, nameLength);
                                pathLength -= nameLength;
                                break;
                            }
                        } while (FSi_TranslateCommand(&dir, FS_COMMAND_READDIR, TRUE) == FS_RESULT_SUCCESS);
                    }
                    dirId = dir.body.dir.pos.own_id;
                } while (dirId != 0);
            }
        }
    }
    return FS_RESULT_SUCCESS;
}

// The default command FS_COMMAND_OPENFILEFAST: reads the file's entry of the allocation table and opens it by its range
static FSResult FSi_OpenFileFastCommand(FSFile *file) {
    FSArchive *arc = file->arc;
    u32 fileId = file->arg.openFileFast.file_id;
    FSArchiveTables *tables = arc->userdata;
    u32 offset = fileId * sizeof(FSArchiveFAT);
    FSArchiveFAT entry;
    FSiSyncReadParam reader;
    FSResult result;

    if (offset >= tables->fat_size) {
        return FS_RESULT_NO_ENTRY;
    }
    reader.arc = arc;
    reader.pos = tables->fat + offset;
    result = FSi_ReadTable(&reader, &entry, sizeof(entry));
    if (result == FS_RESULT_SUCCESS) {
        file->arg.openFileDirect.top = entry.top;
        file->arg.openFileDirect.bottom = entry.bottom;
        file->arg.openFileDirect.fileId = fileId;
        result = FSi_TranslateCommand(file, FS_COMMAND_OPENFILEDIRECT, TRUE);
    }
    return result;
}

// The default command FS_COMMAND_OPENFILEDIRECT: makes the file a range of the archive, with its position at the start
static FSResult FSi_OpenFileDirectCommand(FSFile *file) {
    file->body.file.top = file->arg.openFileDirect.top;
    file->body.file.position = file->arg.openFileDirect.top;
    file->body.file.bottom = file->arg.openFileDirect.bottom;
    file->body.file.fileId = file->arg.openFileDirect.fileId;
    return FS_RESULT_SUCCESS;
}

// The default command FS_COMMAND_READFILE: reads from the archive at the file's position, and moves it on
static FSResult FSi_ReadFileCommand(FSFile *file) {
    FSArchive *arc = file->arc;
    u32 position = file->body.file.position;
    u32 length = file->arg.readFile.length;
    FSArchiveTables *tables = arc->userdata;
    void *buffer = file->arg.readFile.buffer;

    file->body.file.position = position + length;
    return tables->read_func(arc, buffer, position, length);
}

// The default command FS_COMMAND_WRITEFILE: the same, to the archive
static FSResult FSi_WriteFileCommand(FSFile *file) {
    FSArchive *arc = file->arc;
    u32 position = file->body.file.position;
    u32 length = file->arg.writeFile.length;
    FSArchiveTables *tables = arc->userdata;
    const void *buffer = file->arg.writeFile.buffer;

    file->body.file.position = position + length;
    return tables->write_func(arc, buffer, position, length);
}

// The default command for the events and for FS_COMMAND_CLOSEFILE, which have nothing to do
static FSResult FSi_CloseFileCommand(FSFile *file) {
    return FS_RESULT_SUCCESS;
}

// The default commands, by command number (swan's FS_COMMAND_DEFAULT)
static FSResult (*const fsi_default_command[])(FSFile *file) = {
    FSi_ReadFileCommand,  FSi_WriteFileCommand, FSi_SeekDirCommand,      FSi_ReadDirCommand,
    FSi_FindPathCommand,  FSi_GetPathCommand,   FSi_OpenFileFastCommand, FSi_OpenFileDirectCommand,
    FSi_CloseFileCommand, FSi_CloseFileCommand, FSi_CloseFileCommand,    FSi_CloseFileCommand,
    FSi_CloseFileCommand,
};

// Runs a command of a file, through the archive's hook if it has one for the command, and otherwise (or if the hook
// leaves it) through the default commands. With blocking set, it waits for a command that went on apart from the
// thread to end
static FSResult FSi_TranslateCommand(FSFile *file, FSCommandType command, BOOL blocking) {
    FSArchiveTables *tables = file->arc->userdata;
    FSResult result = FS_RESULT_PROC_UNKNOWN;

    if (tables->proc_flag & (1 << command)) {
        result = tables->proc(file, command);
        switch (result) {
        case FS_RESULT_SUCCESS:
        case FS_RESULT_FAILURE:
        case FS_RESULT_UNSUPPORTED:
        case FS_RESULT_PROC_ASYNC:
            break;
        case FS_RESULT_PROC_DEFAULT:
            tables->proc_flag &= ~(1 << command);
            result = FS_RESULT_PROC_UNKNOWN;
            break;
        }
    }
    if (result == FS_RESULT_PROC_UNKNOWN) {
        if (command < NELEMS(fsi_default_command)) {
            result = fsi_default_command[command](file);
        } else {
            result = FS_RESULT_UNSUPPORTED;
        }
    }
    if (blocking) {
        result = FSi_WaitForArchiveCompletion(file, result);
    }
    return result;
}

// FS_COMMAND_READFILE of the archive's interface: asks the archive to read from the file, as much as it has left
static FSResult FSi_ROMFAT_ReadFile(FSArchive *arc, FSFile *file, void *buffer, u32 *length) {
    FSFileRange *range = &file->prop->file;
    u32 left = range->bottom - range->position;
    u32 requested = *length;

    if (requested > left) {
        *length = left;
    }
    file->arg.readFile.buffer = buffer;
    file->arg.readFile.requested = requested;
    file->arg.readFile.length = *length;
    return FSi_TranslateCommand(file, FS_COMMAND_READFILE, FALSE);
}

// FS_COMMAND_WRITEFILE of the interface
static FSResult FSi_ROMFAT_WriteFile(FSArchive *arc, FSFile *file, const void *buffer, u32 *length) {
    FSFileRange *range = &file->prop->file;
    u32 left = range->bottom - range->position;
    u32 requested = *length;

    if (requested > left) {
        *length = left;
    }
    file->arg.writeFile.buffer = buffer;
    file->arg.writeFile.requested = requested;
    file->arg.writeFile.length = *length;
    return FSi_TranslateCommand(file, FS_COMMAND_WRITEFILE, FALSE);
}

// FS_COMMAND_SEEKDIR of the interface: opens the directory with the number that the low half of id says
// (the high half is the number of the file that comes next), at the position
static FSResult FSi_ROMFAT_SeekDirectory(FSArchive *arc, FSFile *file, u32 id, u32 position) {
    FSDirPos *pos = &file->arg.seekDirectory;
    FSResult result;

    pos->own_id = id;
    pos->index = id >> 16;
    pos->pos = position;
    file->arc = arc;
    pos->arc = arc;
    result = FSi_TranslateCommand(file, FS_COMMAND_SEEKDIR, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        file->stat |= FS_FILE_STATUS_IS_DIR;
        file->stat &= ~FS_FILE_STATUS_IS_FILE;
        file->arc = arc;
        file->prop = &file->body;
    }
    return result;
}

// FS_COMMAND_READDIR of the interface
static FSResult FSi_ROMFAT_ReadDirectory(FSArchive *arc, FSFile *file, FSDirectoryEntryInfo *info) {
    FSDirReadArgs *args = &file->arg.readDirectory;
    FSDirEntry entry;
    FSResult result;

    args->entry = &entry;
    args->skipName = FALSE;
    result = FSi_TranslateCommand(file, FS_COMMAND_READDIR, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        info->shortnameLength = 0;
        info->longnameLength = entry.name_len;
        MI_CpuCopy8(entry.name, info->longname, info->longnameLength);
        info->longname[info->longnameLength] = '\0';
        if (entry.is_directory) {
            info->attributes = FS_ATTRIBUTE_IS_DIRECTORY;
            info->id = entry.id.dir.own_id | entry.id.dir.index << 16;
            info->filesize = 0;
        } else {
            FSArchiveTables *tables;
            u32 offset;

            info->attributes = 0;
            info->id = entry.id.file.file_id;
            info->filesize = 0;
            tables = arc->userdata;
            offset = info->id * sizeof(FSArchiveFAT);
            if (offset < tables->fat_size) {
                FSArchiveFAT fat;
                FSiSyncReadParam reader;

                reader.arc = arc;
                reader.pos = tables->fat + offset;
                if (FSi_ReadTable(&reader, &fat, sizeof(fat)) == FS_RESULT_SUCCESS) {
                    info->filesize = fat.bottom - fat.top;
                    if (FSi_IsDsiOnlyRomOffset(arc, fat.top)) {
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
static FSResult FSi_ROMFAT_FindPath(FSArchive *arc, u32 baseDirId, const char *path, u32 *targetId, BOOL isDirectory) {
    FSFile file;
    FSEntryId id;
    FSResult result;

    FS_InitFile(&file);
    file.arg.findPath.start.own_id = baseDirId;
    file.arg.findPath.start.index = 0;
    file.arg.findPath.start.pos = 0;
    file.arg.findPath.isDirectory = isDirectory;
    file.arg.findPath.result = &id;
    file.arc = arc;
    file.arg.findPath.start.arc = arc;
    file.arg.findPath.path = path;
    result = FSi_TranslateCommand(&file, FS_COMMAND_FINDPATH, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        if (isDirectory) {
            *targetId = id.dir.own_id;
        } else {
            *targetId = id.file.file_id;
        }
    }
    return result;
}

// FS_COMMAND_GETPATH of the interface. The command leaves the length of the path in pathLength, and what this gives
// back to the caller is the size of the buffer that it started with
static FSResult FSi_ROMFAT_GetPath(FSArchive *arc, FSFile *file, u32 arg0, char *buffer, u32 *length) {
    FSPathBuffer *args = &file->arg.getPath;
    FSResult result;

    args->buffer = buffer;
    args->pathLength = 0;
    args->dirId = 0;
    args->length = *length;
    result = FSi_TranslateCommand(file, FS_COMMAND_GETPATH, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        *length = args->length;
    }
    return result;
}

// FS_COMMAND_OPENFILEFAST of the interface
static FSResult FSi_ROMFAT_OpenFileFast(FSArchive *arc, FSFile *file, u32 fileId, u32 arg1) {
    FSFileID *args = &file->arg.openFileFast;
    FSResult result;

    args->file_id = fileId;
    args->arc = arc;
    result = FSi_TranslateCommand(file, FS_COMMAND_OPENFILEFAST, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        file->stat |= FS_FILE_STATUS_IS_FILE;
        file->stat &= ~FS_FILE_STATUS_IS_DIR;
        file->arc = arc;
        file->prop = &file->body;
    }
    return result;
}

// FS_COMMAND_OPENFILEDIRECT of the interface
static FSResult FSi_ROMFAT_OpenFileDirect(FSArchive *arc, FSFile *file, u32 top, u32 bottom, u32 *fileId) {
    FSFileRangeArgs *args = &file->arg.openFileDirect;
    FSResult result;

    args->top = top;
    args->bottom = bottom;
    args->fileId = *fileId;
    result = FSi_TranslateCommand(file, FS_COMMAND_OPENFILEDIRECT, TRUE);
    if (result == FS_RESULT_SUCCESS) {
        file->stat |= FS_FILE_STATUS_IS_FILE;
        file->stat &= ~FS_FILE_STATUS_IS_DIR;
        file->arc = arc;
        file->prop = &file->body;
    }
    return result;
}

// FS_COMMAND_CLOSEFILE of the interface
static FSResult FSi_ROMFAT_CloseFile(FSArchive *arc, FSFile *file) {
    FSResult result = FSi_TranslateCommand(file, FS_COMMAND_CLOSEFILE, TRUE);

    file->prop = NULL;
    file->stat &= ~(FS_FILE_STATUS_IS_FILE | FS_FILE_STATUS_IS_DIR);
    return result;
}

// The events of the interface, which run FS_COMMAND_ACTIVATE, FS_COMMAND_IDLE, FS_COMMAND_SUSPEND and
// FS_COMMAND_RESUME through the archive's hook, on a file of no use but to carry the archive
static void FSi_ROMFAT_Activate(FSArchive *arc) {
    FSFile file;

    FS_InitFile(&file);
    file.arc = arc;
    FSi_TranslateCommand(&file, FS_COMMAND_ACTIVATE, FALSE);
}

static void FSi_ROMFAT_Idle(FSArchive *arc) {
    FSFile file;

    FS_InitFile(&file);
    file.arc = arc;
    FSi_TranslateCommand(&file, FS_COMMAND_IDLE, FALSE);
}

static void FSi_ROMFAT_Suspend(FSArchive *arc) {
    FSFile file;

    FS_InitFile(&file);
    file.arc = arc;
    FSi_TranslateCommand(&file, FS_COMMAND_SUSPEND, FALSE);
}

static void FSi_ROMFAT_Resume(FSArchive *arc) {
    FSFile file;

    FS_InitFile(&file);
    file.arc = arc;
    FSi_TranslateCommand(&file, FS_COMMAND_RESUME, FALSE);
}

// FS_COMMAND_OPENFILE of the interface: finds the file that a path names, and opens it
static FSResult FSi_ROMFAT_OpenFile(FSArchive *arc, FSFile *file, u32 baseDirId, const char *path, u32 mode) {
    u32 fileId;
    FSResult result = FSi_ROMFAT_FindPath(arc, baseDirId, path, &fileId, FALSE);

    if (result == FS_RESULT_SUCCESS) {
        result = FSi_ROMFAT_OpenFileFast(arc, file, fileId, mode);
    }
    return result;
}

// FS_COMMAND_SEEKFILE of the interface: moves the position of the file, from its start, its position or its end, and
// gives the position
static FSResult FSi_ROMFAT_SeekFile(FSArchive *arc, FSFile *file, s32 *offset, int whence) {
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
static FSResult FSi_ROMFAT_GetFileLength(FSArchive *arc, FSFile *file, u32 *length) {
    FSFileRange *range = &file->prop->file;

    *length = range->bottom - range->top;
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_GETFILEPOSITION of the interface
static FSResult FSi_ROMFAT_GetFilePosition(FSArchive *arc, FSFile *file, u32 *position) {
    FSFileRange *range = &file->prop->file;

    *position = range->position - range->top;
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_UNMOUNT of the interface: forgets the tables. It also asks whether they were loaded, and does nothing
// with the answer
static void FSi_ROMFAT_Unmount(FSArchive *arc) {
    FSArchiveTables *tables = arc->userdata;

    FS_IsArchiveTableLoaded(arc);
    tables->base = 0;
    tables->fat = 0;
    tables->fat_size = 0;
    tables->fnt = 0;
    tables->fnt_size = 0;
    tables->fat_bak = 0;
    tables->fnt_bak = 0;
}

// FS_COMMAND_GETARCHIVECAPS of the interface: the ROM archive has no capabilities (the game never sends it)
static FSResult FSi_ROMFAT_GetArchiveCaps(FSArchive *arc, u32 *value) {
    *value = 0;
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_OPENDIRECTORY of the interface: finds the directory that a path names, and opens it
static FSResult FSi_ROMFAT_OpenDirectory(FSArchive *arc, FSFile *file, u32 baseDirId, const char *path, u32 arg2) {
    u32 dirId = 0;
    FSResult result = FSi_ROMFAT_FindPath(arc, baseDirId, path, &dirId, TRUE);

    if (result == FS_RESULT_SUCCESS) {
        result = FSi_ROMFAT_SeekDirectory(arc, file, dirId, 0);
    }
    return result;
}

// FS_COMMAND_CLOSEDIRECTORY of the interface
static FSResult FSi_ROMFAT_CloseDirectory(FSArchive *arc, FSFile *file) {
    file->prop = NULL;
    file->stat &= ~(FS_FILE_STATUS_IS_FILE | FS_FILE_STATUS_IS_DIR);
    return FS_RESULT_SUCCESS;
}

// FS_COMMAND_GETPATHINFO of the interface
static FSResult FSi_ROMFAT_GetPathInfo(FSArchive *arc, u32 baseDirId, const char *path, FSPathInfo *info) {
    u32 id = 0;
    FSResult result = FS_RESULT_ERROR;

    MI_CpuFill8(info, 0, sizeof(*info));
    if (FSi_ROMFAT_FindPath(arc, baseDirId, path, &id, TRUE) == FS_RESULT_SUCCESS) {
        info->attributes = FS_ATTRIBUTE_IS_DIRECTORY;
        info->id = id;
        result = FS_RESULT_SUCCESS;
    } else if (FSi_ROMFAT_FindPath(arc, baseDirId, path, &id, FALSE) == FS_RESULT_SUCCESS) {
        FSArchiveTables *tables;
        u32 offset;

        info->attributes = 0;
        info->id = id;
        info->filesize = 0;
        tables = arc->userdata;
        offset = id * sizeof(FSArchiveFAT);
        if (offset < tables->fat_size) {
            FSArchiveFAT fat;
            FSiSyncReadParam reader;

            reader.arc = arc;
            reader.pos = tables->fat + offset;
            if (FSi_ReadTable(&reader, &fat, sizeof(fat)) == FS_RESULT_SUCCESS) {
                info->filesize = fat.bottom - fat.top;
                if (FSi_IsDsiOnlyRomOffset(arc, fat.top)) {
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
static FSResult FSi_ROMFAT_GetArchiveResource(FSArchive *arc, FSArchiveResource *resource) {
    const CARDRomHeader *header = (const CARDRomHeader *)CARD_GetRomHeader();

    resource->unk_20[0] = 0;
    resource->unk_20[1] = 0;
    resource->unk_20[2] = 0;
    resource->unk_20[3] = 0;
    resource->totalSize = header->rom_size;
    resource->availableSize = 0;
    resource->maxFileSize = 0x7fffffff;
    resource->maxDirectorySize = 0x7fffffff;
    return FS_RESULT_SUCCESS;
}

// The interface of an archive that has tables
static const FSArchiveInterface FS_VTABLE_BASE = {
    .readFile = FSi_ROMFAT_ReadFile,
    .writeFile = FSi_ROMFAT_WriteFile,
    .seekDirectory = FSi_ROMFAT_SeekDirectory,
    .readDirectory = FSi_ROMFAT_ReadDirectory,
    .findPath = FSi_ROMFAT_FindPath,
    .getPath = FSi_ROMFAT_GetPath,
    .openFileFast = FSi_ROMFAT_OpenFileFast,
    .openFileDirect = FSi_ROMFAT_OpenFileDirect,
    .closeFile = FSi_ROMFAT_CloseFile,
    .activate = FSi_ROMFAT_Activate,
    .idle = FSi_ROMFAT_Idle,
    .suspend = FSi_ROMFAT_Suspend,
    .resume = FSi_ROMFAT_Resume,
    .openFile = FSi_ROMFAT_OpenFile,
    .seekFile = FSi_ROMFAT_SeekFile,
    .getFileLength = FSi_ROMFAT_GetFileLength,
    .getFilePosition = FSi_ROMFAT_GetFilePosition,
    .unmount = FSi_ROMFAT_Unmount,
    .getArchiveCaps = FSi_ROMFAT_GetArchiveCaps,
    .getPathInfo = FSi_ROMFAT_GetPathInfo,
    .getArchiveResource = FSi_ROMFAT_GetArchiveResource,
    .openDirectory = FSi_ROMFAT_OpenDirectory,
    .closeDirectory = FSi_ROMFAT_CloseDirectory,
};

// The default functions to read and write an archive: it is in memory, at the base
static FSResult FSi_ReadMemCallback(FSArchive *arc, void *buffer, u32 position, u32 length) {
    MI_CpuCopy8(FS_GetArchiveOffset(arc, position), buffer, length);
    return FS_RESULT_SUCCESS;
}

static FSResult FSi_WriteMemCallback(FSArchive *arc, const void *buffer, u32 position, u32 length) {
    MI_CpuCopy8(buffer, FS_GetArchiveOffset(arc, position), length);
    return FS_RESULT_SUCCESS;
}

// NitroSDK's FS_LoadArchive: gives an archive its tables, in the storage at the base or in memory, and the functions
// to read and write it (the defaults for NULL), and mounts it
BOOL FS_LoadArchive(FSArchive *arc, u32 base, u32 fat, u32 fatSize, u32 fnt, u32 fntSize, FS_ARCHIVE_READ_FUNC readFunc,
                    FS_ARCHIVE_WRITE_FUNC writeFunc) {
    FSArchiveTables *tables = &arc->tables;

    tables->base = base;
    tables->fat_size = fatSize;
    tables->fnt_size = fntSize;
    tables->fat = fat;
    tables->fnt = fnt;
    tables->fnt_bak = fnt;
    tables->fat_bak = fat;
    tables->read_func = readFunc != NULL ? readFunc : FSi_ReadMemCallback;
    tables->write_func = writeFunc != NULL ? writeFunc : FSi_WriteMemCallback;
    tables->load_mem = NULL;
    return FS_MountArchive(arc, tables, &FS_VTABLE_BASE, 0);
}

// NitroSDK's FS_UnloadArchive: unmounts the archive
BOOL FS_UnloadArchive(FSArchive *arc) {
    return FS_UnmountArchive(arc);
}

// The length of a file whose archive has this interface, without sending a command to the archive. Returns whether
// it could
BOOL FSi_GetFileLengthIfProc(FSFile *file, u32 *length) {
    return file->arc->vtbl == &FS_VTABLE_BASE && FSi_ROMFAT_GetFileLength(file->arc, file, length) == FS_RESULT_SUCCESS;
}

// and the position in it
BOOL FSi_GetFilePositionIfProc(FSFile *file, u32 *position) {
    return file->arc->vtbl == &FS_VTABLE_BASE &&
           FSi_ROMFAT_GetFilePosition(file->arc, file, position) == FS_RESULT_SUCCESS;
}

// NitroSDK's FS_SetArchiveProc: sets the hook, and the commands that it is run for; neither goes without the other
void FS_SetArchiveProc(FSArchive *arc, FS_ARCHIVE_PROC_FUNC proc, u32 procMask) {
    FSArchiveTables *tables = &arc->tables;

    if (procMask == 0) {
        proc = NULL;
    } else if (proc == NULL) {
        procMask = 0;
    }
    tables->proc = proc;
    tables->proc_flag = procMask;
}

// NitroSDK's FS_LoadArchiveTables: copies the tables into the memory (aligned up to 32 bytes) and keeps reading them
// from there. Returns the size of memory they need, and loads them only if the memory has that much
u32 FS_LoadArchiveTables(FSArchive *arc, void *memory, u32 memorySize) {
    FSArchiveTables *tables;
    u32 size;
    u32 aligned;
    FSFile file;

    if (memory != NULL && FS_IsArchiveTableLoaded(arc)) {
        FS_UnloadArchiveTables(arc);
    }
    tables = arc->userdata;
    size = (tables->fat_size + tables->fnt_size + 0x3f) & ~0x1f;
    if (size <= memorySize) {
        aligned = ((u32)memory + 0x1f) & ~0x1f;
        FS_InitFile(&file);
        if (FS_OpenFileDirect(&file, arc, tables->fat, tables->fat + tables->fat_size, -1)) {
            if (FS_ReadFile(&file, (void *)aligned, tables->fat_size) < 0) {
                MI_CpuFill8((void *)aligned, 0, tables->fat_size);
            }
            FS_CloseFile(&file);
        }
        tables->fat = aligned;
        aligned += tables->fat_size;
        if (FS_OpenFileDirect(&file, arc, tables->fnt, tables->fnt + tables->fnt_size, -1)) {
            if (FS_ReadFile(&file, (void *)aligned, tables->fnt_size) < 0) {
                MI_CpuFill8((void *)aligned, 0, tables->fnt_size);
            }
            FS_CloseFile(&file);
        }
        tables->fnt = aligned;
        tables->load_mem = memory;
        arc->flag |= FS_ARCHIVE_FLAG_TABLE_LOAD;
    }
    return size;
}

// NitroSDK's FS_UnloadArchiveTables: reads the tables from the storage again, and returns the memory they were in
void *FS_UnloadArchiveTables(FSArchive *arc) {
    void *memory = NULL;

    if (FS_IsArchiveLoaded(arc)) {
        FSArchiveTables *tables = arc->userdata;
        BOOL suspended = FS_SuspendArchive(arc);

        if (FS_IsArchiveTableLoaded(arc)) {
            arc->flag &= ~FS_ARCHIVE_FLAG_TABLE_LOAD;
            memory = tables->load_mem;
            tables->fat = tables->fat_bak;
            tables->fnt = tables->fnt_bak;
            tables->load_mem = NULL;
        }
        if (suspended) {
            FS_ResumeArchive(arc);
        }
    }
    return memory;
}

u32 FS_GetArchiveBase(const FSArchive *arc) {
    FSArchiveTables *tables = arc->userdata;

    return tables->base;
}

// The memory that an offset of the archive is at, when the archive is in memory at its base
static void *FS_GetArchiveOffset(const FSArchive *arc, u32 offset) {
    FSArchiveTables *tables = arc->userdata;

    return (void *)(tables->base + offset);
}

static BOOL FS_IsArchiveTableLoaded(const FSArchive *arc) {
    return (arc->flag & FS_ARCHIVE_FLAG_TABLE_LOAD) != 0;
}

u32 FS_GetFileImageTop(const FSFile *file) {
    return file->body.file.top;
}
