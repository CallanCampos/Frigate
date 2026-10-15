#include "nnsys/g2d.h"

// NitroSystem's g2d_MultiCellAnimation.c: multi-cell animations, whose frames each show a multi-cell, a set of nodes
// that each play a cell animation at a position, with the frame's scale, rotation and translation over them all.
// GetMaxCellAnimNum_, GetMaxNodeNum_, the inlines and the play mode names are guesses; NNS_G2dRestartMCAnimation and
// NNS_G2dTraverseMCNodes, which swan doesn't name, are the SDK's; the rest are swan's names

static void SetMCDataToMCInstanceImpl_(NNSG2dMultiCellInstance *instance, const NNSG2dMultiCellData *mcData,
                                       u16 totalVideoFrame);
static u16 GetMaxCellAnimNum_(const NNSG2dMultiCellDataBank *mcBank);
static u16 GetMaxNodeNum_(const NNSG2dMultiCellDataBank *mcBank);

// How a node's cell animation plays when its multi-cell is shown: from its start, or on from where it would be had it
// played since the multi-cell animation began
enum {
    NNS_G2D_MCANIM_PLAYMODE_RESET,
    NNS_G2D_MCANIM_PLAYMODE_CONTINUE,
};

// A node's cell animation in the bank's array, and its play mode
static inline u16 GetMCNodeCellAnimIdx_(const NNSG2dMultiCellHierarchyData *node) {
    return (node->nodeAttr & 0xff00) >> 8;
}

static inline u32 GetMCNodePlayMode_(const NNSG2dMultiCellHierarchyData *node) {
    return node->nodeAttr & 0xf;
}

// What a sequence's frames hold, one of NNS_G2D_ANIMELEM_*
static inline u32 GetAnimSequenceElementType_(const NNSG2dAnimSequence *seq) {
    return seq->animType & 0xff;
}

// The frames a sequence lasts
static inline u32 GetAnimSequenceTotalVideoFrame_(const NNSG2dAnimSequence *seq) {
    u32 total = 0;
    u32 i;

    for (i = 0; i < seq->numFrames; i++) {
        total += seq->pAnmFrameArray[i].frames;
    }
    return total;
}

// Starts a node's sequence on its cell animation, played on to the multi-cell animation's time if the node plays on
static inline void SetNodeCellAnimation_(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimBankData *anims,
                                         const NNSG2dMultiCellHierarchyData *node, u32 totalVideoFrame) {
    const NNSG2dAnimSequence *seq = NNS_G2dGetAnimSequenceByIdx(anims, node->animSequenceIdx);

    NNS_G2dSetCellAnimationSequence(cellAnim, seq);
    cellAnim->animCtrl.bActive = TRUE;
    if (GetMCNodePlayMode_(node) == NNS_G2D_MCANIM_PLAYMODE_CONTINUE) {
        u32 seqFrames = GetAnimSequenceTotalVideoFrame_(seq);

        if (NNSi_G2dIsAnimCtrlLoopAnim(&cellAnim->animCtrl)) {
            NNS_G2dTickCellAnimation(cellAnim, (totalVideoFrame % seqFrames) << FX32_SHIFT);
        } else {
            if (totalVideoFrame < seqFrames) {
                seqFrames = totalVideoFrame;
            }
            NNS_G2dTickCellAnimation(cellAnim, seqFrames << FX32_SHIFT);
        }
    }
}

static void SetMCDataToMCInstanceImpl_(NNSG2dMultiCellInstance *instance, const NNSG2dMultiCellData *mcData,
                                       u16 totalVideoFrame) {
    u16 i;

    instance->pCurrentMultiCell = mcData;
    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimArray;

        for (i = 0; i < mcData->numCellAnim; i++) {
            cellAnims[i].bInited = FALSE;
        }
        // Nodes that share a cell animation set it up once, by the first of them
        for (i = 0; i < mcData->numNodes; i++) {
            const NNSG2dMultiCellHierarchyData *node = &mcData->pHierDataArray[i];
            NNSG2dMCCellAnimation *cellAnim = &cellAnims[GetMCNodeCellAnimIdx_(node)];

            if (!cellAnim->bInited) {
                SetNodeCellAnimation_(&cellAnim->cellAnim, instance->pAnimDataBank, node, totalVideoFrame);
                cellAnim->bInited = TRUE;
            }
        }
    } else {
        NNSG2dNode *nodes = instance->pCellAnimArray;

        for (i = 0; i < mcData->numNodes; i++) {
            const NNSG2dMultiCellHierarchyData *node = &mcData->pHierDataArray[i];

            SetNodeCellAnimation_(nodes[i].pContent, instance->pAnimDataBank, node, totalVideoFrame);
            nodes[i].bVisible = TRUE;
            NNSi_G2dSrtcSetTrans(&nodes[i].srtCtrl, node->pos.x, node->pos.y);
        }
    }
}

