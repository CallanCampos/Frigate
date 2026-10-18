#ifndef POKEBW2_DWC_EXT_H
#define POKEBW2_DWC_EXT_H

#include "types.h"
#include "dwc/darray.h"
#include "dwc/error.h"
// GameSpy's common code in overlay 11, which the HTTP code and the rest of GameSpy's code call
#include "dwc/gsAvailable.h"
#include "dwc/gsCore.h"
#include "dwc/gsDebug.h"
#include "dwc/gsMemory.h"
#include "dwc/gsPlatformSocket.h"
#include "dwc/gsPlatformUtil.h"
#include "dwc/gsSoap.h"
#include "dwc/gs_screen_print.h"
#include "dwc/sake.h"
#include "dwc/soap.h"
#include "nitro/os.h"
#include "system/rtc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
// For CPS's address (CPSMyIp), which is set once the library has started
#include "nitrowifi/cps.h"
#include "nitrowifi/soc.h"

// What the DWC code of overlay 260 (GameSpy's Sake, the competition client and Nintendo's Gdb) calls in the main
// module and in overlays 11 and 189. The ROM names none of it, so these are the addresses with a word on what each
// does, read off the callers

// The C library's string functions that the main module links, not taken from <string.h>: MWCC builds them in place
// (memset and memcpy of a size it knows, strcpy of a literal, strcmp and strlen) when <string.h> declares them,
// where the ROM calls them. A file whose ROM has them built in place (the memsets of gpiBuddy.c and gpiOperation.c)
// includes <string.h> before this header instead
#ifndef POKEBW2_STRING_H
void *memset(void *dest, int value, u32 size);
void *memcpy(void *dest, const void *src, u32 size);
void *memmove(void *dest, const void *src, u32 size);
int memcmp(const void *a, const void *b, u32 size);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, u32 size);
char *strcat(char *dest, const char *src);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, u32 size);
u32 strcspn(const char *s, const char *set);
u32 strlen(const char *s);
char *strchr(const char *s, int c);
char *strstr(const char *s, const char *find);
#endif
// The rest is in the C headers above (MSL's)

// What overlay 189's HTTP code (the NHTTP client and the GameSpy HTTP code) calls in the main module and overlay 11.
// NitroSDK's thread library, which it also calls, is in nitro/os.h
// NitroSDK's STD functions are in nitro/std.h, CPS's in nitrowifi/cps.h, and GameSpy's common code of overlay 11 in
// the dwc/gs*.h headers above

// The formatted texts of NitroSDK's OS (OS_SPrintf, OS_SNPrintf and OS_VSNPrintf) are in nitro/os.h,
// NitroSDK's STD's (STD_TSNPrintf) in nitro/std.h, and the Wi-Fi library's one error in dwc/error.h

// Gets the token and the challenge that the player logs in to the authentication service with
void func_ov011_0215f9b4(const char **authToken, const char **challenge);

// The login ticket of the player's profile, which the Wi-Fi library keeps; returns FALSE if it has none
BOOL func_ov011_0215f9e0(SakeLoginTicket *ticket);
// Overlay 189's, which keep the list of the hosts that may be reached: adds the host of a URL, or removes it (the
// second), returning a BOOL
BOOL DWCi_SetGHTTPRootCA(const char *url);
BOOL DWCi_RemoveGHTTPRootCA(const char *url);
// Starts and ends the module (the first returns a BOOL), and runs it for a frame
BOOL DWC_InitGHTTP(int unused);
BOOL DWC_ShutdownGHTTP(void);
BOOL DWC_ProcessGHTTP(void);
// A post: the form (or message) that a request sends
typedef struct GHIPost GHIPost;
// Adds a file made of bytes in memory to a post (a form), with the name of its field, the name of the file and its type
int ghttpPostAddFileFromMemory(GHIPost *post, const char *field, const void *bytes, int size, const char *name,
                               const char *type);
// Called with an error code, to log it
int DWCi_HandleGHTTPError(int error);
// The state of a transfer that was started with a request of the module, and cancelling it
int ghttpGetState(int request);
void ghttpCancelRequest(int request);

#endif // POKEBW2_DWC_EXT_H
