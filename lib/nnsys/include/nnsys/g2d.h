#ifndef POKEBW2_NNSYS_G2D_H
#define POKEBW2_NNSYS_G2D_H

#include "types.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

typedef struct {
    u32 format;
    u32 extendedPalette;
    u32 size;
    void *rawData;
} NNSG2dPaletteData;

typedef struct {
    u16 height;
    u16 width;
    u32 pixelFormat;
    u32 mappingType;
    u32 characterFormat;
    u32 size;
    void *rawData;
} NNSG2dCharacterData;

typedef struct {
    u16 width;
    u16 height;
    u16 colorMode;
    u16 format;
    u32 size;
    u32 rawData[1];
} NNSG2dScreenData;

// The palettes present in a palette file that holds only some of them
typedef struct {
    u16 numPalette;
    u16 pad;
    void *paletteIndexTable;
} NNSG2dPaletteCompressInfo;

// A block of a file's user extended data, found by its kind
typedef struct {
    u32 blkTypeID;
    u32 blkSize;
} NNSG2dUserExDataBlock;

// A G2D file: its header, then numBlocks blocks, each starting with its kind and its size with this header. These are
// the SDK's own types, though G3D's resource files have the same layout
typedef struct {
    u32 signature;
    u16 byteOrder;
    u16 version;
    u32 fileSize;
    u16 headerSize;
    u16 numBlocks;
} NNSG2dBinaryFileHeader;

typedef struct {
    u32 kind;
    u32 size;
} NNSG2dBinaryBlockHeader;

#define NNS_G2D_BINBLK_SIG_CHARDATA 0x43484152            // 'CHAR'
#define NNS_G2D_BINBLK_SIG_PALETTEDATA 0x504C5454         // 'PLTT'
#define NNS_G2D_BINBLK_SIG_PALETTECOMPRESSINFO 0x50434D50 // 'PCMP'
#define NNS_G2D_BINBLK_SIG_SCREENDATA 0x5343524E          // 'SCRN'
#define NNS_G2D_BINBLK_SIG_MULTICELLBANK 0x4D43424B       // 'MCBK'
#define NNS_G2D_BINBLK_SIG_CELLBANK 0x4345424B            // 'CEBK'
#define NNS_G2D_BINBLK_SIG_ANIMBANK 0x41424E4B            // 'ABNK'

#define NNS_G2D_USEREXDATA_CELLATTR 0x55434154
#define NNS_G2D_USEREXDATA_ANMATTR 0x55414154

static inline const NNSG2dUserExDataBlock *NNSi_G2dGetUserExDataBlkByID(const NNSG2dUserExDataBlock *blk, u32 id) {
    if (blk != NULL && blk->blkTypeID == id) {
        return blk;
    }
    return NULL;
}

// A cell, the OAMs that are drawn together, and the box or circle that holds them
typedef struct {
    s16 x;
    s16 y;
} NNSG2dSVec2;

typedef struct {
    fx32 x;
    fx32 y;
} NNSG2dFVec2;

typedef struct {
    NNSG2dSVec2 maxBounding;
    NNSG2dSVec2 minBounding;
} NNSG2dCellBoundingRectS16;

#define NNSi_G2D_CELLATTR_BOUNDINGSPHERE_MASK 0x3f
#define NNSi_G2D_CELLATTR_BOUNDINGRECT_SHIFT 11

typedef struct {
    u16 numOAMAttrs;
    u16 cellAttr;
    void *pOamAttrArray;
} NNSG2dCellData;

// A cell of a bank whose cells have bounding rects
typedef struct {
    NNSG2dCellData cellData;
    NNSG2dCellBoundingRectS16 boundingRect;
} NNSG2dCellDataWithBR;

static inline u8 NNSi_G2dGetCellBoundingSphereR(const NNSG2dCellData *cell) {
    u8 r = cell->cellAttr & NNSi_G2D_CELLATTR_BOUNDINGSPHERE_MASK;

    return r << 2;
}

// Where each cell's characters are in the source data, for cell banks whose characters are transferred to VRAM
typedef struct {
    u32 srcDataOffset;
    u32 szByte;
} NNSG2dCellVramTransferData;

typedef struct {
    u32 szByteMax;
    NNSG2dCellVramTransferData *pCellTransferDataArray;
} NNSG2dVramTransferData;

// A cell bank's attributes: whether its cells are NNSG2dCellDataWithBR
#define NNS_G2D_CELLBK_ATTR_CELLWITHBR 0x1

typedef struct NNSG2dCellDataBank {
    u16 numCells;
    u16 cellBankAttr;
    NNSG2dCellData *pCellDataArrayHead;
    u32 mappingMode;
    NNSG2dVramTransferData *pVramTransferData;
    void *pStringBank;
    NNSG2dUserExDataBlock *pExtendedData;
} NNSG2dCellDataBank;

// Returns u32 and tests with an if: as a BOOL, or returning the comparison, MWCC folds the 0/1 result into the test
// that uses it, where g2d_CellAnimation.c's ApplyCurrentAnimResult_ builds it and tests it again
static inline u32 NNS_G2dCellDataBankHasVramTransferData(const NNSG2dCellDataBank *bank) {
    if (bank->pVramTransferData != NULL) {
        return TRUE;
    }
    return FALSE;
}

// A multi-cell: its nodes, each a cell animation at a position
typedef struct {
    u16 animSequenceIdx;
    NNSG2dSVec2 pos;
    // The index of the node's cell animation in bits 8 to 15
    u16 nodeAttr;
} NNSG2dMultiCellHierarchyData;

typedef struct {
    u16 numNodes;
    u16 numCellAnim;
    NNSG2dMultiCellHierarchyData *pHierDataArray;
} NNSG2dMultiCellData;

typedef struct NNSG2dMultiCellDataBank {
    u16 numMultiCellData;
    u16 pad;
    NNSG2dMultiCellData *pMultiCellDataArray;
    void *pHierarchyDataArray;
    void *pStringBank;
    NNSG2dUserExDataBlock *pExtendedData;
} NNSG2dMultiCellDataBank;

// The user extended attributes of cells, numAttribute words each
typedef struct {
    u32 *pAttr;
} NNSG2dUserExCellAttr;

typedef struct {
    u16 numCells;
    u16 numAttribute;
    NNSG2dUserExCellAttr *pCellAttrArray;
} NNSG2dUserExCellAttrBank;

static inline const NNSG2dUserExCellAttrBank *NNS_G2dGetUserExCellAttrBankFromCellBank(const NNSG2dCellDataBank *bank) {
    const NNSG2dUserExDataBlock *blk = NNSi_G2dGetUserExDataBlkByID(bank->pExtendedData, NNS_G2D_USEREXDATA_CELLATTR);

    if (blk != NULL) {
        return (const NNSG2dUserExCellAttrBank *)(blk + 1);
    }
    return NULL;
}