// The most cell animations, and the most nodes, of the bank's multi-cells
static u16 GetMaxCellAnimNum_(const NNSG2dMultiCellDataBank *mcBank) {
    u16 max = 0;
    u16 i;

    for (i = 0; i < mcBank->numMultiCellData; i++) {
        const NNSG2dMultiCellData *mcData = NNS_G2dGetMultiCellDataByIdx(mcBank, i);

        if (mcData->numCellAnim > max) {
            max = mcData->numCellAnim;
        }
    }
    return max;
}

// Shows the multi-cell of the current frame, with the frame's scale, rotation and translation
static inline void SetCurrentFrameMultiCell_(NNSG2dMultiCellAnimation *mcAnim) {
    if (mcAnim->animCtrl.pActiveCurrent->frames != 0) {
        const NNSG2dAnimDataSRT *elem = NNS_G2dGetAnimCtrlCurrentElement(&mcAnim->animCtrl);
        const NNSG2dMultiCellData *mcData = NNS_G2dGetMultiCellDataByIdx(mcAnim->pMultiCellDataBank, elem->index);
        u32 type = GetAnimSequenceElementType_(mcAnim->animCtrl.pAnimSequence);

        NNSi_G2dSrtcInitControl(&mcAnim->srtCtrl, NNS_G2D_SRTCONTROLTYPE_SRT);
        if (type != NNS_G2D_ANIMELEM_INDEX) {
            if (type == NNS_G2D_ANIMELEM_INDEX_T) {
                const NNSG2dAnimDataT *t = (const NNSG2dAnimDataT *)elem;

                NNSi_G2dSrtcSetTrans(&mcAnim->srtCtrl, t->px, t->py);
            } else {
                NNSi_G2dSrtcSetSRTScale(&mcAnim->srtCtrl, elem->sx, elem->sy);
                NNSi_G2dSrtcSetSRTRotZ(&mcAnim->srtCtrl, elem->rotZ);
                NNSi_G2dSrtcSetTrans(&mcAnim->srtCtrl, elem->px, elem->py);
            }
        }
        SetMCDataToMCInstanceImpl_(&mcAnim->multiCellInstance, mcData, mcAnim->totalVideoFrame);
    }
}

void NNS_G2dSetAnimSequenceToMCAnimation(NNSG2dMultiCellAnimation *mcAnim, const NNSG2dAnimSequence *seq) {
    NNS_G2dBindAnimCtrl(&mcAnim->animCtrl, seq);
    mcAnim->totalVideoFrame = 0;
    SetCurrentFrameMultiCell_(mcAnim);
}

static inline void InitMCAnimation_(NNSG2dMultiCellAnimation *mcAnim, const NNSG2dMultiCellDataBank *mcBank) {
    NNS_G2dInitAnimCtrl(&mcAnim->animCtrl);
    mcAnim->pMultiCellDataBank = mcBank;
    NNSi_G2dSrtcInitControl(&mcAnim->srtCtrl, NNS_G2D_SRTCONTROLTYPE_SRT);
    mcAnim->totalVideoFrame = 0;
}

