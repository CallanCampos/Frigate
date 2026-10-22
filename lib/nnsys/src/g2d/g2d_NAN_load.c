#include "g2di_load.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_NAN_load.c: finds the animation bank in a loaded NANR or NMAR file and turns its offsets into
// pointers in place

static BOOL GetUnpackedAnimBankImpl_(void *file, NNSG2dAnimBankData **anims);

static BOOL GetUnpackedAnimBankImpl_(void *file, NNSG2dAnimBankData **anims) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_ANIMBANK);

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

    NNSi_G2dUnpackOffset(bank->pSequenceArrayHead, bank);
    NNSi_G2dUnpackOffset(bank->pFrameArray, bank);
    NNSi_G2dUnpackOffset(bank->pAnimationData, bank);

    seqArray = bank->pSequenceArrayHead;
    frameArray = bank->pFrameArray;
    animData = bank->pAnimationData;
    for (i = 0; i < bank->numSequences; i++) {
        NNSi_G2dUnpackOffset(seqArray[i].pAnmFrameArray, frameArray);
        for (j = 0; j < seqArray[i].numFrames; j++) {
            // The SDK assigns the macro's result again here
            seqArray[i].pAnmFrameArray[j].pContent =
                NNSi_G2dUnpackOffset(seqArray[i].pAnmFrameArray[j].pContent, animData);
        }
    }

    if (bank->pExtendedData != NULL) {
        NNSG2dUserExAnimAttrBank *attrBank;
        u32 seqIdx;
        u32 frmIdx;

        NNSi_G2dUnpackOffset(bank->pExtendedData, bank);
        attrBank = (NNSG2dUserExAnimAttrBank *)((NNSG2dUserExDataBlock *)bank->pExtendedData + 1);
        NNSi_G2dUnpackOffset(attrBank->pAnmSeqAttrArray, attrBank);
        for (seqIdx = 0; seqIdx < attrBank->numSequences; seqIdx++) {
            NNSG2dUserExAnimSequenceAttr *seqAttr = &attrBank->pAnmSeqAttrArray[seqIdx];

            NNSi_G2dUnpackOffset(seqAttr->pAttr, attrBank);
            NNSi_G2dUnpackOffset(seqAttr->pAnmFrmAttrArray, attrBank);
            for (frmIdx = 0; frmIdx < seqAttr->numFrames; frmIdx++) {
                NNSG2dUserExAnimFrameAttr *frmAttr = &seqAttr->pAnmFrmAttrArray[frmIdx];

                NNSi_G2dUnpackOffset(frmAttr->pAttr, attrBank);
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
