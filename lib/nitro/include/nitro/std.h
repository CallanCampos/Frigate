#ifndef POKEBW2_NITRO_STD_H
#define POKEBW2_NITRO_STD_H

#include "types.h"
#include <stdarg.h>

// NitroSDK's STD, the standard string functions of the SDK (TwlSDK's libstd). Their names are the SDK's; swan named the
// length one NNS_STD_StrLen

// Copies the string and its terminator, returning dst
char *STD_CopyString(char *dst, const char *src);
// Copies at most size - 1 characters and a terminator, returning the length of src
int STD_CopyLString(char *dst, const char *src, int size);
// Where sub is in str, or NULL
char *STD_SearchString(const char *str, const char *sub);
// STD_GetStringLength (swan's name is NNS_STD_StrLen)
int STD_GetStringLength(const char *str);
// The length of the string, or maxlen if it is longer
int STD_GetStringNLength(const char *str, int maxlen);
// STD_CompareString: the difference of the first characters that differ, as unsigned
int STD_CompareString(const char *a, const char *b);
// The same for the first len characters
int STD_CompareNString(const char *a, const char *b, int len);
// Reads values out of a text, like sscanf, and returns how many; -1 if the text ends first
int STD_TSScanf(const char *src, const char *fmt, ...);
// The same with the pointers in a list
int STD_TVSScanf(const char *src, const char *fmt, va_list args);

// Writes a formatted text to dst, with no limit on its length, and returns the length
int STD_TSPrintf(char *dst, const char *fmt, ...);
// STD_TSNPrintf (swan's name is OS_SNPrintf): the same, at most len - 1 characters and a NUL
int STD_TSNPrintf(char *dst, u32 len, const char *fmt, ...);
// STD_TVSNPrintf (swan's name is OS_VSNPrintf): the same with the arguments in a list. Returns the length the text
// would have without the limit
int STD_TVSNPrintf(char *dst, u32 len, const char *fmt, va_list args);

#endif // POKEBW2_NITRO_STD_H
