// NitroSDK's file system (libfs): the archive that files over memory belong to. FS_CreateFileFromMemory (swan's
// extfs_fopen) opens a range of memory as a file of an archive that has no name, no tables and the default functions
// that read memory (its base is 0, so a position is an address). Its hook also answers FS_COMMAND_READDIR by listing
// the registered archives, as the entries of a root directory. The file name is a guess from the function's purpose; no
// string in the ROM names it, and ntrtwl's reconstruction of NitroSDK 4.2 has this code in fs_mem.c. The functions and
// objects have NitroSDK's names, as that reconstruction has them.

#include "nitro/fs.h"
#include "nitro/os.h"
#include "nitro/std.h"

static FSArchive fsi_arc_mem; // swan's g_ExtFileSystem
static BOOL fsi_mem_init;

// The hook of the memory archive: a directory read lists the archives, and the commands that look for a path or a
// number are not supported. The directory's position is the archive it reads next
static FSResult FSi_MemArchiveProc(FSFile *file, FSCommandType command) {
    FSResult result;

    switch (command) {
    case FS_COMMAND_READDIR: {
        FSArchive *arc = (FSArchive *)file->body.dir.pos.pos;
        FSDirEntry *entry = file->arg.readDirectory.entry;
        const char *name;

        if (arc == NULL) {
            entry->name_len = 0;
            entry->is_directory = FALSE;
            result = FS_RESULT_FAILURE;
        } else {
            name = FS_GetArchiveName(arc);
            entry->name_len = STD_GetStringLength(name) + 1;
            entry->is_directory = TRUE;
            if (!file->arg.readDirectory.skipName) {
                STD_TSPrintf(entry->name, "%s:", name);
                entry->id.dir.arc = file->arc;
                entry->id.dir.own_id = 0;
                entry->id.dir.index = 0;
                entry->id.dir.pos = 0;
            }
            result = FS_RESULT_SUCCESS;
            file->body.dir.pos.pos = (u32)arc->next;
        }
        break;
    }
    case FS_COMMAND_SEEKDIR:
    case FS_COMMAND_FINDPATH:
    case FS_COMMAND_GETPATH:
    case FS_COMMAND_OPENFILEFAST:
        result = FS_RESULT_UNSUPPORTED;
        break;
    default:
        result = FS_RESULT_PROC_DEFAULT;
        break;
    }
    return result;
}

// Makes the memory archive once: its hook set for every command, and nothing of the tables. The program ends if it
// cannot be mounted
static void FSi_InitMem(void) {
    OSIntrMode interrupts = OS_DisableInterrupts();

    if (!fsi_mem_init || !FS_IsArchiveLoaded(&fsi_arc_mem)) {
        FS_InitArchive(&fsi_arc_mem);
        FS_SetArchiveProc(&fsi_arc_mem, FSi_MemArchiveProc, 0xffffffff);
        if (!FS_LoadArchive(&fsi_arc_mem, 0, 0, 0, 0, 0, NULL, NULL)) {
            OS_Terminate();
        }
        fsi_mem_init = TRUE;
    }
    OS_RestoreInterrupts(interrupts);
}

// NitroSDK's FS_CreateFileFromMemory: opens the memory from data to data + size as a file
BOOL FS_CreateFileFromMemory(FSFile *file, void *data, u32 size) {
    FSi_InitMem();
    return FS_OpenFileDirect(file, &fsi_arc_mem, (u32)data, (u32)data + size, 0);
}
