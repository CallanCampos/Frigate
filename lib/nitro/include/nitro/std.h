#ifndef POKEBW2_NITRO_STD_H
#define POKEBW2_NITRO_STD_H

#include "types.h"
#include <stdarg.h>

// NitroSDK's STD, the standard string functions of the SDK (TwlSDK's libstd). Their symbols keep the default names
// for now, and the comments give the SDK's; swan named the length one NNS_STD_StrLen

// STD_CopyString: copies the string and its terminator, returning dst
char *func_0207f7a4(char *dst, const char *src);
// STD_CopyLString: copies at most size - 1 characters and a terminator, returning the length of src
int func_0207f7cc(char *dst, const char *src, int size);
// STD_SearchString: where sub is in str, or NULL
char *func_0207f874(const char *str, const char *sub);
// STD_GetStringLength (swan's name is NNS_STD_StrLen)
int NNS_STD_StrLen(const char *str);
// STD_GetStringNLength: the length of the string, or maxlen if it is longer
int func_0207f900(const char *str, int maxlen);
// STD_CompareString: the difference of the first characters that differ, as unsigned
int STD_CompareString(const char *a, const char *b);
// STD_CompareNString: the same for the first len characters
int func_0207f9c8(const char *a, const char *b, int len);
// STD_TSScanf: reads values out of a text, like sscanf, and returns how many; -1 if the text ends first
int func_0207fa4c(const char *src, const char *fmt, ...);
// STD_TVSScanf: the same with the pointers in a list
int func_0207fac0(const char *src, const char *fmt, va_list args);

// STD_TSPrintf: writes a formatted text to dst, with no limit on its length, and returns the length
int func_020800e8(char *dst, const char *fmt, ...);
// STD_TSNPrintf (swan's name is OS_SNPrintf): the same, at most len - 1 characters and a NUL
int OS_SNPrintf(char *dst, u32 len, const char *fmt, ...);
// STD_TVSNPrintf (swan's name is OS_VSNPrintf): the same with the arguments in a list. Returns the length the text
// would have without the limit
int OS_VSNPrintf(char *dst, u32 len, const char *fmt, va_list args);

#endif // POKEBW2_NITRO_STD_H
