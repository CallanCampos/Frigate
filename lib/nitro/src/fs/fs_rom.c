// NitroSDK's file system (libfs): the archive "rom", the game's ROM. It reads the files from the game card through the
// card library (the file allocation table and the name table are the ROM header's), locks the card for the commands
// that use it, and refuses to open what lies in the part of the ROM that only the DSi mode reads, when the console runs
// in DS mode. A game that did not boot from the ROM gets an archive without files. The file name is NitroSDK's own
// (fs_rom.c). The functions have NitroSDK's names where they are known.

#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/os.h"
#include "nitro/std.h"

// What the ROM header holds for a table that is not there, besides 0
#define FS_ROM_NO_TABLE 0xffffffff

// The commands that the hook of the ROM archive acts on
#define FS_ROM_PROC_MASK                                                                                               \
    ((1 << FS_COMMAND_WRITEFILE) | (1 << FS_COMMAND_OPENFILEDIRECT) | (1 << FS_COMMAND_ACTIVATE) |                     \
     (1 << FS_COMMAND_IDLE))

static FSArchive sRomFileSystem; // swan's g_RomFSFileSystem
static u32 sDefaultDmaNo;        // swan's FS_DMA_NO
static u32 sCardLockId;

// Whether the archive is the ROM and the position is in the part of the ROM that the console reads only in the DSi
// mode, which starts where the header says. A file there cannot be opened in the DS mode
BOOL FSi_IsDsiOnlyRomOffset(const FSArchive *arc, u32 position) {
    BOOL locked = FALSE;

    if (STD_CompareString(FS_GetArchiveName(arc), "rom") == 0 && !OS_IsRunOnTwl() &&
        position >= CARD_GetOwnRomHeader()->twlRegionStart << 19) {
        locked = TRUE;
    }
    return locked;
}

// The card read ended: the archive's command ends with it, with an error if the card was pulled out
static void FSi_RomReadEnd(void *arg) {
    FS_NotifyArchiveAsyncEnd(arg, CARD_IsPulledOut() ? FS_RESULT_ERROR : FS_RESULT_SUCCESS);
}

// The ROM archive's read function: reads from the card, apart from the caller's thread
static FSResult FSi_ReadRom(FSArchive *arc, void *buffer, u32 position, u32 length) {
    CARDi_ReadRom(sDefaultDmaNo, position, buffer, length, FSi_RomReadEnd, arc, TRUE);
    return FS_RESULT_PROC_ASYNC;
}

// The hook of the ROM archive: it locks the card while the archive is active, refuses to write, and refuses to open a
// range of the part that only the DSi mode reads
static FSResult FSi_RomArchiveProc(FSFile *file, FSCommandType command) {
    switch (command) {
    case FS_COMMAND_ACTIVATE:
        CARD_LockRom(sCardLockId);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_IDLE:
        CARD_UnlockRom(sCardLockId);
        return FS_RESULT_SUCCESS;
    case FS_COMMAND_OPENFILEDIRECT:
        if (FSi_IsDsiOnlyRomOffset(file->arc, file->arg.openFileDirect.top)) {
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
static FSResult FSi_EmptyArchiveProc(FSFile *file, FSCommandType command) {
    if (command == FS_COMMAND_WRITEFILE) {
        return FS_RESULT_UNSUPPORTED;
    }
    return FS_RESULT_PROC_DEFAULT;
}

// The read and write functions of the ROM archive when it has no files
static FSResult FSi_EmptyRead(FSArchive *arc, void *buffer, u32 position, u32 length) {
    return FS_RESULT_UNSUPPORTED;
}

static FSResult FSi_EmptyWrite(FSArchive *arc, const void *buffer, u32 position, u32 length) {
    return FS_RESULT_UNSUPPORTED;
}

// TwlSDK's FSi_OverrideRomArchive, the default of a weak hook that a program overrides to supply its own ROM archive:
// FSi_InitRomArchive calls it for a game that did not boot from the ROM, in place of the ROM header's tables. The
// default does nothing, and what it returns is not used
static BOOL FSi_OverrideRomArchive(FSArchive *arc) {
    return FALSE;
}

// NitroSDK's FSi_InitRomArchive: makes the archive "rom" with the default DMA channel given. A game that booted from
// the ROM has the ROM header's tables, if it has any; otherwise the archive has no files
void FSi_InitRomArchive(u32 dma) {
    CARD_Init();
    sDefaultDmaNo = dma;
    sCardLockId = OS_GetLockID();
    FS_InitArchive(&sRomFileSystem);
    FS_RegisterArchiveName(&sRomFileSystem, "rom", 3);
    if (OS_GetBootType() == OS_BOOTTYPE_ROM) {
        const ROMHeaderRegion *fnt = &CARD_GetOwnRomHeader()->fnt;
        const ROMHeaderRegion *fat = &CARD_GetOwnRomHeader()->fat;

        FS_SetArchiveProc(&sRomFileSystem, FSi_RomArchiveProc, FS_ROM_PROC_MASK);
        if (fnt->offset != FS_ROM_NO_TABLE && fnt->offset != 0 && fat->offset != FS_ROM_NO_TABLE && fat->offset != 0) {
            FS_LoadArchive(&sRomFileSystem, 0, fat->offset, fat->size, fnt->offset, fnt->size, FSi_ReadRom, NULL);
        }
    } else {
        FSi_OverrideRomArchive(&sRomFileSystem);
    }
    if (!FS_IsArchiveLoaded(&sRomFileSystem)) {
        FS_SetArchiveProc(&sRomFileSystem, FSi_EmptyArchiveProc, 0xffffffff);
        FS_LoadArchive(&sRomFileSystem, 0, 0, 0, 0, 0, FSi_EmptyRead, FSi_EmptyWrite);
    }
    FS_SetCurrentDirectory("rom:");
}

// NitroSDK's FS_SetDefaultDMA: the DMA channel that the ROM archive reads with, or FS_DMA_NOT_USE. Returns the one
// before. The archive is not running a command meanwhile
u32 FS_SetDefaultDMA(u32 dma) {
    u32 interrupts = OS_DisableInterrupts();
    u32 previous = sDefaultDmaNo;
    BOOL suspended = FS_SuspendArchive(&sRomFileSystem);

    sDefaultDmaNo = dma;
    if (suspended) {
        FS_ResumeArchive(&sRomFileSystem);
    }
    OS_RestoreInterrupts(interrupts);
    return previous;
}

// NitroSDK's FS_LoadTable: loads the ROM archive's tables into the memory given, which it reads them from then on
u32 FS_LoadTable(void *memory, u32 size) {
    return FS_LoadArchiveTables(&sRomFileSystem, memory, size);
}

// Opens the ROM from start, for size bytes, as a file with no number
BOOL romfs_fopen_raw(FSFile *file, u32 start, u32 size) {
    return FS_OpenFileDirect(file, &sRomFileSystem, start, start + size, 0xffff);
}