static inline const NNSG2dUserExCellAttrBank *
NNS_G2dGetUserExCellAttrBankFromMCBank(const NNSG2dMultiCellDataBank *bank) {
    const NNSG2dUserExDataBlock *blk = NNSi_G2dGetUserExDataBlkByID(bank->pExtendedData, NNS_G2D_USEREXDATA_CELLATTR);

    if (blk != NULL) {
        return (const NNSG2dUserExCellAttrBank *)(blk + 1);
    }
    return NULL;
}

// Animations: banks of sequences of frames. A frame's content is an index, an NNSG2dAnimDataSRT or an
// NNSG2dAnimDataT, by the sequence's animType (its low 8 bits, as the multi-cell animation reads it)
enum {
    NNS_G2D_ANIMELEM_INDEX,
    NNS_G2D_ANIMELEM_INDEX_SRT,
    NNS_G2D_ANIMELEM_INDEX_T,
};

typedef struct {
    u16 index;
    u16 rotZ;
    fx32 sx;
    fx32 sy;
    s16 px;
    s16 py;
} NNSG2dAnimDataSRT;

typedef struct {
    u16 index;
    u16 pad;
    s16 px;
    s16 py;
} NNSG2dAnimDataT;

typedef struct {
    void *pContent;
    u16 frames;
    u16 pad;
} NNSG2dAnimFrameData;

// How a sequence plays: once or looping, forward or forward and then back
enum {
    NNS_G2D_ANIMATIONPLAYMODE_INVALID,
    NNS_G2D_ANIMATIONPLAYMODE_FORWARD,
    NNS_G2D_ANIMATIONPLAYMODE_FORWARD_LOOP,
    NNS_G2D_ANIMATIONPLAYMODE_REVERSE,
    NNS_G2D_ANIMATIONPLAYMODE_REVERSE_LOOP,
};

typedef struct {
    u16 numFrames;
    u16 loopStartFrameIdx;
    u32 animType;
    u32 playMode;
    NNSG2dAnimFrameData *pAnmFrameArray;
} NNSG2dAnimSequence;

typedef struct NNSG2dAnimBankData {
    u16 numSequences;
    u16 numTotalFrames;
    NNSG2dAnimSequence *pSequenceArrayHead;
    NNSG2dAnimFrameData *pFrameArray;
    void *pAnimationData;
    void *pStringBank;
    NNSG2dUserExDataBlock *pExtendedData;
} NNSG2dAnimBankData;

// The user extended attributes of animation sequences and their frames, numAttribute words each
typedef struct {
    u32 *pAttr;
} NNSG2dUserExAnimFrameAttr;

typedef struct {
    u16 numFrames;
    u16 pad;
    u32 *pAttr;
    NNSG2dUserExAnimFrameAttr *pAnmFrmAttrArray;
} NNSG2dUserExAnimSequenceAttr;

typedef struct {
    u16 numSequences;
    u16 numAttribute;
    NNSG2dUserExAnimSequenceAttr *pAnmSeqAttrArray;
} NNSG2dUserExAnimAttrBank;

static inline const NNSG2dUserExAnimAttrBank *NNS_G2dGetUserExAnimAttrBank(const NNSG2dAnimBankData *bank) {
    const NNSG2dUserExDataBlock *blk = NNSi_G2dGetUserExDataBlkByID(bank->pExtendedData, NNS_G2D_USEREXDATA_ANMATTR);

    if (blk != NULL) {
        return (const NNSG2dUserExAnimAttrBank *)(blk + 1);
    }
    return NULL;
}

static inline const NNSG2dUserExAnimSequenceAttr *NNS_G2dGetUserExAnimSequenceAttr(const NNSG2dUserExAnimAttrBank *bank,
                                                                                   u16 idx) {
    if (idx < bank->numSequences) {
        return &bank->pAnmSeqAttrArray[idx];
    }
    return NULL;
}

// Plays an animation sequence, with a callback at the last frame, at every frame or at one frame
enum {
    NNS_G2D_ANMCALLBACKTYPE_NONE,
    NNS_G2D_ANMCALLBACKTYPE_LAST_FRM,
    NNS_G2D_ANMCALLBACKTYPE_SPEC_FRM,
    NNS_G2D_ANMCALLBACKTYPE_EVER_FRM,
};

typedef void (*NNSG2dAnmCallBackPtr)(u32 param, fx32 currentFrame);

typedef struct {
    u32 type;
    u32 param;
    NNSG2dAnmCallBackPtr pFunc;
    u16 frameIdx;
} NNSG2dCallBackFunctor;

typedef struct {
    const NNSG2dAnimFrameData *pCurrent;
    const NNSG2dAnimFrameData *pActiveCurrent;
    BOOL bReverse;
    BOOL bActive;
    fx32 currentTime;
    fx32 speed;
    u32 overriddenPlayMode;
    const NNSG2dAnimSequence *pAnimSequence;
    NNSG2dCallBackFunctor callbackFunctor;
} NNSG2dAnimController;

// The scale, rotation and translation of an animated cell, with flags for which of them are set. The flag and type
// names, and the union with a matrix, which NNSi_G2dSrtcSetInitialValue's 0x18-byte clear suggests, are guesses
enum {
    NNS_G2D_SRTCONTROLTYPE_INVALID,
    NNS_G2D_SRTCONTROLTYPE_SRT,
};

enum {
    NNS_G2D_SRTFLAG_IDENTITY = 0,
    NNS_G2D_SRTFLAG_SCALE = 1 << 1,
    NNS_G2D_SRTFLAG_ROTZ = 1 << 2,
    NNS_G2D_SRTFLAG_TRANS = 1 << 3,
};

typedef struct {
    NNSG2dFVec2 scale;
    NNSG2dSVec2 trans;
    u16 rotZ;
    u16 SRT_EnableFlag;
} NNSG2dSRT;

typedef struct {
    u32 type;
    union {
        NNSG2dSRT srtData;
        MtxFx32 affineMtx;
    } data;
} NNSG2dSRTControl;

void NNSi_G2dSrtcSetTrans(NNSG2dSRTControl *srtCtrl, s16 x, s16 y);
void NNSi_G2dSrtcSetSRTRotZ(NNSG2dSRTControl *srtCtrl, u16 rotZ);
void NNSi_G2dSrtcSetSRTScale(NNSG2dSRTControl *srtCtrl, fx32 sx, fx32 sy);
void NNSi_G2dSrtcInitControl(NNSG2dSRTControl *srtCtrl, u32 type);
void NNSi_G2dSrtcSetInitialValue(NNSG2dSRTControl *srtCtrl);

// A node of a multi-cell: its content, visible or not, at a position. The node type names are guesses
enum {
    NNS_G2D_NODETYPE_INVALID,
    NNS_G2D_NODETYPE_CELL,
};

typedef struct {
    void *pContent;
    u32 type;
    BOOL bVisible;
    NNSG2dSRTControl srtCtrl;
} NNSG2dNode;

void NNSi_G2dInitializeNode(NNSG2dNode *node, u32 type);

// A cell animation, and a multi-cell one, which animates cell animations together
#define NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE 0xffffffff