// Sets up a multi-cell animation whose nodes each have a cell animation, from the nodes and their cell animations. It
// sets up the multi-cell animation too, which NNS_G2dInitMCAnimationInstance then does again
static inline void InitMCAnimationNotShareCellAnim_(NNSG2dMultiCellAnimation *mcAnim, NNSG2dNode *nodes,
                                                    NNSG2dCellAnimation *cellAnims, u16 num,
                                                    const NNSG2dAnimBankData *anims, const NNSG2dCellDataBank *cells,
                                                    const NNSG2dMultiCellDataBank *mcBank) {
    u16 i;

    mcAnim->multiCellInstance.mcType = NNS_G2D_MCTYPE_DONOT_SHARE_CELLANIM;
    mcAnim->multiCellInstance.pCurrentMultiCell = NULL;
    mcAnim->multiCellInstance.pAnimDataBank = anims;
    mcAnim->multiCellInstance.pCellAnimArray = nodes;
    for (i = 0; i < num; i++) {
        NNSi_G2dInitializeNode(&nodes[i], NNS_G2D_NODETYPE_CELL);
        nodes[i].pContent = &cellAnims[i];
        NNS_G2dInitCellAnimation(&cellAnims[i], NNS_G2dGetAnimSequenceByIdx(anims, 0), cells);
    }
    InitMCAnimation_(mcAnim, mcBank);
}

void NNS_G2dInitMCAnimationInstance(NNSG2dMultiCellAnimation *mcAnim, void *work, const NNSG2dAnimBankData *anims,
                                    const NNSG2dCellDataBank *cells, const NNSG2dMultiCellDataBank *mcBank,
                                    u32 mcType) {
    NNSG2dMultiCellInstance *instance;
    NNSG2dCellAnimation *cellAnims;
    NNSG2dMCCellAnimation *mcCellAnims;
    u16 numCellAnims;
    u16 numNodes;
    u16 i;

    if (mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        // Cell animations for the most a multi-cell of the bank uses, shared by their index
        mcAnim->multiCellInstance.mcType = NNS_G2D_MCTYPE_SHARE_CELLANIM;
        instance = &mcAnim->multiCellInstance;
        instance->pAnimDataBank = anims;
        instance->pCellAnimArray = work;
        numCellAnims = GetMaxCellAnimNum_(mcBank);
        mcCellAnims = instance->pCellAnimArray;
        for (i = 0; i < numCellAnims; i++) {
            NNSG2dMCCellAnimation *cellAnim = &mcCellAnims[i];

            NNS_G2dInitCellAnimation(&cellAnim->cellAnim, NNS_G2dGetAnimSequenceByIdx(anims, 0), cells);
            cellAnim->bInited = TRUE;
        }
    } else {
        // A cell animation for each node: the work holds the most nodes of a multi-cell of the bank, and then their
        // cell animations
        numNodes = GetMaxNodeNum_(mcBank);
        cellAnims = (NNSG2dCellAnimation *)((NNSG2dNode *)work + numNodes);
        InitMCAnimationNotShareCellAnim_(mcAnim, work, cellAnims, numNodes, anims, cells, mcBank);
    }
    InitMCAnimation_(mcAnim, mcBank);
}

u32 NNS_G2dGetMCWorkAreaSize(const NNSG2dMultiCellDataBank *mcBank, u32 mcType) {
    if (mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        return GetMaxCellAnimNum_(mcBank) * sizeof(NNSG2dMCCellAnimation);
    } else {
        return GetMaxNodeNum_(mcBank) * (sizeof(NNSG2dNode) + sizeof(NNSG2dCellAnimation));
    }
}

static u16 GetMaxNodeNum_(const NNSG2dMultiCellDataBank *mcBank) {
    u16 max = 0;
    u16 i;

    for (i = 0; i < mcBank->numMultiCellData; i++) {
        const NNSG2dMultiCellData *mcData = NNS_G2dGetMultiCellDataByIdx(mcBank, i);

        if (mcData->numNodes > max) {
            max = mcData->numNodes;
        }
    }
    return max;
}

void NNS_G2dTickMCInstance(NNSG2dMultiCellInstance *instance, fx32 frames) {
    u16 i;

    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimArray;

        for (i = 0; i < instance->pCurrentMultiCell->numCellAnim; i++) {
            NNS_G2dTickCellAnimation(&cellAnims[i].cellAnim, frames);
        }
    } else {
        NNSG2dNode *nodes = instance->pCellAnimArray;

        for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
            NNS_G2dTickCellAnimation(nodes[i].pContent, frames);
        }
    }
}

