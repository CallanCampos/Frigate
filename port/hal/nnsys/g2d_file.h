// The DS layouts of the G2D structs that NitroSystem's file loaders relocate in place (lib/nnsys/src/g2d/*_load.c and
// g2d_Load.c, through g2di_load.h's NNS_G2D_UNPACK_OFFSET_PTR). In a file each pointer field holds a u32 offset from
// the start of some struct, and the DS adds that struct's address to it, so the file's bytes become the runtime struct.
// On a 64-bit host the runtime structs of nnsys/g2d.h hold 8-byte pointers and no longer match the file: these are the
// file's view, a u32 where g2d.h has a pointer, field for field, read from the file by port/hal/nnsys/g2d_load.c to
// build the host's structs. Some fields have names of their own (rawData for g2d.h's pRawData, size for szByte), which
// port/tests/nnsys/ds_layout_probe.c pairs with g2d.h's.
//
// The sizes and offsets are MWCC's, which i386 SysV reproduces exactly (port/docs/research/compile-probe.md): the
// asserts below are the decomp's structs compiled for i386. Plain C types besides nnsys/g2d.h's pointer-free ones, so
// that the tests can include this header.
#ifndef PORT_HAL_NNSYS_G2D_FILE_H
#define PORT_HAL_NNSYS_G2D_FILE_H

#include "nnsys/g2d.h"
#include <stddef.h>

// NNSG2dCharacterData, an NCGR file's CHAR block
typedef struct PortG2dFileCharacterData {
    u16 height;
    u16 width;
    u32 pixelFormat;
    u32 mappingType;
    u32 characterFormat;
    u32 size;
    u32 rawData; // From the struct
} PortG2dFileCharacterData;

// NNSG2dPaletteData, an NCLR file's PLTT block
typedef struct PortG2dFilePaletteData {
    u32 format;
    u32 extendedPalette;
    u32 size;
    u32 rawData; // From the struct
} PortG2dFilePaletteData;

// NNSG2dPaletteCompressInfo, an NCLR file's PCMP block
typedef struct PortG2dFilePaletteCompressInfo {
    u16 numPalette;
    u16 pad;
    u32 paletteIndexTable; // From the struct
} PortG2dFilePaletteCompressInfo;

// NNSG2dCellData and NNSG2dCellDataWithBR, the cells of an NCER file's CEBK block
typedef struct PortG2dFileCellData {
    u16 numOAMAttrs;
    u16 cellAttr;
    u32 pOamAttrArray; // From the first OAM, which follows the last cell
} PortG2dFileCellData;

typedef struct PortG2dFileCellDataWithBR {
    PortG2dFileCellData cellData;
    NNSG2dCellBoundingRectS16 boundingRect;
} PortG2dFileCellDataWithBR;

// NNSG2dVramTransferData, after a cell bank whose characters go to VRAM cell by cell
typedef struct PortG2dFileVramTransferData {
    u32 szByteMax;
    u32 pCellTransferDataArray; // From this struct
} PortG2dFileVramTransferData;

// NNSG2dCellDataBank, an NCER file's CEBK block
typedef struct PortG2dFileCellDataBank {
    u16 numCells;
    u16 cellBankAttr;
    u32 pCellDataArrayHead; // From the bank
    u32 mappingMode;
    u32 pVramTransferData; // From the bank, or 0 for none
    u32 pStringBank;       // Never relocated: the runtime struct keeps the offset
    u32 pExtendedData;     // From the bank, or 0 for none
} PortG2dFileCellDataBank;

// NNSG2dMultiCellData, the multi-cells of an NMCR file's MCBK block
typedef struct PortG2dFileMultiCellData {
    u16 numNodes;
    u16 numCellAnim;
    u32 pHierDataArray; // From the bank's hierarchy array
} PortG2dFileMultiCellData;

// NNSG2dMultiCellDataBank, an NMCR file's MCBK block
typedef struct PortG2dFileMultiCellDataBank {
    u16 numMultiCellData;
    u16 pad;
    u32 pMultiCellDataArray; // From the bank
    u32 pHierarchyDataArray; // From the bank
    u32 pStringBank;         // Never relocated
    u32 pExtendedData;       // From the bank, or 0 for none
} PortG2dFileMultiCellDataBank;

// NNSG2dUserExCellAttr and NNSG2dUserExCellAttrBank, the user extended attributes of cells, after an
// NNSG2dUserExDataBlock header
typedef struct PortG2dFileUserExCellAttr {
    u32 pAttr; // From the attribute bank
} PortG2dFileUserExCellAttr;

typedef struct PortG2dFileUserExCellAttrBank {
    u16 numCells;
    u16 numAttribute;
    u32 pCellAttrArray; // From the attribute bank
} PortG2dFileUserExCellAttrBank;