typedef struct {
    NNSG2dAnimController animCtrl;
    const NNSG2dCellData *pCurrentCell;
    const NNSG2dCellDataBank *pCellDataBank;
    u32 cellTransferStateHandle;
    NNSG2dSRTControl srtCtrl;
} NNSG2dCellAnimation;

enum {
    NNS_G2D_MCTYPE_DONOT_SHARE_CELLANIM,
    NNS_G2D_MCTYPE_SHARE_CELLANIM,
};

// A multi-cell's cell animations: with NNS_G2D_MCTYPE_SHARE_CELLANIM, an array of NNSG2dMCCellAnimation that nodes
// share by their cell animation index; otherwise an NNSG2dNode per node, each with its own cell animation
typedef struct {
    const NNSG2dMultiCellData *pCurrentMultiCell;
    const NNSG2dAnimBankData *pAnimDataBank;
    u32 mcType;
    void *pCellAnimArray;
} NNSG2dMultiCellInstance;

// totalVideoFrame counts the frames of the multi-cells shown before the current one, for the nodes that play on
typedef struct {
    NNSG2dAnimController animCtrl;
    u16 totalVideoFrame;
    u16 pad;
    NNSG2dMultiCellInstance multiCellInstance;
    const NNSG2dMultiCellDataBank *pMultiCellDataBank;
    NNSG2dSRTControl srtCtrl;
} NNSG2dMultiCellAnimation;

// A cell animation of a multi-cell instance's array
typedef struct {
    NNSG2dCellAnimation cellAnim;
    // Whether the current multi-cell has set it up yet
    BOOL bInited;
} NNSG2dMCCellAnimation;

// Find the block of their kind in a loaded graphics file and turn its offsets into pointers in place, returning FALSE
// if the file has none. swan calls the first four NNS_G2DPrepareBGChar, NNS_G2DPrepareObjChar, NNS_G2DPrepareScreen
// and RelocatePaletteResGetDataPtr
BOOL NNS_G2dGetUnpackedBGCharacterData(void *file, NNSG2dCharacterData **character);
BOOL NNS_G2dGetUnpackedCharacterData(void *file, NNSG2dCharacterData **character);
BOOL NNS_G2dGetUnpackedScreenData(void *file, NNSG2dScreenData **screen);
BOOL NNS_G2dGetUnpackedPaletteData(void *file, NNSG2dPaletteData **palette);
BOOL NNS_G2dGetUnpackedPaletteCompressInfo(void *file, NNSG2dPaletteCompressInfo **info);
BOOL NNS_G2dGetUnpackedCellBank(void *file, NNSG2dCellDataBank **cells);
BOOL NNS_G2dGetUnpackedAnimBank(void *file, NNSG2dAnimBankData **anims);
BOOL NNS_G2dGetUnpackedMultiCellBank(void *file, NNSG2dMultiCellDataBank **cells);
BOOL NNS_G2dGetUnpackedMCAnimBank(void *file, NNSG2dAnimBankData **anims);

// The block of the given kind in a loaded G2D file, or NULL. swan calls it NNS_G2DFindDataBlock
NNSG2dBinaryBlockHeader *NNS_G2dFindBinaryBlock(NNSG2dBinaryFileHeader *file, u32 kind);

// Turn the offsets in a character, BG character or palette block into pointers, as the functions above do. swan calls
// them NNS_G2DRelocateObjChar, NNS_G2DRelocateBGChar and NNS_G2DRelocatePLTTHeader, and doesn't name
// NNSi_G2dUnpackNCLCmpInfo
void NNS_G2dUnpackNCG(NNSG2dCharacterData *character);
void NNS_G2dUnpackBGNCG(NNSG2dCharacterData *character);
void NNS_G2dUnpackNCL(NNSG2dPaletteData *palette);
void NNSi_G2dUnpackNCLCmpInfo(NNSG2dPaletteCompressInfo *info);

// Turn a cell or multi-cell bank's offsets into pointers, as NNS_G2dGetUnpackedCellBank and
// NNS_G2dGetUnpackedMultiCellBank do after finding it, and those of either's user extended cell attributes, which swan
// doesn't name
void NNS_G2dUnpackNCE(NNSG2dCellDataBank *bank);
void NNS_G2dUnpackNMC(NNSG2dMultiCellDataBank *bank);
void NNSi_G2dUnpackUserExCellAttrBank(NNSG2dUserExCellAttrBank *attrBank);
const NNSG2dCellData *NNS_G2dGetCellDataByIdx(const NNSG2dCellDataBank *bank, u16 idx);
const NNSG2dMultiCellData *NNS_G2dGetMultiCellDataByIdx(const NNSG2dMultiCellDataBank *bank, u16 idx);

// Turns an animation bank's offsets into pointers, as NNS_G2dGetUnpackedAnimBank does after finding it
void NNS_G2dUnpackNAN(NNSG2dAnimBankData *bank);

// NNS_G2dGetAnimSequenceByIdx, and controlling an animation
const NNSG2dAnimSequence *NNS_G2dGetAnimSequenceByIdx(const NNSG2dAnimBankData *bank, u16 idx);
void *NNS_G2dGetAnimCtrlCurrentElement(const NNSG2dAnimController *animCtrl);
BOOL NNS_G2dTickAnimCtrl(NNSG2dAnimController *animCtrl, fx32 frames);
BOOL NNS_G2dSetAnimCtrlCurrentFrame(NNSG2dAnimController *animCtrl, u16 frameIdx);
u16 NNS_G2dGetAnimCtrlCurrentFrame(const NNSG2dAnimController *animCtrl);
void NNS_G2dInitAnimCtrl(NNSG2dAnimController *animCtrl);
void NNS_G2dInitAnimCallBackFunctor(NNSG2dCallBackFunctor *functor);
void NNS_G2dResetAnimCtrlState(NNSG2dAnimController *animCtrl);
void NNS_G2dBindAnimCtrl(NNSG2dAnimController *animCtrl, const NNSG2dAnimSequence *seq);
void NNS_G2dSetAnimCtrlCallBackFunctor(NNSG2dAnimController *animCtrl, u32 type, u32 param, NNSG2dAnmCallBackPtr pFunc);
void NNS_G2dSetAnimCtrlCallBackFunctorAtAnimFrame(NNSG2dAnimController *animCtrl, u32 param, NNSG2dAnmCallBackPtr pFunc,
                                                  u16 frameIdx);
BOOL NNSi_G2dIsAnimCtrlLoopAnim(const NNSG2dAnimController *animCtrl);

// Cell animations: NNS_G2dInitCellAnimationVramTransfered transfers each cell's characters to VRAM as it is shown
void NNS_G2dInitCellAnimation(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimSequence *seq,
                              const NNSG2dCellDataBank *bank);
void NNS_G2dInitCellAnimationVramTransfered(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimSequence *seq,
                                            const NNSG2dCellDataBank *bank, u32 vramStateHandle, u32 dstAddr3D,
                                            u32 dstAddr2DMain, u32 dstAddr2DSub, const void *pSrcNCGR,
                                            const void *pSrcNCBR, u32 szSrcData);
