#include "nnsys/g2d.h"

// NitroSystem's VRAM locations: an address in VRAM for the 3D engine and each 2D engine, as image and palette proxies
// keep them. The file name is a guess from the functions' NNSi_G2d prefix: the ROM has no string for it, and ntrtwl's
// reconstruction of NitroSystem has the functions in g2d_Image.c

void NNSi_G2dInitializeVRamLocation(NNSG2dVRamLocation *location) {
    int i;

    for (i = 0; i < NNS_G2D_VRAM_TYPE_MAX; i++) {
        location->baseAddrOfVram[i] = NNS_G2D_VRAM_ADDR_NONE;
    }
}

void NNSi_G2dSetVramLocation(NNSG2dVRamLocation *location, NNS_G2D_VRAM_TYPE type, u32 addr) {
    location->baseAddrOfVram[type] = addr;
}

u32 NNSi_G2dGetVramLocation(const NNSG2dVRamLocation *location, NNS_G2D_VRAM_TYPE type) {
    return location->baseAddrOfVram[type];
}
