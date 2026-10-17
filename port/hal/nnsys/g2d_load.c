// Host versions of NitroSystem's G2D file loaders (lib/nnsys/src/g2d/g2d_NCG_load.c, g2d_NCL_load.c, g2d_NCE_load.c,
// g2d_NMC_load.c, g2d_NAN_load.c, g2d_NFT_load.c and g2d_Load.c's NNSi_G2dUnpackUserExCellAttrBank). On the DS each
// turns a loaded file's offsets into pointers in place (NNSi_G2dUnpackOffset), so the file's bytes become the runtime
// structs of nnsys/g2d.h. A host's runtime structs hold 8-byte pointers, so each function here reads the file through
// its DS layout (port/hal/nnsys/g2d_file.h) and builds a tree of host structs instead:
//   - every value is copied as it is, and each pointer is set where the DS's relocation would point it;
//   - a pointer to raw data (pixels, palettes, OAM attributes, hierarchy nodes, transfer entries, animation frames'
//     contents, user attributes, glyphs) points into the file's bytes, as on the DS, since those keep their layout;
//   - a pointer to structs that hold pointers (cells, multi-cells, sequences, frames, attribute arrays, font widths and
//     code maps) points to a host array of the host structs, built at the host's element size;
//   - a field the DS doesn't relocate (pStringBank, and a frame no sequence reaches) keeps its offset, as on the DS.
// The file's bytes stay as they were, except that a version 1.0 font gets its glyphs' flags cleared, which the DS does
// in the file too. port/cmake/nnsys.cmake compiles the decomp's files with these functions renamed port_sdk_<name>; the
// other functions of those files (NNS_G2dFindBinaryBlock, NNS_G2dGetCellDataByIdx, NNS_G2dGetMultiCellDataByIdx,
// NNS_G2dGetAnimSequenceByIdx, NNS_G2dGetUnpackedScreenData, which relocates nothing) stay the decomp's.
//
// Each tree is registered in the relocation registry (port/hal/nnsys/reloc.h) under the file struct's address, with
// the file's bytes from its header to the last one read as its range, so it dies when the heap block holding the file
// is freed. Unpacking the same struct again rebuilds its tree in place, at the same root address, as the DS relocates
// the bytes then loaded at that address. That covers a file loaded into a buffer the platform layer never sees freed (a
// static or stack buffer, a work buffer kept across loads):
//   - when the bytes the tree was built from are still there, the rebuild reuses every host struct, so each keeps its
//     address, and every field the game changed since (clact.c moves a palette's rawData and shrinks its size, and
//     rewrites a character's mappingType) gets the file's value again, as a new load and unpack gives on the DS. The
//     host never writes the file's bytes, so it can't tell such a load of the same file from a second unpack with no
//     load between; on the DS that one would add the base to every pointer again, which no working game relies on;
//   - when they differ (another file), the host structs are built anew and the old ones freed.
#include "hal/nnsys/g2d_load.h"
#include "hal/nnsys/g2d_file.h"
#include "hal/trap.h"
#include "nitro/os.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A host allocation a tree owns. A tree's blocks are listed in the order its build allocated them
typedef struct ArenaBlock {
    struct ArenaBlock *next;
    size_t size;
    _Alignas(16) unsigned char data[];
} ArenaBlock;

// The file bytes a tree was built from, to tell whether a file loaded at the same address later is the same
typedef struct SnapshotRange {
    uintptr_t at;
    size_t size;
} SnapshotRange;

typedef struct Snapshot {
    SnapshotRange *ranges;
    size_t count;
    unsigned char *bytes;
} Snapshot;

// A tree: the root the game gets, first, so that the tree the registry holds is the root (port_reloc_find returns what
// the loaders return), and what it owns
typedef struct G2dTree {
    union {
        NNSG2dCharacterData character;
        NNSG2dPaletteData palette;
        NNSG2dPaletteCompressInfo paletteCmp;
        NNSG2dCellDataBank cellBank;
        NNSG2dMultiCellDataBank multiCellBank;
        NNSG2dAnimBankData animBank;
        NNSG2dUserExCellAttrBank cellExAttr;
        PortG2dFont font;
    } root;
    PortRelocKind kind;
    ArenaBlock *arena;
    Snapshot snapshot;
} G2dTree;

_Static_assert(offsetof(G2dTree, root) == 0, "the registry's tree must be the root");

// A user extended data block as the DS finds it, its header and then the attribute bank: NNSi_G2dGetUserExDataBlkByID
// and the code after it take the bank at blk + 1
typedef struct UserExCellAttrBlock {
    NNSG2dUserExDataBlock header;
    NNSG2dUserExCellAttrBank bank;
} UserExCellAttrBlock;

typedef struct UserExAnimAttrBlock {
    NNSG2dUserExDataBlock header;
    NNSG2dUserExAnimAttrBank bank;
} UserExAnimAttrBlock;

_Static_assert(offsetof(UserExCellAttrBlock, bank) == sizeof(NNSG2dUserExDataBlock), "the bank must follow the header");
_Static_assert(offsetof(UserExAnimAttrBlock, bank) == sizeof(NNSG2dUserExDataBlock), "the bank must follow the header");

// One build: the tree it fills, the file's extent, and the bytes read so far
typedef struct G2dBuild {
    G2dTree *tree;
    ArenaBlock **arenaNext; // The link to the tree's next block to reuse, or to set to a new one
    uintptr_t fileBegin;
    uintptr_t fileEnd; // 0 when unknown
    uintptr_t readEnd; // Past the last byte read
    SnapshotRange *ranges;
    size_t rangeCount;
    size_t rangeCapacity;
} G2dBuild;

static char sLastError[256];
static PortG2dAnimStats sAnimStats;

static _Noreturn void OutOfMemory(size_t size) {
    fprintf(stderr, "port: g2d: out of host memory for %zu bytes\n", size);
    abort();
}

// The address offset bytes past base, as NNSi_G2dUnpackOffset makes it (without the DS's 32-bit wrap, which no file
// relies on: the sum stays inside the file)
static void *Rel(const void *base, u32 offset) {
    return (void *)((uintptr_t)base + offset);
}