void NNS_G2dSetCellAnimationSequence(NNSG2dCellAnimation *cellAnim, const NNSG2dAnimSequence *seq);
void NNS_G2dTickCellAnimation(NNSG2dCellAnimation *cellAnim, fx32 frames);
void NNS_G2dSetCellAnimationCurrentFrame(NNSG2dCellAnimation *cellAnim, u16 frameIdx);
void NNS_G2dRestartCellAnimation(NNSG2dCellAnimation *cellAnim);
void NNS_G2dSetCellAnimationSpeed(NNSG2dCellAnimation *cellAnim, fx32 speed);

// Multi-cell animations, whose work is NNS_G2dGetMCWorkAreaSize bytes
u32 NNS_G2dGetMCWorkAreaSize(const NNSG2dMultiCellDataBank *bank, u32 mcType);
void NNS_G2dInitMCAnimationInstance(NNSG2dMultiCellAnimation *mcAnim, void *work, const NNSG2dAnimBankData *anims,
                                    const NNSG2dCellDataBank *cells, const NNSG2dMultiCellDataBank *mcBank, u32 mcType);
void NNS_G2dSetAnimSequenceToMCAnimation(NNSG2dMultiCellAnimation *mcAnim, const NNSG2dAnimSequence *seq);
void NNS_G2dTickMCInstance(NNSG2dMultiCellInstance *instance, fx32 frames);
void NNS_G2dTickMCAnimation(NNSG2dMultiCellAnimation *mcAnim, fx32 frames);
void NNS_G2dSetMCAnimationCurrentFrame(NNSG2dMultiCellAnimation *mcAnim, u16 frameIdx);
void NNS_G2dSetMCAnimationSpeed(NNSG2dMultiCellAnimation *mcAnim, fx32 speed);
// Restarts each cell animation of a multi-cell instance
void NNS_G2dRestartMCCellAnimations(NNSG2dMultiCellInstance *instance);
void NNS_G2dRestartMCAnimation(NNSG2dMultiCellAnimation *mcAnim);

// Called for each node of a multi-cell instance with its cell animation; returning FALSE stops the traversal
typedef BOOL (*NNSG2dMCTraverseNodeCallBack)(u32 param, const NNSG2dMultiCellHierarchyData *node,
                                             NNSG2dCellAnimation *cellAnim, u16 nodeIdx);

void NNS_G2dTraverseMCNodes(const NNSG2dMultiCellInstance *instance, NNSG2dMCTraverseNodeCallBack callback, u32 param);

// Where an image or palette is in VRAM, for the 3D engine and each 2D engine, and loading one there
typedef enum {
    NNS_G2D_VRAM_TYPE_3DMAIN,
    NNS_G2D_VRAM_TYPE_2DMAIN,
    NNS_G2D_VRAM_TYPE_2DSUB,
    NNS_G2D_VRAM_TYPE_MAX,
} NNSG2dVRamType;

#define NNS_G2D_VRAM_ADDR_NONE 0xffffffff

typedef struct {
    u32 baseAddrOfVram[NNS_G2D_VRAM_TYPE_MAX];
} NNSG2dVRamLocation;

// Sets every address of a location to NNS_G2D_VRAM_ADDR_NONE
void NNSi_G2dInitializeVRamLocation(NNSG2dVRamLocation *location);
void NNSi_G2dSetVramLocation(NNSG2dVRamLocation *location, NNSG2dVRamType type, u32 addr);
u32 NNSi_G2dGetVramLocation(const NNSG2dVRamLocation *location, NNSG2dVRamType type);

// The SDK's enums: the renderer core shifts the mapping type's fields as signed
typedef struct {
    GXTexSizeS sizeS;
    GXTexSizeT sizeT;
    GXTexFmt fmt;
    BOOL bExtendedPlt;
    GXTexPlttColor0 plttUse;
    GXOBJVRamModeChar mappingType;
} NNSG2dImageAttr;

typedef struct {
    NNSG2dVRamLocation vramLocation;
    NNSG2dImageAttr attr;
} NNSG2dImageProxy;

typedef struct {
    u32 fmt;
    BOOL bExtendedPlt;
    NNSG2dVRamLocation vramLocation;
} NNSG2dImagePaletteProxy;

void NNS_G2dInitImageProxy(NNSG2dImageProxy *proxy);
void NNS_G2dSetImageLocation(NNSG2dImageProxy *proxy, NNSG2dVRamType type, u32 addr);
u32 NNS_G2dGetImageLocation(const NNSG2dImageProxy *proxy, NNSG2dVRamType type);
BOOL NNS_G2dIsImageReadyToUse(const NNSG2dImageProxy *proxy, NNSG2dVRamType type);
void NNS_G2dInitImagePaletteProxy(NNSG2dImagePaletteProxy *proxy);
void NNS_G2dSetImagePaletteLocation(NNSG2dImagePaletteProxy *proxy, NNSG2dVRamType type, u32 addr);
u32 NNS_G2dGetImagePaletteLocation(const NNSG2dImagePaletteProxy *proxy, NNSG2dVRamType type);
BOOL NNS_G2dIsImagePaletteReadyToUse(const NNSG2dImagePaletteProxy *proxy, NNSG2dVRamType type);
void NNS_G2dLoadImage1DMapping(const NNSG2dCharacterData *data, u32 baseAddr, NNSG2dVRamType type,
                               NNSG2dImageProxy *proxy);
void NNS_G2dLoadImage2DMapping(const NNSG2dCharacterData *data, u32 baseAddr, NNSG2dVRamType type,
                               NNSG2dImageProxy *proxy);
void NNS_G2dLoadImageVramTransfer(const NNSG2dCharacterData *data, u32 baseAddr, NNSG2dVRamType type,
                                  NNSG2dImageProxy *proxy);
void NNS_G2dLoadPalette(const NNSG2dPaletteData *data, u32 addr, NNSG2dVRamType type, NNSG2dImagePaletteProxy *proxy);
void NNS_G2dLoadPaletteEx(const NNSG2dPaletteData *data, const NNSG2dPaletteCompressInfo *cmpInfo, u32 addr,
                          NNSG2dVRamType type, NNSG2dImagePaletteProxy *proxy);

// The OAM managers, each of a range of OAMs of an engine, which they fill each frame and send at the vertical blank
typedef enum {
    NNS_G2D_OAMTYPE_MAIN,
    NNS_G2D_OAMTYPE_SUB,
    NNS_G2D_OAMTYPE_SOFTWAREEMULATION,
    NNS_G2D_OAMTYPE_MAX,
} NNSG2dOamType;

// The affine index of an OAM that uses no affine parameters, and what NNS_G2dEntryOamManagerAffine returns when full
#define NNS_G2D_OAM_AFFINE_IDX_NONE 0xfffe

