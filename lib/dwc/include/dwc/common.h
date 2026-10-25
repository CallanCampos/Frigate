#ifndef POKEBW2_DWC_COMMON_H
#define POKEBW2_DWC_COMMON_H

#include "types.h"

// DWC's common helpers of the friends' match (overlay 11's dwc_common.c): the key-value texts of the friends'
// statuses, the match's random numbers and the length of a text of u16 characters. The functions carry the SDK's
// names (see dwc_common.c).

// The texts of the statuses: a text of sep, key, sep and value written to out, or appended to it, both returning the
// text's length, and the value of a key read to dest (only measured if dest is NULL), returning its length or -1
int DWC_SetCommonKeyValueString(const char *key, const char *value, char *out, char sep);
int DWC_AddCommonKeyValueString(const char *key, const char *value, char *out, char sep);
int DWC_GetCommonValueString(const char *key, char *dest, const char *text, char sep);

// A random number below max (all 32 bits if max is 0), from a generator seeded with the console's address and clock
u32 DWCi_GetMathRand32(u32 max);
// The length of a text of u16 characters
u32 DWCi_WStrLen(const u16 *str);

#endif // POKEBW2_DWC_COMMON_H
