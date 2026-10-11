#ifndef POKEBW2_DWC_EXT_H
#define POKEBW2_DWC_EXT_H

#include <stdarg.h>
#include "types.h"
#include "nitro/os.h"
#include "dwc/sake.h"
#include "dwc/soap.h"

// What the DWC code of overlay 260 (GameSpy's Sake, the competition client and Nintendo's Gdb) calls in the main
// module and in overlays 11 and 189. The ROM names none of it, so these are the addresses with a word on what each
// does, read off the callers

// The C library's string functions that the main module links
void *memset(void *dest, int value, u32 size);
void *memcpy(void *dest, const void *src, u32 size);
void *memmove(void *dest, const void *src, u32 size);
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
// Of NitroSDK, the thread library: the message queue is initialized with the array it will hold (OS_InitMessageQueue's
// code), a thread's end is waited for (OS_JoinThread's), and the thread that runs is the second word of the
// scheduler's state, which swan names g_IsThreadSwitchRequested
void func_0207acc4(OSMessageQueue *queue, OSMessage *messages, s32 count);
#define OS_MESSAGE_BLOCK 1
BOOL OS_SendMessage(OSMessageQueue *queue, OSMessage message, s32 flags);
void func_0207a800(OSThread *thread);
extern struct {
    int unk_00;
    OSThread *current;
} g_IsThreadSwitchRequested;
// NitroSDK's STD_GetStringLength with a limit: the length of the string, or the limit if it is longer
int func_0207f900(const char *s, int max);
// The socket library of overlay 11, whose functions take and return negative error codes. The address and port of a
// socket address are in network order, and its first byte is its size
typedef struct SocAddrIn {
    u8 len;
    u8 family;
    u16 port;
    u32 addr;
} SocAddrIn;
typedef struct SocHostEnt {
    char *name;
    char **aliases;
    s16 addrType;
    s16 addrLength;
    u8 **addrList;
} SocHostEnt;
int func_ov011_02157bd0(int domain, int type, int protocol);
int func_ov011_02157c10(int sock, const SocAddrIn *address);
int func_ov011_02157c50(int sock, void *buffer, int size, int flags);
int func_ov011_02157cbc(int sock, const void *buffer, int size, int flags);
int func_ov011_02157d30(int sock, int how);
int func_ov011_02157d38(int sock);
SocHostEnt *func_ov011_02157d40(const char *host);
int func_ov011_02158034(const char *text, u32 *address);
int func_ov011_02158174(int sock, void *sslInfo);
void func_ov011_021575f8(void);
// Seeds the random number generator of the secure connections with some bytes
void func_ov011_02159d64(const void *data, u32 size);

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