// A pointer field the DS leaves as it is in the file: the offset, as a pointer
static void *Unrelocated(u32 offset) {
    return (void *)(uintptr_t)offset;
}

// A zeroed host struct of size bytes for the tree: its next block when a rebuild of the same file reuses them, in the
// order the first build allocated them, or a new one
static void *Alloc(G2dBuild *build, size_t size) {
    ArenaBlock *block = *build->arenaNext;

    if (block != NULL && block->size == size) {
        memset(block->data, 0, size);
    } else {
        block = calloc(1, sizeof(ArenaBlock) + size);
        if (block == NULL) {
            OutOfMemory(size);
        }
        block->size = size;
        block->next = *build->arenaNext;
        *build->arenaNext = block;
    }
    build->arenaNext = &block->next;
    return block->data;
}

// Reads size bytes of the file at address into dest, and notes them in the snapshot. FALSE when they lie outside the
// file, which the DS would read anyway: a file that does is broken, and the host stops on it
static BOOL Read(G2dBuild *build, const void *address, void *dest, size_t size, const char *what) {
    uintptr_t at = (uintptr_t)address;

    if (at < build->fileBegin || (build->fileEnd != 0 && (at > build->fileEnd || size > build->fileEnd - at))) {
        snprintf(sLastError, sizeof(sLastError), "%s at offset 0x%zx of a file of 0x%zx bytes lies outside it", what,
                 (size_t)(at - build->fileBegin), (size_t)(build->fileEnd - build->fileBegin));
        return FALSE;
    }
    memcpy(dest, address, size);
    if (at + size > build->readEnd) {
        build->readEnd = at + size;
    }
    if (size == 0) {
        return TRUE;
    }
    if (build->rangeCount != 0) {
        SnapshotRange *last = &build->ranges[build->rangeCount - 1];

        if (last->at + last->size == at) {
            last->size += size;
            return TRUE;
        }
    }
    if (build->rangeCount == build->rangeCapacity) {
        size_t capacity = build->rangeCapacity != 0 ? build->rangeCapacity * 2 : 16;
        SnapshotRange *ranges = realloc(build->ranges, capacity * sizeof(SnapshotRange));

        if (ranges == NULL) {
            OutOfMemory(capacity * sizeof(SnapshotRange));
        }
        build->ranges = ranges;
        build->rangeCapacity = capacity;
    }
    build->ranges[build->rangeCount].at = at;
    build->ranges[build->rangeCount].size = size;
    build->rangeCount++;
    return TRUE;
}

static void FreeSnapshot(Snapshot *snapshot) {
    free(snapshot->ranges);
    free(snapshot->bytes);
    memset(snapshot, 0, sizeof(*snapshot));
}

// Keeps a copy of every byte the build read
static void TakeSnapshot(G2dBuild *build, Snapshot *snapshot) {
    size_t total = 0;
    unsigned char *bytes;

    for (size_t i = 0; i < build->rangeCount; i++) {
        total += build->ranges[i].size;
    }
    bytes = malloc(total != 0 ? total : 1);
    if (bytes == NULL) {
        OutOfMemory(total);
    }
    total = 0;
    for (size_t i = 0; i < build->rangeCount; i++) {
        memcpy(bytes + total, (const void *)build->ranges[i].at, build->ranges[i].size);
        total += build->ranges[i].size;
    }
    snapshot->ranges = build->ranges;
    snapshot->count = build->rangeCount;
    snapshot->bytes = bytes;
    build->ranges = NULL;
    build->rangeCount = 0;
    build->rangeCapacity = 0;
}

// Whether the memory still holds the bytes the tree was built from. With the file's extent known, a range outside it
// (the earlier file was larger) means it doesn't, without reading memory that may no longer be the file's
static BOOL SnapshotMatches(const Snapshot *snapshot, uintptr_t fileBegin, uintptr_t fileEnd) {
    size_t offset = 0;

    for (size_t i = 0; i < snapshot->count; i++) {
        const SnapshotRange *range = &snapshot->ranges[i];

        if (fileEnd != 0 && (range->at < fileBegin || range->at > fileEnd || range->size > fileEnd - range->at)) {
            return FALSE;
        }
        if (memcmp((const void *)range->at, snapshot->bytes + offset, range->size) != 0) {
            return FALSE;
        }
        offset += range->size;
    }
    return TRUE;
}

static void FreeBlocks(ArenaBlock *block) {
    while (block != NULL) {
        ArenaBlock *next = block->next;

        free(block);
        block = next;
    }
}

static void FreeArena(G2dTree *tree) {
    FreeBlocks(tree->arena);
    tree->arena = NULL;
}

static void DestroyTree(void *tree) {
    G2dTree *g2dTree = tree;

    FreeArena(g2dTree);
    FreeSnapshot(&g2dTree->snapshot);
    free(g2dTree);
}

// NNS_G2dUnpackNCG and NNS_G2dUnpackBGNCG, which are the same: the characters follow the struct
static BOOL BuildCharacter(G2dBuild *build, const u8 *at, NNSG2dCharacterData *character) {
    PortG2dFileCharacterData file;

    if (!Read(build, at, &file, sizeof(file), "NNSG2dCharacterData")) {
        return FALSE;
    }
    character->height = file.height;
    character->width = file.width;
    character->pixelFormat = file.pixelFormat;
    character->mappingType = file.mappingType;
    character->characterFormat = file.characterFormat;
    character->size = file.size;
    character->rawData = Rel(at, file.rawData);
    return TRUE;
}

// NNS_G2dUnpackNCL
static BOOL BuildPalette(G2dBuild *build, const u8 *at, NNSG2dPaletteData *palette) {
    PortG2dFilePaletteData file;

    if (!Read(build, at, &file, sizeof(file), "NNSG2dPaletteData")) {
        return FALSE;
    }
    palette->format = file.format;
    palette->extendedPalette = file.extendedPalette;
    palette->size = file.size;
    palette->rawData = Rel(at, file.rawData);
    return TRUE;
}