// NNSG2dAnimFrameData, NNSG2dAnimSequence and NNSG2dAnimBankData, an NANR or NMAR file's ABNK block
typedef struct PortG2dFileAnimFrameData {
    u32 pContent; // From the bank's animation data
    u16 frames;
    u16 pad;
} PortG2dFileAnimFrameData;

typedef struct PortG2dFileAnimSequence {
    u16 numFrames;
    u16 loopStartFrameIdx;
    u32 animType;
    u32 playMode;
    u32 pAnmFrameArray; // From the bank's frame array
} PortG2dFileAnimSequence;

typedef struct PortG2dFileAnimBankData {
    u16 numSequences;
    u16 numTotalFrames;
    u32 pSequenceArrayHead; // From the bank
    u32 pFrameArray;        // From the bank
    u32 pAnimationData;     // From the bank
    u32 pStringBank;        // Never relocated
    u32 pExtendedData;      // From the bank, or 0 for none
} PortG2dFileAnimBankData;

// NNSG2dUserExAnimFrameAttr, NNSG2dUserExAnimSequenceAttr and NNSG2dUserExAnimAttrBank, the user extended attributes
// of animation sequences and frames, after an NNSG2dUserExDataBlock header
typedef struct PortG2dFileUserExAnimFrameAttr {
    u32 pAttr; // From the attribute bank
} PortG2dFileUserExAnimFrameAttr;

typedef struct PortG2dFileUserExAnimSequenceAttr {
    u16 numFrames;
    u16 pad;
    u32 pAttr;            // From the attribute bank
    u32 pAnmFrmAttrArray; // From the attribute bank
} PortG2dFileUserExAnimSequenceAttr;

typedef struct PortG2dFileUserExAnimAttrBank {
    u16 numSequences;
    u16 numAttribute;
    u32 pAnmSeqAttrArray; // From the attribute bank
} PortG2dFileUserExAnimAttrBank;

// NNSG2dFontInformation, NNSG2dFontWidth and NNSG2dFontCodeMap, an NFTR file's FINF, CWDH and CMAP blocks. Their
// offsets are from the file's header. A width block's table and a code map's mapInfo follow the fixed part
typedef struct PortG2dFileFontInformation {
    u8 fontType;
    s8 linefeed;
    u16 alterCharIndex;
    NNSG2dCharWidths defaultWidth;
    u8 encoding;
    u32 pGlyph; // Always relocated
    u32 pWidth; // 0 for none
    u32 pMap;   // 0 for none
} PortG2dFileFontInformation;

typedef struct PortG2dFileFontWidth {
    u16 indexBegin;
    u16 indexEnd;
    u32 pNext; // 0 for none
} PortG2dFileFontWidth;

typedef struct PortG2dFileFontCodeMap {
    u16 ccodeBegin;
    u16 ccodeEnd;
    u16 mappingMethod;
    u16 reserved;
    u32 pNext; // 0 for none
} PortG2dFileFontCodeMap;

