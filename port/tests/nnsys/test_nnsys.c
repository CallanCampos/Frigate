// Tests of the host's NitroSystem relocation (port/hal/nnsys): the G2D loaders on small files made here, one of each
// format, through the decomp's own functions that use what they return and through the Unpack functions that take a
// struct; the shapes no file of the ROM has (animation sequences outside the bank's frames, font structs inside another
// block); the registry's lifetime through NNS FND's heaps; rebuilding a tree when a buffer is reused, with another file
// or the same one; a read the DS makes past a heap block (port_heap_ds_read_begin, which the copyright screen's load in
// src/gfl/arc_util.c calls), let through in the game's heaps and the main arena and stopped outside them; and, with the
// player's ROM, every G2D file of it against a simulation of the DS's own relocation (test_g2d_rom.c).
//
//   port_nnsys_tests <scenario>        one of the scenarios below, or all of them with "all"
//   port_nnsys_tests ds_read_outside   a read past a block that leaves the game's memory: the program stops (abort)
//   port_nnsys_tests rom <rom.nds>     the ROM-wide equivalence test; exits 77 (skipped) without the ROM
//
// Each scenario ends with every tree released, so that a sanitizer build's leak checker sees what a tree leaves.
#include "hal/nnsys/g2d_file.h"
#include "hal/nnsys/g2d_load.h"
#include "hal/nnsys/reloc.h"
#include "nitro/os.h"
#include "nnsys/fnd.h"
#include "nnsys/g2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define TEST_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(TEST_ASAN)
#define TEST_ASAN 1
#endif
#ifdef TEST_ASAN
#include <sanitizer/asan_interface.h>
#endif

int port_nnsys_rom_test(const char *romPath);

static int sFailed;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            printf("  failed: %s (line %d)\n", #condition, __LINE__);                                                  \
            sFailed = 1;                                                                                               \
        }                                                                                                              \
    } while (0)

// Little-endian stores into a file being made
static void W16(u8 *file, size_t at, u16 value) {
    file[at] = value & 0xff;
    file[at + 1] = value >> 8;
}

static void W32(u8 *file, size_t at, u32 value) {
    W16(file, at, value & 0xffff);
    W16(file, at + 2, value >> 16);
}

// A G2D file's header and its blocks: the header at 0, then each block where BeginBlock is called
static void FileHeader(u8 *file, u32 signature, u16 version, u16 numBlocks) {
    W32(file, 0, signature);
    W16(file, 4, 0xfeff);
    W16(file, 6, version);
    W16(file, 12, 0x10);
    W16(file, 14, numBlocks);
}

static void FileEnd(u8 *file, size_t size) {
    W32(file, 8, (u32)size);
}

static void Block(u8 *file, size_t at, u32 kind, u32 size) {
    W32(file, at, kind);
    W32(file, at + 4, size);
}

static u8 *Copy(const u8 *file, size_t size) {
    u8 *copy = malloc(size);

    memcpy(copy, file, size);
    return copy;
}

// An NCGR file: a CHAR block whose characters follow the struct
#define NCGR_SIZE (0x10 + 0x08 + 0x18 + 0x40)
static void MakeNCGR(u8 *file, u16 height) {
    memset(file, 0, NCGR_SIZE);
    FileHeader(file, 0x4e434752, 0x0101, 1);
    Block(file, 0x10, NNS_G2D_BINBLK_SIG_CHARDATA, 0x08 + 0x18 + 0x40);
    W16(file, 0x18, height);
    W16(file, 0x1a, 4);
    W32(file, 0x1c, GX_TEXFMT_PLTT16);
    W32(file, 0x20, 0x10);
    W32(file, 0x24, 0);
    W32(file, 0x28, 0x40);
    W32(file, 0x2c, 0x18);
    for (int i = 0; i < 0x40; i++) {
        file[0x30 + i] = (u8)(i * 7 + height);
    }
    FileEnd(file, NCGR_SIZE);
}

static void ScenarioCharacter(void) {
    u8 original[NCGR_SIZE];
    u8 *file;
    NNSG2dCharacterData *character;
    NNSG2dCharacterData *again;
    NNSG2dCharacterData *bg;

    MakeNCGR(original, 2);
    file = Copy(original, NCGR_SIZE);
    CHECK(NNS_G2dGetUnpackedCharacterData(file, &character));
    CHECK(character->height == 2 && character->width == 4);
    CHECK(character->pixelFormat == GX_TEXFMT_PLTT16 && character->mappingType == 0x10);
    CHECK(character->characterFormat == 0 && character->size == 0x40);
    CHECK(character->rawData == file + 0x30);
    // The file stays as it was, and unpacking it again, for OBJ or BG, gives the same tree
    CHECK(memcmp(file, original, NCGR_SIZE) == 0);
    CHECK(NNS_G2dGetUnpackedCharacterData(file, &again) && again == character);
    CHECK(NNS_G2dGetUnpackedBGCharacterData(file, &bg) && bg == character);
    CHECK(port_reloc_count_kind(PORT_RELOC_G2D_CHARACTER) == 1);
    // A file with no CHAR block
    W32(file, 0x10, NNS_G2D_BINBLK_SIG_PALETTEDATA);
    CHECK(!NNS_G2dGetUnpackedCharacterData(file + 0, &again) && again == NULL);
    port_reloc_release_range(file, NCGR_SIZE);
    CHECK(port_reloc_count() == 0);
    free(file);

    // The struct on its own, as NNS_G2dUnpackNCG and NNS_G2dUnpackBGNCG take it: no file extent, the same tree
    file = Copy(original, NCGR_SIZE);
    NNS_G2dUnpackNCG((NNSG2dCharacterData *)(file + 0x18));
    character = port_reloc_find(file + 0x18, PORT_RELOC_G2D_CHARACTER);
    CHECK(character != NULL && character->height == 2 && character->mappingType == 0x10);
    CHECK(character != NULL && character->size == 0x40 && character->rawData == file + 0x30);
    port_reloc_release_range(file, NCGR_SIZE);
    NNS_G2dUnpackBGNCG((NNSG2dCharacterData *)(file + 0x18));
    character = port_reloc_find(file + 0x18, PORT_RELOC_G2D_CHARACTER);
    CHECK(character != NULL && character->width == 4 && character->rawData == file + 0x30);
    CHECK(memcmp(file, original, NCGR_SIZE) == 0);
    port_reloc_release_range(file, NCGR_SIZE);
    CHECK(port_reloc_count() == 0);
    free(file);
}

// An NCLR file: a PLTT block and a PCMP block
#define NCLR_SIZE (0x10 + 0x08 + 0x10 + 0x20 + 0x08 + 0x08 + 0x04)
#define NCLR_PCMP (0x10 + 0x08 + 0x10 + 0x20)
static void MakeNCLR(u8 *file) {
    memset(file, 0, NCLR_SIZE);
    FileHeader(file, 0x4e434c52, 0x0100, 2);
    Block(file, 0x10, NNS_G2D_BINBLK_SIG_PALETTEDATA, 0x08 + 0x10 + 0x20);
    W32(file, 0x18, GX_TEXFMT_PLTT16);
    W32(file, 0x1c, 1);
    W32(file, 0x20, 0x20);
    W32(file, 0x24, 0x10);
    for (int i = 0; i < 0x20; i++) {
        file[0x28 + i] = (u8)i;
    }
    Block(file, NCLR_PCMP, NNS_G2D_BINBLK_SIG_PALETTECOMPRESSINFO, 0x08 + 0x08 + 0x04);
    W16(file, NCLR_PCMP + 8, 2);
    W16(file, NCLR_PCMP + 10, 0xbeef);
    W32(file, NCLR_PCMP + 12, 0x08);
    W16(file, NCLR_PCMP + 16, 3);
    W16(file, NCLR_PCMP + 18, 9);
    FileEnd(file, NCLR_SIZE);
}

