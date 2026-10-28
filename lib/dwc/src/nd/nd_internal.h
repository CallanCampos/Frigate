#ifndef POKEBW2_DWC_ND_INTERNAL_H
#define POKEBW2_DWC_ND_INTERNAL_H

#include "types.h"
#include "dwc/apinfo.h"
#include "dwc/base64.h"
#include "dwc/bm.h"
#include "dwc/error.h"
#include "dwc/memfunc.h"
#include "dwc/nd.h"
#include "dwc/nhttp.h"
#include "nitro/os.h"
#include "struct_decls.h"

// What the download library calls outside itself, none of it with a name in the ROM: the Wi-Fi library in the main
// module (its allocator, whose type is 10 for everything here, the one error that it keeps, the access point's
// information and the console's Wi-Fi ID), the C library, and the HTTP client's calls in the front half of overlay 189
// (the sources before nd.c), whose header will take the last group

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

// Engine state and calls shared by the public wrapper and its download engine
struct NdContext {
    // Records a result, and is given to the requests
    DWCNdCallback onResult;
    NHTTPAlloc alloc;
    BOOL running;
    BOOL hasApInfo;
    int request;
    NHTTPFree free;
    char *token;
    char *url;
    // Whether a request is in flight
    BOOL requestActive;
    void (*onCleanedUp)(void);
    // The game's code and the cartridge's, in base64
    char rhGameCodeParam[9];
    char gameCodeParam[9];
    // The attributes of DWC_NdSetAttr, in base64
    char *attrs[3];
};

extern NdContext sNdContext;
extern const char sActionList[];
extern const char sActionContents[];

void SetWifiError(int error, int serverError);
BOOL DWCi_NdSetAttr(const char *attr1, const char *attr2, const char *attr3);
BOOL DWCi_NdStartup(NHTTPAlloc alloc, NHTTPFree free, const char *url, const char *token, const char *gameCode,
                    const char *password, DWCNdCallback onResult);
void DWCi_NdCleanupAsync(void (*onCleanedUp)(void));
BOOL DWCi_NdCancelAsync(void);
BOOL DWCi_NdGetFileListAsync(DWCNdFileInfo *files, u32 offset, u32 count);
BOOL DWCi_NdGetFileAsync(DWCNdFileInfo *file, void *buffer, u32 size, const char *logData);
BOOL DWCi_NdGetProgress(u32 *received, u32 *total);

#endif // POKEBW2_DWC_ND_INTERNAL_H
