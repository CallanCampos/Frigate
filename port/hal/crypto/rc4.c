// Host version of NitroSDK's CRYPTO_RC4Encrypt, which the SDK wrote in assembly. The original is in
// lib/nitro/src/crypto/rc4.c, behind #ifndef PLATFORM_PC; CRYPTO_RC4Init there compiles on the host as it is.
// This one reads and writes memory in the original's order, which shows when the output overlaps the input or the
// state: each pass loads the next s[i] before it stores its output byte, and goes on with the value it loaded.
#include "nitro/crypto/rc4.h"

void CRYPTO_RC4Encrypt(CRYPTORC4Context *context, const void *in, u32 length, void *out) {
    const u8 *src = in;
    u8 *dst = out;
    u8 *s = context->s;
    u8 i = context->i + 1;
    u8 j = context->j;
    u8 si = s[i];
    s32 left;

    // The SDK counts length - 1 down as a signed number, so a length of 0, or above 0x80000000, does nothing
    for (left = (s32)(length - 1); left >= 0; left--) {
        u8 sj;
        u8 data;
        u8 key;

        j += si;
        sj = s[j];
        data = *src++;
        s[i] = sj;
        s[j] = si;
        i++;
        key = s[(u8)(si + sj)];
        si = s[i];
        *dst++ = data ^ key;
    }
    context->i = i - 1;
    context->j = j;
}