static void ScenarioPalette(void) {
    u8 original[NCLR_SIZE];
    u8 *file;
    NNSG2dPaletteData *palette;
    NNSG2dPaletteCompressInfo *info;

    MakeNCLR(original);
    file = Copy(original, NCLR_SIZE);
    CHECK(NNS_G2dGetUnpackedPaletteCompressInfo(file, &info));
    CHECK(NNS_G2dGetUnpackedPaletteData(file, &palette));
    CHECK(palette->format == GX_TEXFMT_PLTT16 && palette->extendedPalette == 1 && palette->size == 0x20);
    CHECK(palette->rawData == file + 0x28);
    CHECK(info->numPalette == 2 && info->pad == 0xbeef);
    CHECK(info->paletteIndexTable == file + NCLR_PCMP + 16);
    CHECK(((u16 *)info->paletteIndexTable)[1] == 9);
    CHECK(memcmp(file, original, NCLR_SIZE) == 0);
    CHECK(port_reloc_count() == 2);
    port_reloc_release_range(file + 4, 1);
    CHECK(port_reloc_count() == 0);
    free(file);

    // The structs on their own, as NNS_G2dUnpackNCL and NNSi_G2dUnpackNCLCmpInfo take them
    file = Copy(original, NCLR_SIZE);
    NNS_G2dUnpackNCL((NNSG2dPaletteData *)(file + 0x18));
    NNSi_G2dUnpackNCLCmpInfo((NNSG2dPaletteCompressInfo *)(file + NCLR_PCMP + 8));
    palette = port_reloc_find(file + 0x18, PORT_RELOC_G2D_PALETTE);
    info = port_reloc_find(file + NCLR_PCMP + 8, PORT_RELOC_G2D_PALETTE_CMP);
    CHECK(palette != NULL && palette->size == 0x20 && palette->rawData == file + 0x28);
    CHECK(info != NULL && info->numPalette == 2 && info->paletteIndexTable == file + NCLR_PCMP + 16);
    CHECK(memcmp(file, original, NCLR_SIZE) == 0);
    port_reloc_release_range(file, NCLR_SIZE);
    CHECK(port_reloc_count() == 0);
    free(file);
}

// An NCER file's CEBK block, with or without bounding rects: two cells, their three OAMs, VRAM transfer data and user
// extended cell attributes. The offsets of the block's data, from the struct (the block's start + 8)
enum {
    CEBK_AT = 0x18,
    CEBK_CELLS = 0x18,
};

static size_t MakeNCER(u8 *file, BOOL withBR) {
    size_t cellSize = withBR ? 0x10 : 0x08;
    size_t oam = CEBK_CELLS + 2 * cellSize;
    size_t transfer = oam + 0x14;
    size_t ext = transfer + 0x08 + 0x10;
    size_t attrBank = ext + 0x08;
    size_t end = attrBank + 0x08 + 0x08 + 0x08;
    size_t size = CEBK_AT + end;

    memset(file, 0, size);
    FileHeader(file, 0x4e434552, 0x0101, 1);
    Block(file, 0x10, NNS_G2D_BINBLK_SIG_CELLBANK, (u32)(size - 0x10));
    W16(file, CEBK_AT + 0, 2);
    W16(file, CEBK_AT + 2, withBR ? NNS_G2D_CELLBK_ATTR_CELLWITHBR : 0);
    W32(file, CEBK_AT + 4, CEBK_CELLS);
    W32(file, CEBK_AT + 8, 1);
    W32(file, CEBK_AT + 12, (u32)transfer);
    W32(file, CEBK_AT + 16, 0x1234);
    W32(file, CEBK_AT + 20, (u32)ext);
    // Cells: one OAM, then two from the OAMs' start
    W16(file, CEBK_AT + CEBK_CELLS, 1);
    W16(file, CEBK_AT + CEBK_CELLS + 2, 0x0805);
    W32(file, CEBK_AT + CEBK_CELLS + 4, 0);
    W16(file, CEBK_AT + CEBK_CELLS + cellSize, 2);
    W16(file, CEBK_AT + CEBK_CELLS + cellSize + 2, 0x0003);
    W32(file, CEBK_AT + CEBK_CELLS + cellSize + 4, 6);
    if (withBR) {
        W16(file, CEBK_AT + CEBK_CELLS + 8, 7);
        W16(file, CEBK_AT + CEBK_CELLS + 14, (u16)-7);
        W16(file, CEBK_AT + CEBK_CELLS + cellSize + 8, 12);
    }
    for (int i = 0; i < 18; i++) {
        file[CEBK_AT + oam + i] = (u8)(0x80 + i);
    }
    W32(file, CEBK_AT + transfer, 0x200);
    W32(file, CEBK_AT + transfer + 4, 8);
    W32(file, CEBK_AT + transfer + 8, 0x100);
    W32(file, CEBK_AT + transfer + 12, 0x80);
    W32(file, CEBK_AT + transfer + 16, 0x180);
    W32(file, CEBK_AT + transfer + 20, 0x40);
    W32(file, CEBK_AT + ext, NNS_G2D_USEREXDATA_CELLATTR);
    W32(file, CEBK_AT + ext + 4, 0x20);
    W16(file, CEBK_AT + attrBank, 2);
    W16(file, CEBK_AT + attrBank + 2, 1);
    W32(file, CEBK_AT + attrBank + 4, 8);
    W32(file, CEBK_AT + attrBank + 8, 0x10);
    W32(file, CEBK_AT + attrBank + 12, 0x14);
    W32(file, CEBK_AT + attrBank + 16, 0xaaaa0001);
    W32(file, CEBK_AT + attrBank + 20, 0xbbbb0002);
    FileEnd(file, size);
    return size;
}

static void CheckCellBank(BOOL withBR) {
    u8 original[0x100];
    size_t size = MakeNCER(original, withBR);
    u8 *file = Copy(original, size);
    const u8 *bankAt = file + CEBK_AT;
    size_t cellSize = withBR ? 0x10 : 0x08;
    const u8 *oam = bankAt + CEBK_CELLS + 2 * cellSize;
    NNSG2dCellDataBank *bank;
    const NNSG2dCellData *cell;
    const NNSG2dUserExCellAttrBank *attrs;

    CHECK(NNS_G2dGetUnpackedCellBank(file, &bank));
    CHECK(bank->numCells == 2 && bank->mappingMode == 1);
    CHECK(bank->pStringBank == (void *)(uintptr_t)0x1234);
    // The decomp's own accessor, at the host's cell size
    cell = NNS_G2dGetCellDataByIdx(bank, 0);
    CHECK(cell->numOAMAttrs == 1 && cell->cellAttr == 0x0805 && cell->pOamAttrArray == oam);
    CHECK(NNSi_G2dGetCellBoundingSphereR(cell) == 5 << 2);
    cell = NNS_G2dGetCellDataByIdx(bank, 1);
    CHECK(cell->numOAMAttrs == 2 && cell->pOamAttrArray == oam + 6);
    CHECK(NNS_G2dGetCellDataByIdx(bank, 2) == NULL);
    if (withBR) {
        CHECK(((const NNSG2dCellDataWithBR *)NNS_G2dGetCellDataByIdx(bank, 0))->boundingRect.maxBounding.x == 7);
        CHECK(((const NNSG2dCellDataWithBR *)NNS_G2dGetCellDataByIdx(bank, 0))->boundingRect.minBounding.y == -7);
        CHECK(((const NNSG2dCellDataWithBR *)NNS_G2dGetCellDataByIdx(bank, 1))->boundingRect.maxBounding.x == 12);
    }
    CHECK(NNS_G2dCellDataBankHasVramTransferData(bank));
    CHECK(bank->pVramTransferData->szByteMax == 0x200);
    CHECK(bank->pVramTransferData->pCellTransferDataArray[1].srcDataOffset == 0x180);
    CHECK((const u8 *)bank->pVramTransferData->pCellTransferDataArray == oam + 0x14 + 8);
    attrs = NNS_G2dGetUserExCellAttrBankFromCellBank(bank);
    CHECK(attrs != NULL);
    if (attrs != NULL) {
        CHECK(attrs->numCells == 2 && attrs->numAttribute == 1);
        CHECK(attrs->pCellAttrArray[0].pAttr[0] == 0xaaaa0001 && attrs->pCellAttrArray[1].pAttr[0] == 0xbbbb0002);
    }
    CHECK(memcmp(file, original, size) == 0);
    port_reloc_release_range(file, size);
    CHECK(port_reloc_count() == 0);
    free(file);
}

static void ScenarioCellBank(void) {
    CheckCellBank(FALSE);
    CheckCellBank(TRUE);
}

