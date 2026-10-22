#include "nnsys/g2d.h"

// NitroSystem's g2d_CellAnimation.c: a cell animation, an animation controller whose frames each show a cell of a
// bank, maybe scaled, rotated and moved, and whose characters may be transferred to VRAM as each cell is shown

static void ApplyCurrentAnimResult_(NNSG2dCellAnimation *cellAnim);

static inline u32 GetAnimSequenceElementType_(u32 animType) {
    return animType & 0xff;
}

static inline BOOL IsCellAnimVramTransferHandleValid_(const NNSG2dCellAnimation *cellAnim) {
    return (BOOL)(cellAnim->cellTransferStateHandle != NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE);
}

// Shows the current frame's cell, with its scale, rotation and translation, and asks for its characters to be
// transferred
static void ApplyCurrentAnimResult_(NNSG2dCellAnimation *cellAnim) {
    const NNSG2dAnimDataSRT *result;
    const NNSG2dCellDataBank *bank;
    u32 elemType;

    if (cellAnim->animCtrl.pActiveCurrent->frames == 0) {
        return;
    }
    (void)NNS_G2dGetAnimCtrlCurrentElement(&cellAnim->animCtrl);
    result = NNS_G2dGetAnimCtrlCurrentElement(&cellAnim->animCtrl);
    bank = cellAnim->pCellDataBank;
    cellAnim->pCurrentCell = NNS_G2dGetCellDataByIdx(bank, result->index);

    elemType = GetAnimSequenceElementType_(cellAnim->animCtrl.pAnimSequence->animType);
    NNSi_G2dSrtcInitControl(&cellAnim->srtCtrl, NNS_G2D_SRTCONTROLTYPE_SRT);
    if (elemType != NNS_G2D_ANIMELEM_INDEX) {
        if (elemType == NNS_G2D_ANIMELEM_INDEX_T) {
            const NNSG2dAnimDataT *resultT = (const NNSG2dAnimDataT *)result;

            NNSi_G2dSrtcSetTrans(&cellAnim->srtCtrl, resultT->px, resultT->py);
        } else {
            NNSi_G2dSrtcSetSRTScale(&cellAnim->srtCtrl, result->sx, result->sy);
            NNSi_G2dSrtcSetSRTRotZ(&cellAnim->srtCtrl, result->rotZ);
            NNSi_G2dSrtcSetTrans(&cellAnim->srtCtrl, result->px, result->py);
        }
    }

    if (NNS_G2dCellDataBankHasVramTransferData(bank) && IsCellAnimVramTransferHandleValid_(cellAnim)) {
        const NNSG2dCellVramTransferData *transfer = &bank->pVramTransferData->pCellTransferDataArray[result->index];

        NNS_G2dSetCellTransferStateRequested(cellAnim->cellTransferStateHandle, transfer->srcDataOffset,
                                             transfer->szByte);
    }
}

static inline void InitCellAnimationImpl_(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimSequence *seq,
                                          const NNSG2dCellDataBank *bank, u32 vramStateHandle) {
    cellAnim->pCellDataBank = bank;
    cellAnim->cellTransferStateHandle = vramStateHandle;
    NNSi_G2dSrtcInitControl(&cellAnim->srtCtrl, NNS_G2D_SRTCONTROLTYPE_SRT);
    NNS_G2dInitAnimCtrl(&cellAnim->animCtrl);
    NNS_G2dSetCellAnimationSequence(cellAnim, seq);
}

void NNS_G2dInitCellAnimation(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimSequence *seq,
                              const NNSG2dCellDataBank *bank) {
    InitCellAnimationImpl_(cellAnim, seq, bank, NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE);
}

void NNS_G2dInitCellAnimationVramTransfered(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimSequence *seq,
                                            const NNSG2dCellDataBank *bank, u32 vramStateHandle, u32 dstAddr3D,
                                            u32 dstAddr2DMain, u32 dstAddr2DSub, const void *pSrcNCGR,
                                            const void *pSrcNCBR, u32 szSrcData) {
    NNSi_G2dInitCellTransferState(vramStateHandle, dstAddr3D, dstAddr2DMain, dstAddr2DSub,
                                  bank->pVramTransferData->szByteMax, pSrcNCGR, pSrcNCBR, szSrcData);
    cellAnim->cellTransferStateHandle = vramStateHandle;
    InitCellAnimationImpl_(cellAnim, seq, bank, vramStateHandle);
}

void NNS_G2dSetCellAnimationSequence(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimSequence *seq) {
    NNS_G2dBindAnimCtrl(&cellAnim->animCtrl, seq);
    ApplyCurrentAnimResult_(cellAnim);
}

void NNS_G2dTickCellAnimation(NNSG2dCellAnimation *cellAnim, fx32 frames) {
    if (NNS_G2dTickAnimCtrl(&cellAnim->animCtrl, frames)) {
        ApplyCurrentAnimResult_(cellAnim);
    }
}

void NNS_G2dSetCellAnimationCurrentFrame(NNSG2dCellAnimation *cellAnim, u16 frameIdx) {
    if (NNS_G2dSetAnimCtrlCurrentFrame(&cellAnim->animCtrl, frameIdx)) {
        ApplyCurrentAnimResult_(cellAnim);
    }
}

void NNS_G2dRestartCellAnimation(NNSG2dCellAnimation *cellAnim) {
    NNS_G2dResetAnimCtrlState(&cellAnim->animCtrl);
    cellAnim->animCtrl.bActive = TRUE;
    ApplyCurrentAnimResult_(cellAnim);
}

void NNS_G2dSetCellAnimationSpeed(NNSG2dCellAnimation *cellAnim, fx32 speed) {
    cellAnim->animCtrl.speed = speed;
}
