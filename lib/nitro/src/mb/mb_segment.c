// The multiboot parent's view of the program it hands out: reading it into a buffer as a list of segments, registering
// it, and cutting it into the blocks that go to a child (libmb). The names are descriptive, except swan's MB_*
#include "mb_private.h"

// The kind of memory each segment of a program is loaded to: the ROM header, ARM9 code and ARM7 code
static const u32 sSegmentKinds[MB_SEGMENT_COUNT] = {
    MB_SEGMENT_KIND_HEADER,
    MB_SEGMENT_KIND_ARM9,
    MB_SEGMENT_KIND_ARM7,
};

// The parts of a program's secure area that a ROM image keeps
static const MBSecureAreaPart sSecureAreaPartsDSi[] = {
    { 0x4000, 0x1400 },
    { 0x7400, 0x0c00 },
    { 0, 0 },
};

static const MBSecureAreaPart sSecureAreaParts[] = {
    { 0x4000, 0x1000 },
    { 0x7000, 0x1000 },
    { 0, 0 },
};

// Which of the two lists above applies to the program being read
static const MBSecureAreaPart *sSecureAreaTable = sSecureAreaParts;
// The next number to give a registered program
static u8 sRegisteredCount;

static void MBSegment_CopyRange(const MBSecureWindow *window, u32 start, u32 end, BOOL clear);
static void MBFile_InitSegments(MBCommFile *file, const ROMHeader *header);
static void MBFile_SetupSegment(const ROMHeader *header, const u32 *kind, MBSegment *segment, u32 *cursor);
static BOOL MBFile_IsSegmentArea(u32 index, u32 addr, u32 size);
static BOOL MBFile_IsRecvArea(u32 index, u32 addr, u32 size);

u32 MB_GetSegmentLength(FSFile *file) {
    const ROMHeader *header = NULL;
    u32 length = 0;
    u8 buf[0x60];

    if (file != NULL) {
        u32 pos = romfs_ftell(file);

        if (romfs_fread(file, buf, sizeof(buf)) >= sizeof(buf)) {
            header = (const ROMHeader *)buf;
        }
        romfs_fseek(file, pos, FS_SEEK_SET);
    } else {
        header = (const ROMHeader *)MB_ROM_HEADER_BUF;
    }
    if (header != NULL) {
        length = header->arm9.size + MB_SEGMENT_FIXED_SIZE + header->arm7.size;
        if (length < MB_SEGMENT_LENGTH_MIN) {
            length = MB_SEGMENT_LENGTH_MIN;
        }
    }
    return length;
}

// Records in a cache the archive it reads from, with the first characters of its name
static inline void MBCache_SetArchive(MBCache *cache, FSFile *file) {
    int length = 0;
    FSArchive *archive = file->arc;

    while (length < 3 && archive->name[length] != 0) {
        length++;
    }
    sys_memcpy(archive->name, cache->archiveName, length);
    cache->nameLength = length;
    cache->archive = archive;
}

// The ROM size a header gives, or 16 MB where it gives none
static inline u32 MBi_GetRomSize(const ROMHeader *header) {
    u32 size = header->romSize;

    if (size == 0) {
        size = 0x1000000;
    }
    return size;
}

