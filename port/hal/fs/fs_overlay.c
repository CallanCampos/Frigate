// NitroSDK's file system (libfs), overlays, on the host: the decomp's lib/nitro/src/fs/fs_overlay.c (which
// port/hal/replaced.txt keeps out of the game's sources) with the overlay's memory replaced. Every overlay's code is in
// the program, so loading one reads no image: where the DS clears the overlay's .bss and reads its .data from the ROM,
// the host sets its writable statics back from the snapshot (port/hal/fs/ovsec.h), and zeroes the host memory that
// stands in for a static the game made a root heap in (the field's g_FieldHeapMemory, port/hal/gfl/heap_policy.h). The
// rest stays the DS's, so that the game's overlay bookkeeping (src/gfl/gf_overlay.c) sees what it sees on the DS:
//   - FS_LoadOverlayInfo reads the overlay's header from the ROM's table, through the ROM archive, and finds its file.
//     FSOverlayInfo's pointers keep the DS addresses of the header (where the image would load, its static
//     initializers), which nothing on the host dereferences: GFL's memory-collision check compares those ranges, so it
//     refuses exactly the loads the DS refuses, and a reset happens exactly where the DS would copy the overlay in;
//   - FS_LoadOverlayImage fails as the DS's does when the overlay's file can't be opened;
//   - FS_StartOverlay has no digest to check (the game boots from the card), no image to expand and no static
//     initializers to run: they are code of the DS image, and only overlay 165 has any (DS Protect's decryptors, which
//     the port leaves out, PLAN.md §4.5);
//   - FS_EndOverlay has no destructors to call: the chain of global objects is MSL's C++ runtime, which nothing in the
//     game uses, and the host's C library replaces;
//   - an unloaded overlay keeps its statics until its next load resets them (the DS would let the next overlay at its
//     address overwrite them, which only a bug would read).
// The ROM's header of an overlay is eight little-endian words: the overlay's number, its load address, the sizes of its
// image and its .bss, the start and end of its static initializers, its file's number, and the size of the stored
// image in 24 bits with the flags in the top byte (the DS's FSOverlayInfo, as it lays out in the DS's memory).
#include "hal/fs/ovsec.h"
#include "hal/gfl/heap_policy.h"
#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The table of overlay headers of a processor, when the game was given it in memory. NULL is the ROM's, found through
// its header; -1 is none, with which a game that multiboot downloaded has no overlays
typedef struct FSOverlayTable {
    const u8 *address;
    u32 size;
} FSOverlayTable;

static FSOverlayTable fsi_ovt9;
static FSOverlayTable fsi_ovt7;
static FSArchive *sRomArchive;

static u32 FSi_ReadWord(const u8 *data) {
    return (u32)data[0] | (u32)data[1] << 8 | (u32)data[2] << 16 | (u32)data[3] << 24;
}

// Fills an FSOverlayInfo from the overlay's header as the ROM (or a table in memory) has it. The DS reads the header
// over the struct; the host's struct has 64-bit pointers, so the words go to their fields
static void FSi_ReadOverlayHeader(FSOverlayInfo *info, const u8 *header) {
    u32 sizeAndFlags = FSi_ReadWord(header + 0x1c);

    info->id = FSi_ReadWord(header + 0x00);
    info->ramAddress = (u8 *)(uintptr_t)FSi_ReadWord(header + 0x04);
    info->ramSize = FSi_ReadWord(header + 0x08);
    info->bssSize = FSi_ReadWord(header + 0x0c);
    info->sinitStart = (void (**)(void))(uintptr_t)FSi_ReadWord(header + 0x10);
    info->sinitEnd = (void (**)(void))(uintptr_t)FSi_ReadWord(header + 0x14);
    info->fileId = FSi_ReadWord(header + 0x18);
    info->compressedSize = sizeAndFlags & 0xffffff;
    info->flag = sizeAndFlags >> 24;
}

// NitroSDK's FSi_InitOverlay: sets the overlay tables and finds the archive "rom". It also takes the overlays'
// snapshot, once per process, before any overlay can have run: an overlay's code runs only once the game loaded it
void FSi_InitOverlay(void) {
    if (OS_GetBootType() == OS_BOOTTYPE_DOWNLOAD_MB) {
        fsi_ovt9.address = (const u8 *)-1;
        fsi_ovt9.size = 0;
        fsi_ovt7.address = (const u8 *)-1;
        fsi_ovt7.size = 0;
    } else {
        fsi_ovt9.address = NULL;
        fsi_ovt9.size = 0;
        fsi_ovt7.address = NULL;
        fsi_ovt7.size = 0;
    }
    sRomArchive = FS_FindArchive("rom", 3);
    port_ovsec_init();
}

// NitroSDK's FSi_GetOverlayBinarySize: the size that an overlay's image has in the ROM, without its zeroed data
u32 FSi_GetOverlayBinarySize(const FSOverlayInfo *info) {
    if (info->flag & FS_OVERLAY_FLAG_COMP) {
        return info->compressedSize;
    }
    return info->ramSize;
}

