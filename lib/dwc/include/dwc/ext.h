#ifndef POKEBW2_DWC_EXT_H
#define POKEBW2_DWC_EXT_H

#include "types.h"
#include "dwc/darray.h"
#include "dwc/error.h"
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
// where the ROM calls them
void *memset(void *dest, int value, u32 size);
void *memcpy(void *dest, const void *src, u32 size);
void *memmove(void *dest, const void *src, u32 size);
int memcmp(const void *a, const void *b, u32 size);
char *strcpy(char *dest, const char *src);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, u32 size);
u32 strcspn(const char *s, const char *set);
u32 strlen(const char *s);
char *strchr(const char *s, int c);
char *strstr(const char *s, const char *find);
// The rest is in the C headers above (MSL's)

// What overlay 189's HTTP code (the NHTTP client and the GameSpy HTTP code) calls in the main module and overlay 11.
// NitroSDK's thread library, which it also calls, is in nitro/os.h
// NitroSDK's STD functions are in nitro/std.h, and CPS's in nitrowifi/cps.h

// GameSpy's common code in overlay 11, used by the HTTP code: realloc and free (the ones beside the allocator above),
// the clock in milliseconds and a sleep, and the sockets (the numbers that the socket functions return are -1 for an
// error). The library's dynamic arrays are in dwc/darray.h
void *func_ov011_0216ddec(void *ptr, u32 size);
u32 func_ov011_0216e324(void);
void func_ov011_0216e360(u32 milliseconds);
void func_ov011_0216e31c(void);
int func_ov011_0216de20(int family, int type, int protocol);
int func_ov011_0216de40(int sock, int *error);
int func_ov011_0216de7c(int sock, int how);
int func_ov011_0216dec4(int sock, const void *address, int size);
int func_ov011_0216def0(int sock, const void *buffer, int size, int flags);
int func_ov011_0216df24(int sock, void *buffer, int size, int flags);
int func_ov011_0216df90(const char *host);
int func_ov011_0216dfa8(int sock);
int func_ov011_0216dfb4(int sock, int *readable, int *writable, int *failed);
int func_ov011_0216e054(int sock, int blocking);
int func_ov011_0216e0d0(int sock, int size);
int func_ov011_0216e10c(int sock);
void func_ov011_0216e2b4(void *lookup);
int func_ov011_0216e26c(const char *host, void **lookup);
int func_ov011_0216e2bc(void *lookup);

// The formatted texts of NitroSDK's OS (OS_SPrintf, OS_SNPrintf and OS_VSNPrintf) are in nitro/os.h,
// NitroSDK's STD's (STD_TSNPrintf) in nitro/std.h, and the Wi-Fi library's one error in dwc/error.h

// Overlay 11: allocates, zeroed by the caller, and frees
void *func_ov011_0216dddc(u32 size);
void func_ov011_0216ddfc(void *ptr);
// Whether the library's one asynchronous process is running (2 once it ended?), and the calls that drive it
int func_ov011_0216c9f4(void);
void func_ov011_0216c910(int a0);
void func_ov011_0216c998(void);
void func_ov011_0216c77c(void);
// Gets the token and the challenge that the player logs in to the authentication service with
void func_ov011_0215f9b4(const char **authToken, const char **challenge);
extern char data_ov011_0219069c[];
// The secret key of the game
extern char data_ov011_021918bc[];
// The state of the library, 1 once it has started
extern int data_ov011_02190698;
// The text that a SOAP writer has made, and its length
const char *func_ov011_0216fecc(SoapWriter *writer);
int func_ov011_0216fed0(SoapWriter *writer);
// Copies at most size - 1 characters of a text to dest, ends it and returns the count
u32 func_ov011_0216ee7c(char *dest, const char *src, u32 size);
// Copies the text and returns the copy, or 0
char *func_ov011_0216e2c8(const char *text);
// Reports an error to the library, with a code, a type, a flag and the text
void func_ov011_0216cca8(int category, int type, int level, const char *format, ...);

// Overlay 11's GameSpy code that GT2 (dwc/gt2Main.h) calls and that isn't decompiled yet: the rest of the sockets
// (close, bind, recvfrom with the size of the address it writes, sendto, getsockname, whether a socket can send now,
// and the end of the socket library), with addresses as NitroWiFi's SocAddrIn, and gt2Auth.c's challenges: a random
// challenge of 32 characters, the response to one (both return the buffer) and whether a response is the right one
int func_ov011_0216de30(int sock);
int func_ov011_0216de8c(int sock, const void *address, int size);
int func_ov011_0216df00(int sock, void *buffer, int size, int flags, void *address, int *addressSize);
int func_ov011_0216df34(int sock, const void *buffer, int size, int flags, const void *address, int addressSize);
int func_ov011_0216df74(int sock, void *address, int *size);
int func_ov011_0216e12c(int sock);
void func_ov011_0216e320(void);
u8 *func_ov011_0217c7ec(u8 *challenge);
u8 *func_ov011_0217c87c(u8 *response, const u8 *challenge);
BOOL func_ov011_0217c91c(const u8 *response, const u8 *expected);

// Overlay 11's HTTP client: posts a SOAP message to a URL with the SOAPAction header and calls the function with the
// answer; returns the request, or 0 when it could not start
typedef struct GsHttpRequest {
    u8 unk_00[0x24];
    void *handle;
} GsHttpRequest;
typedef void (*GsHttpResponseFunc)(int status, int a1, SoapReader *reader, void *userData);
GsHttpRequest *func_ov011_0216ea4c(const char *url, const char *soapAction, SoapWriter *writer, GsHttpResponseFunc func,
                                   void *userData);
// The same with a callback that gives the data and its size, and a content type
GsHttpRequest *func_ov011_0216eae0(const char *url, const char *soapAction, void *writer, GsHttpResponseFunc func,
                                   const void *a4, void *userData);
// Stops a request
void func_ov011_0216ca68(void *handle);

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