// An NMCR file's MCBK block: two multi-cells over three hierarchy nodes, and user extended cell attributes
static void ScenarioMultiCellBank(void) {
    enum { AT = 0x18, CELLS = 0x14, HIER = CELLS + 0x10, EXT = HIER + 0x18, ATTR = EXT + 0x08, END = ATTR + 0x0c };
    u8 original[AT + END] = { 0 };
    u8 *file;
    NNSG2dMultiCellDataBank *bank;
    const NNSG2dMultiCellData *cell;
    const NNSG2dUserExCellAttrBank *attrs;

    FileHeader(original, 0x4e4d4352, 0x0100, 1);
    Block(original, 0x10, NNS_G2D_BINBLK_SIG_MULTICELLBANK, 0x08 + END);
    W16(original, AT, 2);
    W32(original, AT + 4, CELLS);
    W32(original, AT + 8, HIER);
    W32(original, AT + 12, 0x777);
    W32(original, AT + 16, EXT);
    W16(original, AT + CELLS, 1);
    W16(original, AT + CELLS + 2, 1);
    W32(original, AT + CELLS + 4, 0);
    W16(original, AT + CELLS + 8, 2);
    W16(original, AT + CELLS + 10, 2);
    W32(original, AT + CELLS + 12, 8);
    for (int i = 0; i < 3; i++) {
        W16(original, AT + HIER + i * 8, (u16)(10 + i));
        W16(original, AT + HIER + i * 8 + 2, (u16)(-i));
        W16(original, AT + HIER + i * 8 + 6, (u16)(i << 8));
    }
    W32(original, AT + EXT, NNS_G2D_USEREXDATA_CELLATTR);
    W32(original, AT + EXT + 4, 0x14);
    W16(original, AT + ATTR, 1);
    W16(original, AT + ATTR + 2, 1);
    W32(original, AT + ATTR + 4, 8);
    W32(original, AT + ATTR + 8, 0x0c);
    FileEnd(original, AT + END);

    file = Copy(original, AT + END);
    CHECK(NNS_G2dGetUnpackedMultiCellBank(file, &bank));
    CHECK(bank->numMultiCellData == 2 && bank->pStringBank == (void *)(uintptr_t)0x777);
    CHECK(bank->pHierarchyDataArray == file + AT + HIER);
    cell = NNS_G2dGetMultiCellDataByIdx(bank, 1);
    CHECK(cell != NULL && cell->numNodes == 2 && cell->numCellAnim == 2);
    CHECK(cell != NULL && cell->pHierDataArray[0].animSequenceIdx == 11 && cell->pHierDataArray[1].nodeAttr == 2 << 8);
    CHECK(NNS_G2dGetMultiCellDataByIdx(bank, 0)->pHierDataArray == (void *)(file + AT + HIER));
    attrs = NNS_G2dGetUserExCellAttrBankFromMCBank(bank);
    CHECK(attrs != NULL && attrs->numCells == 1 && attrs->pCellAttrArray[0].pAttr == (u32 *)(file + AT + ATTR + 0x0c));
    CHECK(memcmp(file, original, AT + END) == 0);
    port_reloc_release_range(file, AT + END);
    CHECK(port_reloc_count() == 0);
    free(file);

    // The struct on its own, as NNS_G2dUnpackNMC takes it
    file = Copy(original, AT + END);
    NNS_G2dUnpackNMC((NNSG2dMultiCellDataBank *)(file + AT));
    bank = port_reloc_find(file + AT, PORT_RELOC_G2D_MULTICELL);
    CHECK(bank != NULL && bank->numMultiCellData == 2 && bank->pHierarchyDataArray == file + AT + HIER);
    CHECK(bank != NULL && bank->pMultiCellDataArray[1].pHierDataArray == (void *)(file + AT + HIER + 8));
    attrs = bank != NULL ? NNS_G2dGetUserExCellAttrBankFromMCBank(bank) : NULL;
    CHECK(attrs != NULL && attrs->pCellAttrArray[0].pAttr == (u32 *)(file + AT + ATTR + 0x0c));
    CHECK(memcmp(file, original, AT + END) == 0);
    port_reloc_release_range(file, AT + END);
    CHECK(port_reloc_count() == 0);
    free(file);
}

// An NANR file's ABNK block: two sequences over the bank's four frames, one frame no sequence reaches, T contents, and
// user extended attributes of the sequences and their frames
static void ScenarioAnimBank(void) {
    enum {
        AT = 0x18,
        SEQS = 0x18,
        FRAMES = SEQS + 0x20,
        DATA = FRAMES + 0x20,
        EXT = DATA + 0x20,
        ATTR = EXT + 0x08,
        END = ATTR + 0x08 + 0x18 + 0x0c + 0x10,
    };
    u8 original[AT + END] = { 0 };
    u8 *file;
    NNSG2dAnimBankData *bank;
    const NNSG2dAnimSequence *seq;
    const NNSG2dUserExAnimAttrBank *attrs;
    const NNSG2dUserExAnimSequenceAttr *seqAttr;
    size_t attrSeqs = 0x08;
    size_t attrFrames = attrSeqs + 0x18;
    size_t attrWords = attrFrames + 0x0c;

    FileHeader(original, 0x4e414e52, 0x0100, 1);
    Block(original, 0x10, NNS_G2D_BINBLK_SIG_ANIMBANK, 0x08 + END);
    W16(original, AT, 2);
    W16(original, AT + 2, 4);
    W32(original, AT + 4, SEQS);
    W32(original, AT + 8, FRAMES);
    W32(original, AT + 12, DATA);
    W32(original, AT + 16, 0);
    W32(original, AT + 20, EXT);
    // Sequence 0: frames 0 and 1; sequence 1: frame 3. Frame 2 is no sequence's
    W16(original, AT + SEQS, 2);
    W32(original, AT + SEQS + 4, NNS_G2D_ANIMELEM_INDEX_T);
    W32(original, AT + SEQS + 8, NNS_G2D_ANIMATIONPLAYMODE_FORWARD_LOOP);
    W32(original, AT + SEQS + 12, 0);
    W16(original, AT + SEQS + 16, 1);
    W16(original, AT + SEQS + 18, 0);
    W32(original, AT + SEQS + 20, NNS_G2D_ANIMELEM_INDEX);
    W32(original, AT + SEQS + 24, NNS_G2D_ANIMATIONPLAYMODE_FORWARD);
    W32(original, AT + SEQS + 28, 3 * 8);
    for (int k = 0; k < 4; k++) {
        W32(original, AT + FRAMES + k * 8, (u32)(k * 8));
        W16(original, AT + FRAMES + k * 8 + 4, (u16)(4 + k));
        W16(original, AT + DATA + k * 8, (u16)(100 + k));
    }
    W32(original, AT + EXT, NNS_G2D_USEREXDATA_ANMATTR);
    W32(original, AT + EXT + 4, END - EXT);
    W16(original, AT + ATTR, 2);
    W16(original, AT + ATTR + 2, 1);
    W32(original, AT + ATTR + 4, (u32)attrSeqs);
    W16(original, AT + ATTR + attrSeqs, 2);
    W32(original, AT + ATTR + attrSeqs + 4, (u32)attrWords);
    W32(original, AT + ATTR + attrSeqs + 8, (u32)attrFrames);
    W16(original, AT + ATTR + attrSeqs + 12, 1);
    W32(original, AT + ATTR + attrSeqs + 16, (u32)attrWords + 4);
    W32(original, AT + ATTR + attrSeqs + 20, (u32)attrFrames + 8);
    W32(original, AT + ATTR + attrFrames, (u32)attrWords + 8);
    W32(original, AT + ATTR + attrFrames + 4, (u32)attrWords + 12);
    W32(original, AT + ATTR + attrFrames + 8, (u32)attrWords);
    for (int i = 0; i < 4; i++) {
        W32(original, AT + ATTR + attrWords + i * 4, 0x5000u + i);
    }
    FileEnd(original, AT + END);

    file = Copy(original, AT + END);
    CHECK(NNS_G2dGetUnpackedAnimBank(file, &bank));
    CHECK(bank->numSequences == 2 && bank->numTotalFrames == 4 && bank->pAnimationData == file + AT + DATA);
    seq = NNS_G2dGetAnimSequenceByIdx(bank, 0);
    CHECK(seq != NULL && seq->numFrames == 2 && seq->animType == NNS_G2D_ANIMELEM_INDEX_T);
    CHECK(seq != NULL && seq->playMode == NNS_G2D_ANIMATIONPLAYMODE_FORWARD_LOOP);
    CHECK(seq != NULL && seq->pAnmFrameArray == &bank->pFrameArray[0]);
    CHECK(seq != NULL && ((const NNSG2dAnimDataT *)seq->pAnmFrameArray[1].pContent)->index == 101);
    CHECK(seq != NULL && seq->pAnmFrameArray[1].frames == 5);
    seq = NNS_G2dGetAnimSequenceByIdx(bank, 1);
    CHECK(seq != NULL && seq->pAnmFrameArray == &bank->pFrameArray[3]);
    CHECK(seq != NULL && *(const u16 *)seq->pAnmFrameArray[0].pContent == 103);
    // The frame no sequence reaches keeps its offset, as on the DS
    CHECK(bank->pFrameArray[2].pContent == (void *)(uintptr_t)16);
    CHECK(NNS_G2dGetAnimSequenceByIdx(bank, 2) == NULL);
    attrs = NNS_G2dGetUserExAnimAttrBank(bank);
    CHECK(attrs != NULL && attrs->numSequences == 2 && attrs->numAttribute == 1);
    seqAttr = attrs != NULL ? NNS_G2dGetUserExAnimSequenceAttr(attrs, 0) : NULL;
    CHECK(seqAttr != NULL && seqAttr->numFrames == 2 && seqAttr->pAttr[0] == 0x5000);
    CHECK(seqAttr != NULL && seqAttr->pAnmFrmAttrArray[0].pAttr[0] == 0x5002);
    CHECK(seqAttr != NULL && seqAttr->pAnmFrmAttrArray[1].pAttr[0] == 0x5003);
    seqAttr = attrs != NULL ? NNS_G2dGetUserExAnimSequenceAttr(attrs, 1) : NULL;
    CHECK(seqAttr != NULL && seqAttr->pAttr[0] == 0x5001 && seqAttr->pAnmFrmAttrArray[0].pAttr[0] == 0x5000);
    // NMAR files go through the same bank
    CHECK(NNS_G2dGetUnpackedMCAnimBank(file, &(NNSG2dAnimBankData *){ NULL }));
    CHECK(port_reloc_count() == 1);
    CHECK(memcmp(file, original, AT + END) == 0);
    port_reloc_release_range(file, AT + END);
    CHECK(port_reloc_count() == 0);
    free(file);

    // The struct on its own, as NNS_G2dUnpackNAN takes it
    file = Copy(original, AT + END);
    NNS_G2dUnpackNAN((NNSG2dAnimBankData *)(file + AT));
    bank = port_reloc_find(file + AT, PORT_RELOC_G2D_ANIM_BANK);
    seq = bank != NULL ? NNS_G2dGetAnimSequenceByIdx(bank, 1) : NULL;
    CHECK(seq != NULL && seq->pAnmFrameArray == &bank->pFrameArray[3]);
    CHECK(seq != NULL && *(const u16 *)seq->pAnmFrameArray[0].pContent == 103);
    CHECK(bank != NULL && bank->pFrameArray[2].pContent == (void *)(uintptr_t)16);
    attrs = bank != NULL ? NNS_G2dGetUserExAnimAttrBank(bank) : NULL;
    seqAttr = attrs != NULL ? NNS_G2dGetUserExAnimSequenceAttr(attrs, 0) : NULL;
    CHECK(seqAttr != NULL && seqAttr->pAnmFrmAttrArray[1].pAttr[0] == 0x5003);
    CHECK(memcmp(file, original, AT + END) == 0);
    port_reloc_release_range(file, AT + END);
    CHECK(port_reloc_count() == 0);
    free(file);
}