BOOL MB_ReadSegment(FSFile *file, void *buf, u32 len) {
    ROMHeader *header = buf;
    u8 *dst;
    BOOL ok = FALSE;
    BOOL ownFile;
    u32 remaining;
    u32 romSize;
    u32 pos;
    MBCache *cache;
    MBRomOffsets *offsets;
    FSFile ownFileBuf;

    if (file == NULL) {
        if (!(rom_get_header()->unitCode & 2)) {
            sSecureAreaTable = sSecureAreaParts;
        } else {
            sSecureAreaTable = sSecureAreaPartsDSi;
        }
    }
    if (len >= sizeof(ROMHeader) + 4) {
        ownFile = FALSE;
        cache = NULL;
        offsets = NULL;
        dst = (u8 *)buf + sizeof(ROMHeader);
        remaining = len - sizeof(ROMHeader);
        if (file != NULL) {
            pos = romfs_ftell(file);
            if (romfs_fread(file, buf, 0x160) < 0x160) {
                remaining = 0;
            }
            romSize = MBi_GetRomSize(header);
        } else {
            romSize = MBi_GetRomSize((const ROMHeader *)MB_ROM_HEADER_BUF);
            ownFile = TRUE;
            file = &ownFileBuf;
            finit(file);
            if (!fopen_raw(file, func_0207062c("rom", 3), 0, romSize + 0x88, -1)) {
                return FALSE;
            }
            pos = romfs_ftell(file);
            sys_memcpy((const void *)MB_ROM_HEADER_BUF, buf, 0x160);
            header->unk_60 |= 0x406000;
        }

        if (remaining >= 0x88) {
            if (ownFile) {
                sys_memcpy(func_0206fed0(), dst, 0x88);
            } else {
                romfs_fseek(file, pos + romSize, FS_SEEK_SET);
                romfs_fread(file, dst, 0x88);
            }
            dst += 0x88;
            remaining -= 0x88;
        } else {
            remaining = 0;
        }

        if (remaining >= 0x70) {
            cache = (MBCache *)dst;
            MBCache_Clear(cache);
            remaining -= 0x70;
            dst += 0x70;
            MBCache_Add(cache, 0, 0x160, buf, MB_CACHE_ENTRY_KEPT);
            MBCache_SetArchive(cache, file);
        } else {
            remaining = 0;
        }

        if (remaining >= 0x10) {
            offsets = (MBRomOffsets *)dst;
            offsets->segment[0] = 0;
            offsets->segment[1] = pos + func_02072010(file) + header->arm9RomOffset;
            offsets->segment[2] = pos + func_02072010(file) + header->arm7RomOffset;
            dst += 0x10;
            remaining -= 0x10;
        } else {
            remaining = 0;
        }

        if (remaining >= header->arm9.size + header->arm7.size) {
            u32 base = func_02072010(file);
            u32 arm9Size = header->arm9.size;
            u32 arm9Pos = offsets->segment[1] - base;
            u8 *arm9Dst = dst;

            if (ownFile && arm9Pos < 0x8000) {
                arm9Size -= 0x8000 - arm9Pos;
                arm9Dst = dst + (0x8000 - arm9Pos);
                arm9Pos = 0x8000;
            }
            romfs_fseek(file, arm9Pos, FS_SEEK_SET);
            romfs_fread(file, arm9Dst, arm9Size);
            MBCache_Add(cache, offsets->segment[1], header->arm9.size, dst, MB_CACHE_ENTRY_KEPT);
            arm9Size = header->arm9.size;
            romfs_fseek(file, offsets->segment[2] - base, FS_SEEK_SET);
            romfs_fread(file, dst + arm9Size, header->arm7.size);
            MBCache_Add(cache, offsets->segment[2], header->arm7.size, dst + arm9Size, MB_CACHE_ENTRY_KEPT);
            ok = TRUE;
        } else if (remaining >= 3 * MB_CACHE_BLOCK_SIZE) {
            u32 base = func_02072010(file);
            u32 arm9Addr = offsets->segment[1];
            u32 arm9Pos = arm9Addr - base;
            u32 size = MB_CACHE_BLOCK_SIZE;
            u8 *arm9Dst = dst;

            if (ownFile && arm9Pos < 0x8000) {
                size -= 0x8000 - arm9Pos;
                arm9Dst = dst + (0x8000 - arm9Pos);
                arm9Pos = 0x8000;
            }
            romfs_fseek(file, arm9Pos, FS_SEEK_SET);
            romfs_fread(file, arm9Dst, size);
            MBCache_Add(cache, arm9Addr, MB_CACHE_BLOCK_SIZE, dst, MB_CACHE_ENTRY_KEPT);
            romfs_fseek(file, arm9Addr + MB_CACHE_BLOCK_SIZE - base, FS_SEEK_SET);
            romfs_fread(file, dst + MB_CACHE_BLOCK_SIZE, MB_CACHE_BLOCK_SIZE);
            MBCache_Add(cache, arm9Addr + MB_CACHE_BLOCK_SIZE, MB_CACHE_BLOCK_SIZE, dst + MB_CACHE_BLOCK_SIZE,
                        MB_CACHE_ENTRY_READY);
            romfs_fseek(file, arm9Addr + 2 * MB_CACHE_BLOCK_SIZE - base, FS_SEEK_SET);
            romfs_fread(file, dst + 2 * MB_CACHE_BLOCK_SIZE, MB_CACHE_BLOCK_SIZE);
            MBCache_Add(cache, arm9Addr + 2 * MB_CACHE_BLOCK_SIZE, MB_CACHE_BLOCK_SIZE, dst + 2 * MB_CACHE_BLOCK_SIZE,
                        MB_CACHE_ENTRY_READY);
            ok = TRUE;
        }
        romfs_fseek(file, pos, FS_SEEK_SET);

        if (ownFile) {
            romfs_fclose(&ownFileBuf);
            if (ok) {
                MBSecureWindow window;
                const MBSecureAreaPart *part = sSecureAreaTable;

                window.start = header->arm9RomOffset;
                window.srcBase = header->arm9.ramAddr - header->arm9RomOffset;
                window.dstBase = (u32)cache->entries[1].buf - header->arm9RomOffset;
                window.length = len;
                MBSegment_CopyRange(&window, 0x4000, 0x8000, TRUE);
                while (part->length != 0) {
                    MBSegment_CopyRange(&window, part->start, part->start + part->length, FALSE);
                    part++;
                }
                {
                u8 *arm9 = cache->entries[1].buf;

                *(u32 *)(arm9 + ((u32)AutoloadCallback - header->arm9.ramAddr)) = MB_ARM_BX_LR;
            }
            }
        }
        if (ok) {
            cp15_flushDC(buf, len);
        }
    }
    return ok;
}

