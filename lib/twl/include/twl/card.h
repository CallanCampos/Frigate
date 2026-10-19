#ifndef POKEBW2_TWL_CARD_H
#define POKEBW2_TWL_CARD_H

#include "types.h"
#include "nitro/card.h"

// TwlSDK's part of the card library in the LTD autoload: the interface of the DSi's new DMA channels, which CARD reads
// the ROM with on a DSi
extern const CARDDmaInterface CARDi_NdmaInterface;
// TwlSDK's part of CARDi_InitRom for a DSi, in the LTD autoload
void CARDi_InitTwlRom(void);
u32 func_027676b4(const CARDTwlRomHeader *header);
void func_02767a14(void *work, const CARDTwlRomHeader *header, u32 arenaStart, u32 workSize, CARDiRomReader cpuReader,
                   CARDiRomReader dmaReader, void *readerWork);
void func_02767e84(void *work, void *dst, u32 src, u32 len);
void func_0276787c(void *work);

#endif // POKEBW2_TWL_CARD_H
