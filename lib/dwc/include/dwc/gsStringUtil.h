#ifndef POKEBW2_DWC_GSSTRINGUTIL_H
#define POKEBW2_DWC_GSSTRINGUTIL_H

#include "types.h"

// GameSpy's texts (gsStringUtil.c), in overlay 11: the conversion of UTF-8 texts to the UCS-2 texts of the Wi-Fi
// library, and back for one character

// Writes a character as UTF-8 and returns how many bytes it took
int _UCS2CharToUTF8String(u16 c, char *utf8);
// Copies a UTF-8 text, or its first length bytes, as a UCS-2 text it allocates (NULL for a NULL text)
u16 *UTF8ToUCS2StringAlloc(const char *utf8);
u16 *UTF8ToUCS2StringAllocLen(const char *utf8, int length);
// Converts at most length bytes of a UTF-8 text into at most count characters, and ends the text; returns the count
// with the end, or 0 when the text ends in a character
int UTF8ToUCS2StringLen(const char *utf8, int length, u16 *ucs2, int count);
// Copies at most size - 1 characters of a text to dest, ends it and returns the count (a name of ours)
u32 gsiSafeStrcpyA(char *dest, const char *src, u32 size);

#endif // POKEBW2_DWC_GSSTRINGUTIL_H
