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

// What gamestats.c calls outside itself, none of it with a name in the ROM

// The main module's Wi-Fi library (its allocator and its error), NitroSDK's formatted texts and the C library are in
// the headers above, but for the C library's string functions: MWCC builds them in place (memcpy of a size it knows,
// strlen) when <string.h> declares them, where the ROM calls them
u32 strlen(const char *s);
char *strncpy(char *dest, const char *src, u32 size);
int strncmp(const char *a, const char *b, u32 size);
void *memcpy(void *dest, const void *src, u32 size);

// The front half of overlay 189, DWC's use of the HTTP client, is in dwc/dwc_ghttp.h

#endif // POKEBW2_DWC_GAMESTATS_INTERNAL_H
