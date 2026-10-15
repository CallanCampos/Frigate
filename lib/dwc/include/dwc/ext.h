#ifndef POKEBW2_DWC_EXT_H
#define POKEBW2_DWC_EXT_H

#include <stdarg.h>
#include "types.h"
#include "nitro/os.h"
#include "system/rtc.h"
#include "dwc/sake.h"
#include "dwc/soap.h"
// For CPS's address (CPSMyIp), which is set once the library has started
#include "nitrowifi/cps.h"
#include "nitrowifi/soc.h"

// What the DWC code of overlay 260 (GameSpy's Sake, the competition client and Nintendo's Gdb) calls in the main
// module and in overlays 11 and 189. The ROM names none of it, so these are the addresses with a word on what each
// does, read off the callers

// The C library's string functions that the main module links
void *memset(void *dest, int value, u32 size);
void *memcpy(void *dest, const void *src, u32 size);
void *memmove(void *dest, const void *src, u32 size);
int memcmp(const void *a, const void *b, u32 size);
char *strcpy(char *dest, const char *src);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, u32 size);
// What swan names strcspn: it counts the characters before the first one that is in the set
u32 strcspn(const char *s, const char *set);
u32 strlen(const char *s);
char *strchr(const char *s, int c);
char *strstr(const char *s, const char *find);
long atol(const char *s);
// The classification of the characters below 0x80, as bits (0x100 for white space), which swan names __ctype_map
extern u16 __ctype_map[];
// The C library's stream functions, on a file that the stdio of the main module opened
int fseek(void *file, long offset, int whence);
long ftell(void *file);
void rewind(void *file);
int fclose(void *file);
u32 __fread(void *buffer, u32 size, u32 count, void *file);

// What overlay 189's HTTP code (the NHTTP client and the GameSpy HTTP code) calls in the main module and overlay 11.
// NitroSDK's thread library, which it also calls, is in nitro/os.h
// NitroSDK's STD_GetStringLength with a limit: the length of the string, or the limit if it is longer
int func_0207f900(const char *s, int max);
// Seeds the random number generator of the secure connections with some bytes
void CPS_SslAddRandomSeed(const void *data, u32 size);

// GameSpy's common code in overlay 11, used by the HTTP code: realloc and free (the ones beside the allocator above),
// the clock in milliseconds and a sleep, and the sockets (the numbers that the socket functions return are -1 for an
// error). The array functions are the ones of the library's dynamic arrays
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
void *func_ov011_02171514(int elementSize, int growBy, void (*freeFunc)(void *element));
void func_ov011_02171558(void *array);
int func_ov011_02171588(void *array);
void *func_ov011_0217158c(void *array, int index);
void func_ov011_021715a4(void *array, const void *element);
void func_ov011_02171600(void *array, const void *element, int (*compare)(const void *a, const void *b));
void func_ov011_02171668(void *array, int index);
int func_ov011_021716ac(void *array, const void *element, int (*compare)(const void *a, const void *b), int start,
                        int sorted);
// Removes the element at index without calling the array's freeFunc on it (gt2.c's only caller frees it itself
// first)
void func_ov011_02171634(void *array, int index);

// Writes at most size bytes of a formatted text to dest, returning the length it would have, or a negative number
int func_0207a2c0(char *dest, u32 size, const char *format, ...);
// The same with the arguments in a list
int func_0207a2dc(char *dest, u32 size, const char *format, va_list args);
// Reads values out of a text, like sscanf
int func_0207fa4c(const char *text, const char *format, ...);
// Writes a formatted text to dest (sprintf)
int func_0207a290(char *dest, const char *format, ...);

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
// The text that a SOAP writer has made, and its length
const char *func_ov011_0216fecc(SoapWriter *writer);
int func_ov011_0216fed0(SoapWriter *writer);
// Copies at most size - 1 characters of a text to dest, ends it and returns the count
u32 func_ov011_0216ee7c(char *dest, const char *src, u32 size);
// Copies the text and returns the copy, or 0
char *func_ov011_0216e2c8(const char *text);
// Reports an error to the library, with a code, a type, a flag and the text
void func_ov011_0216cca8(int category, int type, int level, const char *format, ...);

// Overlay 11's lower socket layer, which gt2.c's "Udp Engine" builds on; not decompiled, so these are addresses
// with a word on what each does, read off gt2.c's callers. An "address" is its own opaque object (not a raw ip),
// read with the two accessors below
u32 func_ov011_0217d380(const void *address);
u16 func_ov011_0217d384(const void *address);
// Formats an address as text (its ip and port) into buf, and returns buf
const char *func_ov011_0217ed14(u32 ip, u16 port, char *buf);
// Creates the socket at socketOut, bound to localAddress, with byte buffer sizes and a socket error callback;
// returns 0, or an error
int func_ov011_0217d144(void **socketOut, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                        void (*onSocketError)(void *socket));
// Sets the socket's unknown-message callback
void func_ov011_0217d3a0(void *socket, BOOL (*callback)(void *unused, u32 ip, u16 port, const void *data, int len));
// Sets the socket's incoming-connection-attempt callback
void func_ov011_0217d188(void *socket, void (*callback)(void *unused, void *handle, u32 ip, u16 port,
                                                        u32 elapsedMilliseconds, const void *data, int len));
// The socket's own local address, once bound
u32 func_ov011_0217d388(void *socket);
u16 func_ov011_0217d38c(void *socket);
// Shuts the socket down, in two steps
void func_ov011_0217d158(void *socket);
void func_ov011_0217d168(void *socket);
// A connection's state, its biggest message size, closing it and sending a message of size bytes
int func_ov011_0217d364(void *handle);
int func_ov011_0217d390(void *handle);
int func_ov011_0217d394(void *handle);
int func_ov011_0217d290(void *handle, const void *message, int size, BOOL reliable);
// Starts a connection attempt with its callback table (attempt result, received, closed, ping, in that order).
// Writes the handle through outHandle when non-NULL. If blocking, pumps the socket until completion;
// otherwise returns after starting. Returns the lower connection status.
int func_ov011_0217d1a0(void *socket, void **outHandle, const char *addrText, const void *message, int length,
                        u32 timeoutMs, const void *table, BOOL blocking);
// Accepts a pending connection and installs its callback table; returns FALSE unless it is awaiting acceptance
BOOL func_ov011_0217d190(void *handle, const void *table);
// Rejects a pending connection, optionally sending a rejection message
void func_ov011_0217d198(void *handle, const void *message, int length);

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
BOOL DWCi_SetGHTTPRootCA(const char *url);
BOOL DWCi_RemoveGHTTPRootCA(const char *url);
// Starts and ends the module (the first returns a BOOL), and runs it for a frame
BOOL DWC_InitGHTTP(int unused);
BOOL DWC_ShutdownGHTTP(void);
BOOL DWC_ProcessGHTTP(void);
// A post: the form (or message) that a request sends
typedef struct GHIPost GHIPost;
// Adds a file made of bytes in memory to a post (a form), with the name of its field, the name of the file and its type
int ghttpPostAddFileFromMemory(GHIPost *post, const char *field, const void *bytes, int size, const char *name, const char *type);
// Called with an error code, to log it
int DWCi_HandleGHTTPError(int error);
// The state of a transfer that was started with a request of the module, and cancelling it
int ghttpGetState(int request);
void ghttpCancelRequest(int request);

#endif // POKEBW2_DWC_EXT_H
