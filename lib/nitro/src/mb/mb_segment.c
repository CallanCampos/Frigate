// The multiboot parent's view of the program it hands out: reading it into a buffer as a list of segments, registering
// it, and cutting it into the blocks that go to a child (libmb). It is the SDK's mb_fileinfo.c, as ntrtwl's
// reconstruction of NitroSDK has it. The names are NitroSDK's, as that reconstruction has them, except
// MBCache_SetArchive, MBi_GetRomSize, sRegisteredCount and sSecureAreaPartsDSi, which are ours
#include "mb_private.h"

// The kind of memory each segment of a program is loaded to: the ROM header, ARM9 code and ARM7 code
static const u32 MBi_defaultLoadSegList[MB_DL_SEGMENT_NUM] = {
    MB_SEG_ROMHEADER,
    MB_SEG_ARM9STATIC,
    MB_SEG_ARM7STATIC,
};

// The parts of a program's secure area that a ROM image keeps
static const CARDRomRegion sSecureAreaPartsDSi[] = {
    { 0x4000, 0x1400 },
    { 0x7400, 0x0c00 },
    { 0, 0 },
};

static const CARDRomRegion mbi_seg_header_default[] = {
    { 0x4000, 0x1000 },
    { 0x7000, 0x1000 },
    { 0, 0 },
};

// Which of the two lists above applies to the program being read
static const CARDRomRegion *mbi_seg_header = mbi_seg_header_default;
// The next number to give a registered program
static u8 sRegisteredCount;

static void MBi_ReadSegmentHeader(const MBiSegmentHeaderInfo *window, u32 start, u32 end, BOOL clear);
static void MBi_MakeDownloadFileInfo(MBCommFile *file, const CARDRomHeader *header);
static void MBi_SetSegmentInfo(const CARDRomHeader *header, const u32 *kind, MbSegmentInfo *segment, u32 *cursor);
static BOOL MBi_IsAbleToRecv(u32 index, u32 addr, u32 size);
static BOOL IsAbleToLoad(u32 index, u32 addr, u32 size);

u32 MB_GetSegmentLength(FSFile *file) {
    const CARDRomHeader *header = NULL;
    u32 length = 0;
    u8 buf[0x60];

    if (file != NULL) {
        u32 pos = FS_GetFilePosition(file);

        if (FS_ReadFile(file, buf, sizeof(buf)) >= sizeof(buf)) {
            header = (const CARDRomHeader *)buf;
        }
        FS_SeekFile(file, pos, FS_SEEK_SET);
    } else {
        header = (const CARDRomHeader *)HW_ROM_HEADER_BUF;
    }
    if (header != NULL) {
        length = header->arm9.size + MB_SEGMENT_FIXED_SIZE + header->arm7.size;
        if (length < MB_SEGMENT_BUFFER_MIN) {
            length = MB_SEGMENT_BUFFER_MIN;
        }
    }
    return length;
}

// Records in a cache the archive it reads from, with the first characters of its name
static inline void MBCache_SetArchive(MBiCacheList *cache, FSFile *file) {
    int length = 0;
    FSArchive *archive = file->arc;

    while (length < 3 && archive->name[length] != 0) {
        length++;
    }
    MI_CpuCopy8(archive->name, cache->arc_name, length);
    cache->arc_name_len = length;
    cache->arc_pointer = archive;
}

// The ROM size a header gives, or 16 MB where it gives none
static inline u32 MBi_GetRomSize(const CARDRomHeader *header) {
    u32 size = header->rom_size;

    if (size == 0) {
        size = 0x1000000;
    }
    return size;
}

