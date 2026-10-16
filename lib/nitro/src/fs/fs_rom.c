// NitroSDK's file system (libfs): the archive "rom", the game's ROM. It reads the files from the game card through the
// card library (the file allocation table and the name table are the ROM header's), locks the card for the commands
// that use it, and refuses to open what lies in the part of the ROM that only the DSi mode reads, when the console runs
// in DS mode. A game that did not boot from the ROM gets an archive without files. The file name is NitroSDK's own
// (fs_rom.c). The functions are under swan's names, with NitroSDK's in their comments.

#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/os.h"

// What the ROM header holds for a table that is not there, besides 0
#define FS_ROM_NO_TABLE 0xffffffff

// The commands that the hook of the ROM archive acts on
#define FS_ROM_PROC_MASK                                                                                               \
    ((1 << FS_COMMAND_WRITEFILE) | (1 << FS_COMMAND_OPENFILEDIRECT) | (1 << FS_COMMAND_ACTIVATE) |                     \
     (1 << FS_COMMAND_IDLE))

static FSArchive sRomFileSystem;
static u32 sDefaultDmaNo;
static u32 sCardLockId;

// Whether the archive is the ROM and the position is in the part of the ROM that the console reads only in the DSi
// mode, which starts where the header says. A file there cannot be opened in the DS mode
BOOL func_02072130(const FSArchive *arc, u32 position) {
    BOOL locked = FALSE;

    if (STD_CompareString(func_02070a24(arc), "rom") == 0 && !hw_isDSi() &&
        position >= rom_get_header()->twlRegionStart << 19) {
        locked = TRUE;
    }
    return locked;
}

// The card read ended: the archive's command ends with it, with an error if the card was pulled out
static void func_02072164(void *arg) {
    func_02070b88(arg, func_0206ff5c() ? FS_RESULT_ERROR : FS_RESULT_SUCCESS);
}

// The ROM archive's read function: reads from the card, apart from the caller's thread
static FSResult func_0207217c(FSArchive *arc, void *buffer, u32 position, u32 length) {
    func_0206fd24(sDefaultDmaNo, position, buffer, length, func_02072164, arc, TRUE);
    return FS_RESULT_PROC_ASYNC;
}

// The hook of the ROM archive: it locks the card while the archive is active, refuses to write, and refuses to open a
// range of the part that only the DSi mode reads
static FSResult func_020721a8(FSFile *file, FSCommandType command) {
    switch (command) {
    case FS_COMMAND_ACTIVATE:
        func_0206eef0(sCardLockId);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_IDLE:
        func_0206ef38(sCardLockId);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_OPENFILEDIRECT:
        if (func_02072130(file->arc, file->arg.openFileDirect.top)) {
            return FS_RESULT_PERMISSION_DENIED;
        }
        return FS_RESULT_PROC_UNKNOWN;
    case FS_COMMAND_WRITEFILE:
        return FS_RESULT_UNSUPPORTED;
    default:
        return FS_RESULT_PROC_DEFAULT;
    }
}

// The hook of the ROM archive when it has no files: it refuses to write, and leaves the other commands to the
// archive's own functions, which refuse them too
static FSResult func_0207221c(FSFile *file, FSCommandType command) {
    if (command == FS_COMMAND_WRITEFILE) {
        return FS_RESULT_UNSUPPORTED;
    }
    return FS_RESULT_PROC_DEFAULT;
}

// The read and write functions of the ROM archive when it has no files
static FSResult arg_8(FSArchive *arc, void *buffer, u32 position, u32 length) {
    return FS_RESULT_UNSUPPORTED;
}

static FSResult arg_C(FSArchive *arc, const void *buffer, u32 position, u32 length) {
    return FS_RESULT_UNSUPPORTED;
}

// What fs_init_core calls for a game that did not boot from the ROM, in place of the ROM header's tables: it does
// nothing, and what it returns is not used
static BOOL func_02072234(FSArchive *arc) {
    return FALSE;
}

// NitroSDK's FSi_InitRomArchive: makes the archive "rom" with the default DMA channel given. A game that booted from
// the ROM has the ROM header's tables, if it has any; otherwise the archive has no files
void fs_init_core(u32 dma) {
    cart_read_thread_init();
    sDefaultDmaNo = dma;
    sCardLockId = cart_key_create();
    fs_create(&sRomFileSystem);
    fs_open(&sRomFileSystem, "rom", 3);
    if (sys_get_boot_indicator() == OS_BOOTTYPE_ROM) {
        const ROMHeaderRegion *fnt = &rom_get_header()->fnt;
        const ROMHeaderRegion *fat = &rom_get_header()->fat;

        func_02071ec4(&sRomFileSystem, func_020721a8, FS_ROM_PROC_MASK);
        if (fnt->offset != FS_ROM_NO_TABLE && fnt->offset != 0 && fat->offset != FS_ROM_NO_TABLE && fat->offset != 0) {
            fs_bind(&sRomFileSystem, 0, fat->offset, fat->size, fnt->offset, fnt->size, func_0207217c, NULL);
        }
    } else {
        func_02072234(&sRomFileSystem);
    }
    if (!FS_IsArchiveMounted(&sRomFileSystem)) {
        func_02071ec4(&sRomFileSystem, func_0207221c, 0xffffffff);
        fs_bind(&sRomFileSystem, 0, 0, 0, 0, 0, arg_8, arg_C);
    }
    func_02070680("rom:");
}

// NitroSDK's FS_SetDefaultDMA: the DMA channel that the ROM archive reads with, or FS_DMA_NOT_USE. Returns the one
// before. The archive is not running a command meanwhile
u32 fs_set_dma_id(u32 dma) {
    u32 interrupts = CPU_IRQDisable();
    u32 previous = sDefaultDmaNo;
    BOOL suspended = func_02070ad4(&sRomFileSystem);

    sDefaultDmaNo = dma;
    if (suspended) {
        func_02070b44(&sRomFileSystem);
    }
    CPU_SetIRQMask(interrupts);
    return previous;
}

// NitroSDK's FS_LoadTable: loads the ROM archive's tables into the memory given, which it reads them from then on
u32 func_02072358(void *memory, u32 size) {
    return func_02071edc(&sRomFileSystem, memory, size);
}

// Opens the ROM from start, for size bytes, as a file with no number
BOOL romfs_fopen_raw(FSFile *file, u32 start, u32 size) {
    return fopen_raw(file, &sRomFileSystem, start, start + size, 0xffff);
}
