// NitroSDK's files over memory on the host: the decomp's fs_memfile.c, compiled as it is from its own path through
// this file (port/hal/fs/replaced.txt keeps it out of the game's own sources). FS_CreateFileFromMemory opens a range of
// memory as a file of the memory archive, whose base is 0, so that a file's range and position are 32-bit addresses
// ((u32)data, cut from a 64-bit host pointer) that the archive's default commands read and write through
// (fs_command_default.c's FSi_ReadMemCallback). The host keeps the pointer whole instead:
//   - FS_CreateFileFromMemory, renamed out of the way before the include and written again below, opens the file as
//     the DS's does, with the same 32-bit range, and notes the file with the pointer the game gave;
//   - the memory archive's hook, which gets every command with its file, reads and writes a noted file through its
//     pointer (FS_COMMAND_READFILE, FS_COMMAND_WRITEFILE, as the default commands would at the file's position) and
//     forgets it when it is closed. Every other command goes to the decomp's hook, FSi_MemoryArchiveProc.
// A file is found by its FSFile, then checked against the range noted with it. A copy of an FSFile, which the noted
// one doesn't name, is found by its range among the noted files when they all agree on the memory it names. A read or
// write the host can't place stops the program with a message, rather than guess the pointer from its low 32 bits.
#include "nitro/fs.h"

#define FS_CreateFileFromMemory FSi_DsCreateFileFromMemory
// The decomp's file keeps addresses in 32-bit words, as the DS does; its warnings about them are silenced for the file
// alone, as port/CMakeLists.txt does for every file of the game, and the compile probe counts them from the file's own
// path (port/tools/compile-probe)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "lib/nitro/src/fs/fs_memfile.c"
#pragma GCC diagnostic pop
#undef FS_CreateFileFromMemory

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// A file over memory: its FSFile, the 32-bit range the archive gave it, and the memory the game gave
typedef struct FSiHostMemoryFile {
    const FSFile *file;
    u32 top;
    u32 bottom;
    u8 *data;
} FSiHostMemoryFile;

static FSiHostMemoryFile *sMemoryFiles;
static u32 sMemoryFileCount;
static u32 sMemoryFileCapacity;

static FSResult FSi_HostMemoryArchiveProc(FSFile *file, FSCommandType command);

// Notes a file opened over memory, in place of what was noted for its FSFile before
static void FSi_NoteMemoryFile(const FSFile *file, const void *data) {
    FSiHostMemoryFile *entry = NULL;
    u32 i;

    for (i = 0; i < sMemoryFileCount; i++) {
        if (sMemoryFiles[i].file == file) {
            entry = &sMemoryFiles[i];
            break;
        }
    }
    if (entry == NULL) {
        if (sMemoryFileCount == sMemoryFileCapacity) {
            u32 capacity = sMemoryFileCapacity != 0 ? sMemoryFileCapacity * 2 : 16;
            FSiHostMemoryFile *files = realloc(sMemoryFiles, capacity * sizeof(*files));

            if (files == NULL) {
                fprintf(stderr, "port: fs: no memory to note a file over memory\n");
                exit(1);
            }
            sMemoryFiles = files;
            sMemoryFileCapacity = capacity;
        }
        entry = &sMemoryFiles[sMemoryFileCount++];
    }
    entry->file = file;
    entry->top = (u32)file->body.file.top;
    entry->bottom = (u32)file->body.file.bottom;
    // FS_WriteFile writes through the same memory, which the game gave as const as the SDK's prototype has it
    entry->data = (u8 *)(uintptr_t)data;
}

static void FSi_ForgetMemoryFile(const FSFile *file) {
    u32 i;

    for (i = 0; i < sMemoryFileCount; i++) {
        if (sMemoryFiles[i].file == file) {
            sMemoryFiles[i] = sMemoryFiles[--sMemoryFileCount];
            return;
        }
    }
}

// The host memory of length bytes at the file's 32-bit position (above). NULL when no noted file names it
static u8 *FSi_FindMemory(const FSFile *file, u32 position, u32 length) {
    u32 top = (u32)file->body.file.top;
    u32 bottom = (u32)file->body.file.bottom;
    u8 *found = NULL;
    u32 i;

    if (position < top || position > bottom || length > bottom - position) {
        return NULL;
    }
    for (i = 0; i < sMemoryFileCount; i++) {
        const FSiHostMemoryFile *entry = &sMemoryFiles[i];
        if (entry->file == file && entry->top == top && entry->bottom == bottom) {
            return entry->data + (position - top);
        }
    }
    // A copy of a noted FSFile: the noted files whose range holds the file's must all name the same memory
    for (i = 0; i < sMemoryFileCount; i++) {
        const FSiHostMemoryFile *entry = &sMemoryFiles[i];
        if (entry->top <= top && bottom <= entry->bottom) {
            u8 *memory = entry->data + (position - entry->top);
            if (found != NULL && found != memory) {
                return NULL;
            }
            found = memory;
        }
    }
    return found;
}

static u8 *FSi_HostMemory(const FSFile *file, u32 position, u32 length) {
    u8 *memory = FSi_FindMemory(file, position, length);

    if (memory == NULL) {
        fprintf(stderr,
                "port: fs: a file over memory (FS_CreateFileFromMemory) has its data at %#x to %#x, cut to 32 bits, "
                "and the host has no pointer noted for it at %#x (%u bytes)\n",
                (u32)file->body.file.top, (u32)file->body.file.bottom, position, length);
        exit(1);
    }
    return memory;
}

// The memory archive's hook on the host (above): the default commands of a read and a write, through the noted pointer
static FSResult FSi_HostMemoryArchiveProc(FSFile *file, FSCommandType command) {
    u32 position = (u32)file->body.file.position;
    u32 length;

    switch (command) {
    case FS_COMMAND_READFILE:
        length = file->arg.readFile.length;
        MI_CpuCopy8(FSi_HostMemory(file, position, length), file->arg.readFile.buffer, length);
        file->body.file.position = (s32)(position + length);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_WRITEFILE:
        length = file->arg.writeFile.length;
        MI_CpuCopy8(file->arg.writeFile.buffer, FSi_HostMemory(file, position, length), length);
        file->body.file.position = (s32)(position + length);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_CLOSEFILE:
        // The default command of a close has nothing to do. Answering it here keeps the command in the hook's mask,
        // which FS_RESULT_PROC_DEFAULT would take it out of
        FSi_ForgetMemoryFile(file);
        return FS_RESULT_SUCCESS;
    default:
        return FSi_MemoryArchiveProc(file, command);
    }
}

// NitroSDK's FS_CreateFileFromMemory: the decomp's, then the host's hook on the memory archive and the file noted with
// its pointer
BOOL FS_CreateFileFromMemory(FSFile *file, const void *data, u32 size) {
    BOOL opened = FSi_DsCreateFileFromMemory(file, data, size);
    FSArchiveTables *tables = sMemoryArchive.userdata;

    if (tables->proc != FSi_HostMemoryArchiveProc) {
        FS_SetArchiveProc(&sMemoryArchive, FSi_HostMemoryArchiveProc, 0xffffffff);
    }
    if (opened) {
        FSi_NoteMemoryFile(file, data);
    }
    return opened;
}
