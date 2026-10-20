#include "g2di_load.h"
#include "nnsys/g2d.h"

// What NitroSystem's G2D file loaders share: finding a block in a loaded file, and the user extended cell attributes
// of cell and multi-cell banks. The file name is a guess

NNSG2dBinaryBlockHeader *NNS_G2dFindBinaryBlock(NNSG2dBinaryFileHeader *file, u32 kind) {
    NNSG2dBinaryBlockHeader *blk = (NNSG2dBinaryBlockHeader *)((u8 *)file + file->headerSize);
    u16 i;

    for (i = 0; i < file->numBlocks; i++) {
        if (blk->kind == kind) {
            return blk;
        }
        blk = (NNSG2dBinaryBlockHeader *)((u8 *)blk + blk->size);
    }
    return NULL;
}

void NNSi_G2dUnpackUserExCellAttrBank(NNSG2dUserExCellAttrBank *attrBank) {
    u16 i;

    NNSi_G2dUnpackOffset(attrBank->pCellAttrArray, attrBank);
    for (i = 0; i < attrBank->numCells; i++) {
        // The SDK assigns the macro's result again here
        attrBank->pCellAttrArray[i].pAttr = NNSi_G2dUnpackOffset(attrBank->pCellAttrArray[i].pAttr, attrBank);
    }
}
