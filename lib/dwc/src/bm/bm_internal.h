#ifndef POKEBW2_DWC_BM_INTERNAL_H
#define POKEBW2_DWC_BM_INTERNAL_H

#include "types.h"
#include "nitro/nvram.h"
#include "nitro/rtc.h"

// What the files of bm/ call from each other and outside them, and no one else does

// dwc_nvram.c. Reads up to size bytes of a page of the Wi-Fi settings, or writes them, as far as the page goes, and
// returns the number of bytes, or -1 when it can't
int func_0205dca4(void *dst, u32 size, u32 page);
int func_0205dd40(u32 page, const void *src, u32 size);

// dwc_nvram.c. The first call of any of these finds where the Wi-Fi settings are, once: 0x400 bytes before the user
// settings, whose offset the NVRAM's header holds. Returns 0, or -1 when the header can't be read
int DWCi_BACKUPlInit(void);
// The version byte of the NVRAM's header, 0x10 for 0xff, or 0 when it can't be read. The extra pages of the settings
// (0x11 to 0x13) are only there from 0x20 on
u32 func_0205dc78(void);

// dwc_bm_check.c. Checks the Wi-Fi settings of an NTR or of a TWL console in a work area of 0x1000 bytes, repairs what
// can be and returns 0, or one of the negative DWCBMInitResult values of dwc/bm.h
int func_0205e094(void *work);
int func_0205e0f0(void *work);

#endif // POKEBW2_DWC_BM_INTERNAL_H
