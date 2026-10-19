#ifndef POKEBW2_DWC_GSLARGEINT_H
#define POKEBW2_DWC_GSLARGEINT_H

#include "types.h"
// The hash that gsLargeIntAddToMD5 adds a number to
#include "dwc/md5.h"

// GameSpy's large integers (gsLargeInt.c), in overlay 11, which the signature check (dwc/gsCrypt.h) and the
// authentication's keys are made of

// A number of up to 64 words, the lowest first, and how many of them it uses (GameSpy's gsLargeInt_t)
typedef struct BigNum {
    u32 length;
    u32 data[0x40];
} BigNum;

// The power of a number modulo an odd modulus, which gives FALSE for a modulus of 0 or an even one
BOOL gsLargeIntPowerMod(const BigNum *base, const BigNum *exponent, const BigNum *modulus, BigNum *result);
// Sets a number from a text of hexadecimal digits, the most significant first; reverses the order of a number's
// bytes; and adds its bytes, the most significant first, to a hash
BOOL gsLargeIntSetFromHexString(BigNum *number, const char *hex);
BOOL gsLargeIntReverseBytes(BigNum *number);
void gsLargeIntAddToMD5(const BigNum *number, HashContext *context);

#endif // POKEBW2_DWC_GSLARGEINT_H
