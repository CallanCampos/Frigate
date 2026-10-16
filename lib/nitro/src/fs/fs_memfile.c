// NitroSDK's file system (libfs): the archive that files over memory belong to. FS_CreateFileFromMemory (swan's
// extfs_fopen) opens a range of memory as a file of an archive that has no name, no tables and the default functions
// that read memory (its base is 0, so a position is an address). Its hook also answers FS_COMMAND_READDIR by listing
// the registered archives, as the entries of a root directory. The file name is a guess from the function's purpose; no
// string in the ROM names it. The functions have NitroSDK's names where they are known.

#include "nitro/fs.h"
#include "nitro/os.h"

static FSArchive sMemoryArchive; // swan's g_ExtFileSystem
static BOOL sMemoryArchiveReady;

// The hook of the memory archive: a directory read lists the archives, and the commands that look for a path or a
// number are not supported. The directory's position is the archive it reads next
static FSResult FSi_MemoryArchiveProc(FSFile *file, FSCommandType command) {
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
        name = FS_GetArchiveName(arc);
        entry->nameLength = STD_GetStringLength(name) + 1;
        entry->isDirectory = TRUE;
        if (!file->arg.readDirectory.skipName) {
            STD_TSPrintf(entry->name, "%s:", name);
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
static void FSi_InitMemoryArchive(void) {
    u32 interrupts = OS_DisableInterrupts();

    if (!sMemoryArchiveReady || !FS_IsArchiveMounted(&sMemoryArchive)) {
        FS_InitArchive(&sMemoryArchive);
        FS_SetArchiveProc(&sMemoryArchive, FSi_MemoryArchiveProc, 0xffffffff);
        if (!FS_LoadArchive(&sMemoryArchive, 0, 0, 0, 0, 0, NULL, NULL)) {
            sys_exit();
        }
        sMemoryArchiveReady = TRUE;
    }
    OS_RestoreInterrupts(interrupts);
}

// NitroSDK's FS_CreateFileFromMemory: opens the memory from data to data + size as a file
BOOL FS_CreateFileFromMemory(FSFile *file, const void *data, u32 size) {
    FSi_InitMemoryArchive();
    return FS_OpenFileDirect(file, &sMemoryArchive, (u32)data, (u32)data + size, 0);
}
