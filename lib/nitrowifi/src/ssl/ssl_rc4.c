#include "ssl_internal.h"

// SSL's own RC4, a plain C rewrite of the same algorithm as NitroSDK's hand-written lib/nitro/src/crypto/rc4.c.
// pokediamond's NitroWiFi names the object SSL_rca4.o and its two functions CPSi_rc4_init and CPSi_rc4_crypt

void CPSi_rc4_init(CPSRc4Context *context, const u8 *key, int keyLength) {
    int i;
    u8 *s;
    u8 j;
    u8 keyIndex;

    context->i = 0;
    context->j = 0;
    s = context->s;
    for (i = 0; i < 256; i++) {
        s[i] = (u8)i;
    }

    keyIndex = 0;
    j = 0;
    for (i = 0; i < 256; i++) {
        u8 t = s[i];
        u8 tk = (u8)(t + key[keyIndex]);

        j = (u8)(j + tk);
        s[i] = s[j];
        s[j] = t;
        keyIndex++;
        if (keyIndex >= keyLength) {
            keyIndex = 0;
        }
    }
}

void CPSi_rc4_crypt(CPSRc4Context *context, void *buf, int length) {
    u8 *p = buf;
    int n;
    u8 i = context->i;
    u8 j = context->j;
    u8 *s = context->s;

    for (n = 0; n < length; n++) {
        u8 t, u;

        i++;
        t = s[i];
        j = (u8)(j + t);
        u = s[j];
        s[i] = u;
        s[j] = t;
        p[n] ^= s[(u8)(t + u)];
    }

    context->i = i;
    context->j = j;
}
