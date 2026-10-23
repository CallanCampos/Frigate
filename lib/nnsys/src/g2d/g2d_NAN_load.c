#include "g2di_load.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_NAN_load.c: finds the animation bank in a loaded NANR or NMAR file and turns its offsets into
// pointers in place

static BOOL GetUnpackedAnimBankImpl_(void *file, NNSG2dAnimBankData **anims);

static BOOL GetUnpackedAnimBankImpl_(void *file, NNSG2dAnimBankData **anims) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BLKSIG_ANIMBANK);

    if (blk != NULL) {
        NNS_G2dUnpackNAN((NNSG2dAnimBankData *)(blk + 1));
        *anims = (NNSG2dAnimBankData *)(blk + 1);
        return TRUE;
    }
    *anims = NULL;
    return FALSE;
}

BOOL NNS_G2dGetUnpackedAnimBank(void *file, NNSG2dAnimBankData **anims) {
    return GetUnpackedAnimBankImpl_(file, anims);
}

BOOL NNS_G2dGetUnpackedMCAnimBank(void *file, NNSG2dAnimBankData **anims) {
    return GetUnpackedAnimBankImpl_(file, anims);
}

void NNS_G2dUnpackNAN(NNSG2dAnimBankData *bank) {
    u16 i;
    u16 j;
    NNSG2dAnimFrameData *frameArray;
    NNSG2dAnimSequence *seqArray;
    void *animData;

    NNS_G2D_UNPACK_OFFSET_PTR(bank->pSequenceArrayHead, bank);
    NNS_G2D_UNPACK_OFFSET_PTR(bank->pFrameArrayHead, bank);
    NNS_G2D_UNPACK_OFFSET_PTR(bank->pAnimContents, bank);

    seqArray = bank->pSequenceArrayHead;
    frameArray = bank->pFrameArrayHead;
    animData = bank->pAnimContents;
    for (i = 0; i < bank->numSequences; i++) {
        NNS_G2D_UNPACK_OFFSET_PTR(seqArray[i].pAnmFrameArray, frameArray);
        for (j = 0; j < seqArray[i].numFrames; j++) {
            // The SDK assigns the macro's result again here
            seqArray[i].pAnmFrameArray[j].pContent =
                NNS_G2D_UNPACK_OFFSET_PTR(seqArray[i].pAnmFrameArray[j].pContent, animData);
        }
    }

    if (bank->pExtendedData != NULL) {
        NNSG2dUserExAnimAttrBank *attrBank;
        u32 seqIdx;
        u32 frmIdx;

        NNS_G2D_UNPACK_OFFSET_PTR(bank->pExtendedData, bank);
        attrBank = (NNSG2dUserExAnimAttrBank *)((NNSG2dUserExDataBlock *)bank->pExtendedData + 1);
        NNS_G2D_UNPACK_OFFSET_PTR(attrBank->pAnmSeqAttrArray, attrBank);
        for (seqIdx = 0; seqIdx < attrBank->numSequences; seqIdx++) {
            NNSG2dUserExAnimSequenceAttr *seqAttr = &attrBank->pAnmSeqAttrArray[seqIdx];

            NNS_G2D_UNPACK_OFFSET_PTR(seqAttr->pAttr, attrBank);
            NNS_G2D_UNPACK_OFFSET_PTR(seqAttr->pAnmFrmAttrArray, attrBank);
            for (frmIdx = 0; frmIdx < seqAttr->numFrames; frmIdx++) {
                NNSG2dUserExAnimFrameAttr *frmAttr = &seqAttr->pAnmFrmAttrArray[frmIdx];

                NNS_G2D_UNPACK_OFFSET_PTR(frmAttr->pAttr, attrBank);
            }
        }
    }
}

const NNSG2dAnimSequence *NNS_G2dGetAnimSequenceByIdx(const NNSG2dAnimBankData *bank, u16 idx) {
    if (bank->numSequences > idx) {
        return &bank->pSequenceArrayHead[idx];
    }
    return NULL;
}
