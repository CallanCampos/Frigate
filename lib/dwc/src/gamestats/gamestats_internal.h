#ifndef POKEBW2_DWC_GAMESTATS_INTERNAL_H
#define POKEBW2_DWC_GAMESTATS_INTERNAL_H

#include "types.h"
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
#ifdef PLATFORM_PC
// Host libc uses native size_t for string lengths and memory extents. Keep MWCC's original built-in suppression below.
#include <string.h>
#else
u32 strlen(const char *s);
char *strncpy(char *dest, const char *src, u32 size);
int strncmp(const char *a, const char *b, u32 size);
void *memcpy(void *dest, const void *src, u32 size);
#endif

// The front half of overlay 189: the HTTP client of the game statistics
// Starts and ends the client, and runs it once; the first starts one more user of it
BOOL DWC_InitGHTTP(int unused);
BOOL DWC_ShutdownGHTTP(void);
BOOL DWC_ProcessGHTTP(void);
// The callback with the answer, its size and the status, and the one when the request has been sent
typedef void (*GamestatsAnsweredFunc)(char *data, int size, int status, void *arg);
typedef void (*GamestatsSentFunc)(void);
// Starts a request for the URL and returns its number, or a negative number
int DWC_GetGHTTPData(const char *url, int a1, int a2, GamestatsSentFunc sent, GamestatsAnsweredFunc answered,
                     void *arg);
// Cancels a request
void DWC_CancelGHTTPRequest(int request);

#endif // POKEBW2_DWC_GAMESTATS_INTERNAL_H