// MWCC's layouts, from the decomp's structs compiled for i386 (port/tests/nnsys/ds_layout_probe.c checks them there)
#define PORT_G2D_ASSERT_SIZE(type, size) _Static_assert(sizeof(type) == (size), #type " is not the DS's size")
#define PORT_G2D_ASSERT_AT(type, field, offset)                                                                        \
    _Static_assert(offsetof(type, field) == (offset), #type "." #field " is not at the DS's offset")

PORT_G2D_ASSERT_SIZE(PortG2dFileCharacterData, 0x18);
PORT_G2D_ASSERT_AT(PortG2dFileCharacterData, pixelFormat, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileCharacterData, mappingType, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileCharacterData, characterFormat, 0x0c);
PORT_G2D_ASSERT_AT(PortG2dFileCharacterData, size, 0x10);
PORT_G2D_ASSERT_AT(PortG2dFileCharacterData, rawData, 0x14);
PORT_G2D_ASSERT_SIZE(PortG2dFilePaletteData, 0x10);
PORT_G2D_ASSERT_AT(PortG2dFilePaletteData, rawData, 0x0c);
PORT_G2D_ASSERT_SIZE(PortG2dFilePaletteCompressInfo, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFilePaletteCompressInfo, paletteIndexTable, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileCellData, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileCellData, pOamAttrArray, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileCellDataWithBR, 0x10);
PORT_G2D_ASSERT_AT(PortG2dFileCellDataWithBR, boundingRect, 0x08);
PORT_G2D_ASSERT_SIZE(PortG2dFileVramTransferData, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileVramTransferData, pCellTransferDataArray, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileCellDataBank, 0x18);
PORT_G2D_ASSERT_AT(PortG2dFileCellDataBank, pCellDataArrayHead, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileCellDataBank, mappingMode, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileCellDataBank, pVramTransferData, 0x0c);
PORT_G2D_ASSERT_AT(PortG2dFileCellDataBank, pStringBank, 0x10);
PORT_G2D_ASSERT_AT(PortG2dFileCellDataBank, pExtendedData, 0x14);
PORT_G2D_ASSERT_SIZE(PortG2dFileMultiCellData, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileMultiCellData, pHierDataArray, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileMultiCellDataBank, 0x14);
PORT_G2D_ASSERT_AT(PortG2dFileMultiCellDataBank, pMultiCellDataArray, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileMultiCellDataBank, pHierarchyDataArray, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileMultiCellDataBank, pStringBank, 0x0c);
PORT_G2D_ASSERT_AT(PortG2dFileMultiCellDataBank, pExtendedData, 0x10);
PORT_G2D_ASSERT_SIZE(PortG2dFileUserExCellAttr, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileUserExCellAttrBank, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileUserExCellAttrBank, pCellAttrArray, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileAnimFrameData, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileAnimFrameData, frames, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileAnimSequence, 0x10);
PORT_G2D_ASSERT_AT(PortG2dFileAnimSequence, animType, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileAnimSequence, playMode, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileAnimSequence, pAnmFrameArray, 0x0c);
PORT_G2D_ASSERT_SIZE(PortG2dFileAnimBankData, 0x18);
PORT_G2D_ASSERT_AT(PortG2dFileAnimBankData, pSequenceArrayHead, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileAnimBankData, pFrameArray, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileAnimBankData, pAnimationData, 0x0c);
PORT_G2D_ASSERT_AT(PortG2dFileAnimBankData, pStringBank, 0x10);
PORT_G2D_ASSERT_AT(PortG2dFileAnimBankData, pExtendedData, 0x14);
PORT_G2D_ASSERT_SIZE(PortG2dFileUserExAnimFrameAttr, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileUserExAnimSequenceAttr, 0x0c);
PORT_G2D_ASSERT_AT(PortG2dFileUserExAnimSequenceAttr, pAttr, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileUserExAnimSequenceAttr, pAnmFrmAttrArray, 0x08);
PORT_G2D_ASSERT_SIZE(PortG2dFileUserExAnimAttrBank, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileUserExAnimAttrBank, pAnmSeqAttrArray, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileFontInformation, 0x14);
PORT_G2D_ASSERT_AT(PortG2dFileFontInformation, alterCharIndex, 0x02);
PORT_G2D_ASSERT_AT(PortG2dFileFontInformation, defaultWidth, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileFontInformation, encoding, 0x07);
PORT_G2D_ASSERT_AT(PortG2dFileFontInformation, pGlyph, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileFontInformation, pWidth, 0x0c);
PORT_G2D_ASSERT_AT(PortG2dFileFontInformation, pMap, 0x10);
PORT_G2D_ASSERT_SIZE(PortG2dFileFontWidth, 0x08);
PORT_G2D_ASSERT_AT(PortG2dFileFontWidth, pNext, 0x04);
PORT_G2D_ASSERT_SIZE(PortG2dFileFontCodeMap, 0x0c);
PORT_G2D_ASSERT_AT(PortG2dFileFontCodeMap, mappingMethod, 0x04);
PORT_G2D_ASSERT_AT(PortG2dFileFontCodeMap, pNext, 0x08);

// The pointer-free structs that a host struct points at in the file, which must keep the DS's layout on the host too
PORT_G2D_ASSERT_SIZE(NNSG2dBinaryFileHeader, 0x10);
PORT_G2D_ASSERT_SIZE(NNSG2dBinaryBlockHeader, 0x08);
PORT_G2D_ASSERT_SIZE(NNSG2dUserExDataBlock, 0x08);
PORT_G2D_ASSERT_SIZE(NNSG2dCellBoundingRectS16, 0x08);
PORT_G2D_ASSERT_SIZE(NNSG2dCellVramTransferData, 0x08);
PORT_G2D_ASSERT_SIZE(NNSG2dMultiCellHierarchyData, 0x08);
PORT_G2D_ASSERT_SIZE(NNSG2dAnimDataSRT, 0x10);
PORT_G2D_ASSERT_SIZE(NNSG2dAnimDataT, 0x08);
PORT_G2D_ASSERT_SIZE(NNSG2dCharWidths, 0x03);
PORT_G2D_ASSERT_SIZE(NNSG2dFontGlyph, 0x08);
PORT_G2D_ASSERT_SIZE(NNSG2dCMapScanEntry, 0x04);
PORT_G2D_ASSERT_SIZE(NNSG2dScreenData, 0x10);

#endif // PORT_HAL_NNSYS_G2D_FILE_H