// Clears or copies the part of a window between two ROM offsets that lies in the secure area
static void MBSegment_CopyRange(const MBSecureWindow *window, u32 start, u32 end, BOOL clear) {
    u32 limit;

    if (start < 0x4000) {
        start = 0x4000;
    }
    if (end > 0x8000) {
        end = 0x8000;
    }
    if (start < window->start) {
        start = window->start;
    }
    limit = window->start + window->length;
    if (end > limit) {
        end = limit;
    }
    if (start < end) {
        if (clear) {
            sys_memset((void *)(window->dstBase + start), 0, end - start);
        } else {
            sys_memcpy((const void *)(window->srcBase + start), (void *)(window->dstBase + start), end - start);
        }
    }
}

BOOL MB_RegisterFile(const MBGameRegistry *gameReg, const void *buf) {
    u32 slot = 0xff;
    u32 irq;
    u8 i;
    MBCommFile *file;

    irq = CPU_IRQDisable();
    if (!MBWm_IsStarted()) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    if (MB_PARENT_WORK->fileCount + 1 > MB_FILE_MAX) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    for (i = 0; i < MB_FILE_MAX; i++) {
        if (MB_PARENT_WORK->files[i].registry == gameReg) {
            CPU_SetIRQMask(irq);
            return FALSE;
        }
        if (!MB_PARENT_WORK->files[i].valid) {
            slot = i;
            break;
        }
    }
    if (i == MB_FILE_MAX) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }

    MB_PARENT_WORK->files[slot].registry = gameReg;
    file = &MB_PARENT_WORK->files[slot];
    MBFile_InitSegments(file, buf);
    sys_memcpy(gameReg->userParam, file->userParam, sizeof(file->userParam));
    if (!MBFile_BuildLayout(&MB_PARENT_WORK->files[slot].layout, file)) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    MBGameInfo_Build(&MB_PARENT_WORK->files[slot].record, gameReg, &gMBCommWork->user);
    MB_PARENT_WORK->files[slot].record.slot = slot;
    MBGameInfo_Link(&MB_PARENT_WORK->files[slot].record);
    MB_PARENT_WORK->files[slot].record.gameNo = sRegisteredCount++;
    MB_PARENT_WORK->files[slot].memberMask = 1;
    MB_PARENT_WORK->files[slot].buf = buf;
    MB_PARENT_WORK->files[slot].cache = &((MBSegmentImage *)buf)->cache;
    MB_PARENT_WORK->files[slot].segmentRomOffsets = ((const MBSegmentImage *)buf)->romOffsets.segment;
    if (MB_PARENT_WORK->files[slot].cache->entries[3].state != 0) {
        if (!MBTask_IsThreadStarted()) {
            MBTask_Clear(&MB_PARENT_WORK->task);
            MBTask_StartThread(&MB_PARENT_WORK->thread, 0x800);
        }
    }
    MB_PARENT_WORK->files[slot].valid = 1;
    MB_PARENT_WORK->fileCount++;
    CPU_SetIRQMask(irq);
    return TRUE;
}

// Fills in the segments of a registered program from its ROM header
static void MBFile_InitSegments(MBCommFile *file, const ROMHeader *header) {
    int i;
    const u32 *kind;
    MBSegment *segment;
    u32 cursor = MB_RECV_AREA_START;

    file->arm9Entry = header->arm9Entry;
    file->arm7Entry = header->arm7Entry;
    kind = sSegmentKinds;
    segment = file->segment;
    for (i = 0; i < MB_SEGMENT_COUNT; i++) {
        MBFile_SetupSegment(header, kind, segment, &cursor);
        segment++;
        kind++;
    }
    sys_memcpy((const u8 *)header + 0x160, file->romHeaderExtra, sizeof(file->romHeaderExtra));
}

