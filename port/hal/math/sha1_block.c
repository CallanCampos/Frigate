// Host version of NitroSDK's SHA-1 compression, sha1_block, which the SDK wrote in assembly. The original is in
// lib/nitro/src/math/sha1_block.c, which the host build doesn't compile. It adds the hash of the context's 64-byte
// block to the five state words and changes nothing else: the block is read as big-endian words, expanded in a ring
// of 16 words, and run through the 80 rounds.
#include "nitro/math.h"

// The round constants of the four groups of 20 rounds, which the SDK keeps in the 16 bytes in front of sha1_block
// (sha1_block_constants)
static const u32 sRoundConstants[4] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

static u32 RotateLeft(u32 x, int count) {
    return (x << count) | (x >> (32 - count));
}

void sha1_block(MATHSHA1Context *context) {
    u32 words[16];
    u32 a, b, c, d, e;
    int t;

    for (t = 0; t < 16; t++) {
        const u8 *bytes = &context->block.bytes[4 * t];

        words[t] = ((u32)bytes[0] << 24) | ((u32)bytes[1] << 16) | ((u32)bytes[2] << 8) | bytes[3];
    }
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    for (t = 0; t < 80; t++) {
        u32 f;
        u32 sum;

        // From round 16, each round's word replaces the one 16 rounds back in the ring
        if (t >= 16) {
            words[t & 15] =
                RotateLeft(words[(t - 16) & 15] ^ words[(t - 14) & 15] ^ words[(t - 8) & 15] ^ words[(t - 3) & 15], 1);
        }
        if (t < 20) {
            f = (b & c) | (~b & d);
        } else if (t < 40) {
            f = b ^ c ^ d;
        } else if (t < 60) {
            f = (b & c) | (b & d) | (c & d);
        } else {
            f = b ^ c ^ d;
        }
        sum = f + sRoundConstants[t / 20] + e + words[t & 15] + RotateLeft(a, 5);
        e = d;
        d = c;
        c = RotateLeft(b, 30);
        b = a;
        a = sum;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
}
