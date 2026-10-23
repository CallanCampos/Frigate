#include "nnsys/g2d.h"

// NitroSystem's palette swap table, which the renderer draws each OAM's palette through. The file name is
// NitroSystem's, as ntrtwl's reconstruction of NitroSystem has it: the ROM has no string for it, and the renderer is
// the only caller

u16 NNS_G2dGetPaletteTableValue(const NNSG2dPaletteSwapTable *tbl, u16 beforeIdx) {
    return tbl->paletteIndex[beforeIdx];
}