BOOL MB_ReadSegment(FSFile *file, void *buf, u32 len) {
    CARDRomHeader *header = buf;
    u8 *dst;
    BOOL ok = FALSE;
    BOOL ownFile;
    u32 remaining;
    u32 romSize;
    u32 pos;
    MBiCacheList *cache;
    MBRomOffsets *offsets;
    FSFile ownFileBuf;

    if (file == NULL) {
        if (!(CARD_GetOwnRomHeader()->product_id & 2)) {
            mbi_seg_header = mbi_seg_header_default;
        } else {
            mbi_seg_header = sSecureAreaPartsDSi;
        }
    }
    if (len >= sizeof(CARDRomHeader) + 4) {
        ownFile = FALSE;
        cache = NULL;
        offsets = NULL;
        dst = (u8 *)buf + sizeof(CARDRomHeader);
        remaining = len - sizeof(CARDRomHeader);
        if (file != NULL) {
            pos = FS_GetFilePosition(file);
            if (FS_ReadFile(file, buf, 0x160) < 0x160) {
                remaining = 0;
            }
            romSize = MBi_GetRomSize(header);
        } else {
            romSize = MBi_GetRomSize((const CARDRomHeader *)HW_ROM_HEADER_BUF);
            ownFile = TRUE;
            file = &ownFileBuf;
            FS_InitFile(file);
            if (!FS_OpenFileDirect(file, FS_FindArchive("rom", 3), 0, romSize + 0x88, -1)) {
                return FALSE;
            }
            pos = FS_GetFilePosition(file);
            MI_CpuCopy8((const void *)HW_ROM_HEADER_BUF, buf, 0x160);
            header->normalCommandControl |= 0x406000;
        }

        if (remaining >= 0x88) {
            if (ownFile) {
                MI_CpuCopy8(CARDi_GetRomAuthCode(), dst, 0x88);
            } else {
                FS_SeekFile(file, pos + romSize, FS_SEEK_SET);
                FS_ReadFile(file, dst, 0x88);
            }
            dst += 0x88;
            remaining -= 0x88;
        } else {
            remaining = 0;
        }

        if (remaining >= 0x70) {
            cache = (MBiCacheList *)dst;
            MBi_InitCache(cache);
            remaining -= 0x70;
            dst += 0x70;
            MBi_AttachCacheBuffer(cache, 0, 0x160, buf, MB_CACHE_STATE_LOCKED);
            MBCache_SetArchive(cache, file);
        } else {
            remaining = 0;
        }

        if (remaining >= 0x10) {
            offsets = (MBRomOffsets *)dst;
            offsets->segment[0] = 0;
            offsets->segment[1] = pos + FS_GetFileImageTop(file) + header->main_rom_offset;
            offsets->segment[2] = pos + FS_GetFileImageTop(file) + header->sub_rom_offset;
            dst += 0x10;
            remaining -= 0x10;
        } else {
            remaining = 0;
        }

        if (remaining >= header->arm9.size + header->arm7.size) {
            u32 base = FS_GetFileImageTop(file);
            u32 arm9Size = header->arm9.size;
            u32 arm9Pos = offsets->segment[1] - base;
            u8 *arm9Dst = dst;

            if (ownFile && arm9Pos < 0x8000) {
                arm9Size -= 0x8000 - arm9Pos;
                arm9Dst = dst + (0x8000 - arm9Pos);
                arm9Pos = 0x8000;
            }
            FS_SeekFile(file, arm9Pos, FS_SEEK_SET);
            FS_ReadFile(file, arm9Dst, arm9Size);
            MBi_AttachCacheBuffer(cache, offsets->segment[1], header->arm9.size, dst, MB_CACHE_STATE_LOCKED);
            arm9Size = header->arm9.size;
            FS_SeekFile(file, offsets->segment[2] - base, FS_SEEK_SET);
            FS_ReadFile(file, dst + arm9Size, header->arm7.size);
            MBi_AttachCacheBuffer(cache, offsets->segment[2], header->arm7.size, dst + arm9Size, MB_CACHE_STATE_LOCKED);
            ok = TRUE;
        } else if (remaining >= 3 * MB_CACHE_PAGE_SIZE) {
            u32 base = FS_GetFileImageTop(file);
            u32 arm9Addr = offsets->segment[1];
            u32 arm9Pos = arm9Addr - base;
            u32 size = MB_CACHE_PAGE_SIZE;
            u8 *arm9Dst = dst;

            if (ownFile && arm9Pos < 0x8000) {
                size -= 0x8000 - arm9Pos;
                arm9Dst = dst + (0x8000 - arm9Pos);
                arm9Pos = 0x8000;
            }
            FS_SeekFile(file, arm9Pos, FS_SEEK_SET);
            FS_ReadFile(file, arm9Dst, size);
            MBi_AttachCacheBuffer(cache, arm9Addr, MB_CACHE_PAGE_SIZE, dst, MB_CACHE_STATE_LOCKED);
            FS_SeekFile(file, arm9Addr + MB_CACHE_PAGE_SIZE - base, FS_SEEK_SET);
            FS_ReadFile(file, dst + MB_CACHE_PAGE_SIZE, MB_CACHE_PAGE_SIZE);
            MBi_AttachCacheBuffer(cache, arm9Addr + MB_CACHE_PAGE_SIZE, MB_CACHE_PAGE_SIZE, dst + MB_CACHE_PAGE_SIZE,
                                  MB_CACHE_STATE_READY);
            FS_SeekFile(file, arm9Addr + 2 * MB_CACHE_PAGE_SIZE - base, FS_SEEK_SET);
            FS_ReadFile(file, dst + 2 * MB_CACHE_PAGE_SIZE, MB_CACHE_PAGE_SIZE);
            MBi_AttachCacheBuffer(cache, arm9Addr + 2 * MB_CACHE_PAGE_SIZE, MB_CACHE_PAGE_SIZE,
                                  dst + 2 * MB_CACHE_PAGE_SIZE, MB_CACHE_STATE_READY);
            ok = TRUE;
        }
        FS_SeekFile(file, pos, FS_SEEK_SET);

        if (ownFile) {
            FS_CloseFile(&ownFileBuf);
            if (ok) {
                MBiSegmentHeaderInfo window;
                const CARDRomRegion *part = mbi_seg_header;
                u8 *patch;

                window.rom_src = header->main_rom_offset;
                window.mem_src = header->arm9.ramAddr - header->main_rom_offset;
                window.mem_dst = (u32)cache->list[1].ptr - header->main_rom_offset;
                window.len = len;
                MBi_ReadSegmentHeader(&window, 0x4000, 0x8000, TRUE);
                while (part->length != 0) {
                    MBi_ReadSegmentHeader(&window, part->offset, part->offset + part->length, FALSE);
                    part++;
                }
                // Stub out the autoload callback in the child's copy of ARM9 with a BX LR
                patch = cache->list[1].ptr;
                patch += (u32)AutoloadCallback - header->arm9.ramAddr;
                *(u32 *)patch = MB_ARM_BX_LR;
            }
        }
        if (ok) {
            DC_FlushRange(buf, len);
        }
    }
    return ok;
}

