#ifndef POKEBW2_DWC_GAMESTATS_INTERNAL_H
#define POKEBW2_DWC_GAMESTATS_INTERNAL_H

#include "types.h"
#include "dwc/dwc_ghttp.h"
#include "dwc/error.h"
// Overlay 11's base 64 (B64Encode)
#include "dwc/gsPlatformUtil.h"
#include "dwc/memfunc.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include <stdlib.h>
#include <string.h>

// The main module's Wi-Fi library (its allocator and its error), NitroSDK's formatted texts and the C library are in
// the headers above. Through <string.h>, MWCC copies the 20-byte secret in place, as the ROM does, and calls memcpy
// for the larger copies

// The front half of overlay 189, DWC's use of the HTTP client, is in dwc/dwc_ghttp.h

#endif // POKEBW2_DWC_GAMESTATS_INTERNAL_H