// NNSi_G2dUnpackNCLCmpInfo
static BOOL BuildPaletteCompressInfo(G2dBuild *build, const u8 *at, NNSG2dPaletteCompressInfo *info) {
    PortG2dFilePaletteCompressInfo file;

    if (!Read(build, at, &file, sizeof(file), "NNSG2dPaletteCompressInfo")) {
        return FALSE;
    }
    info->numPalette = file.numPalette;
    info->pad = file.pad;
    info->paletteIndexTable = Rel(at, file.paletteIndexTable);
    return TRUE;
}

// NNSi_G2dUnpackUserExCellAttrBank: the attribute array, and each cell's attributes, from the bank
static BOOL BuildUserExCellAttrBank(G2dBuild *build, const u8 *at, NNSG2dUserExCellAttrBank *attrBank) {
    PortG2dFileUserExCellAttrBank file;
    const u8 *attrArray;
    NNSG2dUserExCellAttr *cellAttrs;

    if (!Read(build, at, &file, sizeof(file), "NNSG2dUserExCellAttrBank")) {
        return FALSE;
    }
    attrBank->numCells = file.numCells;
    attrBank->numAttribute = file.numAttribute;
    attrArray = Rel(at, file.pCellAttrArray);
    cellAttrs = Alloc(build, file.numCells * sizeof(NNSG2dUserExCellAttr));
    for (u16 i = 0; i < file.numCells; i++) {
        PortG2dFileUserExCellAttr attr;

        if (!Read(build, attrArray + i * sizeof(attr), &attr, sizeof(attr), "NNSG2dUserExCellAttr")) {
            return FALSE;
        }
        cellAttrs[i].pAttr = Rel(at, attr.pAttr);
    }
    attrBank->pCellAttrArray = cellAttrs;
    return TRUE;
}

// The user extended data of a cell or multi-cell bank, which NNS_G2dUnpackNCE and NNS_G2dUnpackNMC take for cell
// attributes whatever its ID (UnPackExtendedData_): the header as it is, then the attribute bank
static BOOL BuildUserExCellAttrBlock(G2dBuild *build, const u8 *at, NNSG2dUserExDataBlock **dest) {
    UserExCellAttrBlock *block = Alloc(build, sizeof(UserExCellAttrBlock));

    if (!Read(build, at, &block->header, sizeof(block->header), "NNSG2dUserExDataBlock")) {
        return FALSE;
    }
    *dest = &block->header;
    return BuildUserExCellAttrBank(build, at + sizeof(NNSG2dUserExDataBlock), &block->bank);
}

// NNS_G2dUnpackNCE: the cells, at the host's NNSG2dCellData or NNSG2dCellDataWithBR size as the bank says, whose OAM
// attributes follow the last cell; the VRAM transfer data; the user extended data
static BOOL BuildCellBank(G2dBuild *build, const u8 *at, NNSG2dCellDataBank *bank) {
    PortG2dFileCellDataBank file;
    const u8 *cellArray;
    const u8 *oamArray;
    BOOL withBR;
    size_t fileCellSize;

    if (!Read(build, at, &file, sizeof(file), "NNSG2dCellDataBank")) {
        return FALSE;
    }
    bank->numCells = file.numCells;
    bank->cellBankAttr = file.cellBankAttr;
    bank->mappingMode = file.mappingMode;
    bank->pStringBank = Unrelocated(file.pStringBank);

    withBR = (file.cellBankAttr & NNS_G2D_CELLBK_ATTR_CELLWITHBR) != 0;
    fileCellSize = withBR ? sizeof(PortG2dFileCellDataWithBR) : sizeof(PortG2dFileCellData);
    cellArray = Rel(at, file.pCellDataArrayHead);
    oamArray = cellArray + file.numCells * fileCellSize;
    if (withBR) {
        NNSG2dCellDataWithBR *cells = Alloc(build, file.numCells * sizeof(NNSG2dCellDataWithBR));

        for (u16 i = 0; i < file.numCells; i++) {
            PortG2dFileCellDataWithBR cell;

            if (!Read(build, cellArray + i * fileCellSize, &cell, sizeof(cell), "NNSG2dCellDataWithBR")) {
                return FALSE;
            }
            cells[i].cellData.numOAMAttrs = cell.cellData.numOAMAttrs;
            cells[i].cellData.cellAttr = cell.cellData.cellAttr;
            cells[i].cellData.pOamAttrArray = Rel(oamArray, cell.cellData.pOamAttrArray);
            cells[i].boundingRect = cell.boundingRect;
        }
        bank->pCellDataArrayHead = &cells->cellData;
    } else {
        NNSG2dCellData *cells = Alloc(build, file.numCells * sizeof(NNSG2dCellData));

        for (u16 i = 0; i < file.numCells; i++) {
            PortG2dFileCellData cell;

            if (!Read(build, cellArray + i * fileCellSize, &cell, sizeof(cell), "NNSG2dCellData")) {
                return FALSE;
            }
            cells[i].numOAMAttrs = cell.numOAMAttrs;
            cells[i].cellAttr = cell.cellAttr;
            cells[i].pOamAttrArray = Rel(oamArray, cell.pOamAttrArray);
        }
        bank->pCellDataArrayHead = cells;
    }

    if (file.pVramTransferData != 0) {
        const u8 *transferAt = Rel(at, file.pVramTransferData);
        PortG2dFileVramTransferData transferFile;
        NNSG2dVramTransferData *transfer = Alloc(build, sizeof(NNSG2dVramTransferData));

        if (!Read(build, transferAt, &transferFile, sizeof(transferFile), "NNSG2dVramTransferData")) {
            return FALSE;
        }
        transfer->szByteMax = transferFile.szByteMax;
        transfer->pCellTransferDataArray = Rel(transferAt, transferFile.pCellTransferDataArray);
        bank->pVramTransferData = transfer;
    }

    if (file.pExtendedData != 0 &&
        !BuildUserExCellAttrBlock(build, Rel(at, file.pExtendedData), &bank->pExtendedData)) {
        return FALSE;
    }
    return TRUE;
}

