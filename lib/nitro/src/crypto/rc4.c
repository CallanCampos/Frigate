// NitroSDK's RC4 (libcrypto's rc4.c), linked at the end of the game's own code in ARM9 main. The names are the SDK's;
// CRYPTO_RC4Encrypt was hand-written assembly, so it is an asm function here.
#include "nitro/crypto/rc4.h"

// The longest key used: the rest of a longer one is ignored
#define RC4_KEY_LEN_MAX 16

void CRYPTO_RC4Init(CRYPTORC4Context *context, const void *key, u32 key_len) {
    const u8 *keyBytes = key;
    u8 *s;
    int i;
    u8 k;
    u8 j;

    if (key_len > RC4_KEY_LEN_MAX) {
        key_len = RC4_KEY_LEN_MAX;
    }
    context->i = 0;
    context->j = 0;
    s = context->s;
    for (i = 0; i < 256; i++) {
        s[i] = i;
    }
    j = 0;
    k = 0;
    for (i = 0; i < 256; i++) {
        u8 t = s[i];

        j = j + (t + keyBytes[k]);
        k++;
        if (k == (u8)key_len) {
            k = 0;
        }
        s[i] = s[j];
        s[j] = t;
    }
}

// Hand-written in assembly: the counters i and j are kept in the top byte of their registers, so that adding to them
// wraps at 256 by itself, the next s[i] is loaded before the loop and at the end of each pass, and the state is
// stored back through the context pointer advanced to s. MWCC can't produce this (it pads the odd register list
// with r3, among others)
asm void CRYPTO_RC4Encrypt(CRYPTORC4Context *context, const void *in, u32 length, void *out) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, lr}
    ldrb r7, [r0, #0]
    ldrb r6, [r0, #1]
    add r0, r0, #4
    mov lr, #0x1000000
    add r7, lr, r7, lsl #24
    mov r6, r6, lsl #24
    ldrb r9, [r0, r7, lsr #24]
    subs r2, r2, #1
    bmi @end
@loop:
    add r6, r6, r9, lsl #24
    subs r2, r2, #1
    ldrb r8, [r0, r6, lsr #24]
    ldrb r5, [r1], #1
    strb r8, [r0, r7, lsr #24]
    strb r9, [r0, r6, lsr #24]
    add r4, r9, r8
    and r4, r4, #0xff
    add r7, r7, lr
    ldrb r4, [r0, r4]
    ldrb r9, [r0, r7, lsr #24]
    eor r5, r5, r4
    strb r5, [r3], #1
    bge @loop
@end:
    sub r7, r7, lr
    mov r7, r7, lsr #24
    mov r6, r6, lsr #24
    strb r7, [r0, #-4]
    strb r6, [r0, #-3]
    ldmia sp!, {r4, r5, r6, r7, r8, r9, pc}
}