// NitroSDK's FS_ClearOverlayImage: the DS invalidates the caches over the overlay's memory and clears its .bss; the
// host clears the overlay's bss section, and the host memory that stands in for a root heap's memory in one of the
// overlay's zeroed statics (port/hal/gfl/heap_policy.h): those statics are in the bss section, or in the combined
// layout in the one section, which the load's port_ovsec_restore_data zeroes from the snapshot
void FS_ClearOverlayImage(FSOverlayInfo *info) {
    const PortOvsecOverlay *overlay = port_ovsec_get(info->id);

    port_ovsec_clear_bss(info->id);
    if (overlay != NULL) {
        port_heap_root_memory_reset(overlay->bss.start, overlay->bss.size);
        port_heap_root_memory_reset(overlay->data.start, overlay->data.size);
    }
}

// NitroSDK's FS_GetOverlayFileID: the archive and the number of the overlay's file
FSFileID FS_GetOverlayFileID(const FSOverlayInfo *info) {
    FSFileID fileId;

    fileId.arc = sRomArchive;
    fileId.file_id = info->fileId;
    return fileId;
}

// NitroSDK's FS_LoadOverlayInfo: reads an overlay's header from a processor's table, in memory or in the ROM, and finds
// its file. The DS also refuses an overlay number from the build's count on (FS_OVERLAY_COUNT, a symbol of its linker
// script), which is the number of headers in the ROM's table: the table's size alone refuses them here
BOOL FS_LoadOverlayInfo(FSOverlayInfo *info, MIProcessor target, u32 id) {
    BOOL result = FALSE;
    u32 offset = id * FS_OVERLAY_HEADER_SIZE;
    const FSOverlayTable *table = &fsi_ovt9;
    u8 header[FS_OVERLAY_HEADER_SIZE];

    if (target != MI_PROCESSOR_ARM9) {
        table = &fsi_ovt7;
    }
    if (table->address != NULL) {
        if (offset < table->size) {
            FSFile file;

            FS_InitFile(&file);
            FSi_ReadOverlayHeader(info, table->address + offset);
            info->target = target;
            if (FS_OpenFileFast(&file, FS_GetOverlayFileID(info))) {
                info->filePos = FS_GetFileImageTop(&file);
                info->fileSize = FS_GetFileLength(&file);
                FS_CloseFile(&file);
                result = TRUE;
            }
        }
    } else {
        const CARDRomRegion *region;

        if (target == MI_PROCESSOR_ARM9) {
            region = &CARD_GetOwnRomHeader()->main_ovt;
        } else {
            region = &CARD_GetOwnRomHeader()->sub_ovt;
        }
        if (offset < region->length) {
            FSFile file;

            FS_InitFile(&file);
            // BUG: The end of the table is passed where the function takes a size, so the range of the file runs past
            // the table (only its first 0x20 bytes are read). Kept, as in the decomp's file
#ifdef BUGFIX
            if (FS_CreateFileFromRom(&file, region->offset + offset, region->length - offset)) {
#else
            if (FS_CreateFileFromRom(&file, region->offset + offset, region->offset + region->length)) {
#endif
                if (FS_ReadFile(&file, header, FS_OVERLAY_HEADER_SIZE) != FS_OVERLAY_HEADER_SIZE) {
                    FS_CloseFile(&file);
                } else {
                    FS_CloseFile(&file);
                    FSi_ReadOverlayHeader(info, header);
                    info->target = target;
                    if (FS_OpenFileFast(&file, FS_GetOverlayFileID(info))) {
                        info->filePos = FS_GetFileImageTop(&file);
                        info->fileSize = FS_GetFileLength(&file);
                        FS_CloseFile(&file);
                        result = TRUE;
                    }
                }
            }
        }
    }
    return result;
}

// NitroSDK's FS_LoadOverlayImage: where the DS reads the image into the overlay's memory, after clearing its .bss, the
// host sets the overlay's statics back: its bss section cleared, its data section from the snapshot. The file is
// opened as the DS opens it, and has to hold the image the DS would read
BOOL FS_LoadOverlayImage(FSOverlayInfo *info) {
    BOOL result = FALSE;
    FSFile file;

    FS_InitFile(&file);
    if (FS_OpenFileFast(&file, FS_GetOverlayFileID(info))) {
        FS_ClearOverlayImage(info);
        if (FS_GetFileLength(&file) >= FSi_GetOverlayBinarySize(info)) {
            port_ovsec_restore_data(info->id);
            result = TRUE;
        }
        FS_CloseFile(&file);
    }
    return result;
}

// NitroSDK's FS_StartOverlay: nothing to check, expand or initialize on the host (above)
void FS_StartOverlay(FSOverlayInfo *info) {
    (void)info;
}

// NitroSDK's FS_EndOverlay: no destructors to call on the host (above)
void FS_EndOverlay(FSOverlayInfo *info) {
    (void)info;
}

BOOL FS_UnloadOverlayImage(FSOverlayInfo *info) {
    FS_EndOverlay(info);
    return TRUE;
}

// NitroSDK's FS_LoadOverlay: reads an overlay's header, sets its statics back and starts it
BOOL FS_LoadOverlay(MIProcessor target, u32 id) {
    BOOL result = FALSE;
    FSOverlayInfo info;

    if (FS_LoadOverlayInfo(&info, target, id) && FS_LoadOverlayImage(&info)) {
        FS_StartOverlay(&info);
        result = TRUE;
    }
    return result;
}

// NitroSDK's FS_UnloadOverlay: ends an overlay
BOOL FS_UnloadOverlay(MIProcessor target, u32 id) {
    FSOverlayInfo info;

    if (FS_LoadOverlayInfo(&info, target, id) && FS_UnloadOverlayImage(&info)) {
        return TRUE;
    }
    return FALSE;
}
