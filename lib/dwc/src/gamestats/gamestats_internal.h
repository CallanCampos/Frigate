#ifndef POKEBW2_DWC_GAMESTATS_INTERNAL_H
#define POKEBW2_DWC_GAMESTATS_INTERNAL_H

#include "types.h"
#include "nitro/math.h"

// What gamestats.c calls outside itself, none of it with a name in the ROM

// The main module's: the Wi-Fi library's allocator and error
void *func_020586e4(int type, u32 size);
void func_02058728(int type, void *ptr, int unused);
void func_02058814(void *ptr);
BOOL func_020584b0(void);
void func_020584cc(int type, int code);
// The Wi-Fi library's error, as its type and its group
int func_020583b0(int *type, int *group);
// Formats a text like snprintf, and into a bigger buffer without a size
int func_0207a2c0(char *dest, u32 size, const char *format, ...);
int func_0207a290(char *dest, const char *format, ...);

// The C library
u32 strlen(const char *s);
char *strncpy(char *dest, const char *src, u32 size);
int strncmp(const char *a, const char *b, u32 size);
unsigned long strtoul(const char *text, char **end, int base);
void *memcpy(void *dest, const void *src, u32 size);

// Overlay 11's base64 encoder: encodes size bytes of src into dest, with the alphabet 1 or 2
void func_ov011_0216e878(const void *src, char *dest, int size, int alphabet);

// The front half of overlay 189: the HTTP client of the game statistics
// Starts and ends the client, and runs it once; the first starts one more user of it
BOOL DWC_InitGHTTP(int unused);
BOOL DWC_ShutdownGHTTP(void);
BOOL DWC_ProcessGHTTP(void);
// The callback with the answer, its size and the status, and the one when the request has been sent
typedef void (*GamestatsAnsweredFunc)(char *data, int size, int status, void *arg);
typedef void (*GamestatsSentFunc)(void);
// Starts a request for the URL and returns its number, or a negative number
int DWC_GetGHTTPData(const char *url, int a1, int a2, GamestatsSentFunc sent, GamestatsAnsweredFunc answered, void *arg);
// Cancels a request
void DWC_CancelGHTTPRequest(int request);

#endif // POKEBW2_DWC_GAMESTATS_INTERNAL_H
