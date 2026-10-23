#include "nnsys/g2d.h"

// NitroSystem's g2d_NSC_load.c: finds the screen data in a loaded NSCR file, which holds no offsets

BOOL NNS_G2dGetUnpackedScreenData(void *file, NNSG2dScreenData **screen) {
    NNSG2dBinaryBlockHeader *blk = NNS_G2dFindBinaryBlock(file, NNS_G2D_BINBLK_SIG_SCRDATA);

    if (blk != NULL) {
        *screen = (NNSG2dScreenData *)(blk + 1);
        return TRUE;
    }
    *screen = NULL;
    return FALSE;
}