// Clears or copies the part of a window between two ROM offsets that lies in the secure area
static void MBi_ReadSegmentHeader(const MBiSegmentHeaderInfo *window, u32 start, u32 end, BOOL clear) {
    u32 limit;

    if (start < 0x4000) {
        start = 0x4000;
    }
    if (end > 0x8000) {
        end = 0x8000;
    }
    if (start < window->rom_src) {
        start = window->rom_src;
    }
    limit = window->rom_src + window->len;
    if (end > limit) {
        end = limit;
    }
    if (start < end) {
        if (clear) {
            MI_CpuFill8((void *)(window->mem_dst + start), 0, end - start);
        } else {
            MI_CpuCopy8((const void *)(window->mem_src + start), (void *)(window->mem_dst + start), end - start);
        }
    }
}

BOOL MB_RegisterFile(const MBGameRegistry *gameReg, const void *buf) {
    u32 slot = 0xff;
    OSIntrMode irq;
    u8 i;
    MBCommFile *file;

    irq = OS_DisableInterrupts();
    if (!MBi_IsStarted()) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    if (pPwork->file_num + 1 > MB_MAX_FILE) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    for (i = 0; i < MB_MAX_FILE; i++) {
        if (pPwork->fileinfo[i].registry == gameReg) {
            OS_RestoreInterrupts(irq);
            return FALSE;
        }
        if (!pPwork->fileinfo[i].valid) {
            slot = i;
            break;
        }
    }
    if (i == MB_MAX_FILE) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }

    pPwork->fileinfo[slot].registry = gameReg;
    file = &pPwork->fileinfo[slot];
    MBi_MakeDownloadFileInfo(file, buf);
    MI_CpuCopy8(gameReg->userParam, file->userParam, sizeof(file->userParam));
    if (!MBi_MakeBlockInfoTable(&pPwork->fileinfo[slot].layout, file)) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    MBi_MakeGameInfo(&pPwork->fileinfo[slot].record, gameReg, &mbc->user);
    pPwork->fileinfo[slot].record.fileNo = slot;
    MB_AddGameInfo(&pPwork->fileinfo[slot].record);
    pPwork->fileinfo[slot].record.seqNoFixed = sRegisteredCount++;
    pPwork->fileinfo[slot].memberMask = 1;
    pPwork->fileinfo[slot].buf = buf;
    pPwork->fileinfo[slot].cache = &((MBSegmentImage *)buf)->cache;
    pPwork->fileinfo[slot].segmentRomOffsets = ((const MBSegmentImage *)buf)->romOffsets.segment;
    if (pPwork->fileinfo[slot].cache->list[3].state != 0) {
        if (!MBi_IsTaskAvailable()) {
            MBi_InitTaskInfo(&pPwork->cur_task);
            MBi_InitTaskThread(&pPwork->task_work, 0x800);
        }
    }
    pPwork->fileinfo[slot].valid = 1;
    pPwork->file_num++;
    OS_RestoreInterrupts(irq);
    return TRUE;
}

