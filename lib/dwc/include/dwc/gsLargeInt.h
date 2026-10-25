#ifndef POKEBW2_DWC_GSLARGEINT_H
#define POKEBW2_DWC_GSLARGEINT_H

#include "dwc/nonport.h"
// The hash that gsLargeIntAddToMD5 adds a number to
#include "dwc/md5.h"

// GameSpy's large integers (gsLargeInt.c), in overlay 11, which the signature check (dwc/gsCrypt.h) and the
// authentication's keys are made of

// The size of a number's word, in bytes and in bits; the sizeof makes a bit index divided by it unsigned
#define GS_LARGEINT_DIGIT_SIZE_BYTES (sizeof(gsi_u32))
#define GS_LARGEINT_DIGIT_SIZE_BITS (GS_LARGEINT_DIGIT_SIZE_BYTES * 8)

// A number of up to 64 words, the lowest first, and how many of them it uses (GameSpy's gsLargeInt_t)
typedef struct gsLargeInt_s {
    gsi_u32 mLength;
    gsi_u32 mData[0x40];
} gsLargeInt_t;

// The power of a number modulo an odd modulus, which gives gsi_false for a modulus of 0 or an even one
gsi_bool gsLargeIntPowerMod(const gsLargeInt_t *base, const gsLargeInt_t *exponent, const gsLargeInt_t *modulus,
                            gsLargeInt_t *result);
// Sets a number from a text of hexadecimal digits, the most significant first; reverses the order of a number's
// bytes; and adds its bytes, the most significant first, to a hash
gsi_bool gsLargeIntSetFromHexString(gsLargeInt_t *number, const char *hex);
gsi_bool gsLargeIntReverseBytes(gsLargeInt_t *number);
void gsLargeIntAddToMD5(const gsLargeInt_t *number, MD5_CTX *context);

#endif // POKEBW2_DWC_GSLARGEINT_H
