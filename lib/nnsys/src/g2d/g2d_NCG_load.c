#include "g2di_load.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_NCG_load.c: finds the character data in a loaded NCGR file, for OBJ or for BG, and turns its offset
// into a pointer in place

BOOL NNS_G2dGetUnpackedCharacterData(void *file, NNSG2dCharacterData **character) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_CHARACTERDATA);

    if (blk != NULL) {
        NNS_G2dUnpackNCG((NNSG2dCharacterData *)(blk + 1));
        *character = (NNSG2dCharacterData *)(blk + 1);
        return TRUE;
    }
    *character = NULL;
    return FALSE;
}

void NNS_G2dUnpackNCG(NNSG2dCharacterData *character) {
    NNS_G2D_UNPACK_OFFSET_PTR(character->pRawData, character);
}

BOOL NNS_G2dGetUnpackedBGCharacterData(void *file, NNSG2dCharacterData **character) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_CHARACTERDATA);

    if (blk != NULL) {
        NNS_G2dUnpackBGNCG((NNSG2dCharacterData *)(blk + 1));
        *character = (NNSG2dCharacterData *)(blk + 1);
        return TRUE;
    }
    *character = NULL;
    return FALSE;
}

void NNS_G2dUnpackBGNCG(NNSG2dCharacterData *character) {
    NNS_G2D_UNPACK_OFFSET_PTR(character->pRawData, character);
}
