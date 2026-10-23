// Checks port/hal/nnsys/g2d_file.h's DS layouts against the decomp's own structs as MWCC lays them out. Compiled, never
// run, for i386 SysV, whose layouts are MWCC's (port/docs/research/compile-probe.md), without PLATFORM_PC, so that
// nnsys/g2d.h's pointers are 4 bytes: each file struct must have the size of its runtime struct, and each field the
// offset of the runtime field it stands for. port/cmake/nnsys.cmake runs it as the test nnsys_ds_layout.
#include "hal/nnsys/g2d_file.h"

_Static_assert(sizeof(void *) == 4, "the probe must be compiled for a 32-bit target");

#define SAME_SIZE(file, runtime) _Static_assert(sizeof(file) == sizeof(runtime), #file " differs from " #runtime)
#define SAME_FIELD_AT(file, runtime, fileField, runtimeField) \
    _Static_assert(offsetof(file, fileField) == offsetof(runtime, runtimeField), \
                   #file "." #fileField " differs from " #runtime)
#define SAME_AT(file, runtime, field) SAME_FIELD_AT(file, runtime, field, field)

SAME_SIZE(PortG2dFileCharacterData, NNSG2dCharacterData);
SAME_FIELD_AT(PortG2dFileCharacterData, NNSG2dCharacterData, height, H);
SAME_FIELD_AT(PortG2dFileCharacterData, NNSG2dCharacterData, width, W);
SAME_FIELD_AT(PortG2dFileCharacterData, NNSG2dCharacterData, pixelFormat, pixelFmt);
SAME_AT(PortG2dFileCharacterData, NNSG2dCharacterData, mappingType);
SAME_FIELD_AT(PortG2dFileCharacterData, NNSG2dCharacterData, characterFormat, characterFmt);
SAME_FIELD_AT(PortG2dFileCharacterData, NNSG2dCharacterData, size, szByte);
SAME_FIELD_AT(PortG2dFileCharacterData, NNSG2dCharacterData, rawData, pRawData);

SAME_SIZE(PortG2dFilePaletteData, NNSG2dPaletteData);
SAME_FIELD_AT(PortG2dFilePaletteData, NNSG2dPaletteData, format, fmt);
SAME_FIELD_AT(PortG2dFilePaletteData, NNSG2dPaletteData, extendedPalette, bExtendedPlt);
SAME_FIELD_AT(PortG2dFilePaletteData, NNSG2dPaletteData, size, szByte);
SAME_FIELD_AT(PortG2dFilePaletteData, NNSG2dPaletteData, rawData, pRawData);

SAME_SIZE(PortG2dFilePaletteCompressInfo, NNSG2dPaletteCompressInfo);
SAME_AT(PortG2dFilePaletteCompressInfo, NNSG2dPaletteCompressInfo, numPalette);
SAME_FIELD_AT(PortG2dFilePaletteCompressInfo, NNSG2dPaletteCompressInfo, pad, pad16);
SAME_FIELD_AT(PortG2dFilePaletteCompressInfo, NNSG2dPaletteCompressInfo, paletteIndexTable, pPlttIdxTbl);

SAME_SIZE(PortG2dFileCellData, NNSG2dCellData);
SAME_AT(PortG2dFileCellData, NNSG2dCellData, numOAMAttrs);
SAME_AT(PortG2dFileCellData, NNSG2dCellData, cellAttr);
SAME_AT(PortG2dFileCellData, NNSG2dCellData, pOamAttrArray);

SAME_SIZE(PortG2dFileCellDataWithBR, NNSG2dCellDataWithBR);
SAME_AT(PortG2dFileCellDataWithBR, NNSG2dCellDataWithBR, cellData);
SAME_AT(PortG2dFileCellDataWithBR, NNSG2dCellDataWithBR, boundingRect);

SAME_SIZE(PortG2dFileVramTransferData, NNSG2dVramTransferData);
SAME_AT(PortG2dFileVramTransferData, NNSG2dVramTransferData, szByteMax);
SAME_AT(PortG2dFileVramTransferData, NNSG2dVramTransferData, pCellTransferDataArray);

SAME_SIZE(PortG2dFileCellDataBank, NNSG2dCellDataBank);
SAME_AT(PortG2dFileCellDataBank, NNSG2dCellDataBank, numCells);
SAME_AT(PortG2dFileCellDataBank, NNSG2dCellDataBank, cellBankAttr);
SAME_AT(PortG2dFileCellDataBank, NNSG2dCellDataBank, pCellDataArrayHead);
SAME_AT(PortG2dFileCellDataBank, NNSG2dCellDataBank, mappingMode);
SAME_AT(PortG2dFileCellDataBank, NNSG2dCellDataBank, pVramTransferData);
SAME_AT(PortG2dFileCellDataBank, NNSG2dCellDataBank, pStringBank);
SAME_AT(PortG2dFileCellDataBank, NNSG2dCellDataBank, pExtendedData);

SAME_SIZE(PortG2dFileMultiCellData, NNSG2dMultiCellData);
SAME_AT(PortG2dFileMultiCellData, NNSG2dMultiCellData, numNodes);
SAME_AT(PortG2dFileMultiCellData, NNSG2dMultiCellData, numCellAnim);
SAME_AT(PortG2dFileMultiCellData, NNSG2dMultiCellData, pHierDataArray);

SAME_SIZE(PortG2dFileMultiCellDataBank, NNSG2dMultiCellDataBank);
SAME_AT(PortG2dFileMultiCellDataBank, NNSG2dMultiCellDataBank, numMultiCellData);
SAME_FIELD_AT(PortG2dFileMultiCellDataBank, NNSG2dMultiCellDataBank, pad, pad16);
SAME_AT(PortG2dFileMultiCellDataBank, NNSG2dMultiCellDataBank, pMultiCellDataArray);
SAME_AT(PortG2dFileMultiCellDataBank, NNSG2dMultiCellDataBank, pHierarchyDataArray);
SAME_AT(PortG2dFileMultiCellDataBank, NNSG2dMultiCellDataBank, pStringBank);
SAME_AT(PortG2dFileMultiCellDataBank, NNSG2dMultiCellDataBank, pExtendedData);

SAME_SIZE(PortG2dFileUserExCellAttr, NNSG2dUserExCellAttr);
SAME_AT(PortG2dFileUserExCellAttr, NNSG2dUserExCellAttr, pAttr);

SAME_SIZE(PortG2dFileUserExCellAttrBank, NNSG2dUserExCellAttrBank);
SAME_AT(PortG2dFileUserExCellAttrBank, NNSG2dUserExCellAttrBank, numCells);
SAME_AT(PortG2dFileUserExCellAttrBank, NNSG2dUserExCellAttrBank, numAttribute);
SAME_AT(PortG2dFileUserExCellAttrBank, NNSG2dUserExCellAttrBank, pCellAttrArray);

SAME_SIZE(PortG2dFileAnimFrameData, NNSG2dAnimFrameData);
SAME_AT(PortG2dFileAnimFrameData, NNSG2dAnimFrameData, pContent);
SAME_AT(PortG2dFileAnimFrameData, NNSG2dAnimFrameData, frames);
SAME_FIELD_AT(PortG2dFileAnimFrameData, NNSG2dAnimFrameData, pad, pad16);

SAME_SIZE(PortG2dFileAnimSequence, NNSG2dAnimSequence);
SAME_AT(PortG2dFileAnimSequence, NNSG2dAnimSequence, numFrames);
SAME_AT(PortG2dFileAnimSequence, NNSG2dAnimSequence, loopStartFrameIdx);
SAME_AT(PortG2dFileAnimSequence, NNSG2dAnimSequence, animType);
SAME_AT(PortG2dFileAnimSequence, NNSG2dAnimSequence, playMode);
SAME_AT(PortG2dFileAnimSequence, NNSG2dAnimSequence, pAnmFrameArray);

SAME_SIZE(PortG2dFileAnimBankData, NNSG2dAnimBankData);
SAME_AT(PortG2dFileAnimBankData, NNSG2dAnimBankData, numSequences);
SAME_AT(PortG2dFileAnimBankData, NNSG2dAnimBankData, numTotalFrames);
SAME_AT(PortG2dFileAnimBankData, NNSG2dAnimBankData, pSequenceArrayHead);
SAME_FIELD_AT(PortG2dFileAnimBankData, NNSG2dAnimBankData, pFrameArray, pFrameArrayHead);
SAME_FIELD_AT(PortG2dFileAnimBankData, NNSG2dAnimBankData, pAnimationData, pAnimContents);
SAME_AT(PortG2dFileAnimBankData, NNSG2dAnimBankData, pStringBank);
SAME_AT(PortG2dFileAnimBankData, NNSG2dAnimBankData, pExtendedData);

SAME_SIZE(PortG2dFileUserExAnimFrameAttr, NNSG2dUserExAnimFrameAttr);
SAME_AT(PortG2dFileUserExAnimFrameAttr, NNSG2dUserExAnimFrameAttr, pAttr);

SAME_SIZE(PortG2dFileUserExAnimSequenceAttr, NNSG2dUserExAnimSequenceAttr);
SAME_AT(PortG2dFileUserExAnimSequenceAttr, NNSG2dUserExAnimSequenceAttr, numFrames);
SAME_FIELD_AT(PortG2dFileUserExAnimSequenceAttr, NNSG2dUserExAnimSequenceAttr, pad, pad16);
SAME_AT(PortG2dFileUserExAnimSequenceAttr, NNSG2dUserExAnimSequenceAttr, pAttr);
SAME_AT(PortG2dFileUserExAnimSequenceAttr, NNSG2dUserExAnimSequenceAttr, pAnmFrmAttrArray);

SAME_SIZE(PortG2dFileUserExAnimAttrBank, NNSG2dUserExAnimAttrBank);
SAME_AT(PortG2dFileUserExAnimAttrBank, NNSG2dUserExAnimAttrBank, numSequences);
SAME_AT(PortG2dFileUserExAnimAttrBank, NNSG2dUserExAnimAttrBank, numAttribute);
SAME_AT(PortG2dFileUserExAnimAttrBank, NNSG2dUserExAnimAttrBank, pAnmSeqAttrArray);

SAME_SIZE(PortG2dFileFontInformation, NNSG2dFontInformation);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, fontType);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, linefeed);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, alterCharIndex);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, defaultWidth);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, encoding);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, pGlyph);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, pWidth);
SAME_AT(PortG2dFileFontInformation, NNSG2dFontInformation, pMap);

// The variable-length structs: the fixed part, then the table at the runtime struct's flexible member
_Static_assert(sizeof(PortG2dFileFontWidth) == offsetof(NNSG2dFontWidth, widthTable), "NNSG2dFontWidth's table moved");
SAME_AT(PortG2dFileFontWidth, NNSG2dFontWidth, indexBegin);
SAME_AT(PortG2dFileFontWidth, NNSG2dFontWidth, indexEnd);
SAME_AT(PortG2dFileFontWidth, NNSG2dFontWidth, pNext);

_Static_assert(sizeof(PortG2dFileFontCodeMap) == offsetof(NNSG2dFontCodeMap, mapInfo), "NNSG2dFontCodeMap's map moved");
SAME_AT(PortG2dFileFontCodeMap, NNSG2dFontCodeMap, ccodeBegin);
SAME_AT(PortG2dFileFontCodeMap, NNSG2dFontCodeMap, ccodeEnd);
SAME_AT(PortG2dFileFontCodeMap, NNSG2dFontCodeMap, mappingMethod);
SAME_AT(PortG2dFileFontCodeMap, NNSG2dFontCodeMap, reserved);
SAME_AT(PortG2dFileFontCodeMap, NNSG2dFontCodeMap, pNext);