// An ABNK block whose sequences don't all lie whole in the bank's frames, which no file of the ROM has: sequences 0 and
// 1 share frame 1, which the host relocates once where the DS adds the base twice; sequence 2 runs from frame 3 past
// the bank's last frame; sequence 3 doesn't start on a frame. The last two get frames of their own, read where the DS
// reads them, and the bank's frames that a sequence starting on one of them reaches are relocated, as on the DS
static void ScenarioAnimFrames(void) {
    enum { AT = 0x18, SEQS = 0x18, FRAMES = SEQS + 4 * 0x10, DATA = FRAMES + 4 * 8, END = DATA + 4 * 8 };
    static const struct {
        u16 numFrames;
        u32 offset;
    } sequences[4] = { { 2, 0 }, { 2, 8 }, { 2, 3 * 8 }, { 1, 4 } };
    u8 original[AT + END] = { 0 };
    u8 *file;
    const u8 *animData;
    NNSG2dAnimBankData *bank;
    const NNSG2dAnimSequence *seq;
    const NNSG2dAnimSequence *other;
    PortG2dAnimStats before = *port_g2d_anim_stats();

    FileHeader(original, 0x4e414e52, 0x0100, 1);
    Block(original, 0x10, NNS_G2D_BINBLK_SIG_ANIMBANK, 0x08 + END);
    W16(original, AT, 4);
    W16(original, AT + 2, 4);
    W32(original, AT + 4, SEQS);
    W32(original, AT + 8, FRAMES);
    W32(original, AT + 12, DATA);
    for (int i = 0; i < 4; i++) {
        W16(original, AT + SEQS + i * 0x10, sequences[i].numFrames);
        W32(original, AT + SEQS + i * 0x10 + 4, NNS_G2D_ANIMELEM_INDEX);
        W32(original, AT + SEQS + i * 0x10 + 8, NNS_G2D_ANIMATIONPLAYMODE_FORWARD);
        W32(original, AT + SEQS + i * 0x10 + 12, sequences[i].offset);
    }
    // Frame k's content is at k * 8 in the animation data, which holds 100 + k there and 0x40 + k after it
    for (int k = 0; k < 4; k++) {
        W32(original, AT + FRAMES + k * 8, (u32)(k * 8));
        W16(original, AT + FRAMES + k * 8 + 4, (u16)(4 + k));
        W16(original, AT + DATA + k * 8, (u16)(100 + k));
        W16(original, AT + DATA + k * 8 + 4, (u16)(0x40 + k));
    }
    FileEnd(original, AT + END);

    file = Copy(original, AT + END);
    animData = file + AT + DATA;
    CHECK(NNS_G2dGetUnpackedAnimBank(file, &bank));
    // Every frame of the bank's is reached by a sequence that starts on a frame, once or more: each is relocated once
    for (int k = 0; k < 4; k++) {
        CHECK(bank->pFrameArray[k].pContent == animData + k * 8 && bank->pFrameArray[k].frames == 4 + k);
    }
    seq = NNS_G2dGetAnimSequenceByIdx(bank, 0);
    other = NNS_G2dGetAnimSequenceByIdx(bank, 1);
    CHECK(seq->pAnmFrameArray == &bank->pFrameArray[0] && other->pAnmFrameArray == &bank->pFrameArray[1]);
    CHECK(seq->pAnmFrameArray[1].pContent == animData + 8 && other->pAnmFrameArray[0].pContent == animData + 8);
    CHECK(port_g2d_anim_stats()->framesSharedBySequences == before.framesSharedBySequences + 1);
    CHECK(port_g2d_anim_stats()->framesNeverRelocated == before.framesNeverRelocated);
    // Past the last frame: frame 3, then the animation data's first 8 bytes read as a frame
    seq = NNS_G2dGetAnimSequenceByIdx(bank, 2);
    CHECK(seq->numFrames == 2 && seq->pAnmFrameArray != &bank->pFrameArray[3]);
    CHECK(seq->pAnmFrameArray[0].pContent == animData + 3 * 8 && seq->pAnmFrameArray[0].frames == 7);
    CHECK(seq->pAnmFrameArray[1].pContent == animData + 100 && seq->pAnmFrameArray[1].frames == 0x40);
    CHECK(seq->pAnmFrameArray[1].pad == 0);
    CHECK(*(const u16 *)seq->pAnmFrameArray[0].pContent == 103);
    // From the middle of frame 0: its frames and pad as the content's offset, then frame 1's offset as frames and pad
    seq = NNS_G2dGetAnimSequenceByIdx(bank, 3);
    CHECK(seq->numFrames == 1 && seq->pAnmFrameArray != &bank->pFrameArray[0]);
    CHECK(seq->pAnmFrameArray[0].pContent == animData + 4);
    CHECK(seq->pAnmFrameArray[0].frames == 8 && seq->pAnmFrameArray[0].pad == 0);
    CHECK(memcmp(file, original, AT + END) == 0);
    port_reloc_release_range(file, AT + END);
    CHECK(port_reloc_count() == 0);
    free(file);
}