// NNS_G2dUnpackNMC: the multi-cells, whose nodes stay in the file's hierarchy array, and the user extended data
static BOOL BuildMultiCellBank(G2dBuild *build, const u8 *at, NNSG2dMultiCellDataBank *bank) {
    PortG2dFileMultiCellDataBank file;
    const u8 *cellArray;
    u8 *hierarchy;
    NNSG2dMultiCellData *cells;

    if (!Read(build, at, &file, sizeof(file), "NNSG2dMultiCellDataBank")) {
        return FALSE;
    }
    bank->numMultiCellData = file.numMultiCellData;
    bank->pad = file.pad;
    bank->pStringBank = Unrelocated(file.pStringBank);
    cellArray = Rel(at, file.pMultiCellDataArray);
    hierarchy = Rel(at, file.pHierarchyDataArray);
    bank->pHierarchyDataArray = hierarchy;

    cells = Alloc(build, file.numMultiCellData * sizeof(NNSG2dMultiCellData));
    for (u16 i = 0; i < file.numMultiCellData; i++) {
        PortG2dFileMultiCellData cell;

        if (!Read(build, cellArray + i * sizeof(cell), &cell, sizeof(cell), "NNSG2dMultiCellData")) {
            return FALSE;
        }
        cells[i].numNodes = cell.numNodes;
        cells[i].numCellAnim = cell.numCellAnim;
        cells[i].pHierDataArray = Rel(hierarchy, cell.pHierDataArray);
    }
    bank->pMultiCellDataArray = cells;

    if (file.pExtendedData != 0 &&
        !BuildUserExCellAttrBlock(build, Rel(at, file.pExtendedData), &bank->pExtendedData)) {
        return FALSE;
    }
    return TRUE;
}

// A frame array of numFrames at frames, each frame's content relocated from the animation data
static NNSG2dAnimFrameData *BuildFrames(G2dBuild *build, const u8 *frames, u16 numFrames, const u8 *animData) {
    NNSG2dAnimFrameData *hostFrames = Alloc(build, numFrames * sizeof(NNSG2dAnimFrameData));

    for (u16 i = 0; i < numFrames; i++) {
        PortG2dFileAnimFrameData frame;

        if (!Read(build, frames + i * sizeof(frame), &frame, sizeof(frame), "NNSG2dAnimFrameData")) {
            return NULL;
        }
        hostFrames[i].pContent = Rel(animData, frame.pContent);
        hostFrames[i].frames = frame.frames;
        hostFrames[i].pad = frame.pad;
    }
    return hostFrames;
}

// The user extended attributes of an animation bank's sequences and frames, all from the attribute bank
static BOOL BuildUserExAnimAttrBlock(G2dBuild *build, const u8 *at, NNSG2dUserExDataBlock **dest) {
    UserExAnimAttrBlock *block = Alloc(build, sizeof(UserExAnimAttrBlock));
    const u8 *attrAt = at + sizeof(NNSG2dUserExDataBlock);
    PortG2dFileUserExAnimAttrBank file;
    const u8 *seqAttrArray;
    NNSG2dUserExAnimSequenceAttr *seqAttrs;

    if (!Read(build, at, &block->header, sizeof(block->header), "NNSG2dUserExDataBlock") ||
        !Read(build, attrAt, &file, sizeof(file), "NNSG2dUserExAnimAttrBank")) {
        return FALSE;
    }
    *dest = &block->header;
    block->bank.numSequences = file.numSequences;
    block->bank.numAttribute = file.numAttribute;
    seqAttrArray = Rel(attrAt, file.pAnmSeqAttrArray);
    seqAttrs = Alloc(build, file.numSequences * sizeof(NNSG2dUserExAnimSequenceAttr));
    for (u16 i = 0; i < file.numSequences; i++) {
        PortG2dFileUserExAnimSequenceAttr seqAttr;
        const u8 *frameAttrArray;
        NNSG2dUserExAnimFrameAttr *frameAttrs;

        if (!Read(build, seqAttrArray + i * sizeof(seqAttr), &seqAttr, sizeof(seqAttr),
                  "NNSG2dUserExAnimSequenceAttr")) {
            return FALSE;
        }
        seqAttrs[i].numFrames = seqAttr.numFrames;
        seqAttrs[i].pad = seqAttr.pad;
        seqAttrs[i].pAttr = Rel(attrAt, seqAttr.pAttr);
        frameAttrArray = Rel(attrAt, seqAttr.pAnmFrmAttrArray);
        frameAttrs = Alloc(build, seqAttr.numFrames * sizeof(NNSG2dUserExAnimFrameAttr));
        for (u16 j = 0; j < seqAttr.numFrames; j++) {
            PortG2dFileUserExAnimFrameAttr frameAttr;

            if (!Read(build, frameAttrArray + j * sizeof(frameAttr), &frameAttr, sizeof(frameAttr),
                      "NNSG2dUserExAnimFrameAttr")) {
                return FALSE;
            }
            frameAttrs[j].pAttr = Rel(attrAt, frameAttr.pAttr);
        }
        seqAttrs[i].pAnmFrmAttrArray = frameAttrs;
    }
    block->bank.pAnmSeqAttrArray = seqAttrs;
    return TRUE;
}