// Fills in the segments of a registered program from its ROM header
static void MBi_MakeDownloadFileInfo(MBCommFile *file, const CARDRomHeader *header) {
    int i;
    const u32 *kind;
    MbSegmentInfo *segment;
    u32 cursor = MB_ARM7_STATIC_RECV_BUFFER;

    file->arm9Entry = header->main_entry_address;
    file->arm7Entry = header->sub_entry_address;
    kind = MBi_defaultLoadSegList;
    segment = file->segment;
    for (i = 0; i < MB_DL_SEGMENT_NUM; i++) {
        MBi_SetSegmentInfo(header, kind, segment, &cursor);
        segment++;
        kind++;
    }
    MI_CpuCopy8((const u8 *)header + 0x160, file->romHeaderExtra, sizeof(file->romHeaderExtra));
}

// Describes where one segment is loaded in the child, and checks that it fits
static void MBi_SetSegmentInfo(const CARDRomHeader *header, const u32 *kind, MbSegmentInfo *segment, u32 *cursor) {
    switch (*kind) {
    case MB_SEG_ARM9STATIC: {
        const ROMHeaderLoad *load = &header->arm9;

        if (load->ramAddr >= 0x2000000 && load->ramAddr < 0x22c0000 && load->ramAddr + load->size <= 0x22c0000) {
            segment->size = load->size;
            segment->load_addr = load->ramAddr;
            segment->recv_addr = load->ramAddr;
            segment->target = FALSE;
        } else {
            OS_Terminate();
        }
        break;
    }
    case MB_SEG_ARM7STATIC: {
        const ROMHeaderLoad *load = &header->arm7;
        BOOL invalid = FALSE;
        BOOL relocate = FALSE;
        u32 end = load->ramAddr + load->size;

        if (load->ramAddr >= 0x2000000 && load->ramAddr < 0x23fe800) {
            if (end > 0x2300000) {
                if (end < 0x23fe800 && load->size <= 0x40000) {
                    relocate = TRUE;
                } else {
                    invalid = TRUE;
                }
            }
        } else if (load->ramAddr >= 0x3040000 && load->ramAddr < 0x3057000) {
            if (end <= 0x3057000) {
                relocate = TRUE;
            } else {
                invalid = TRUE;
            }
        } else {
            invalid = TRUE;
        }
        if (invalid == TRUE) {
            OS_Terminate();
        }
        segment->size = load->size;
        segment->load_addr = load->ramAddr;
        if (!relocate) {
            segment->recv_addr = load->ramAddr;
        } else {
            segment->recv_addr = *cursor;
            *cursor += load->size;
        }
        segment->target = TRUE;
        break;
    }
    case MB_SEG_ROMHEADER:
        segment->size = 0x160;
        segment->load_addr = MB_ROM_HEADER_ADDRESS;
        segment->recv_addr = MB_ROM_HEADER_ADDRESS;
        segment->target = FALSE;
        break;
    }
}

