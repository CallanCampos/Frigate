// NitroSDK's file system (libfs): the archive that files over memory belong to. FS_CreateFileFromMemory (swan's
// extfs_fopen) opens a range of memory as a file of an archive that has no name, no tables and the default functions
// that read memory (its base is 0, so a position is an address). Its hook also answers FS_COMMAND_READDIR by listing
// the registered archives, as the entries of a root directory. The file name is a guess from the function's purpose; no
// string in the ROM names it. The functions are under swan's names, with NitroSDK's in their comments.

#include "nitro/fs.h"
#include "nitro/os.h"

static FSArchive sMemoryArchive;
static BOOL sMemoryArchiveReady;

// The hook of the memory archive: a directory read lists the archives, and the commands that look for a path or a
// number are not supported. The directory's position is the archive it reads next
static FSResult func_02072014(FSFile *file, FSCommandType command) {
    switch (command) {
    case FS_COMMAND_READDIR: {
        FSArchive *arc = (FSArchive *)file->body.dir.pos.position;
        FSDirEntry *entry = file->arg.readDirectory.entry;
        const char *name;

        if (arc == NULL) {
            entry->nameLength = 0;
            entry->isDirectory = FALSE;
            return FS_RESULT_FAILURE;
        }
        name = func_02070a24(arc);
        entry->nameLength = NNS_STD_StrLen(name) + 1;
        entry->isDirectory = TRUE;
        if (!file->arg.readDirectory.skipName) {
            func_020800e8(entry->name, "%s:", name);
            entry->id.dir.arc = file->arc;
            entry->id.dir.dirId = 0;
            entry->id.dir.fileId = 0;
            entry->id.dir.position = 0;
        }
        file->body.dir.pos.position = (u32)arc->next;
        return FS_RESULT_SUCCESS;
    }
    case FS_COMMAND_SEEKDIR:
    case FS_COMMAND_FINDPATH:
    case FS_COMMAND_GETPATH:
    case FS_COMMAND_OPENFILEFAST:
        return FS_RESULT_UNSUPPORTED;
    default:
        return FS_RESULT_PROC_DEFAULT;
    }
}

// Makes the memory archive once: its hook set for every command, and nothing of the tables. The program ends if it
// cannot be mounted
static void func_02072094(void) {
    u32 interrupts = CPU_IRQDisable();

    if (!sMemoryArchiveReady || !FS_IsArchiveMounted(&sMemoryArchive)) {
        fs_create(&sMemoryArchive);
        func_02071ec4(&sMemoryArchive, func_02072014, 0xffffffff);
        if (!fs_bind(&sMemoryArchive, 0, 0, 0, 0, 0, NULL, NULL)) {
            sys_exit();
        }
        sMemoryArchiveReady = TRUE;
    }
    CPU_SetIRQMask(interrupts);
}

// NitroSDK's FS_CreateFileFromMemory: opens the memory from data to data + size as a file
BOOL extfs_fopen(FSFile *file, const void *data, u32 size) {
    func_02072094();
    return fopen_raw(file, &sMemoryArchive, (u32)data, (u32)data + size, 0);
}