// A range of OAMs or affine parameters that a manager owns, and the next one free
typedef struct {
    u16 fromIdx;
    u16 toIdx;
    u16 currentIdx;
} NNSG2dOamManagedRegion;

typedef struct {
    NNSG2dOamType type;
    NNSG2dOamManagedRegion managedAttrRegion;
    NNSG2dOamManagedRegion managedAffineRegion;
    u16 managerID;
    BOOL bFastTransferMode;
    fx32 spriteZoffsetStep;
} NNSG2dOamManagerInstance;

void NNS_G2dInitOamManagerModule(void);
BOOL NNS_G2dGetNewOamManagerInstanceAsFastTransferMode(NNSG2dOamManagerInstance *man, u16 from, u16 num, NNSG2dOamType type);
BOOL NNS_G2dEntryOamManagerOamWithAffineIdx(NNSG2dOamManagerInstance *man, const GXOamAttr *oam, u16 affineIdx);
u16 NNS_G2dEntryOamManagerAffine(NNSG2dOamManagerInstance *man, const MtxFx22 *mtx);
// Sends the manager's OAMs and affine parameters to the hardware. swan calls it NNS_G2dApplyAndResetOamManagerBuffer
void NNS_G2dApplyOamManagerToHW(NNSG2dOamManagerInstance *man);
void NNS_G2dResetOamManagerBuffer(NNSG2dOamManagerInstance *man);

// The renderer, which draws cells to the surfaces of the screens
typedef struct {
    NNSG2dFVec2 posTopLeft;
    NNSG2dFVec2 sizeView;
} NNSG2dViewRect;

enum {
    NNS_G2D_SURFACETYPE_MAIN3D,
    NNS_G2D_SURFACETYPE_MAIN2D,
    NNS_G2D_SURFACETYPE_SUB2D,
    NNS_G2D_SURFACETYPE_MAX,
};

typedef BOOL (*NNSG2dOamRegisterFunction)(const GXOamAttr *oam, u16 affineIdx, BOOL doubleAffine);
typedef u16 (*NNSG2dAffineRegisterFunction)(const MtxFx22 *mtx);
typedef BOOL (*NNSG2dRndCellCullingFunction)(const NNSG2dCellData *cell, const MtxFx32 *mtx,
                                             const NNSG2dViewRect *view);

// The renderer core, which draws a cell's OAMs to one surface (g2d_RendererCore.c), with the current matrix: its
// translation and a matrix cache for 2D surfaces, or a 4x3 matrix for the 3D surface. The callbacks before a cell or an
// OAM may clear bDrawEnable to skip it
enum {
    NNS_G2D_RND_AFFINE_OVERWRITE_NONE,
    NNS_G2D_RND_AFFINE_OVERWRITE_NORMAL,
    NNS_G2D_RND_AFFINE_OVERWRITE_DOUBLE,
};

#define NNS_G2D_RENDERERFLIP_NONE 0
#define NNS_G2D_RENDERERFLIP_H 0x1
#define NNS_G2D_RENDERERFLIP_V 0x2

struct NNSG2dRenderSurface;
struct NNSG2dRndCore2DMtxCache;

typedef struct NNSG2dRndCoreInstance {
    struct NNSG2dRenderSurface *pCurrentTargetSurface;
    u32 affineOverwriteMode;
    const NNSG2dImageProxy *pImageProxy;
    const NNSG2dImagePaletteProxy *pPaletteProxy;
    u32 base2DCharOffset;
    u32 baseTexAddr3D;
    u32 basePltAddr3D;
    NNSG2dOamRegisterFunction pFuncOamRegister;
    NNSG2dAffineRegisterFunction pFuncOamAffineRegister;
    u32 flipFlag;
    const struct NNSG2dRndCore2DMtxCache *pCurrentMxtCacheFor2D;
    const MtxFx32 *pCurrentMtx;
    BOOL bDrawEnable;
    fx32 zFor3DSoftwareSprite;
    // The OAM being drawn, which the callbacks before an OAM may change
    GXOamAttr currentOam;
    MtxFx43 mtx3D;
} NNSG2dRndCoreInstance;

// A 2D affine matrix that the renderer shares among the cells drawn with it, with its affine parameter index per
// surface affineIndex[flip][type - NNS_G2D_SURFACETYPE_MAIN2D], by OAM flip (H in bit 0, V in bit 1) and 2D surface
// type, is NNS_G2D_OAM_AFFINE_IDX_NOT_CACHED until the core registers the flipped matrix for it
typedef struct NNSG2dRndCore2DMtxCache {
    MtxFx22 m22;
    u16 affineIndex[4][NNS_G2D_SURFACETYPE_MAX - NNS_G2D_SURFACETYPE_MAIN2D];
} NNSG2dRndCore2DMtxCache;

#define NNS_G2D_OAM_AFFINE_IDX_NOT_CACHED 0xffff

typedef void (*NNSG2dRndCoreDrawCellCallBack)(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell);
typedef void (*NNSG2dRndCoreDrawOamCallBack)(NNSG2dRndCoreInstance *core, const NNSG2dCellData *cell, u16 oamIdx);

struct NNSG2dRendererInstance;
struct NNSG2dRenderSurface;
typedef void (*NNSG2dRndDrawCellCallBack)(struct NNSG2dRendererInstance *rend, struct NNSG2dRenderSurface *surface,
                                          const NNSG2dCellData *cell, const MtxFx32 *mtx);
typedef void (*NNSG2dRndDrawOamCallBack)(struct NNSG2dRendererInstance *rend, struct NNSG2dRenderSurface *surface,
                                         const NNSG2dCellData *cell, u16 oamIdx, const MtxFx32 *mtx);

// The first six fields are the core's surface (NNSG2dRndCoreSurface in the SDK, flattened here); the renderer sets
// the core's callbacks to its own, which call the surface's
typedef struct NNSG2dRenderSurface {
    NNSG2dViewRect viewRect;
    BOOL bActive;
    u32 type;
    NNSG2dRndCoreDrawCellCallBack pBeforeDrawCellBackFuncCore;
    NNSG2dRndCoreDrawCellCallBack pAfterDrawCellBackFuncCore;
    NNSG2dRndCoreDrawOamCallBack pBeforeDrawOamBackFuncCore;
    NNSG2dRndCoreDrawOamCallBack pAfterDrawOamBackFuncCore;
    NNSG2dOamRegisterFunction pFuncOamRegister;
    NNSG2dAffineRegisterFunction pFuncOamAffineRegister;
    struct NNSG2dRenderSurface *pNextSurface;
    NNSG2dRndCellCullingFunction pFuncVisibilityCulling;
    NNSG2dRndDrawCellCallBack pBeforeDrawCellBackFunc;
    NNSG2dRndDrawCellCallBack pAfterDrawCellBackFunc;
    NNSG2dRndDrawOamCallBack pBeforeDrawOamBackFunc;
    NNSG2dRndDrawOamCallBack pAfterDrawOamBackFunc;
} NNSG2dRenderSurface;

