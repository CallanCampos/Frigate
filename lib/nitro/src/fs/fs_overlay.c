// NitroSDK's file system (libfs): overlays. They are found in the ROM's tables of overlay headers, read into memory,
// checked against the digests of the build when the game was not booted from the ROM, and started and ended with their
// static initializers and destructors. The file name is a guess from NitroSDK's own layout (fs_overlay.c); no string
// in the ROM names it. The functions are under swan's names, with NitroSDK's in their comments.

#include "nitro/fs.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/syscall.h"
#include <NMWException.h>

// The table of overlay headers of a processor, when the game was given it in memory. A table that is NULL is the ROM's,
// found through its header; -1 is none, with which the game that multiboot downloaded has no overlays
typedef struct FSOverlayTable {
    const u8 *address;
    u32 size;
} FSOverlayTable;

// The key of the digests: NitroSDK keeps its address and length, and each check copies it
typedef struct FSDigestKey {
    const u8 *data;
    u32 size;
} FSDigestKey;

static FSOverlayTable sArm9Overlays;
static FSOverlayTable sArm7Overlays;
static FSDigestKey sDigestKey;
static FSArchive *sRomArchive;
static BOOL sLtdOverlayLoaded;

#define FS_OVERLAY_DIGEST_KEY_SIZE 64

static const u8 sOverlayDigestKey[FS_OVERLAY_DIGEST_KEY_SIZE] = {
    0x21, 0x06, 0xc0, 0xde, 0xba, 0x98, 0xce, 0x3f, 0xa6, 0x92, 0xe3, 0x9d, 0x46, 0xf2, 0xed, 0x01,
    0x76, 0xe3, 0xcc, 0x08, 0x56, 0x23, 0x63, 0xfa, 0xca, 0xd4, 0xec, 0xdf, 0x9a, 0x62, 0x78, 0x34,
    0x8f, 0x6d, 0x63, 0x3c, 0xfe, 0x22, 0xca, 0x92, 0x20, 0x88, 0x97, 0x23, 0xd2, 0xcf, 0xae, 0xc2,
    0x32, 0x67, 0x8d, 0xfe, 0xca, 0x83, 0x64, 0x98, 0xac, 0xfd, 0x3e, 0x37, 0x87, 0x46, 0x58, 0x24,
};

static BOOL func_020710a8(const FSOverlayDigest *expected, const void *data, u32 size, BOOL masked);

// NitroSDK's FSi_InitOverlay: sets the overlay tables and the digest key, and finds the archive "rom"
void func_02070ebc(void) {
    if (sys_get_boot_indicator() == OS_BOOTTYPE_DOWNLOAD_MB) {
        sArm9Overlays.address = (const u8 *)-1;
        sArm9Overlays.size = 0;
        sArm7Overlays.address = (const u8 *)-1;
        sArm7Overlays.size = 0;
    } else {
        sArm9Overlays.address = NULL;
        sArm9Overlays.size = 0;
        sArm7Overlays.address = NULL;
        sArm7Overlays.size = 0;
    }
    sDigestKey.data = sOverlayDigestKey;
    sDigestKey.size = sizeof(sOverlayDigestKey);
    sRomArchive = func_0207062c("rom", 3);
}

// NitroSDK's FSi_GetOverlayBinarySize: the size that an overlay's image has in the ROM, without its zeroed data
u32 sys_get_overlay_size(const FSOverlayInfo *info) {
    if (info->flag & FS_OVERLAY_FLAG_COMPRESS) {
        return info->compressedSize;
    }
    return info->ramSize;
}

// NitroSDK's FS_ClearOverlayImage: invalidates the caches over the overlay's memory, and clears the zeroed data that
// follows its image
void sys_prepare_overlay_area(const FSOverlayInfo *info) {
    u8 *ram = info->ramAddress;
    u32 ramSize = info->ramSize;
    u32 totalSize = ramSize + info->bssSize;

    cp15_invalidateIC(ram, totalSize);
    cp15_invalidateDC(ram, totalSize);
    sys_memset(ram + ramSize, 0, totalSize - ramSize);
}

// NitroSDK's FS_GetOverlayFileID: the archive and the number of the overlay's file
FSFileID sys_get_overlay_file_id(const FSOverlayInfo *info) {
    FSFileID fileId;

    fileId.arc = sRomArchive;
    fileId.file_id = info->fileId;
    return fileId;
}