// An NFTR file: FINF, CGLP, two CWDH blocks and three CMAP blocks (direct, table and scan), each linked to the next
static size_t MakeNFTR(u8 *file, u16 version) {
    enum {
        FINF = 0x10,
        CGLP = FINF + 0x08 + 0x14,
        CWDH1 = CGLP + 0x08 + 0x08 + 0x20,
        CWDH2 = CWDH1 + 0x08 + 0x08 + 0x0c,
        CMAP1 = CWDH2 + 0x08 + 0x08 + 0x04,
        CMAP2 = CMAP1 + 0x08 + 0x0c + 0x04,
        CMAP3 = CMAP2 + 0x08 + 0x0c + 0x08,
        END = CMAP3 + 0x08 + 0x0c + 0x0c,
    };

    memset(file, 0, END);
    FileHeader(file, NNS_G2D_BINFILE_SIG_FONTDATA, version, 7);
    Block(file, FINF, NNS_G2D_BINBLK_SIG_FINFDATA, 0x08 + 0x14);
    file[FINF + 8] = 1;
    file[FINF + 9] = 12;
    W16(file, FINF + 10, 2);
    file[FINF + 12] = 0;
    file[FINF + 13] = 6;
    file[FINF + 14] = 7;
    file[FINF + 15] = 1;
    W32(file, FINF + 16, CGLP + 8);
    W32(file, FINF + 20, CWDH1 + 8);
    W32(file, FINF + 24, CMAP1 + 8);
    Block(file, CGLP, NNS_G2D_BINBLK_SIG_CGLPDATA, 0x08 + 0x08 + 0x20);
    file[CGLP + 8] = 8;
    file[CGLP + 9] = 8;
    W16(file, CGLP + 10, 8);
    file[CGLP + 14] = 1;
    file[CGLP + 15] = 2;
    // Widths of glyphs 0 to 2, then of glyph 3
    Block(file, CWDH1, NNS_G2D_BINBLK_SIG_CWDHDATA, 0x08 + 0x08 + 0x0c);
    W16(file, CWDH1 + 8, 0);
    W16(file, CWDH1 + 10, 2);
    W32(file, CWDH1 + 12, CWDH2 + 8);
    for (int i = 0; i < 3; i++) {
        file[CWDH1 + 16 + i * 3] = (u8)i;
        file[CWDH1 + 17 + i * 3] = (u8)(5 + i);
        file[CWDH1 + 18 + i * 3] = (u8)(6 + i);
    }
    Block(file, CWDH2, NNS_G2D_BINBLK_SIG_CWDHDATA, 0x08 + 0x08 + 0x04);
    W16(file, CWDH2 + 8, 3);
    W16(file, CWDH2 + 10, 3);
    file[CWDH2 + 18] = 9;
    // 'A' to 'B' directly from glyph 0; 'a' to 'b' by table; then a scan of two codes
    Block(file, CMAP1, NNS_G2D_BINBLK_SIG_CMAPDATA, 0x08 + 0x0c + 0x04);
    W16(file, CMAP1 + 8, 'A');
    W16(file, CMAP1 + 10, 'B');
    W16(file, CMAP1 + 12, NNS_G2D_MAPMETHOD_DIRECT);
    W32(file, CMAP1 + 16, CMAP2 + 8);
    W16(file, CMAP1 + 20, 0);
    Block(file, CMAP2, NNS_G2D_BINBLK_SIG_CMAPDATA, 0x08 + 0x0c + 0x08);
    W16(file, CMAP2 + 8, 'a');
    W16(file, CMAP2 + 10, 'b');
    W16(file, CMAP2 + 12, NNS_G2D_MAPMETHOD_TABLE);
    W32(file, CMAP2 + 16, CMAP3 + 8);
    W16(file, CMAP2 + 20, 2);
    W16(file, CMAP2 + 22, 1);
    Block(file, CMAP3, NNS_G2D_BINBLK_SIG_CMAPDATA, 0x08 + 0x0c + 0x0c);
    W16(file, CMAP3 + 8, 0x3000);
    W16(file, CMAP3 + 10, 0x30ff);
    W16(file, CMAP3 + 12, NNS_G2D_MAPMETHOD_SCAN);
    W16(file, CMAP3 + 20, 2);
    W16(file, CMAP3 + 22, 0x3001);
    W16(file, CMAP3 + 24, 3);
    W16(file, CMAP3 + 26, 0x3042);
    W16(file, CMAP3 + 28, 1);
    FileEnd(file, END);
    return END;
}

static void ScenarioFont(void) {
    u8 original[0x100];
    size_t size = MakeNFTR(original, 0x0102);
    u8 *file = Copy(original, size);
    NNSG2dFont font;
    const PortG2dFont *tree;

    NNS_G2dFontInitUTF16(&font, file);
    CHECK(font.pRes != NULL);
    if (font.pRes == NULL) {
        free(file);
        return;
    }
    CHECK(font.pRes->fontType == 1 && font.pRes->linefeed == 12 && font.pRes->alterCharIndex == 2);
    CHECK(font.pRes->defaultWidth.glyphWidth == 6 && font.pRes->encoding == 1);
    CHECK(font.pRes->pGlyph->cellWidth == 8 && font.pRes->pGlyph->flags == 2);
    // The decomp's lookups, through the code maps and widths
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 'B') == 1);
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 'a') == 2);
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 0x3042) == 1);
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 0x3001) == 3);
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 0x3002) == NNS_G2D_GLYPH_INDEX_NOT_FOUND);
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 'z') == NNS_G2D_GLYPH_INDEX_NOT_FOUND);
    CHECK(NNS_G2dFontGetCharWidthsFromIndex(&font, 1)->glyphWidth == 6);
    CHECK(NNS_G2dFontGetCharWidthsFromIndex(&font, 3)->charWidth == 9);
    CHECK(NNS_G2dFontGetCharWidthsFromIndex(&font, 4) == &font.pRes->defaultWidth);
    CHECK(NNS_G2dFontGetCharWidths(&font, 'b')->charWidth == 7);
    CHECK(memcmp(file, original, size) == 0);
    port_reloc_release_range(file, size);
    CHECK(port_reloc_count() == 0);
    free(file);

    // The file on its own, as NNSi_G2dUnpackNFT takes it: the same tree, found by the file's header. The glyphs are at
    // 0x34, past CGLP's header, and the first widths at 0x64, past CWDH's
    file = Copy(original, size);
    NNSi_G2dUnpackNFT((NNSG2dBinaryFileHeader *)file);
    tree = port_reloc_find(file, PORT_RELOC_G2D_FONT);
    CHECK(tree != NULL && tree->info.linefeed == 12 && tree->info.pGlyph == (void *)(file + 0x34));
    CHECK(tree != NULL && tree->info.pWidth == port_g2d_font_node(tree, file + 0x64, NNS_G2D_BINBLK_SIG_CWDHDATA));
    CHECK(tree != NULL && tree->info.pWidth != NULL && tree->info.pWidth->pNext != NULL &&
          tree->info.pWidth->pNext->widthTable[0].charWidth == 9);
    CHECK(tree != NULL && tree->info.pMap != NULL && tree->info.pMap->pNext != NULL &&
          tree->info.pMap->pNext->pNext != NULL && tree->info.pMap->pNext->pNext->pNext == NULL);
    CHECK(memcmp(file, original, size) == 0);
    port_reloc_release_range(file, size);
    CHECK(port_reloc_count() == 0);
    free(file);

    // Version 1.0 has no glyph flags: the DS clears the byte in the file
    size = MakeNFTR(original, 0x0100);
    file = Copy(original, size);
    NNS_G2dFontInitUTF16(&font, file);
    CHECK(font.pRes != NULL && font.pRes->pGlyph->flags == 0);
    CHECK(memcmp(file, original, 0x10 + 0x08 + 0x14 + 0x08 + 7) == 0);
    CHECK(file[0x10 + 0x08 + 0x14 + 0x08 + 7] == 0);
    port_reloc_release_all();
    free(file);
}

