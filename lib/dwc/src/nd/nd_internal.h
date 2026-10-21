#ifndef POKEBW2_DWC_ND_INTERNAL_H
#define POKEBW2_DWC_ND_INTERNAL_H

#include "types.h"
#include "dwc/apinfo.h"
#include "dwc/bm.h"
#include "dwc/common.h"
#include "dwc/error.h"
#include "dwc/memfunc.h"
#include "dwc/nhttp.h"
#include "nitro/os.h"

// What nd.c calls outside itself, none of it with a name in the ROM: the Wi-Fi library in the main module (its
// allocator, whose type is 10 for everything here, the one error that it keeps, the access point's information and
// the console's Wi-Fi ID), the C library, and the HTTP client's calls in the front half of overlay 189 (the sources
// before nd.c), whose header will take the last group

// The C library's string functions, not taken from <string.h>: MWCC builds them in place (strlen, memcpy of a size it
// knows) when <string.h> declares them, where the ROM calls them
#ifdef PLATFORM_PC
// Host libc uses native size_t; the original declarations below preserve MWCC's call generation.
#include <string.h>
#else
u32 strlen(const char *s);
char *strncpy(char *dest, const char *src, u32 size);
int strncmp(const char *a, const char *b, u32 size);
#endif

// Overlay 189's front half: the numbers of a text (the HTTP client's own functions are in dwc/nhttp.h)
int NHTTPi_strToInt(const char *text, int size);
int NHTTPi_intToStr(char *dest, u32 value);

#endif // POKEBW2_DWC_ND_INTERNAL_H