// NitroSDK's FS_LoadOverlayInfo: reads an overlay's header from a processor's table, in memory or in the ROM, and finds
// its file
BOOL sys_read_overlay_header(FSOverlayInfo *info, MIProcessor target, u32 id) {
    BOOL result = FALSE;
    u32 offset = id * FS_OVERLAY_HEADER_SIZE;
    const FSOverlayTable *table = &sArm9Overlays;

    if (target != MI_PROCESSOR_ARM9) {
        table = &sArm7Overlays;
    }
    if (id < FS_OVERLAY_COUNT || hw_isDSi()) {
        if (table->address != NULL) {
            if (offset < table->size) {
                FSFile file;

                finit(&file);
                sys_memcpy(table->address + offset, info, FS_OVERLAY_HEADER_SIZE);
                info->target = target;
                if (romfs_fopen_id(&file, sys_get_overlay_file_id(info))) {
                    info->filePos = func_02072010(&file);
                    info->fileSize = romfs_fgetsize(&file);
                    romfs_fclose(&file);
                    result = TRUE;
                }
            }
        } else {
            const ROMHeaderRegion *region;

            if (target == MI_PROCESSOR_ARM9) {
                region = &rom_get_header()->arm9Overlay;
            } else {
                region = &rom_get_header()->arm7Overlay;
            }
            if (offset < region->size) {
                FSFile file;

                finit(&file);
                // BUG: The end of the table is passed where the function takes a size, so the range of the file runs
                // past the table (only its first 0x20 bytes are read)
#ifdef BUGFIX
                if (romfs_fopen_raw(&file, region->offset + offset, region->size - offset)) {
#else
                if (romfs_fopen_raw(&file, region->offset + offset, region->offset + region->size)) {
#endif
                    if (romfs_fread(&file, info, FS_OVERLAY_HEADER_SIZE) != FS_OVERLAY_HEADER_SIZE) {
                        romfs_fclose(&file);
                    } else {
                        romfs_fclose(&file);
                        info->target = target;
                        if (romfs_fopen_id(&file, sys_get_overlay_file_id(info))) {
                            info->filePos = func_02072010(&file);
                            info->fileSize = romfs_fgetsize(&file);
                            romfs_fclose(&file);
                            result = TRUE;
                        }
                    }
                }
            }
        }
    }
    return result;
}

// NitroSDK's FS_LoadOverlayImage: reads the overlay's image into its memory
BOOL sys_mount_overlay(const FSOverlayInfo *info) {
    BOOL result = FALSE;
    FSFile file;

    finit(&file);
    if (romfs_fopen_id(&file, sys_get_overlay_file_id(info))) {
        u32 size = sys_get_overlay_size(info);

        sys_prepare_overlay_area(info);
        if (size == romfs_fread(&file, info->ramAddress, size)) {
            result = TRUE;
        }
        romfs_fclose(&file);
    }
    return result;
}

// NitroSDK's FSi_CompareDigest: whether the HMAC-SHA1 of some data, under the key of the digests, is the digest that
// the build made for it. With masked set, the SHA-1 computes with two words of each block cleared; without it, a DSi
// lets its BIOS do the work
static BOOL func_020710a8(const FSOverlayDigest *expected, const void *data, u32 size, BOOL masked) {
    u32 i;
    u8 digest[MATH_SHA1_DIGEST_SIZE];
    u8 key[FS_OVERLAY_DIGEST_KEY_SIZE];

    sys_memset(digest, 0, sizeof(digest));
    sys_memcpy(sDigestKey.data, key, sDigestKey.size);
    if (!masked && hw_isDSi()) {
        func_0208287c(digest, data, size, key, sDigestKey.size);
    } else {
        u32 previous = 0;

        if (masked) {
            previous = sha1_set_masked(TRUE);
        }
        MATH_CalcHMACSHA1(digest, data, size, key, sDigestKey.size);
        if (masked) {
            sha1_set_masked(previous);
        }
    }
    for (i = 0; i < MATH_SHA1_DIGEST_SIZE; i += sizeof(u32)) {
        if (*(u32 *)(digest + i) != *(const u32 *)(expected->bytes + i)) {
            break;
        }
    }
    return i == MATH_SHA1_DIGEST_SIZE;
}

// NitroSDK's FS_StartOverlay: checks the image's digest when the game was not booted from the ROM, expands the image
// if it is compressed, and calls its static initializers
void sys_start_overlay(const FSOverlayInfo *info) {
    u32 size = sys_get_overlay_size(info);
    void (**initializer)(void);
    void (**initializersEnd)(void);

    if ((u32)info->ramAddress >= (u32)LTD_AUTOLOAD_0_TEXT_START &&
        (u32)info->ramAddress < (u32)LTD_AUTOLOAD_0_BSS_END && hw_isDSi() && !sLtdOverlayLoaded) {
        sLtdOverlayLoaded = TRUE;
    }
    if (sys_get_boot_indicator() != OS_BOOTTYPE_ROM) {
        BOOL verified = FALSE;

        if (info->flag & FS_OVERLAY_FLAG_AUTH) {
            u32 id = info->id;

            if (id < SDK_OVERLAY_DIGEST_END - SDK_OVERLAY_DIGEST) {
                verified = func_020710a8(&SDK_OVERLAY_DIGEST[id], info->ramAddress, size, FALSE);
            }
        }
        if (!verified) {
            sys_memset(info->ramAddress, 0, size);
            sys_exit();
            return;
        }
    }
    if (info->flag & FS_OVERLAY_FLAG_COMPRESS) {
        sys_uncomp_blz(info->ramAddress + size);
    }
    cp15_flushDC(info->ramAddress, info->ramSize);
    initializer = info->sinitStart;
    initializersEnd = info->sinitEnd;
    for (; initializer < initializersEnd; initializer++) {
        if (*initializer != NULL) {
            (*initializer)();
        }
    }
}

// NitroSDK's FS_EndOverlay: takes the objects that the overlay made out of the chain of global objects, and calls
// their destructors. A destructor can make new objects, so it starts over until none is left
void sys_call_ovl_dtors(const FSOverlayInfo *info) {
    for (;;) {
        u8 *start = info->ramAddress;
        u8 *end = start + (info->ramSize + info->bssSize);
        DestructorChain *called = NULL;
        DestructorChain *last = NULL;
        DestructorChain *previous;
        DestructorChain *chain;
        DestructorChain *head;
        u32 interrupts = CPU_IRQDisable();

        previous = NULL;
        head = __global_destructor_chain;
        chain = head;
        while (chain != NULL) {
            DestructorChain *next = chain->next;
            u8 *object = chain->object;
            u8 *destructor = (u8 *)chain->destructor;

            if ((object == NULL && destructor >= start && destructor < end) || (object >= start && object < end)) {
                if (last == NULL) {
                    called = chain;
                } else {
                    last->next = chain;
                }
                if (head == chain) {
                    head = next;
                    __global_destructor_chain = next;
                }
                chain->next = NULL;
                last = chain;
                if (previous != NULL) {
                    previous->next = next;
                }
            } else {
                previous = chain;
            }
            chain = next;
        }
        CPU_SetIRQMask(interrupts);
        if (called == NULL) {
            break;
        }
        do {
            DestructorChain *next = called->next;

            if (called->destructor != NULL) {
                called->destructor(called->object);
            }
            called = next;
        } while (called != NULL);
    }
}

// NitroSDK's FS_UnloadOverlayImage
BOOL sys_unmount_overlay(const FSOverlayInfo *info) {
    sys_call_ovl_dtors(info);
    return TRUE;
}

// NitroSDK's FS_LoadOverlay: reads an overlay's header and image, and starts it
BOOL sys_load_overlay(MIProcessor target, u32 id) {
    BOOL result = FALSE;
    FSOverlayInfo info;

    if (sys_read_overlay_header(&info, target, id) && sys_mount_overlay(&info)) {
        sys_start_overlay(&info);
        result = TRUE;
    }
    return result;
}

// NitroSDK's FS_UnloadOverlay: ends an overlay
BOOL sys_unload_overlay(MIProcessor target, u32 id) {
    FSOverlayInfo info;

    if (sys_read_overlay_header(&info, target, id) && sys_unmount_overlay(&info)) {
        return TRUE;
    }
    return FALSE;
}