// An NFTR file whose pointers also lead to width and code map structs inside another block, which no file of the ROM
// has. The DS relocates only the pointers of the structs at blocks' starts, so such a struct's own pNext stays the
// file's offset (and NULL where it is 0). The FINF's widths are a CWDH struct inside the CGLP block, whose pNext is the
// CWDH block's offset; the CWDH block's pNext leads back to it. The CMAP block's pNext leads to a CMAP struct inside
// the CGLP block, whose pNext is the CMAP block's offset
static void ScenarioFontNodes(void) {
    enum {
        FINF = 0x10,
        CGLP = FINF + 0x08 + 0x14,
        WIDTHS = CGLP + 0x08 + 0x08, // Inside the CGLP block
        MAP = WIDTHS + 0x10,         // Inside the CGLP block
        CWDH = MAP + 0x10,
        CMAP = CWDH + 0x08 + 0x08 + 0x04,
        END = CMAP + 0x08 + 0x0c + 0x04,
    };
    u8 original[END] = { 0 };
    u8 *file;
    NNSG2dFont font;
    const PortG2dFont *tree;
    const NNSG2dFontWidth *widths;
    const NNSG2dFontWidth *widthBlock;
    const NNSG2dFontCodeMap *map;
    const NNSG2dFontCodeMap *mapBlock;

    FileHeader(original, NNS_G2D_BINFILE_SIG_FONTDATA, 0x0102, 4);
    Block(original, FINF, NNS_G2D_BINBLK_SIG_FINFDATA, 0x08 + 0x14);
    original[FINF + 8] = 1;
    original[FINF + 9] = 12;
    original[FINF + 13] = 6;
    original[FINF + 14] = 7;
    original[FINF + 15] = 1;
    W32(original, FINF + 16, CGLP + 8);
    W32(original, FINF + 20, WIDTHS);
    W32(original, FINF + 24, CMAP + 8);
    Block(original, CGLP, NNS_G2D_BINBLK_SIG_CGLPDATA, MAP + 0x10 - CGLP);
    original[CGLP + 8] = 8;
    original[CGLP + 9] = 8;
    W16(original, CGLP + 10, 8);
    // Widths of glyphs 0 and 1, then the CWDH block's
    W16(original, WIDTHS, 0);
    W16(original, WIDTHS + 2, 1);
    W32(original, WIDTHS + 4, CWDH + 8);
    original[WIDTHS + 8] = 0;
    original[WIDTHS + 9] = 5;
    original[WIDTHS + 10] = 6;
    original[WIDTHS + 11] = 1;
    original[WIDTHS + 12] = 6;
    original[WIDTHS + 13] = 7;
    // 'x' to 'y' directly from glyph 0, then the CMAP block's
    W16(original, MAP, 'x');
    W16(original, MAP + 2, 'y');
    W16(original, MAP + 4, NNS_G2D_MAPMETHOD_DIRECT);
    W32(original, MAP + 8, CMAP + 8);
    W16(original, MAP + 12, 0);
    // The width of glyph 2, then the widths inside the CGLP block
    Block(original, CWDH, NNS_G2D_BINBLK_SIG_CWDHDATA, 0x08 + 0x08 + 0x04);
    W16(original, CWDH + 8, 2);
    W16(original, CWDH + 10, 2);
    W32(original, CWDH + 12, WIDTHS);
    original[CWDH + 17] = 9;
    original[CWDH + 18] = 10;
    // 'A' to 'B' directly from glyph 0, then the code map inside the CGLP block
    Block(original, CMAP, NNS_G2D_BINBLK_SIG_CMAPDATA, 0x08 + 0x0c + 0x04);
    W16(original, CMAP + 8, 'A');
    W16(original, CMAP + 10, 'B');
    W16(original, CMAP + 12, NNS_G2D_MAPMETHOD_DIRECT);
    W32(original, CMAP + 16, MAP);
    W16(original, CMAP + 20, 0);
    FileEnd(original, END);

    file = Copy(original, END);
    NNS_G2dFontInitUTF16(&font, file);
    tree = port_reloc_find(file, PORT_RELOC_G2D_FONT);
    CHECK(font.pRes != NULL && tree != NULL);
    if (font.pRes == NULL || tree == NULL) {
        free(file);
        return;
    }
    widths = port_g2d_font_node(tree, file + WIDTHS, NNS_G2D_BINBLK_SIG_CWDHDATA);
    widthBlock = port_g2d_font_node(tree, file + CWDH + 8, NNS_G2D_BINBLK_SIG_CWDHDATA);
    map = port_g2d_font_node(tree, file + MAP, NNS_G2D_BINBLK_SIG_CMAPDATA);
    mapBlock = port_g2d_font_node(tree, file + CMAP + 8, NNS_G2D_BINBLK_SIG_CMAPDATA);
    CHECK(widths != NULL && widthBlock != NULL && map != NULL && mapBlock != NULL);
    // Each struct has one host struct, whichever pointer leads to it
    CHECK(font.pRes->pWidth == widths && widthBlock->pNext == widths);
    CHECK(font.pRes->pMap == mapBlock && mapBlock->pNext == map);
    // The pointers of the structs inside a block stay the file's offsets
    CHECK(widths != NULL && widths->indexEnd == 1 && widths->pNext == (void *)(uintptr_t)(CWDH + 8));
    CHECK(map != NULL && map->ccodeBegin == 'x' && map->pNext == (void *)(uintptr_t)(CMAP + 8));
    for (const PortG2dFontNode *node = tree->nodes; node != NULL; node = node->next) {
        CHECK(node->inBlock == (node->at != file + WIDTHS && node->at != file + MAP));
    }
    // The decomp's lookups, which stop at the structs inside the block before their pNext
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 'B') == 1);
    CHECK(NNS_G2dFontFindGlyphIndex(&font, 'y') == 1);
    CHECK(NNS_G2dFontGetCharWidthsFromIndex(&font, 1)->charWidth == 7);
    CHECK(NNS_G2dFontGetCharWidthsFromIndex(&font, 0)->glyphWidth == 5);
    CHECK(widthBlock != NULL && widthBlock->widthTable[0].charWidth == 10);
    CHECK(memcmp(file, original, END) == 0);
    port_reloc_release_range(file, END);
    CHECK(port_reloc_count() == 0);
    free(file);
}

// The registry's lifetimes: a tree goes when the heap memory its file is in goes back, by every NNS FND path
static void ScenarioHeaps(void) {
    size_t heapSize = 0x4000;
    void *memory = malloc(heapSize);
    u8 file[NCGR_SIZE];
    NNSFndHeapHandle heap;
    NNSFndHeapHandle frameHeap;
    u8 *block;
    u8 *other;
    NNSG2dCharacterData *character;

    MakeNCGR(file, 3);

    // An expanded heap: freeing the block, then shrinking one, then destroying the heap
    heap = NNS_FndCreateExpHeapEx(memory, (u32)heapSize, 0);
    block = NNS_FndAllocFromExpHeapEx(heap, NCGR_SIZE, 4);
    other = NNS_FndAllocFromExpHeapEx(heap, NCGR_SIZE, 4);
    memcpy(block, file, NCGR_SIZE);
    memcpy(other, file, NCGR_SIZE);
    CHECK(NNS_G2dGetUnpackedCharacterData(block, &character) && character->rawData == block + 0x30);
    CHECK(NNS_G2dGetUnpackedCharacterData(other, &character));
    CHECK(port_reloc_count() == 2);
    NNS_FndFreeToExpHeap(heap, block);
    CHECK(port_reloc_count() == 1 && port_reloc_find(other + 0x18, PORT_RELOC_G2D_CHARACTER) != NULL);
    // Shrinking the other block to its header keeps nothing of the CHAR block's struct
    CHECK(NNS_FndResizeForMBlockExpHeap(heap, other, 0x10) == 0x10);
    CHECK(port_reloc_count() == 0);
    block = NNS_FndAllocFromExpHeapEx(heap, NCGR_SIZE, -4);
    memcpy(block, file, NCGR_SIZE);
    CHECK(NNS_G2dGetUnpackedCharacterData(block, &character));
    // A block growing frees nothing
    CHECK(NNS_FndResizeForMBlockExpHeap(heap, other, 0x40) == 0x40);
    CHECK(port_reloc_count() == 1);
    NNS_FndDestroyExpHeap(heap);
    CHECK(port_reloc_count() == 0);

    // A frame heap: the bottom, the top, and back to a recorded state
    frameHeap = NNS_FndCreateFrmHeapEx(memory, (u32)heapSize, 0);
    block = NNS_FndAllocFromFrmHeapEx(frameHeap, NCGR_SIZE, 4);
    other = NNS_FndAllocFromFrmHeapEx(frameHeap, NCGR_SIZE, -4);
    memcpy(block, file, NCGR_SIZE);
    memcpy(other, file, NCGR_SIZE);
    CHECK(NNS_G2dGetUnpackedCharacterData(block, &character));
    CHECK(NNS_G2dGetUnpackedCharacterData(other, &character));
    NNS_FndFreeToFrmHeap(frameHeap, NNS_FND_FRMHEAP_RELEASE_HIGH);
    CHECK(port_reloc_count() == 1 && port_reloc_find(block + 0x18, PORT_RELOC_G2D_CHARACTER) != NULL);
    CHECK(NNS_FndRecordStateForFrmHeap(frameHeap, 7));
    other = NNS_FndAllocFromFrmHeapEx(frameHeap, NCGR_SIZE, 4);
    memcpy(other, file, NCGR_SIZE);
    CHECK(NNS_G2dGetUnpackedCharacterData(other, &character));
    u8 *top = NNS_FndAllocFromFrmHeapEx(frameHeap, NCGR_SIZE, -4);
    memcpy(top, file, NCGR_SIZE);
    CHECK(NNS_G2dGetUnpackedCharacterData(top, &character));
    CHECK(port_reloc_count() == 3);
    CHECK(NNS_FndFreeByStateToFrmHeap(frameHeap, 7));
    CHECK(port_reloc_count() == 1 && port_reloc_find(block + 0x18, PORT_RELOC_G2D_CHARACTER) != NULL);
    NNS_FndFreeToFrmHeap(frameHeap, NNS_FND_FRMHEAP_RELEASE_LOW);
    CHECK(port_reloc_count() == 0);
    block = NNS_FndAllocFromFrmHeapEx(frameHeap, NCGR_SIZE, 4);
    memcpy(block, file, NCGR_SIZE);
    CHECK(NNS_G2dGetUnpackedCharacterData(block, &character));
    NNS_FndDestroyFrmHeap(frameHeap);
    CHECK(port_reloc_count() == 0);

    // A unit heap, set up by hand: the game never makes one, so the SDK's creator was dead-stripped
    {
        NNSiFndHeapHead *head = memory;
        NNSiFndUnitHeap *unitHeap = (NNSiFndUnitHeap *)(head + 1);
        u8 *unit = (u8 *)memory + 0x100;

        memset(memory, 0, 0x100);
        head->heapStart = unit;
        head->heapEnd = unit + 0x200;
        unitHeap->unitSize = 0x100;
        unitHeap->freeList.head = NULL;
        memcpy(unit, file, NCGR_SIZE);
        CHECK(NNS_G2dGetUnpackedCharacterData(unit, &character));
        NNS_FndUnitHeapFree(head, unit);
        CHECK(port_reloc_count() == 0 && unitHeap->freeList.head == (NNSiFndUnitBlock *)unit);
    }
    free(memory);
}