// NNS_G2dUnpackNAN: the sequences, the bank's frame array, whose frames point into the animation data, and the user
// extended data. The DS relocates a frame's content through each sequence that reaches it, so a frame no sequence
// reaches keeps its offset, as on the host, and a frame two sequences share gets the base added twice, where the host
// relocates it once. A sequence's frames are the bank's own where they lie whole in the bank's frame array, so
// pointers into it agree. Otherwise (no file does) they are an array of their own, read where the DS reads them: the
// bank's frames a sequence that starts on one of them reaches are relocated, as on the DS, and the file's bytes past
// the bank's array stay as they are, where the DS would add the base to them. A sequence that doesn't start on a frame
// leaves the bank's frames as they are, where the DS would add the base to parts of two frames
static BOOL BuildAnimBank(G2dBuild *build, const u8 *at, NNSG2dAnimBankData *bank) {
    PortG2dFileAnimBankData file;
    const u8 *seqArray;
    const u8 *frameArray;
    u8 *animData;
    NNSG2dAnimFrameData *frames;
    NNSG2dAnimSequence *sequences;
    u8 *reached;
    u32 *contents;

    if (!Read(build, at, &file, sizeof(file), "NNSG2dAnimBankData")) {
        return FALSE;
    }
    bank->numSequences = file.numSequences;
    bank->numTotalFrames = file.numTotalFrames;
    bank->pStringBank = Unrelocated(file.pStringBank);
    seqArray = Rel(at, file.pSequenceArrayHead);
    frameArray = Rel(at, file.pFrameArray);
    animData = Rel(at, file.pAnimationData);
    bank->pAnimationData = animData;

    // The bank's frames, their contents kept as offsets until a sequence reaches them
    frames = Alloc(build, file.numTotalFrames * sizeof(NNSG2dAnimFrameData));
    reached = calloc(file.numTotalFrames != 0 ? file.numTotalFrames : 1, 1);
    contents = calloc(file.numTotalFrames != 0 ? file.numTotalFrames : 1, sizeof(u32));
    if (reached == NULL || contents == NULL) {
        OutOfMemory(file.numTotalFrames * 5u);
    }
    for (u16 k = 0; k < file.numTotalFrames; k++) {
        PortG2dFileAnimFrameData frame;

        if (!Read(build, frameArray + k * sizeof(frame), &frame, sizeof(frame), "NNSG2dAnimFrameData")) {
            free(reached);
            free(contents);
            return FALSE;
        }
        contents[k] = frame.pContent;
        frames[k].pContent = Unrelocated(frame.pContent);
        frames[k].frames = frame.frames;
        frames[k].pad = frame.pad;
    }
    bank->pFrameArray = frames;

    sequences = Alloc(build, file.numSequences * sizeof(NNSG2dAnimSequence));
    for (u16 i = 0; i < file.numSequences; i++) {
        PortG2dFileAnimSequence seq;
        u32 offset;
        u32 first;
        BOOL aligned;

        if (!Read(build, seqArray + i * sizeof(seq), &seq, sizeof(seq), "NNSG2dAnimSequence")) {
            free(reached);
            free(contents);
            return FALSE;
        }
        sequences[i].numFrames = seq.numFrames;
        sequences[i].loopStartFrameIdx = seq.loopStartFrameIdx;
        sequences[i].animType = seq.animType;
        sequences[i].playMode = seq.playMode;
        offset = seq.pAnmFrameArray;
        aligned = offset % sizeof(PortG2dFileAnimFrameData) == 0;
        first = offset / sizeof(PortG2dFileAnimFrameData);
        if (aligned) {
            for (u32 k = first; k < file.numTotalFrames && k - first < seq.numFrames; k++) {
                if (reached[k] < 0xff) {
                    reached[k]++;
                }
            }
        }
        if (aligned && first + seq.numFrames <= file.numTotalFrames) {
            sequences[i].pAnmFrameArray = &frames[first];
        } else {
            sequences[i].pAnmFrameArray = BuildFrames(build, frameArray + offset, seq.numFrames, animData);
            if (sequences[i].pAnmFrameArray == NULL) {
                free(reached);
                free(contents);
                return FALSE;
            }
        }
    }
    bank->pSequenceArrayHead = sequences;

    for (u16 k = 0; k < file.numTotalFrames; k++) {
        if (reached[k] != 0) {
            frames[k].pContent = Rel(animData, contents[k]);
        }
        if (reached[k] > 1) {
            sAnimStats.framesSharedBySequences++;
        } else if (reached[k] == 0) {
            sAnimStats.framesNeverRelocated++;
        }
    }
    free(reached);
    free(contents);

    if (file.pExtendedData != 0 &&
        !BuildUserExAnimAttrBlock(build, Rel(at, file.pExtendedData), &bank->pExtendedData)) {
        return FALSE;
    }
    return TRUE;
}

