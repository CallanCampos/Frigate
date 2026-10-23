#include "nnsys/g2d.h"
#include "nnsys/gfd.h"

// NitroSystem's g2d_CellTransferManager.c: the cell transfer states, one per cell animation whose characters are
// transferred to VRAM as each cell is shown. A cell animation asks for its current cell's characters, a renderer marks
// the engines that drew it, and the update transfers them through the callback for each engine that both asked and
// drew. The names and the file name are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them; the ROM has
// no string for the file

static VramTransferTaskRegisterFuncPtr s_pTaskRegisterFunc;
static NNSG2dCellTransferState *s_pCellStateArray;
static u32 s_numCellState;

static inline void ResetCellTransferState_(NNSG2dCellTransferState *state) {
    NNSi_G2dInitializeVRamLocation(&state->dstVramLocation);
    state->szDst = 0;
    state->pSrcNCGR = NULL;
    state->pSrcNCBR = NULL;
    state->szSrcData = 0;
    state->bActive = FALSE;
    state->bDrawn = 0;
    state->bTransferRequested = 0;
    state->srcOffset = 0;
    state->szByte = 0;
}

NNSG2dCellTransferState *NNSi_G2dGetCellTransferState(u32 handle) {
    return &s_pCellStateArray[handle];
}

void NNSi_G2dInitCellTransferState(u32 handle, u32 dstAddr3D, u32 dstAddr2DMain, u32 dstAddr2DSub, u32 szDst,
                                   const void *pSrcNCGR, const void *pSrcNCBR, u32 szSrcData) {
    NNSG2dCellTransferState *state = &s_pCellStateArray[handle];

    NNSi_G2dInitializeVRamLocation(&state->dstVramLocation);
    if (dstAddr3D != NNS_G2D_VRAM_ADDR_NONE) {
        NNSi_G2dSetVramLocation(&state->dstVramLocation, NNS_G2D_VRAM_TYPE_3DMAIN, dstAddr3D);
    }
    if (dstAddr2DMain != NNS_G2D_VRAM_ADDR_NONE) {
        NNSi_G2dSetVramLocation(&state->dstVramLocation, NNS_G2D_VRAM_TYPE_2DMAIN, dstAddr2DMain);
    }
    if (dstAddr2DSub != NNS_G2D_VRAM_ADDR_NONE) {
        NNSi_G2dSetVramLocation(&state->dstVramLocation, NNS_G2D_VRAM_TYPE_2DSUB, dstAddr2DSub);
    }
    state->szDst = szDst;
    state->pSrcNCGR = pSrcNCGR;
    state->pSrcNCBR = pSrcNCBR;
    state->szSrcData = szSrcData;
}

void NNS_G2dInitCellTransferStateManager(NNSG2dCellTransferState *states, u32 numCellState,
                                         VramTransferTaskRegisterFuncPtr callback) {
    u32 i;

    s_pTaskRegisterFunc = callback;
    s_pCellStateArray = states;
    s_numCellState = numCellState;
    for (i = 0; i < numCellState; i++) {
        ResetCellTransferState_(&states[i]);
    }
}

// andand_bool
// Whether the cell asked for its characters and was drawn by the engine of type
static inline BOOL ShouldRegisterAsVramTransferTask_(const NNSG2dCellTransferState *state, NNS_G2D_VRAM_TYPE type) {
    return (state->bTransferRequested & (1 << type)) && (state->bDrawn & (1 << type));
}

// Transfers the characters of each active cell to each engine that asked for them and drew the cell. The 3D engine
// takes them from the NCBR (bitmap) data and the 2D engines from the NCGR (character) data
void NNS_G2dUpdateCellTransferStateManager(void) {
    static const NNS_GFD_DST_TYPE dstTypes[NNS_G2D_VRAM_TYPE_MAX] = {
        NNS_GFD_DST_3D_TEX_VRAM,
        NNS_GFD_DST_2D_OBJ_CHAR_MAIN,
        NNS_GFD_DST_2D_OBJ_CHAR_SUB,
    };
    u32 i;
    NNS_G2D_VRAM_TYPE type;
    NNSG2dCellTransferState *state;

    for (i = 0; i < s_numCellState; i++) {
        state = &s_pCellStateArray[i];
        if (state->bActive) {
            for (type = NNS_G2D_VRAM_TYPE_3DMAIN; type < NNS_G2D_VRAM_TYPE_MAX; type++) {
                if (ShouldRegisterAsVramTransferTask_(state, type)) {
                    NNS_GFD_DST_TYPE dstType = dstTypes[type];
                    const u8 *pSrc;

                    if (type == NNS_G2D_VRAM_TYPE_3DMAIN) {
                        pSrc = state->pSrcNCBR;
                    } else {
                        pSrc = state->pSrcNCGR;
                    }
                    if (s_pTaskRegisterFunc(dstType, NNSi_G2dGetVramLocation(&state->dstVramLocation, type),
                                            (void *)(pSrc + state->srcOffset), state->szByte)) {
                        state->bTransferRequested &= ~(1 << type);
                    }
                }
            }
            state->bDrawn = 0;
        }
    }
}

void NNS_G2dSetCellTransferStateRequested(u32 handle, u32 srcOffset, u32 szByte) {
    NNSG2dCellTransferState *state = NNSi_G2dGetCellTransferState(handle);

    state->bTransferRequested = 0xffffffff;
    state->srcOffset = srcOffset;
    state->szByte = szByte;
}

u32 NNS_G2dGetNewCellTransferStateHandle(void) {
    u32 i;

    for (i = 0; i < s_numCellState; i++) {
        if (s_pCellStateArray[i].bActive != TRUE) {
            s_pCellStateArray[i].bActive = TRUE;
            return i;
        }
    }
    return NNS_G2D_INVALID_CELL_TRANSFER_STATE_HANDLE;
}

void NNS_G2dFreeCellTransferStateHandle(u32 handle) {
    ResetCellTransferState_(&s_pCellStateArray[handle]);
}