// Plays the cell animations on, or shows the next multi-cell when the frame changes
void NNS_G2dTickMCAnimation(NNSG2dMultiCellAnimation *mcAnim, fx32 frames) {
    u16 lastFrames = mcAnim->animCtrl.pCurrent->frames;

    if (NNS_G2dTickAnimCtrl(&mcAnim->animCtrl, frames)) {
        mcAnim->totalVideoFrame += lastFrames;
        SetCurrentFrameMultiCell_(mcAnim);
    } else {
        NNS_G2dTickMCInstance(&mcAnim->multiCellInstance, frames);
    }
}

void NNS_G2dSetMCAnimationCurrentFrame(NNSG2dMultiCellAnimation *mcAnim, u16 frameIdx) {
    if (NNS_G2dSetAnimCtrlCurrentFrame(&mcAnim->animCtrl, frameIdx)) {
        SetCurrentFrameMultiCell_(mcAnim);
    }
}

void NNS_G2dSetMCAnimationSpeed(NNSG2dMultiCellAnimation *mcAnim, fx32 speed) {
    NNSG2dMultiCellInstance *instance = &mcAnim->multiCellInstance;
    u16 i;

    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimArray;

        mcAnim->animCtrl.speed = speed;
        for (i = 0; i < instance->pCurrentMultiCell->numCellAnim; i++) {
            NNS_G2dSetCellAnimationSpeed(&cellAnims[i].cellAnim, speed);
        }
    } else {
        NNSG2dNode *nodes = mcAnim->multiCellInstance.pCellAnimArray;

        mcAnim->animCtrl.speed = speed;
        for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
            NNS_G2dSetCellAnimationSpeed(nodes[i].pContent, speed);
        }
    }
}

void NNS_G2dRestartMCCellAnimations(NNSG2dMultiCellInstance *instance) {
    u16 i;

    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimArray;

        for (i = 0; i < instance->pCurrentMultiCell->numCellAnim; i++) {
            NNS_G2dSetCellAnimationCurrentFrame(&cellAnims[i].cellAnim, 0);
        }
    } else {
        NNSG2dNode *nodes = instance->pCellAnimArray;

        for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
            NNS_G2dSetCellAnimationCurrentFrame(nodes[i].pContent, 0);
        }
    }
}

// Plays the animation again from its first frame
void NNS_G2dRestartMCAnimation(NNSG2dMultiCellAnimation *mcAnim) {
    NNSG2dMultiCellInstance *instance = &mcAnim->multiCellInstance;
    u16 i;

    mcAnim->totalVideoFrame = 0;
    NNS_G2dSetAnimSequenceToMCAnimation(mcAnim, mcAnim->animCtrl.pAnimSequence);
    mcAnim->animCtrl.bActive = TRUE;
    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimArray;

        for (i = 0; i < instance->pCurrentMultiCell->numCellAnim; i++) {
            NNS_G2dRestartCellAnimation(&cellAnims[i].cellAnim);
        }
    } else {
        NNSG2dNode *nodes = instance->pCellAnimArray;

        for (i = 0; i < instance->pCurrentMultiCell->numNodes; i++) {
            NNS_G2dRestartCellAnimation(nodes[i].pContent);
        }
    }
}

// Calls the callback for each node of the current multi-cell with its cell animation, until it returns FALSE
void NNS_G2dTraverseMCNodes(const NNSG2dMultiCellInstance *instance, NNSG2dMCTraverseNodeCallBack callback, u32 param) {
    u16 i;
    const NNSG2dMultiCellData *mcData = instance->pCurrentMultiCell;
    u16 numNodes = mcData->numNodes;

    if (instance->mcType == NNS_G2D_MCTYPE_SHARE_CELLANIM) {
        NNSG2dMCCellAnimation *cellAnims = instance->pCellAnimArray;

        for (i = 0; i < numNodes; i++) {
            const NNSG2dMultiCellHierarchyData *node = &mcData->pHierDataArray[i];

            if (!callback(param, node, &cellAnims[GetMCNodeCellAnimIdx_(node)].cellAnim, i)) {
                break;
            }
        }
    } else {
        NNSG2dNode *nodes = instance->pCellAnimArray;

        for (i = 0; i < numNodes; i++) {
            if (!callback(param, &mcData->pHierDataArray[i], nodes[i].pContent, i)) {
                break;
            }
        }
    }
}