// A font's width or code map struct, with its table: the entries its indices or codes call for, or the rest of its
// block when that is more. inBlock says whether the DS relocates its pNext (it does for the struct at a block's start)
static PortG2dFontNode *BuildFontNode(G2dBuild *build, PortG2dFont *font, const u8 *at, u32 kind, size_t blockRest,
                                      BOOL inBlock) {
    PortG2dFontNode *node = Alloc(build, sizeof(PortG2dFontNode));

    node->at = at;
    node->kind = kind;
    node->inBlock = inBlock;
    if (kind == NNS_G2D_BINBLK_SIG_FINFDATA) {
        PortG2dFileFontInformation file;
        NNSG2dFontInformation *info = &font->info;

        // The first in the file is the root's; the blocks are built in the file's order
        for (const PortG2dFontNode *other = font->nodes; other != NULL; other = other->next) {
            if (other->host == &font->info) {
                info = Alloc(build, sizeof(NNSG2dFontInformation));
                break;
            }
        }
        if (!Read(build, at, &file, sizeof(file), "NNSG2dFontInformation")) {
            return NULL;
        }
        info->fontType = file.fontType;
        info->linefeed = file.linefeed;
        info->alterCharIndex = file.alterCharIndex;
        info->defaultWidth = file.defaultWidth;
        info->encoding = file.encoding;
        node->host = info;
    } else if (kind == NNS_G2D_BINBLK_SIG_CWDHDATA) {
        PortG2dFileFontWidth file;
        NNSG2dFontWidth *width;
        size_t bytes;

        if (!Read(build, at, &file, sizeof(file), "NNSG2dFontWidth")) {
            return NULL;
        }
        bytes = file.indexEnd >= file.indexBegin ? (file.indexEnd - file.indexBegin + 1) * sizeof(NNSG2dCharWidths) : 0;
        if (blockRest > bytes) {
            bytes = blockRest;
        }
        width = Alloc(build, offsetof(NNSG2dFontWidth, widthTable) + bytes);
        width->indexBegin = file.indexBegin;
        width->indexEnd = file.indexEnd;
        width->pNext = inBlock || file.pNext == 0 ? NULL : Unrelocated(file.pNext);
        if (!Read(build, at + sizeof(file), width->widthTable, bytes, "NNSG2dFontWidth's table")) {
            return NULL;
        }
        node->host = width;
    } else {
        PortG2dFileFontCodeMap file;
        NNSG2dFontCodeMap *map;
        size_t bytes = 0;

        if (!Read(build, at, &file, sizeof(file), "NNSG2dFontCodeMap")) {
            return NULL;
        }
        switch (file.mappingMethod) {
        case NNS_G2D_MAPMETHOD_DIRECT:
            bytes = sizeof(u16);
            break;
        case NNS_G2D_MAPMETHOD_TABLE:
            bytes = file.ccodeEnd >= file.ccodeBegin ? (file.ccodeEnd - file.ccodeBegin + 1) * sizeof(u16) : 0;
            break;
        case NNS_G2D_MAPMETHOD_SCAN: {
            u16 num;

            if (!Read(build, at + sizeof(file), &num, sizeof(num), "NNSG2dCMapInfoScan")) {
                return NULL;
            }
            bytes = offsetof(NNSG2dCMapInfoScan, entries) + num * sizeof(NNSG2dCMapScanEntry);
            break;
        }
        }
        if (blockRest > bytes) {
            bytes = blockRest;
        }
        map = Alloc(build, offsetof(NNSG2dFontCodeMap, mapInfo) + bytes);
        map->ccodeBegin = file.ccodeBegin;
        map->ccodeEnd = file.ccodeEnd;
        map->mappingMethod = file.mappingMethod;
        map->reserved = file.reserved;
        map->pNext = inBlock || file.pNext == 0 ? NULL : Unrelocated(file.pNext);
        if (!Read(build, at + sizeof(file), map->mapInfo, bytes, "NNSG2dFontCodeMap's map")) {
            return NULL;
        }
        node->host = map;
    }
    node->next = font->nodes;
    font->nodes = node;
    return node;
}

void *port_g2d_font_node(const PortG2dFont *font, const void *at, u32 kind) {
    for (const PortG2dFontNode *node = font->nodes; node != NULL; node = node->next) {
        if (node->at == at && node->kind == kind) {
            return node->host;
        }
    }
    return NULL;
}

// The host struct for a pointer of the font, offset from the file's header: the block's node, or one of its own when
// the pointer leads elsewhere
static void *FontTarget(G2dBuild *build, PortG2dFont *font, const u8 *header, u32 offset, u32 kind, BOOL *failed) {
    const u8 *at = Rel(header, offset);
    void *host = port_g2d_font_node(font, at, kind);
    PortG2dFontNode *node;

    if (host != NULL) {
        return host;
    }
    node = BuildFontNode(build, font, at, kind, 0, FALSE);
    if (node == NULL) {
        *failed = TRUE;
        return NULL;
    }
    return node->host;
}