// A buffer reused without a free the platform layer sees: the tree is rebuilt at the same root address, in the same
// host structs when the file's bytes are those it was built from and in new ones when they aren't
static void ScenarioReuse(void) {
    static u8 buffer[0x100];
    u8 cells[0x100];
    u8 chars[NCGR_SIZE];
    size_t size;
    NNSG2dCellDataBank *bank;
    NNSG2dCellDataBank *again;
    const NNSG2dCellData *firstArray;

    size = MakeNCER(cells, FALSE);
    memcpy(buffer, cells, size);
    CHECK(NNS_G2dGetUnpackedCellBank(buffer, &bank));
    firstArray = bank->pCellDataArrayHead;
    // The same bytes: the same tree and host structs
    CHECK(NNS_G2dGetUnpackedCellBank(buffer, &again) && again == bank && bank->pCellDataArrayHead == firstArray);
    // Another file with bounding rects at the same address: the same root, rebuilt
    size = MakeNCER(cells, TRUE);
    memcpy(buffer, cells, size);
    CHECK(NNS_G2dGetUnpackedCellBank(buffer, &again) && again == bank);
    CHECK(bank->cellBankAttr == NNS_G2D_CELLBK_ATTR_CELLWITHBR);
    CHECK(((const NNSG2dCellDataWithBR *)NNS_G2dGetCellDataByIdx(bank, 1))->boundingRect.maxBounding.x == 12);
    CHECK(port_reloc_count() == 1);
    // A different OAM attribute keeps the host structs: only the bytes a tree is built from count
    firstArray = bank->pCellDataArrayHead;
    buffer[CEBK_AT + CEBK_CELLS + 0x20] ^= 0xff;
    CHECK(NNS_G2dGetUnpackedCellBank(buffer, &again) && again == bank && bank->pCellDataArrayHead == firstArray);
    // A character file in the buffer leaves the cell tree (another struct address) until the buffer is released
    MakeNCGR(chars, 5);
    memcpy(buffer, chars, NCGR_SIZE);
    {
        NNSG2dCharacterData *character;

        CHECK(NNS_G2dGetUnpackedCharacterData(buffer, &character) && character->height == 5);
    }
    CHECK(port_reloc_count() == 2);
    CHECK(port_reloc_release_range(buffer, sizeof(buffer)) == 2);
    CHECK(port_reloc_count() == 0);
}

// A file loaded again into a buffer the platform layer never sees freed, after the game changed its tree: the unpack
// gives the file's values again, as a new load and unpack does on the DS, in the host structs it gave before
static void ScenarioRefill(void) {
    static u8 buffer[0x100];
    u8 file[0x100];
    size_t size;

    // A palette whose rawData and size the game moves, as clact.c does to load part of it
    {
        NNSG2dPaletteData *palette;
        NNSG2dPaletteData *again;

        MakeNCLR(file);
        memcpy(buffer, file, NCLR_SIZE);
        CHECK(NNS_G2dGetUnpackedPaletteData(buffer, &palette));
        palette->rawData = (u8 *)palette->rawData + 0x10;
        palette->size -= 0x10;
        memcpy(buffer, file, NCLR_SIZE);
        CHECK(NNS_G2dGetUnpackedPaletteData(buffer, &again) && again == palette);
        CHECK(palette->size == 0x20 && palette->rawData == buffer + 0x28);
        CHECK(port_reloc_release_range(buffer, sizeof(buffer)) == 1);
    }
    // Characters whose mapping the game rewrites, as clact.c does for the OBJ VRAM mode
    {
        NNSG2dCharacterData *character;
        NNSG2dCharacterData *again;

        MakeNCGR(file, 6);
        memcpy(buffer, file, NCGR_SIZE);
        CHECK(NNS_G2dGetUnpackedCharacterData(buffer, &character));
        character->mappingType = GX_OBJVRAMMODE_CHAR_1D_128K;
        memcpy(buffer, file, NCGR_SIZE);
        CHECK(NNS_G2dGetUnpackedCharacterData(buffer, &again) && again == character);
        CHECK(character->mappingType == 0x10 && character->height == 6);
        CHECK(port_reloc_release_range(buffer, sizeof(buffer)) == 1);
    }
    // A cell bank whose cells, transfer data and attributes are changed: back in the same host structs
    {
        NNSG2dCellDataBank *bank;
        NNSG2dCellDataBank *again;
        NNSG2dCellDataWithBR *cells;
        NNSG2dVramTransferData *transfer;
        NNSG2dUserExCellAttrBank *attrs;
        NNSG2dUserExCellAttr *attrArray;

        size = MakeNCER(file, TRUE);
        memcpy(buffer, file, size);
        CHECK(NNS_G2dGetUnpackedCellBank(buffer, &bank));
        cells = (NNSG2dCellDataWithBR *)bank->pCellDataArrayHead;
        transfer = bank->pVramTransferData;
        attrs = (NNSG2dUserExCellAttrBank *)(bank->pExtendedData + 1);
        attrArray = attrs->pCellAttrArray;
        cells[1].cellData.numOAMAttrs = 99;
        cells[0].boundingRect.maxBounding.x = 99;
        transfer->szByteMax = 0;
        bank->pVramTransferData = NULL;
        attrArray[0].pAttr = NULL;
        bank->numCells = 1;
        memcpy(buffer, file, size);
        CHECK(NNS_G2dGetUnpackedCellBank(buffer, &again) && again == bank);
        CHECK(bank->numCells == 2 && bank->pCellDataArrayHead == &cells->cellData);
        CHECK(cells[1].cellData.numOAMAttrs == 2 && cells[0].boundingRect.maxBounding.x == 7);
        CHECK(bank->pVramTransferData == transfer && transfer->szByteMax == 0x200);
        CHECK((NNSG2dUserExCellAttrBank *)(bank->pExtendedData + 1) == attrs && attrs->pCellAttrArray == attrArray);
        CHECK(attrArray[0].pAttr != NULL && attrArray[0].pAttr[0] == 0xaaaa0001);
        CHECK(port_reloc_release_range(buffer, sizeof(buffer)) == 1);
    }
    // A font whose information, widths and code maps are changed
    {
        NNSG2dFont font;
        NNSG2dFontInformation *info;
        NNSG2dFontWidth *widths;
        NNSG2dFontCodeMap *map;

        size = MakeNFTR(file, 0x0102);
        memcpy(buffer, file, size);
        NNS_G2dFontInitUTF16(&font, buffer);
        info = font.pRes;
        CHECK(info != NULL);
        if (info != NULL) {
            widths = info->pWidth;
            map = info->pMap;
            info->linefeed = 99;
            info->pWidth = NULL;
            widths->indexEnd = 0;
            widths->widthTable[1].charWidth = 99;
            map->pNext = NULL;
            memcpy(buffer, file, size);
            NNS_G2dFontInitUTF16(&font, buffer);
            CHECK(font.pRes == info && info->linefeed == 12 && info->pWidth == widths && info->pMap == map);
            CHECK(widths->indexEnd == 2 && widths->widthTable[1].charWidth == 7 && map->pNext != NULL);
            CHECK(NNS_G2dFontFindGlyphIndex(&font, 0x3042) == 1);
        }
        CHECK(port_reloc_release_range(buffer, sizeof(buffer)) == 1);
    }
    CHECK(port_reloc_count() == 0);
}

// Files whose offsets lead out of them: no tree, and the reason
static void ScenarioMalformed(void) {
    u8 file[0x100];
    size_t size = MakeNCER(file, FALSE);

    W32(file, CEBK_AT + 4, 0x10000);
    CHECK(port_g2d_unpack(PORT_RELOC_G2D_CELL_BANK, file, size, file + CEBK_AT) == NULL);
    CHECK(strstr(port_g2d_last_error(), "NNSG2dCellData") != NULL);
    CHECK(port_reloc_count() == 0);
    size = MakeNCER(file, FALSE);
    W16(file, CEBK_AT, 0x400);
    CHECK(port_g2d_unpack(PORT_RELOC_G2D_CELL_BANK, file, size, file + CEBK_AT) == NULL);
    // The same struct with no known extent reads on, as the DS would; the counts here stay in the buffer
    size = MakeNCER(file, FALSE);
    CHECK(port_g2d_unpack(PORT_RELOC_G2D_CELL_BANK, NULL, 0, file + CEBK_AT) != NULL);
    // A struct unpacked on its own is the same tree as through its file
    {
        NNSG2dCellDataBank *bank;

        NNS_G2dUnpackNCE((NNSG2dCellDataBank *)(file + CEBK_AT));
        CHECK(NNS_G2dGetUnpackedCellBank(file, &bank) &&
              bank == port_reloc_find(file + CEBK_AT, PORT_RELOC_G2D_CELL_BANK));
        // The attribute bank of a cell bank without bounding rects, after the extended data block's header
        u8 *attrAt = file + CEBK_AT + 0x5c;
        NNSG2dUserExCellAttrBank *attrs;

        NNSi_G2dUnpackUserExCellAttrBank((NNSG2dUserExCellAttrBank *)attrAt);
        attrs = port_reloc_find(attrAt, PORT_RELOC_G2D_CELL_EXATTR);
        CHECK(attrs != NULL && attrs->numCells == 2 && attrs->pCellAttrArray[1].pAttr[0] == 0xbbbb0002);
    }
    port_reloc_release_all();
    CHECK(port_reloc_count() == 0);
}

