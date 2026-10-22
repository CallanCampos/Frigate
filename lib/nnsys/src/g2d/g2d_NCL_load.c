#include "g2di_load.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_NCL_load.c: finds the palette data in a loaded NCLR file, and the list of the palettes it holds
// when it holds only some, and turns their offsets into pointers in place

BOOL NNS_G2dGetUnpackedPaletteData(void *file, NNSG2dPaletteData **palette) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_PALETTEDATA);

    if (blk != NULL) {
        NNS_G2dUnpackNCL((NNSG2dPaletteData *)(blk + 1));
        *palette = (NNSG2dPaletteData *)(blk + 1);
        return TRUE;
    }
    *palette = NULL;
    return FALSE;
}

BOOL NNS_G2dGetUnpackedPaletteCompressInfo(void *file, NNSG2dPaletteCompressInfo **info) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_PALETTECOMPINFO);

    if (blk != NULL) {
        NNSi_G2dUnpackNCLCmpInfo((NNSG2dPaletteCompressInfo *)(blk + 1));
        *info = (NNSG2dPaletteCompressInfo *)(blk + 1);
        return TRUE;
    }
    *info = NULL;
    return FALSE;
}

void NNSi_G2dUnpackNCLCmpInfo(NNSG2dPaletteCompressInfo *info) {
    NNS_G2D_UNPACK_OFFSET_PTR(info->pPlttIdxTbl, info);
}

void NNS_G2dUnpackNCL(NNSG2dPaletteData *palette) {
    NNS_G2D_UNPACK_OFFSET_PTR(palette->pRawData, palette);
}
