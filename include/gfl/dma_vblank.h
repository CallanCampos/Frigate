#ifndef POKEBW2_GFL_DMA_VBLANK_H
#define POKEBW2_GFL_DMA_VBLANK_H

#include "types.h"

// Writes the DMA channel settings that were requested, which GFL_VBlankUpdate calls. The table is never set up
void DMAVBlank_Update(void);

#endif // POKEBW2_GFL_DMA_VBLANK_H