// The registry on its own: ranges of every width, overlaps at either end, and destructors
static int sDestroyed;

static void CountDestroyed(void *tree) {
    sDestroyed++;
    free(tree);
}

static void ScenarioRegistry(void) {
    static u8 memory[0x10000];

    sDestroyed = 0;
    // A wide range whose key is at its end, so that a release far below the key still finds it
    port_reloc_register(memory + 0x8000, PORT_RELOC_G2D_CELL_BANK, memory, 0x8001, malloc(1), CountDestroyed);
    port_reloc_register(memory + 0x9000, PORT_RELOC_G2D_CHARACTER, memory + 0x9000, 0x10, malloc(1), CountDestroyed);
    port_reloc_register(memory + 0x9000, PORT_RELOC_G2D_PALETTE, memory + 0x9000, 0x10, malloc(1), CountDestroyed);
    port_reloc_register(memory + 0xa000, PORT_RELOC_G2D_CHARACTER, memory + 0xa000, 0x100, malloc(1), CountDestroyed);
    CHECK(port_reloc_count() == 4);
    CHECK(port_reloc_release_range(memory + 0x9010, 0xff0) == 0);
    CHECK(port_reloc_release_range(memory + 0x10, 1) == 1 && sDestroyed == 1);
    CHECK(port_reloc_find(memory + 0x8000, PORT_RELOC_G2D_CELL_BANK) == NULL);
    // Both kinds at one address
    CHECK(port_reloc_release_range(memory + 0x900f, 1) == 2 && sDestroyed == 3);
    // Re-registering replaces the tree, destroying the old one
    port_reloc_register(memory + 0xa000, PORT_RELOC_G2D_CHARACTER, memory + 0xa000, 0x100, malloc(1), CountDestroyed);
    CHECK(sDestroyed == 4 && port_reloc_count() == 1);
    CHECK(port_reloc_release_range(memory + 0xa0ff, 0x1000) == 1 && sDestroyed == 5);
    CHECK(port_reloc_count() == 0);
    for (int i = 0; i < 1000; i++) {
        port_reloc_register(memory + i * 16, PORT_RELOC_G2D_PALETTE, memory + i * 16, 16, malloc(1), CountDestroyed);
    }
    CHECK(port_reloc_count() == 1000);
    CHECK(port_reloc_release_range(memory + 160, 160) == 10);
    port_reloc_release(memory, PORT_RELOC_G2D_PALETTE);
    CHECK(port_reloc_count() == 989);
    port_reloc_release_all();
    CHECK(port_reloc_count() == 0 && sDestroyed == 1005);
}

// Reads every byte of [start, start + size), as the game's load of the copyright screen's characters does
static unsigned ReadSpan(const u8 *start, size_t size) {
    const volatile u8 *p = start;
    unsigned sum = 0;

    for (size_t i = 0; i < size; i++) {
        sum += p[i];
    }
    return sum;
}

// A read the DS makes past a heap block, let through where it stays in the game's memory: past a block into the next
// within its heap; out of a child heap, created in a block of its parent, into the parent; and out of a heap into the
// main arena around it. In the sanitized build the span is readable between the two calls, and what was poisoned is
// poisoned again after them (port/hal/nnsys/fnd_hooks.c)
static void ScenarioDsReads(void) {
    size_t heapSize = 0x4000;
    u8 *memory = malloc(heapSize);
    NNSFndHeapHandle heap = NNS_FndCreateExpHeapEx(memory, (u32)heapSize, 0);
    u8 *block = NNS_FndAllocFromExpHeapEx(heap, 0x40, 8);
    u8 *next = NNS_FndAllocFromExpHeapEx(heap, 0x40, 8);
    u8 *parentBlock;
    NNSFndHeapHandle child;
    u8 *childBlock;
    NNSFndHeapHandle arenaHeap;
    u8 *arenaBlock;

    CHECK(block != NULL && next != NULL && next > block);
    memset(next, 0x5a, 0x40);
    port_heap_ds_read_begin(block, 0x100);
#ifdef TEST_ASAN
    CHECK(!__asan_region_is_poisoned(block, 0x100));
#endif
    CHECK(ReadSpan(block, 0x100) >= 0x40 * 0x5a);
    port_heap_ds_read_end();
#ifdef TEST_ASAN
    // The block's own bytes stay addressable, the head of the next block and the free memory after it are poisoned
    CHECK(!__asan_region_is_poisoned(block, 0x40) && __asan_address_is_poisoned(block + 0x40));
    CHECK(!__asan_region_is_poisoned(next, 0x40) && __asan_address_is_poisoned(next + 0x40));
#endif

    // A child heap in a block of the heap, and a read out of its last block into the parent's memory after it
    parentBlock = NNS_FndAllocFromExpHeapEx(heap, 0x400, 8);
    child = NNS_FndCreateExpHeapEx(parentBlock, 0x400, 0);
    childBlock = NNS_FndAllocFromExpHeapEx(child, 0x40, -8);
    CHECK(child != NULL && childBlock != NULL && childBlock + 0x40 <= parentBlock + 0x400);
    port_heap_ds_read_begin(childBlock, 0x100);
    ReadSpan(childBlock, 0x100);
    port_heap_ds_read_end();
    NNS_FndDestroyExpHeap(child);
    NNS_FndDestroyExpHeap(heap);
    free(memory);

    // A heap carved from the main arena, as the game's are, and a read out of it into the arena after it
    OS_InitArena();
    arenaHeap = NNS_FndCreateExpHeapEx(OS_GetArenaLo(OS_ARENA_MAIN), 0x1000, 0);
    arenaBlock = NNS_FndAllocFromExpHeapEx(arenaHeap, 0x40, -8);
    CHECK(arenaHeap != NULL && arenaBlock != NULL);
    port_heap_ds_read_begin(arenaBlock, 0x400);
    ReadSpan(arenaBlock, 0x400);
    port_heap_ds_read_end();
    NNS_FndDestroyExpHeap(arenaHeap);
}

// A read past a block that leaves the game's memory, past the end of a heap in memory of the host's own: the check
// stops the program (abort), with its message, before anything is read. Run alone, never by "all"
static int ScenarioDsReadOutside(void) {
    size_t heapSize = 0x1000;
    u8 *memory = malloc(heapSize);
    NNSFndHeapHandle heap = NNS_FndCreateExpHeapEx(memory, (u32)heapSize, 0);
    u8 *block = NNS_FndAllocFromExpHeapEx(heap, 0x40, -8);

    port_heap_ds_read_begin(block, 0x2000);
    printf("ds_read_outside: the read went on outside the game's memory\n");
    port_heap_ds_read_end();
    NNS_FndDestroyExpHeap(heap);
    free(memory);
    return 1;
}

static const struct {
    const char *name;
    void (*run)(void);
} sScenarios[] = {
    { "character", ScenarioCharacter }, { "palette", ScenarioPalette },
    { "cell_bank", ScenarioCellBank },  { "multicell_bank", ScenarioMultiCellBank },
    { "anim_bank", ScenarioAnimBank },  { "anim_frames", ScenarioAnimFrames },
    { "font", ScenarioFont },           { "font_nodes", ScenarioFontNodes },
    { "heaps", ScenarioHeaps },         { "reuse", ScenarioReuse },
    { "refill", ScenarioRefill },       { "malformed", ScenarioMalformed },
    { "registry", ScenarioRegistry },   { "ds_reads", ScenarioDsReads },
};

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "all";
    int ran = 0;
    int failures = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (strcmp(name, "rom") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: port_nnsys_tests rom <rom.nds>\n");
            return 2;
        }
        return port_nnsys_rom_test(argv[2]);
    }
    if (strcmp(name, "ds_read_outside") == 0) {
        return ScenarioDsReadOutside();
    }
    for (size_t i = 0; i < sizeof(sScenarios) / sizeof(sScenarios[0]); i++) {
        if (strcmp(name, "all") == 0 || strcmp(name, sScenarios[i].name) == 0) {
            sFailed = 0;
            sScenarios[i].run();
            port_reloc_release_all();
            printf("%s: %s\n", sScenarios[i].name, sFailed ? "FAIL" : "ok");
            failures += sFailed;
            ran++;
        }
    }
    if (ran == 0) {
        fprintf(stderr, "no scenario named %s\n", name);
        return 2;
    }
    return failures != 0;
}