// The palette swap table: the palette each of an OAM's 16 palettes is drawn with
typedef struct {
    u16 paletteIndex[16];
} NNSG2dPaletteSwapTable;

u16 NNS_G2dGetPaletteTableValue(const NNSG2dPaletteSwapTable *tbl, u16 beforeIdx);

// Hints to NNS_G2dBeginRenderingEx: the cells drawn are only translated, or are drawn to the first surface only
enum {
    NNS_G2D_RDR_OPZHINT_NONE = 0,
    NNS_G2D_RDR_OPZHINT_NOT_SR = 1 << 0,
    NNS_G2D_RDR_OPZHINT_LOCK_PARAMS = 1 << 1,
};

// Which of an OAM's attributes the renderer overwrites
enum {
    NNS_G2D_RND_OVERWRITE_NONE = 0,
    NNS_G2D_RND_OVERWRITE_PRIORITY = 1 << 0,
    NNS_G2D_RND_OVERWRITE_PLTTNO = 1 << 1,
    NNS_G2D_RND_OVERWRITE_MOSAIC = 1 << 2,
    NNS_G2D_RND_OVERWRITE_OBJMODE = 1 << 3,
    NNS_G2D_RND_OVERWRITE_PLTTNO_OFFS = 1 << 4,
};

typedef struct NNSG2dRendererInstance {
    NNSG2dRndCoreInstance rendererCore;
    NNSG2dRenderSurface *pTargetSurfaceList;
    NNSG2dRenderSurface *pCurrentSurface;
    const NNSG2dPaletteSwapTable *pPaletteSwapTbl;
    u32 opzHint;
    fx32 spriteZoffsetStep;
    u32 overwriteEnableFlag;
    u16 overwritePriority;
    u16 overwritePlttNo;
    u32 overwriteObjMode;
    BOOL overwriteMosaicFlag;
    u16 overwritePlttNoOffset;
} NNSG2dRendererInstance;

void NNS_G2dInitRenderer(NNSG2dRendererInstance *rend);
void NNS_G2dAddRendererTargetSurface(NNSG2dRendererInstance *rend, NNSG2dRenderSurface *surface);
void NNS_G2dInitRenderSurface(NNSG2dRenderSurface *surface);

// The width and height in pixels of each OAM shape and size, by [shape][size]. The names are guesses: the tables are
// defined by a file before g2d_CellTransferManager.c
extern const u16 NNSi_objSizeWTbl[3][4];
extern const u16 NNSi_objSizeHTbl[3][4];

static inline u16 NNS_G2dGetOamSizeX(const GXOamShape *shape) {
    return NNSi_objSizeWTbl[(*shape & GX_OAM_ATTR0_SHAPE_MASK) >> GX_OAM_ATTR01_SHAPE_SHIFT]
                           [(*shape & GX_OAM_ATTR01_SIZE_MASK) >> GX_OAM_ATTR01_SIZE_SHIFT];
}

static inline u16 NNS_G2dGetOamSizeY(const GXOamShape *shape) {
    return NNSi_objSizeHTbl[(*shape & GX_OAM_ATTR0_SHAPE_MASK) >> GX_OAM_ATTR01_SHAPE_SHIFT]
                           [(*shape & GX_OAM_ATTR01_SIZE_MASK) >> GX_OAM_ATTR01_SIZE_SHIFT];
}

// A cell's OAM, its first three attributes, and copying one to an OAM
typedef struct {
    u16 attr0;
    u16 attr1;
    u16 attr2;
} NNSG2dCellOAMAttrData;

static inline void NNS_G2dCopyCellAsOamAttr(const NNSG2dCellData *cell, u16 idx, GXOamAttr *dst) {
    const NNSG2dCellOAMAttrData *src = (const NNSG2dCellOAMAttrData *)cell->pOamAttrArray + idx;

    dst->attr0 = src->attr0;
    dst->attr1 = src->attr1;
    dst->attr2 = src->attr2;
}

// Drawing an OAM as a textured quad with the 3D engine (g2d_OamSoftwareSpriteDraw.c). The correction function may
// change the texture coordinates of a flipped OAM; the Z offset, when on, is added to each sprite's Z and moved by the
// step after each one
typedef void (*NNSG2dOamSoftEmuUVFlipCorrectFunc)(fx32 *u0, fx32 *v0, fx32 *u1, fx32 *v1, BOOL flipH, BOOL flipV);

void NNS_G2dDrawOneOam3DDirectWithPosFast(s16 posX, s16 posY, s16 posZ, const GXOamAttr *oam,
                                          const NNSG2dImageAttr *texImageAttr, u32 texBaseAddr, u32 pltBaseAddr);
void NNS_G2dBeginRendering(NNSG2dRendererInstance *rend);
void NNS_G2dBeginRenderingEx(NNSG2dRendererInstance *rend, u32 opzHint);
void NNS_G2dEndRendering(void);
void NNS_G2dDrawCell(const NNSG2dCellData *cell);
void NNS_G2dDrawCellAnimation(const NNSG2dCellAnimation *cellAnim);
void NNS_G2dDrawMultiCellAnimation(const NNSG2dMultiCellAnimation *mcAnim);
// The matrix stack that cells are drawn with. NNS_G2dPopMtx ignores its count and pops one
void NNS_G2dPushMtx(void);
void NNS_G2dPopMtx(u16 num);
// swan calls these three CellMatrixTranslate, CellMatrixScale and CellMatrixRotate
void NNS_G2dTranslate(fx32 x, fx32 y, fx32 z);
void NNS_G2dScale(fx32 x, fx32 y, fx32 z);
void NNS_G2dRotZ(fx32 sin, fx32 cos);
const NNSG2dPaletteSwapTable *NNS_G2dGetRendererPaletteTbl(const NNSG2dRendererInstance *rend);
void NNS_G2dSetRendererImageProxy(NNSG2dRendererInstance *rend, const NNSG2dImageProxy *imgProxy,
                                  const NNSG2dImagePaletteProxy *pltProxy);

// While the renderer draws a multi-cell animation that shares its cell animations: the cell animation being drawn,
// and the matrix cache each one was drawn with. The renderer addresses it by its own symbol, as it does only what
// another file defines, and it lies in the .bss after the renderer's (0x021435d8 in Black 2), so the file after
// g2d_Renderer.c defines it. The names are guesses
#define NNSi_G2D_MC_CELLANIM_MAX 256

typedef struct {
    u16 currentCellAnimIdx;
    NNSG2dRndCore2DMtxCache *cellAnimMtxCache[NNSi_G2D_MC_CELLANIM_MAX];
    BOOL bDrawMC;
} NNSiG2dMCRenderState;

extern NNSiG2dMCRenderState NNSi_G2dMCRenderState;

// The renderer core (g2d_RendererCore.c), as the renderer uses it
void NNS_G2dInitRndCore(NNSG2dRndCoreInstance *core);
void NNS_G2dSetRndCoreImageProxy(NNSG2dRndCoreInstance *core, const NNSG2dImageProxy *imgProxy,
                                 const NNSG2dImagePaletteProxy *pltProxy);
