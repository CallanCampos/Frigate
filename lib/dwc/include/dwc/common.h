#ifndef POKEBW2_DWC_COMMON_H
#define POKEBW2_DWC_COMMON_H

#include "types.h"

// The Base64 coding of DWC (TWL-DWC 5.3, ARM9 main), from lib/dwc/src/base/dwc_common.c. The ROM names none of it; the
// SDK's names are in the comments.

// DWC_Base64Encode of the SDK. Writes the bytes at src as text to dst, in groups of four characters with '*' to pad the
// last, and returns the number of characters, or -1 if dstLen is too small. A dst of NULL writes nothing and returns
// the number that it would write
int func_020580b8(const u8 *src, u32 srcLen, char *dst, u32 dstLen);
// DWC_Base64Decode of the SDK. Reads srcLen characters of text at src, which must be a multiple of four, and writes
// the bytes that they stand for to dst; returns the number of bytes, or -1 if srcLen is not a multiple of four or
// dstLen is too small. A dst of NULL writes nothing and returns the number that it would write
int func_02058234(const char *src, int srcLen, u8 *dst, u32 dstLen);

#endif // POKEBW2_DWC_COMMON_H
