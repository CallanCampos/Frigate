#include "g2di_load.h"
#include "nnsys/g2d.h"

// NitroSystem's cell bank loader, g2d_NOB_load.c in ntrtwl's reconstruction of NitroSystem, whose header is
// g2d_NCE_load.h: finds the cell bank in a loaded NCER file and turns its offsets into pointers in place

static void *GetPtrOamArrayHead_(const NNSG2dCellDataBank *bank);
static void UnPackExtendedData_(NNSG2dUserExDataBlock *blk);

// The cells' OAMs follow the last cell
static void *GetPtrOamArrayHead_(const NNSG2dCellDataBank *bank) {
    if (bank->cellBankAttr & NNS_G2D_CELLBK_ATTR_CELLWITHBR) {
        return (NNSG2dCellDataWithBR *)bank->pCellDataArrayHead + bank->numCells;
    }
    return bank->pCellDataArrayHead + bank->numCells;
}

static void UnPackExtendedData_(NNSG2dUserExDataBlock *blk) {
    NNSi_G2dUnpackUserExCellAttrBank((NNSG2dUserExCellAttrBank *)(blk + 1));
}

BOOL NNS_G2dGetUnpackedCellBank(void *file, NNSG2dCellDataBank **cells) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BLKSIG_CELLBANK);

    if (blk != NULL) {
        NNS_G2dUnpackNCE((NNSG2dCellDataBank *)(blk + 1));
        *cells = (NNSG2dCellDataBank *)(blk + 1);
        return TRUE;
    }
    *cells = NULL;
    return FALSE;
}

const NNSG2dCellData *NNS_G2dGetCellDataByIdx(const NNSG2dCellDataBank *bank, u16 idx) {
    if (idx >= bank->numCells) {
        return NULL;
    }
    if (bank->cellBankAttr & NNS_G2D_CELLBK_ATTR_CELLWITHBR) {
        return &((NNSG2dCellDataWithBR *)bank->pCellDataArrayHead)[idx].cellData;
    }
    return &bank->pCellDataArrayHead[idx];
}

void NNS_G2dUnpackNCE(NNSG2dCellDataBank *bank) {
    u16 i;
    void *oamArray;

    NNS_G2D_UNPACK_OFFSET_PTR(bank->pCellDataArrayHead, bank);
    oamArray = GetPtrOamArrayHead_(bank);
    for (i = 0; i < bank->numCells; i++) {
        NNSG2dCellData *cell = (NNSG2dCellData *)NNS_G2dGetCellDataByIdx(bank, i);

        NNS_G2D_UNPACK_OFFSET_PTR(cell->pOamAttrArray, oamArray);
    }

    if (bank->pVramTransferData != NULL) {
        NNSG2dVramTransferData *transfer;

        NNS_G2D_UNPACK_OFFSET_PTR(bank->pVramTransferData, bank);
        transfer = bank->pVramTransferData;
        NNS_G2D_UNPACK_OFFSET_PTR(transfer->pCellTransferDataArray, transfer);
        // It stores the pointer back, though the field already holds it
        bank->pVramTransferData = transfer;
    }

    if (bank->pExtendedData != NULL) {
        NNS_G2D_UNPACK_OFFSET_PTR(bank->pExtendedData, bank);
        UnPackExtendedData_(bank->pExtendedData);
    }
}