void NNS_G2dSetRndCoreOamRegisterFunc(NNSG2dRndCoreInstance *core, NNSG2dOamRegisterFunction oamRegister,
                                      NNSG2dAffineRegisterFunction affineRegister);
void NNS_G2dSetRndCoreAffineOverwriteMode(NNSG2dRndCoreInstance *core, u32 mode);
void NNS_G2dSetRndCoreCurrentMtx3D(const MtxFx32 *mtx);
void NNS_G2dSetRndCoreCurrentMtx2D(const MtxFx32 *mtx, const NNSG2dRndCore2DMtxCache *cache);
void NNS_G2dSetRndCore3DSoftSpriteZvalue(NNSG2dRndCoreInstance *core, fx32 z);
void NNS_G2dSetRndCoreSurface(NNSG2dRndCoreInstance *core, NNSG2dRenderSurface *surface);
BOOL NNS_G2dIsRndCoreFlipH(const NNSG2dRndCoreInstance *core);
BOOL NNS_G2dIsRndCoreFlipV(const NNSG2dRndCoreInstance *core);
void NNS_G2dSetRndCoreFlipMode(NNSG2dRndCoreInstance *core, BOOL flipH, BOOL flipV);
void NNS_G2dRndCoreBeginRendering(NNSG2dRndCoreInstance *core);
void NNS_G2dRndCoreEndRendering(void);
void NNS_G2dRndCoreDrawCell(const NNSG2dCellData *cell);
void NNS_G2dRndCoreDrawCellVramTransfer(const NNSG2dCellData *cell, u32 cellVramTransferHandle);

// The software sprites' automatic z offset (g2d_OamSoftwareSpriteDraw.c), which the renderer turns on around cells
void NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(BOOL flag);
void NNSi_G2dResetOamSoftEmuAutoZOffset(void);
void NNSi_G2dSetOamSoftEmuAutoZOffsetStep(fx32 step);
fx32 NNSi_G2dGetOamSoftEmuAutoZOffsetStep(void);

// ab = a * b for 2D affine matrices; ab may be b. swan's name: the code shows no SDK name
void MAT32_Mul(const MtxFx32 *a, const MtxFx32 *b, MtxFx32 *ab);

// The cell transfer state manager, which transfers the characters of cell animations made with
// NNS_G2dInitCellAnimationVramTransfered through a callback
typedef struct {
    NNSG2dVRamLocation dstVramLocation;
    u32 szDst;
    const void *pSrcNCGR;
    const void *pSrcNCBR;
    u32 szSrcData;
    BOOL bActive;
    u32 bDrawn;             // A bit per NNSG2dVRamType, set when the cell is drawn by that engine
    u32 bTransferRequested; // A bit per NNSG2dVRamType
    u32 srcOffset;
    u32 szByte;
} NNSG2dCellTransferState;

typedef BOOL (*NNSG2dDmaCallBack)(u32 type, u32 dstAddr, void *pSrc, u32 szByte);

void NNS_G2dInitCellTransferStateManager(NNSG2dCellTransferState *states, u32 numCellState, NNSG2dDmaCallBack callback);
void NNS_G2dUpdateCellTransferStateManager(void);
u32 NNS_G2dGetNewCellTransferStateHandle(void);
void NNS_G2dFreeCellTransferStateHandle(u32 handle);
NNSG2dCellTransferState *NNSi_G2dGetCellTransferState(u32 handle);
void NNSi_G2dInitCellTransferState(u32 handle, u32 dstAddr3D, u32 dstAddr2DMain, u32 dstAddr2DSub, u32 szDst,
                                   const void *pSrcNCGR, const void *pSrcNCBR, u32 szSrcData);
void NNS_G2dSetCellTransferStateRequested(u32 handle, u32 srcOffset, u32 szByte);

// Fonts: an NFTR file's font information (FINF), its glyph images (CGLP), their widths (CWDH) and the maps from
// character codes to glyph indices (CMAP). The layouts are the SDK's; flags, which the glyph block's version 1.0 lacks,
// says how the glyphs are rotated (the cases of NNS_G2dCharCanvasDrawChar)
#define NNS_G2D_BINFILE_SIG_FONTDATA 0x4E465452 // 'NFTR'
#define NNS_G2D_BINBLK_SIG_FINFDATA 0x46494E46  // 'FINF'
#define NNS_G2D_BINBLK_SIG_CGLPDATA 0x43474C50  // 'CGLP'
#define NNS_G2D_BINBLK_SIG_CWDHDATA 0x43574448  // 'CWDH'
#define NNS_G2D_BINBLK_SIG_CMAPDATA 0x434D4150  // 'CMAP'

#define NNS_G2D_GLYPH_INDEX_NOT_FOUND 0xffff

typedef struct {
    s8 left;
    u8 glyphWidth;
    s8 charWidth;
} NNSG2dCharWidths;

typedef struct {
    u8 cellWidth;
    u8 cellHeight;
    u16 cellSize;
    s8 baselinePos;
    u8 maxCharWidth;
    u8 bpp;
    u8 flags;
    u8 glyphTable[];
} NNSG2dFontGlyph;

typedef struct NNSG2dFontWidth {
    u16 indexBegin;
    u16 indexEnd;
    struct NNSG2dFontWidth *pNext;
    NNSG2dCharWidths widthTable[];
} NNSG2dFontWidth;

enum {
    NNS_G2D_MAPMETHOD_DIRECT, // mapInfo[0] is the glyph index of ccodeBegin
    NNS_G2D_MAPMETHOD_TABLE,  // mapInfo holds a glyph index per code
    NNS_G2D_MAPMETHOD_SCAN,   // mapInfo is an NNSG2dCMapInfoScan
};

typedef struct {
    u16 ccode;
    u16 index;
} NNSG2dCMapScanEntry;

typedef struct {
    u16 num;
    NNSG2dCMapScanEntry entries[];
} NNSG2dCMapInfoScan;

typedef struct NNSG2dFontCodeMap {
    u16 ccodeBegin;
    u16 ccodeEnd;
    u16 mappingMethod;
    u16 reserved;
    struct NNSG2dFontCodeMap *pNext;
    u16 mapInfo[];
} NNSG2dFontCodeMap;

typedef struct {
    u8 fontType;
    s8 linefeed;
    u16 alterCharIndex;
    NNSG2dCharWidths defaultWidth;
    u8 encoding;
    NNSG2dFontGlyph *pGlyph;
    NNSG2dFontWidth *pWidth;
    NNSG2dFontCodeMap *pMap;
} NNSG2dFontInformation;

// Reads the next character of a string and moves past it
typedef u16 (*NNSiG2dSplitCharCallback)(const void **ppChar);

typedef struct {
    NNSG2dFontInformation *pRes;
    NNSiG2dSplitCharCallback cbCharSpliter;
} NNSG2dFont;

typedef struct {
    const NNSG2dCharWidths *pWidths;
    const u8 *image;
} NNSG2dGlyph;

