#ifndef POKEBW2_DWC_CRYPT_INTERNAL_H
#define POKEBW2_DWC_CRYPT_INTERNAL_H

#include "types.h"
#include "nitro/math.h"

// What crypt.c calls outside itself: NitroSDK's SHA-1 (nitro/math.h), and the compiler runtime's unsigned 64-bit
// division, which only the assembly function DivWords calls by name (compiled C reaches it through the `/` operator,
// so the project has no header of runtime helpers)
u64 _ll_udiv(u64 numerator, u64 denominator);

#endif // POKEBW2_DWC_CRYPT_INTERNAL_H