// NNSi_G2dUnpackNFT: every FINF, CWDH and CMAP block, each relocated from the file's header: a FINF's glyphs (always),
// widths and code maps (when not 0), and each width and code map block's next one (when not 0)
static BOOL BuildFont(G2dBuild *build, const u8 *header, PortG2dFont *font) {
    NNSG2dBinaryFileHeader file;
    const u8 *blk;
    BOOL failed = FALSE;

    if (!Read(build, header, &file, sizeof(file), "NNSG2dBinaryFileHeader")) {
        return FALSE;
    }
    // The nodes first, so that the pointers can find them whichever block comes first
    blk = header + file.headerSize;
    for (int i = 0; i < file.numBlocks; i++) {
        NNSG2dBinaryBlockHeader blockHeader;

        if (!Read(build, blk, &blockHeader, sizeof(blockHeader), "NNSG2dBinaryBlockHeader")) {
            return FALSE;
        }
        if (blockHeader.kind == NNS_G2D_BINBLK_SIG_FINFDATA || blockHeader.kind == NNS_G2D_BINBLK_SIG_CWDHDATA ||
            blockHeader.kind == NNS_G2D_BINBLK_SIG_CMAPDATA) {
            size_t fixed = blockHeader.kind == NNS_G2D_BINBLK_SIG_CWDHDATA   ? sizeof(PortG2dFileFontWidth)
                           : blockHeader.kind == NNS_G2D_BINBLK_SIG_CMAPDATA ? sizeof(PortG2dFileFontCodeMap)
                                                                             : 0;
            size_t used = sizeof(blockHeader) + fixed;
            size_t rest = blockHeader.size > used ? blockHeader.size - used : 0;

            if (BuildFontNode(build, font, blk + sizeof(blockHeader), blockHeader.kind, rest, TRUE) == NULL) {
                return FALSE;
            }
        }
        blk += blockHeader.size;
    }

    // Then the pointers of the structs at blocks' starts, as the DS relocates them
    for (PortG2dFontNode *node = font->nodes; node != NULL; node = node->next) {
        if (!node->inBlock) {
            continue;
        }
        if (node->kind == NNS_G2D_BINBLK_SIG_FINFDATA) {
            PortG2dFileFontInformation info;
            NNSG2dFontInformation *host = node->host;

            if (!Read(build, node->at, &info, sizeof(info), "NNSG2dFontInformation")) {
                return FALSE;
            }
            host->pGlyph = Rel(header, info.pGlyph);
            host->pWidth = info.pWidth != 0
                               ? FontTarget(build, font, header, info.pWidth, NNS_G2D_BINBLK_SIG_CWDHDATA, &failed)
                               : NULL;
            host->pMap = info.pMap != 0
                             ? FontTarget(build, font, header, info.pMap, NNS_G2D_BINBLK_SIG_CMAPDATA, &failed)
                             : NULL;
        } else if (node->kind == NNS_G2D_BINBLK_SIG_CWDHDATA) {
            PortG2dFileFontWidth width;
            NNSG2dFontWidth *host = node->host;

            if (!Read(build, node->at, &width, sizeof(width), "NNSG2dFontWidth")) {
                return FALSE;
            }
            host->pNext = width.pNext != 0
                              ? FontTarget(build, font, header, width.pNext, NNS_G2D_BINBLK_SIG_CWDHDATA, &failed)
                              : NULL;
        } else {
            PortG2dFileFontCodeMap map;
            NNSG2dFontCodeMap *host = node->host;

            if (!Read(build, node->at, &map, sizeof(map), "NNSG2dFontCodeMap")) {
                return FALSE;
            }
            host->pNext = map.pNext != 0
                              ? FontTarget(build, font, header, map.pNext, NNS_G2D_BINBLK_SIG_CMAPDATA, &failed)
                              : NULL;
        }
        if (failed) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL Build(G2dBuild *build, PortRelocKind kind, const u8 *at) {
    G2dTree *tree = build->tree;

    switch (kind) {
    case PORT_RELOC_G2D_CHARACTER:
        return BuildCharacter(build, at, &tree->root.character);
    case PORT_RELOC_G2D_PALETTE:
        return BuildPalette(build, at, &tree->root.palette);
    case PORT_RELOC_G2D_PALETTE_CMP:
        return BuildPaletteCompressInfo(build, at, &tree->root.paletteCmp);
    case PORT_RELOC_G2D_CELL_BANK:
        return BuildCellBank(build, at, &tree->root.cellBank);
    case PORT_RELOC_G2D_MULTICELL:
        return BuildMultiCellBank(build, at, &tree->root.multiCellBank);
    case PORT_RELOC_G2D_ANIM_BANK:
        return BuildAnimBank(build, at, &tree->root.animBank);
    case PORT_RELOC_G2D_CELL_EXATTR:
        return BuildUserExCellAttrBank(build, at, &tree->root.cellExAttr);
    case PORT_RELOC_G2D_FONT:
        return BuildFont(build, at, &tree->root.font);
    default:
        snprintf(sLastError, sizeof(sLastError), "kind %d isn't a G2D file's", (int)kind);
        return FALSE;
    }
}

void *port_g2d_unpack(PortRelocKind kind, const void *file, size_t fileSize, const void *at) {
    G2dBuild build = { 0 };
    G2dTree *tree = port_reloc_find(at, kind);
    BOOL fresh = tree == NULL;
    uintptr_t begin = (uintptr_t)(fileSize != 0 ? file : at);
    uintptr_t end = fileSize != 0 ? begin + fileSize : 0;

    if (tree != NULL) {
        // Rebuilt in place, so that the root keeps its address. The same file reads the same bytes and makes the same
        // allocations, so it gets the same host structs back; another file gets new ones
        if (!SnapshotMatches(&tree->snapshot, begin, end)) {
            FreeArena(tree);
        }
        FreeSnapshot(&tree->snapshot);
        memset(&tree->root, 0, sizeof(tree->root));
    } else {
        tree = calloc(1, sizeof(G2dTree));
        if (tree == NULL) {
            OutOfMemory(sizeof(G2dTree));
        }
        tree->kind = kind;
    }

    build.tree = tree;
    build.arenaNext = &tree->arena;
    build.fileBegin = begin;
    build.fileEnd = end;
    build.readEnd = begin;
    if (!Build(&build, kind, at)) {
        free(build.ranges);
        if (fresh) {
            DestroyTree(tree);
        } else {
            port_reloc_release(at, kind);
        }
        return NULL;
    }
    // Blocks a rebuild didn't reuse (none, for the same file)
    FreeBlocks(*build.arenaNext);
    *build.arenaNext = NULL;
    TakeSnapshot(&build, &tree->snapshot);
    // The range is the file's bytes up to the last one the tree was built from, not the header's size: some files say
    // they are larger than they are (3,562 of the ROM's NCGR files claim 0x2030 bytes and hold 0x1230), and a range
    // past the heap block would let a free of the next block take the tree. Its raw data lies in the same block
    port_reloc_register(at, kind, (const void *)begin, build.readEnd - begin, tree, DestroyTree);
    return &tree->root;
}

const char *port_g2d_last_error(void) {
    return sLastError;
}

const PortG2dAnimStats *port_g2d_anim_stats(void) {
    return &sAnimStats;
}

// The file's size from its header, the extent of every read
static size_t FileSize(const void *file) {
    return ((const NNSG2dBinaryFileHeader *)file)->fileSize;
}

// A file the host can't unpack stops the game, with what went wrong: the DS would go on with pointers out of the file
static void *UnpackOrStop(PortRelocKind kind, const void *file, size_t fileSize, const void *at, const char *function) {
    void *root = port_g2d_unpack(kind, file, fileSize, at);

    if (root == NULL) {
        fprintf(stderr, "port: g2d: %s(%p): %s\n", function, (void *)(fileSize != 0 ? file : at), sLastError);
        fflush(stderr);
        _Exit(PORT_TRAP_EXIT_STATUS);
    }
    return root;
}

// The functions of the decomp's files, in their order there. Each Get finds its block as the DS does, with the
// decomp's NNS_G2dFindBinaryBlock, and the Unpack functions that take a struct build its tree with no known extent

// g2d_NCG_load.c

BOOL NNS_G2dGetUnpackedCharacterData(void *file, NNSG2dCharacterData **character) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_CHARDATA);

    if (blk != NULL) {
        *character = UnpackOrStop(PORT_RELOC_G2D_CHARACTER, file, FileSize(file), blk + 1, __func__);
        return TRUE;
    }
    *character = NULL;
    return FALSE;
}

void NNS_G2dUnpackNCG(NNSG2dCharacterData *character) {
    UnpackOrStop(PORT_RELOC_G2D_CHARACTER, NULL, 0, character, __func__);
}

BOOL NNS_G2dGetUnpackedBGCharacterData(void *file, NNSG2dCharacterData **character) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_CHARDATA);

    if (blk != NULL) {
        *character = UnpackOrStop(PORT_RELOC_G2D_CHARACTER, file, FileSize(file), blk + 1, __func__);
        return TRUE;
    }
    *character = NULL;
    return FALSE;
}

