// NitroSDK's file system (libfs): overlays. They are found in the ROM's tables of overlay headers, read into memory,
// checked against the digests of the build when the game was not booted from the ROM, and started and ended with their
// static initializers and destructors. The file name is NitroSDK's (fs_overlay.c, as ntrtwl's reconstruction of
// NitroSDK has it); no string in the ROM names it. The functions have NitroSDK's names where they are known.

#include "nitro/card.h"
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

static FSOverlayTable fsi_ovt9;
static FSOverlayTable fsi_ovt7;
static FSDigestKey sDigestKey;
static FSArchive *sRomArchive;
static BOOL sLtdOverlayLoaded;

#define FS_OVERLAY_DIGEST_KEY_SIZE 64

static const u8 fsi_def_digest_key[FS_OVERLAY_DIGEST_KEY_SIZE] = {
    0x21, 0x06, 0xc0, 0xde, 0xba, 0x98, 0xce, 0x3f, 0xa6, 0x92, 0xe3, 0x9d, 0x46, 0xf2, 0xed, 0x01,
    0x76, 0xe3, 0xcc, 0x08, 0x56, 0x23, 0x63, 0xfa, 0xca, 0xd4, 0xec, 0xdf, 0x9a, 0x62, 0x78, 0x34,
    0x8f, 0x6d, 0x63, 0x3c, 0xfe, 0x22, 0xca, 0x92, 0x20, 0x88, 0x97, 0x23, 0xd2, 0xcf, 0xae, 0xc2,
    0x32, 0x67, 0x8d, 0xfe, 0xca, 0x83, 0x64, 0x98, 0xac, 0xfd, 0x3e, 0x37, 0x87, 0x46, 0x58, 0x24,
};

static BOOL FSi_CompareDigest(const FSOverlayDigest *expected, const void *data, u32 size, BOOL masked);

// NitroSDK's FSi_InitOverlay: sets the overlay tables and the digest key, and finds the archive "rom"
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
    sDigestKey.data = fsi_def_digest_key;
    sDigestKey.size = sizeof(fsi_def_digest_key);
    sRomArchive = FS_FindArchive("rom", 3);
}

// NitroSDK's FSi_GetOverlayBinarySize: the size that an overlay's image has in the ROM, without its zeroed data
u32 FSi_GetOverlayBinarySize(const FSOverlayInfo *info) {
    if (info->flag & FS_OVERLAY_FLAG_COMP) {
        return info->compressedSize;
    }
    return info->ramSize;
}

// NitroSDK's FS_ClearOverlayImage: invalidates the caches over the overlay's memory, and clears the zeroed data that
// follows its image
void FS_ClearOverlayImage(FSOverlayInfo *info) {
    u8 *ram = info->ramAddress;
    u32 ramSize = info->ramSize;
    u32 totalSize = ramSize + info->bssSize;

    IC_InvalidateRange(ram, totalSize);
    DC_InvalidateRange(ram, totalSize);
    MI_CpuFill8(ram + ramSize, 0, totalSize - ramSize);
}

// NitroSDK's FS_GetOverlayFileID: the archive and the number of the overlay's file
FSFileID FS_GetOverlayFileID(const FSOverlayInfo *info) {
    FSFileID fileId;

    fileId.arc = sRomArchive;
    fileId.file_id = info->fileId;
    return fileId;
}