// Works out where each segment of a program starts in its image, and in blocks
BOOL MBi_MakeBlockInfoTable(MB_BlockInfoTable *layout, const MBCommFile *file) {
    u16 *blockStart = layout->seg_head_blockno;
    u32 offset = 0;
    u8 i;

    if (file == NULL) {
        return FALSE;
    }
    for (i = 0; i < MB_DL_SEGMENT_NUM; i++) {
        layout->seg_src_offset[i] = offset;
        offset += file->segment[i].size;
    }
    blockStart[0] = 0;
    for (i = 0; i < MB_DL_SEGMENT_NUM; i++) {
        u16 end;
        const MbSegmentInfo *segment = &file->segment[i];

        end = blockStart[i] + (u16)((segment->size + mbc->block_size_max - 1) / mbc->block_size_max);
        if (!IsAbleToLoad(i, segment->load_addr, segment->size)) {
            return FALSE;
        }
        if (i < 2) {
            blockStart[i + 1] = end;
        } else {
            layout->block_num = end;
        }
    }
    return TRUE;
}

// Finds the block of a program that a block number stands for
BOOL MBi_get_blockinfo(MB_BlockInfo *source, const MB_BlockInfoTable *layout, u16 block, MBCommFile *file) {
    s8 i;
    MbSegmentInfo *segment;
    u32 offset;

    if (block >= layout->block_num) {
        return FALSE;
    }
    for (i = 2; i >= 0; i--) {
        if (block >= layout->seg_head_blockno[i]) {
            break;
        }
    }
    if (i < 0) {
        return FALSE;
    }
    offset = block - layout->seg_head_blockno[i];
    offset *= mbc->block_size_max;
    segment = &file->segment[i];
    source->size = segment->size - offset;
    if (source->size > mbc->block_size_max) {
        source->size = mbc->block_size_max;
    }
    source->offset = offset + layout->seg_src_offset[i];
    source->child_address = offset + segment->recv_addr;
    source->segment_no = i;
    return TRUE;
}

// Whether a range lies where a program of this segment's kind may be loaded
static BOOL MBi_IsAbleToRecv(u32 index, u32 addr, u32 size) {
    switch (MBi_defaultLoadSegList[index]) {
    case MB_SEG_ROMHEADER:
        if (addr >= MB_ROM_HEADER_ADDRESS && addr + size <= MB_CHILD_ROM_HEADER_BUF_END) {
            return TRUE;
        }
        break;
    case MB_SEG_ARM9STATIC:
        if (addr >= 0x2000000 && addr + size <= 0x22c0000) {
            return TRUE;
        }
        break;
    case MB_SEG_ARM7STATIC:
        if (addr >= 0x22c0000 && addr + size <= 0x2300000) {
            return TRUE;
        }
        if (addr >= 0x2000000 && addr + size <= 0x2300000) {
            return TRUE;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

static BOOL IsAbleToLoad(u32 index, u32 addr, u32 size) {
    switch (MBi_defaultLoadSegList[index]) {
    case MB_SEG_ARM9STATIC:
    case MB_SEG_ROMHEADER:
        return MBi_IsAbleToRecv(index, addr, size);
    case MB_SEG_ARM7STATIC:
        if (addr >= 0x2000000 && addr < 0x23fe800) {
            u32 end = addr + size;

            if (addr < 0x2300000 && end > 0x2300000) {
                return FALSE;
            }
            if (end <= 0x2300000) {
                return TRUE;
            }
            if (end < 0x23fe800 && size <= 0x40000) {
                return TRUE;
            }
            return FALSE;
        } else if (addr >= 0x3040000 && addr < 0x3057000) {
            if (addr + size <= 0x3057000) {
                return TRUE;
            }
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