void NNS_G2dUnpackBGNCG(NNSG2dCharacterData *character) {
    UnpackOrStop(PORT_RELOC_G2D_CHARACTER, NULL, 0, character, __func__);
}

// g2d_NCL_load.c

BOOL NNS_G2dGetUnpackedPaletteData(void *file, NNSG2dPaletteData **palette) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_PALETTEDATA);

    if (blk != NULL) {
        *palette = UnpackOrStop(PORT_RELOC_G2D_PALETTE, file, FileSize(file), blk + 1, __func__);
        return TRUE;
    }
    *palette = NULL;
    return FALSE;
}

BOOL NNS_G2dGetUnpackedPaletteCompressInfo(void *file, NNSG2dPaletteCompressInfo **info) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_PALETTECOMPRESSINFO);

    if (blk != NULL) {
        *info = UnpackOrStop(PORT_RELOC_G2D_PALETTE_CMP, file, FileSize(file), blk + 1, __func__);
        return TRUE;
    }
    *info = NULL;
    return FALSE;
}

void NNSi_G2dUnpackNCLCmpInfo(NNSG2dPaletteCompressInfo *info) {
    UnpackOrStop(PORT_RELOC_G2D_PALETTE_CMP, NULL, 0, info, __func__);
}

void NNS_G2dUnpackNCL(NNSG2dPaletteData *palette) {
    UnpackOrStop(PORT_RELOC_G2D_PALETTE, NULL, 0, palette, __func__);
}

// g2d_NCE_load.c

BOOL NNS_G2dGetUnpackedCellBank(void *file, NNSG2dCellDataBank **cells) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_CELLBANK);

    if (blk != NULL) {
        *cells = UnpackOrStop(PORT_RELOC_G2D_CELL_BANK, file, FileSize(file), blk + 1, __func__);
        return TRUE;
    }
    *cells = NULL;
    return FALSE;
}

void NNS_G2dUnpackNCE(NNSG2dCellDataBank *bank) {
    UnpackOrStop(PORT_RELOC_G2D_CELL_BANK, NULL, 0, bank, __func__);
}

// g2d_NMC_load.c

BOOL NNS_G2dGetUnpackedMultiCellBank(void *file, NNSG2dMultiCellDataBank **cells) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_MULTICELLBANK);

    if (blk != NULL) {
        *cells = UnpackOrStop(PORT_RELOC_G2D_MULTICELL, file, FileSize(file), blk + 1, __func__);
        return TRUE;
    }
    *cells = NULL;
    return FALSE;
}

void NNS_G2dUnpackNMC(NNSG2dMultiCellDataBank *bank) {
    UnpackOrStop(PORT_RELOC_G2D_MULTICELL, NULL, 0, bank, __func__);
}

// g2d_NAN_load.c: an NMAR file's bank is an NANR file's, under the same block kind

static BOOL GetUnpackedAnimBank(void *file, NNSG2dAnimBankData **anims, const char *function) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_ANIMBANK);

    if (blk != NULL) {
        *anims = UnpackOrStop(PORT_RELOC_G2D_ANIM_BANK, file, FileSize(file), blk + 1, function);
        return TRUE;
    }
    *anims = NULL;
    return FALSE;
}

BOOL NNS_G2dGetUnpackedAnimBank(void *file, NNSG2dAnimBankData **anims) {
    return GetUnpackedAnimBank(file, anims, __func__);
}

BOOL NNS_G2dGetUnpackedMCAnimBank(void *file, NNSG2dAnimBankData **anims) {
    return GetUnpackedAnimBank(file, anims, __func__);
}

void NNS_G2dUnpackNAN(NNSG2dAnimBankData *bank) {
    UnpackOrStop(PORT_RELOC_G2D_ANIM_BANK, NULL, 0, bank, __func__);
}

// g2d_NFT_load.c. The header checks are the file's inlines: the signature, and a version at least the one asked for

static BOOL IsBinFileValid(const NNSG2dBinaryFileHeader *header, u32 signature, u16 version) {
    return header != NULL && header->signature == signature && header->version >= version;
}

BOOL NNSi_G2dGetUnpackedFont(void *pNftrFile, NNSG2dFontInformation **ppFont) {
    NNSG2dBinaryBlockHeader *pBlk;
    BOOL bOldVer = FALSE;
    PortG2dFont *font;

    if (!IsBinFileValid(pNftrFile, NNS_G2D_BINFILE_SIG_FONTDATA, 0x102) &&
        !IsBinFileValid(pNftrFile, NNS_G2D_BINFILE_SIG_FONTDATA, 0x101)) {
        if (!IsBinFileValid(pNftrFile, NNS_G2D_BINFILE_SIG_FONTDATA, 0x100)) {
            OS_Terminate();
        }
        bOldVer = TRUE;
    }

    font = UnpackOrStop(PORT_RELOC_G2D_FONT, pNftrFile, FileSize(pNftrFile), pNftrFile, __func__);
    pBlk = NNS_G2dFindBinaryBlock(pNftrFile, NNS_G2D_BINBLK_SIG_FINFDATA);
    if (pBlk == NULL) {
        *ppFont = NULL;
        return FALSE;
    }
    *ppFont = port_g2d_font_node(font, pBlk + 1, NNS_G2D_BINBLK_SIG_FINFDATA);
    // A version 1.0 file's glyph block has no flags: the DS clears the byte, in the file, and so does the host
    if (bOldVer) {
        (*ppFont)->pGlyph->flags = 0;
    }
    return TRUE;
}

void NNSi_G2dUnpackNFT(NNSG2dBinaryFileHeader *pHeader) {
    UnpackOrStop(PORT_RELOC_G2D_FONT, pHeader, FileSize(pHeader), pHeader, __func__);
}

// g2d_Load.c

void NNSi_G2dUnpackUserExCellAttrBank(NNSG2dUserExCellAttrBank *attrBank) {
    UnpackOrStop(PORT_RELOC_G2D_CELL_EXATTR, NULL, 0, attrBank, __func__);
}