// Describes where one segment is loaded in the child, and checks that it fits
static void MBFile_SetupSegment(const ROMHeader *header, const u32 *kind, MBSegment *segment, u32 *cursor) {
    switch (*kind) {
    case MB_SEGMENT_KIND_ARM9: {
        const ROMHeaderLoad *load = &header->arm9;

        if (load->ramAddr >= 0x2000000 && load->ramAddr < 0x22c0000 && load->ramAddr + load->size <= 0x22c0000) {
            segment->size = load->size;
            segment->ramAddr = load->ramAddr;
            segment->recvAddr = load->ramAddr;
            segment->isArm7 = FALSE;
        } else {
            sys_exit();
        }
        break;
    }
    case MB_SEGMENT_KIND_ARM7: {
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
            sys_exit();
        }
        segment->size = load->size;
        segment->ramAddr = load->ramAddr;
        if (!relocate) {
            segment->recvAddr = load->ramAddr;
        } else {
            segment->recvAddr = *cursor;
            *cursor += load->size;
        }
        segment->isArm7 = TRUE;
        break;
    }
    case MB_SEGMENT_KIND_HEADER:
        segment->size = 0x160;
        segment->ramAddr = MB_CHILD_ROM_HEADER_BUF;
        segment->recvAddr = MB_CHILD_ROM_HEADER_BUF;
        segment->isArm7 = FALSE;
        break;
    }
}

// Works out where each segment of a program starts in its image, and in blocks
BOOL MBFile_BuildLayout(MBBlockLayout *layout, const MBCommFile *file) {
    u16 *blockStart = layout->blockStart;
    u32 offset = 0;
    u8 i;

    if (file == NULL) {
        return FALSE;
    }
    for (i = 0; i < MB_SEGMENT_COUNT; i++) {
        layout->segOffset[i] = offset;
        offset += file->segment[i].size;
    }
    blockStart[0] = 0;
    for (i = 0; i < MB_SEGMENT_COUNT; i++) {
        u16 end;
        const MBSegment *segment = &file->segment[i];

        end = blockStart[i] + (u16)((segment->size + gMBCommWork->blockSize - 1) / gMBCommWork->blockSize);
        if (!MBFile_IsRecvArea(i, segment->ramAddr, segment->size)) {
            return FALSE;
        }
        if (i < 2) {
            blockStart[i + 1] = end;
        } else {
            layout->blockTotal = end;
        }
    }
    return TRUE;
}

// Finds the block of a program that a block number stands for
BOOL MBFile_LocateBlock(MBBlockSource *source, const MBBlockLayout *layout, u16 block, const MBCommFile *file) {
    s8 i;
    const MBSegment *segment;
    u32 offset;

    if (block >= layout->blockTotal) {
        return FALSE;
    }
    for (i = 2; i >= 0; i--) {
        if (block >= layout->blockStart[i]) {
            break;
        }
    }
    if (i < 0) {
        return FALSE;
    }
    offset = block - layout->blockStart[i];
    offset *= gMBCommWork->blockSize;
    segment = &file->segment[i];
    source->size = segment->size - offset;
    if (source->size > gMBCommWork->blockSize) {
        source->size = gMBCommWork->blockSize;
    }
    source->bufAddr = offset + layout->segOffset[i];
    source->recvAddr = offset + segment->recvAddr;
    source->segment = i;
    return TRUE;
}

// Whether a range lies where a program of this segment's kind may be loaded
static BOOL MBFile_IsSegmentArea(u32 index, u32 addr, u32 size) {
    switch (sSegmentKinds[index]) {
    case MB_SEGMENT_KIND_HEADER:
        if (addr >= MB_CHILD_ROM_HEADER_BUF && addr + size <= MB_CHILD_ROM_HEADER_BUF_END) {
            return TRUE;
        }
        break;
    case MB_SEGMENT_KIND_ARM9:
        if (addr >= 0x2000000 && addr + size <= 0x22c0000) {
            return TRUE;
        }
        break;
    case MB_SEGMENT_KIND_ARM7:
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

static BOOL MBFile_IsRecvArea(u32 index, u32 addr, u32 size) {
    switch (sSegmentKinds[index]) {
    case MB_SEGMENT_KIND_ARM9:
    case MB_SEGMENT_KIND_HEADER:
        return MBFile_IsSegmentArea(index, addr, size);
    case MB_SEGMENT_KIND_ARM7:
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

// Sends a frame that is in memory to the children in a bitmap
int MBComm_FlushAndSend(u32 length, u16 aidBitmap, void *buf) {
    cp15_flushDC(buf, (length + 0x1f) & ~0x1f);
    cp15_drainWriteBuffer();
    return MBWm_SendToChildren(buf, length, aidBitmap);
}
