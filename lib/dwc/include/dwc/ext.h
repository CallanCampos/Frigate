#ifndef POKEBW2_DWC_EXT_H
#define POKEBW2_DWC_EXT_H

#include <stdarg.h>
#include "types.h"
#include "dwc/sake.h"
#include "dwc/soap.h"

// What the DWC code of overlay 260 (GameSpy's Sake, the competition client and Nintendo's Gdb) calls in the main
// module and in overlays 11 and 189. The ROM names none of it, so these are the addresses with a word on what each
// does, read off the callers

// The C library's string functions that the main module links
void *memset(void *dest, int value, u32 size);
void *memcpy(void *dest, const void *src, u32 size);
char *strcpy(char *dest, const char *src);
int strcmp(const char *a, const char *b);
u32 strlen(const char *s);

// Writes at most size bytes of a formatted text to dest, returning the length it would have, or a negative number
int func_0207a2c0(char *dest, u32 size, const char *format, ...);
// The same with the arguments in a list
int func_0207a2dc(char *dest, u32 size, const char *format, va_list args);
// Reads values out of a text, like sscanf
int func_0207fa4c(const char *text, const char *format, ...);

// The Wi-Fi library's one error: sets it to a type and a code, unless it is already a type 9 error (the main module)
void func_020584cc(int type, int code);
// Whether there is an error, and (the second, with an error type to give) its code
BOOL func_020584b0(void);
int func_02058390(int *type);

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
// Waits for some milliseconds (the main module)
void func_0207aa04(u32 milliseconds);
// A text with the library's service name, which the server URLs are made from
extern char data_ov011_021917bc[];
extern char data_ov011_0219069c[];
// The secret key of the game
extern char data_ov011_021918bc[];
// The state of the library, 1 once it has started
extern int data_ov011_02190698;
// Copies the text and returns the copy, or 0
char *func_ov011_0216e2c8(const char *text);
// Reports an error to the library, with a code, a type, a flag and the text
void func_ov011_0216cca8(int a0, int type, int flag, const char *text);

// Overlay 11's HTTP client: posts a SOAP message to a URL with the SOAPAction header and calls the function with the
// answer; returns the request, or 0 when it could not start
typedef struct GsHttpRequest {
    u8 unk_00[0x24];
    void *handle;
} GsHttpRequest;
typedef void (*GsHttpResponseFunc)(int status, int a1, SoapReader *reader, void *userData);
GsHttpRequest *func_ov011_0216ea4c(const char *url, const char *soapAction, SoapWriter *writer,
                                   GsHttpResponseFunc func, void *userData);
// The same with a callback that gives the data and its size, and a content type
GsHttpRequest *func_ov011_0216eae0(const char *url, const char *soapAction, void *writer, GsHttpResponseFunc func,
                                   const void *a4, void *userData);
// Stops a request
void func_ov011_0216ca68(void *handle);

// The login ticket of the player's profile, which the Wi-Fi library keeps; returns FALSE if it has none
BOOL func_ov011_0215f9e0(SakeLoginTicket *ticket);
// Overlay 189's, which keep the list of the hosts that may be reached: adds the host of a URL, or removes it (the
// second), returning a BOOL
BOOL func_ov189_021a1160(const char *url);
BOOL func_ov189_021a1178(const char *url);
// Starts and ends the module (the first returns a BOOL), and runs it for a frame
BOOL func_ov189_021a118c(int unused);
BOOL func_ov189_021a11a4(void);
BOOL func_ov189_021a11cc(void);
// Sets a header field of a request, for the answer's size and bytes
int func_ov189_021a3148(void *request, const char *a1, int size, const void *bytes, const char *name, const char *type);
// Called with an error code, to log it
void func_ov189_021a13a0(int error);
// The state of a transfer that was started with a request of the module, and cancelling it
int func_ov189_021a3014(void *transfer);
void func_ov189_021a2ff4(void *transfer);

#endif // POKEBW2_DWC_EXT_H
