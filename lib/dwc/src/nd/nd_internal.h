#ifndef POKEBW2_DWC_ND_INTERNAL_H
#define POKEBW2_DWC_ND_INTERNAL_H

#include "types.h"

#include "nitro/os.h"

// What nd.c calls outside itself, none of it with a name in the ROM: the Wi-Fi library's allocator and error in the main
// module, the C library, and the HTTP client's calls in the front half of overlay 189 (the sources before nd.c), whose
// header will take the last group

// The main module's: the Wi-Fi library's allocator (the type is 10 for everything here), and the one error that the
// library keeps
void *func_020586f4(int type, u32 size, int align);
void func_02058728(int type, void *ptr, int unused);
BOOL func_020584b0(void);
void func_020584cc(int type, int code);
int func_02058390(int *code);
// Formats a text like snprintf
int func_0207a2c0(char *dest, u32 size, const char *format, ...);
// Encodes a text of srcLen bytes into dest, which holds size bytes; the length, or -1
int func_020580b8(const char *src, int srcLen, char *dest, int size);
// Waits for a thread to end: NitroSDK's OS_JoinThread
void func_0207a800(OSThread *thread);
BOOL OS_SendMessage(OSMessageQueue *mq, OSMessage msg, s32 flags);

// What the access point's information is, as the Wi-Fi library fills it
typedef struct NdApInfo {
    int type;
    int unk4;
    u8 unk8[4];
    char spot[9];
    u8 unk15[0x2b];
} NdApInfo;
BOOL func_0205b29c(NdApInfo *info);
// The identifiers of the console, as the Wi-Fi library fills them in
typedef struct NdConsoleIds {
    u64 id;
    u8 unk8[12];
} NdConsoleIds;
void func_02056a90(NdConsoleIds *ids);

// The C library
u32 strlen(const char *s);
char *strncpy(char *dest, const char *src, u32 size);
int strncmp(const char *a, const char *b, u32 size);

// Overlay 189's front half: the numbers of the text, and the HTTP client's requests
int func_ov189_0219db50(const char *text, int size);
int func_ov189_0219dbb0(char *dest, u32 value);
void func_ov189_021a0790(void (*callback)(void));
int func_ov189_021a081c(int request, const char *name, const char *value);
int func_ov189_021a08a0(int request, const char *name, char **value);
int func_ov189_021a092c(int request, int value);
int func_ov189_021a102c(int request);
int func_ov189_021a1060(int request);
int func_ov189_021a1080(int request, char **body, int *size);
void *func_ov189_021a10c0(int request);

#endif // POKEBW2_DWC_ND_INTERNAL_H