// NitroSDK's FS_LoadOverlayInfo: reads an overlay's header from a processor's table, in memory or in the ROM, and finds
// its file
BOOL FS_LoadOverlayInfo(FSOverlayInfo *info, MIProcessor target, FSOverlayID id) {
    BOOL result = FALSE;
    u32 offset = id * FS_OVERLAY_HEADER_SIZE;
    const FSOverlayTable *table = &fsi_ovt9;

    if (target != MI_PROCESSOR_ARM9) {
        table = &fsi_ovt7;
    }
    if (id < FS_OVERLAY_COUNT || OS_IsRunOnTwl()) {
        if (table->address != NULL) {
            if (offset < table->size) {
                FSFile file;

                FS_InitFile(&file);
                MI_CpuCopy8(table->address + offset, info, FS_OVERLAY_HEADER_SIZE);
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
                // BUG: The end of the table is passed where the function takes a size, so the range of the file runs
                // past the table (only its first 0x20 bytes are read)
#ifdef BUGFIX
                if (FS_CreateFileFromRom(&file, region->offset + offset, region->length - offset)) {
#else
                if (FS_CreateFileFromRom(&file, region->offset + offset, region->offset + region->length)) {
#endif
                    if (FS_ReadFile(&file, info, FS_OVERLAY_HEADER_SIZE) != FS_OVERLAY_HEADER_SIZE) {
                        FS_CloseFile(&file);
                    } else {
                        FS_CloseFile(&file);
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
    }
    return result;
}

// NitroSDK's FS_LoadOverlayImage: reads the overlay's image into its memory
BOOL FS_LoadOverlayImage(FSOverlayInfo *info) {
    BOOL result = FALSE;
    FSFile file;

    FS_InitFile(&file);
    if (FS_OpenFileFast(&file, FS_GetOverlayFileID(info))) {
        u32 size = FSi_GetOverlayBinarySize(info);

        FS_ClearOverlayImage(info);
        if (size == FS_ReadFile(&file, info->ramAddress, size)) {
            result = TRUE;
        }
        FS_CloseFile(&file);
    }
    return result;
}

// NitroSDK's FSi_CompareDigest: whether the HMAC-SHA1 of some data, under the key of the digests, is the digest that
// the build made for it. With masked set, the SHA-1 computes with two words of each block cleared; without it, a DSi
// lets its BIOS do the work
static BOOL FSi_CompareDigest(const FSOverlayDigest *expected, const void *data, u32 size, BOOL masked) {
    u32 i;
    u8 digest[MATH_SHA1_DIGEST_SIZE];
    u8 key[FS_OVERLAY_DIGEST_KEY_SIZE];

    MI_CpuFill8(digest, 0, sizeof(digest));
    MI_CpuCopy8(sDigestKey.data, key, sDigestKey.size);
    if (!masked && OS_IsRunOnTwl()) {
        SVC_CalcHMACSHA1(digest, data, size, key, sDigestKey.size);
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
void FS_StartOverlay(FSOverlayInfo *info) {
    u32 size = FSi_GetOverlayBinarySize(info);
    void (**initializer)(void);
    void (**initializersEnd)(void);

    if ((u32)info->ramAddress >= (u32)LTD_AUTOLOAD_0_TEXT_START &&
        (u32)info->ramAddress < (u32)LTD_AUTOLOAD_0_BSS_END && OS_IsRunOnTwl() && !sLtdOverlayLoaded) {
        sLtdOverlayLoaded = TRUE;
    }
    if (OS_GetBootType() != OS_BOOTTYPE_ROM) {
        BOOL verified = FALSE;

        if (info->flag & FS_OVERLAY_FLAG_AUTH) {
            u32 id = info->id;

            if (id < SDK_OVERLAY_DIGEST_END - SDK_OVERLAY_DIGEST) {
                verified = FSi_CompareDigest(&SDK_OVERLAY_DIGEST[id], info->ramAddress, size, FALSE);
            }
        }
        if (!verified) {
            MI_CpuFill8(info->ramAddress, 0, size);
            OS_Terminate();
            return;
        }
    }
    if (info->flag & FS_OVERLAY_FLAG_COMP) {
        MIi_UncompressBackward(info->ramAddress + size);
    }
    DC_FlushRange(info->ramAddress, info->ramSize);
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
void FS_EndOverlay(FSOverlayInfo *info) {
    for (;;) {
        u8 *start = info->ramAddress;
        u8 *end = start + (info->ramSize + info->bssSize);
        DestructorChain *called = NULL;
        DestructorChain *last = NULL;
        DestructorChain *previous;
        DestructorChain *chain;
        DestructorChain *head;
        OSIntrMode interrupts = OS_DisableInterrupts();

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
        OS_RestoreInterrupts(interrupts);
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

BOOL FS_UnloadOverlayImage(FSOverlayInfo *info) {
    FS_EndOverlay(info);
    return TRUE;
}

// NitroSDK's FS_LoadOverlay: reads an overlay's header and image, and starts it
BOOL FS_LoadOverlay(MIProcessor target, FSOverlayID id) {
    BOOL result = FALSE;
    FSOverlayInfo info;

    if (FS_LoadOverlayInfo(&info, target, id) && FS_LoadOverlayImage(&info)) {
        FS_StartOverlay(&info);
        result = TRUE;
    }
    return result;
}

// NitroSDK's FS_UnloadOverlay: ends an overlay
BOOL FS_UnloadOverlay(MIProcessor target, FSOverlayID id) {
    FSOverlayInfo info;

    if (FS_LoadOverlayInfo(&info, target, id) && FS_UnloadOverlayImage(&info)) {
        return TRUE;
    }
    return FALSE;
}