typedef struct {
    int width;
    int height;
} NNSG2dTextRect;

// Loads a font: finds the font information in a loaded NFTR file and turns the file's offsets into pointers
BOOL NNSi_G2dGetUnpackedFont(void *pNftrFile, NNSG2dFontInformation **ppFont);
void NNSi_G2dUnpackNFT(NNSG2dBinaryFileHeader *pHeader);

void NNS_G2dFontInitUTF16(NNSG2dFont *pFont, void *pNftrFile);
u16 NNSi_G2dSplitCharUTF16(const void **ppChar);
u16 NNS_G2dFontFindGlyphIndex(const NNSG2dFont *pFont, u16 c);
const NNSG2dCharWidths *NNS_G2dFontGetCharWidthsFromIndex(const NNSG2dFont *pFont, u16 idx);

// A character's widths, or those of the font's alternate character when the font lacks it
static inline const NNSG2dCharWidths *NNS_G2dFontGetCharWidths(const NNSG2dFont *pFont, u16 c) {
    u16 idx = NNS_G2dFontFindGlyphIndex(pFont, c);

    if (idx == NNS_G2D_GLYPH_INDEX_NOT_FOUND) {
        idx = pFont->pRes->alterCharIndex;
    }
    return NNS_G2dFontGetCharWidthsFromIndex(pFont, idx);
}

static inline int NNS_G2dFontGetLineFeed(const NNSG2dFont *pFont) {
    return pFont->pRes->linefeed;
}

// The width of the string up to its end or its first line feed, with hSpace between characters. *pPos is set to the
// next line, or NULL at the end of the text
int NNSi_G2dFontGetStringWidth(const NNSG2dFont *pFont, int hSpace, const void *str, const void **pPos);
int NNSi_G2dFontGetTextHeight(const NNSG2dFont *pFont, int vSpace, const void *txt);
NNSG2dTextRect NNSi_G2dFontGetTextRect(const NNSG2dFont *pFont, int hSpace, int vSpace, const void *txt);

// The rect's copy makes the two copies that NNSi_G2dTextCanvasDrawText's stack shows
static inline NNSG2dTextRect NNS_G2dFontGetTextRect(const NNSG2dFont *pFont, int hSpace, int vSpace, const void *txt) {
    NNSG2dTextRect rect = NNSi_G2dFontGetTextRect(pFont, hSpace, vSpace, txt);
    return rect;
}

// Character canvases: text drawn into characters in VRAM or memory, laid out as a BG's characters or a 1D OBJ's. param
// is the canvas's width in characters for a BG, and the log2 sizes of its largest OBJ for a 1D OBJ
typedef struct NNSG2dCharCanvas NNSG2dCharCanvas;

typedef void (*NNSiG2dDrawGlyphFunc)(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl,
                                     const NNSG2dGlyph *pGlyph);
typedef void (*NNSiG2dClearFunc)(const NNSG2dCharCanvas *pCC, int cl);
typedef void (*NNSiG2dClearAreaFunc)(const NNSG2dCharCanvas *pCC, int cl, int x, int y, int w, int h);

typedef struct {
    NNSiG2dDrawGlyphFunc pDrawGlyph;
    NNSiG2dClearFunc pClear;
    NNSiG2dClearAreaFunc pClearArea;
} NNSiG2dCharCanvasVTable;

struct NNSG2dCharCanvas {
    u8 *charBase;
    int areaWidth;
    int areaHeight;
    u8 dstBpp;
    u32 param;
    const NNSiG2dCharCanvasVTable *vtable;
};

typedef enum {
    NNS_G2D_CHARA_COLORMODE_16 = 4,
    NNS_G2D_CHARA_COLORMODE_256 = 8,
} NNSG2dCharaColorMode;

void NNS_G2dCharCanvasInitForBG(NNSG2dCharCanvas *pCC, void *charBase, int areaWidth, int areaHeight,
                                NNSG2dCharaColorMode colorMode);
void NNS_G2dCharCanvasInitForOBJ1D(NNSG2dCharCanvas *pCC, void *charBase, int areaWidth, int areaHeight,
                                   NNSG2dCharaColorMode colorMode);
// Draws a character with its top left at x, y in color cl and returns its width
int NNS_G2dCharCanvasDrawChar(const NNSG2dCharCanvas *pCC, const NNSG2dFont *pFont, int x, int y, int cl, u16 ccode);
void NNS_G2dMapScrToCharText(void *scnBase, int areaWidth, int areaHeight, int areaLeft, int areaTop, int scnWidth,
                             int charNo, int cplt);
int NNS_G2dCalcRequireOBJ1D(u32 areaWidth, u32 areaHeight);
int NNS_G2dArrangeOBJ1D(GXOamAttr *oam, int areaWidth, int areaHeight, int x, int y, int color, int charName,
                        int vramMode);

// Text canvases: a font drawing into a character canvas, with spacing between characters and lines. Text is drawn in
// the direction dir, (1, 0) for horizontal text, the line feed going 90 degrees clockwise from it. The flags place the
// text's box at x, y and align its lines
typedef struct {
    const NNSG2dCharCanvas *pCanvas;
    const NNSG2dFont *pFont;
    int hSpace;
    int vSpace;
} NNSG2dTextCanvas;

typedef struct {
    s8 x;
    s8 y;
} NNSG2dTextDirection;

#define NNS_G2D_VERTICALORIGIN_TOP 0x1
#define NNS_G2D_VERTICALORIGIN_MIDDLE 0x2
#define NNS_G2D_VERTICALORIGIN_BOTTOM 0x4
#define NNS_G2D_HORIZONTALORIGIN_LEFT 0x8
#define NNS_G2D_HORIZONTALORIGIN_CENTER 0x10
#define NNS_G2D_HORIZONTALORIGIN_RIGHT 0x20
#define NNS_G2D_VERTICALALIGN_TOP 0x40
#define NNS_G2D_VERTICALALIGN_MIDDLE 0x80
#define NNS_G2D_VERTICALALIGN_BOTTOM 0x100
#define NNS_G2D_HORIZONTALALIGN_LEFT 0x200
#define NNS_G2D_HORIZONTALALIGN_CENTER 0x400
#define NNS_G2D_HORIZONTALALIGN_RIGHT 0x800

void NNSi_G2dTextCanvasDrawString(const NNSG2dTextCanvas *pTxn, int x, int y, int cl, const void *str,
                                  const void **ppEnd, NNSG2dTextDirection dir);
void NNSi_G2dTextCanvasDrawText(const NNSG2dTextCanvas *pTxn, int x, int y, int cl, u32 flags, const void *txt,
                                NNSG2dTextDirection dir);
void NNSi_G2dTextCanvasDrawTextRect(const NNSG2dTextCanvas *pTxn, int x, int y, int w, int h, int cl, u32 flags,
                                    const void *txt, NNSG2dTextDirection dir);

#endif // POKEBW2_NNSYS_G2D_H
