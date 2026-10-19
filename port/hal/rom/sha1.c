// SHA-1, as FIPS 180-4 defines it (section 6.1): 64-byte blocks, five 32-bit words of state, 80 rounds in four groups
// of 20, each with its own function and constant, over a message schedule kept as a ring of 16 words. The platform
// layer checks the player's ROM and its files with it at startup (port/hal/rom/rom.c), its 512 MiB the most of the
// program's start, and the run directory hashes the screens with it at every frame (port/app/run_dir.c); the game's
// own SHA-1 is NitroSDK's (port/hal/math/sha1_block.c). Where the compiler targets the ARMv8 SHA instructions (every
// Apple Silicon Mac), the blocks go through them, nearly three times as fast; elsewhere through the C rounds below.
#include "hal/rom/sha1.h"
#include <string.h>
#if defined(__aarch64__) && defined(__ARM_FEATURE_SHA2)
#include <arm_neon.h>
#define SHA1_ARMV8
#endif

#ifndef SHA1_ARMV8

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// The schedule's word t (t >= 16) from the ring of the last 16
#define SCHEDULE(w, t)                                                                                                 \
    ((w)[(t) & 15] = ROTL((w)[((t) + 13) & 15] ^ (w)[((t) + 8) & 15] ^ (w)[((t) + 2) & 15] ^ (w)[(t) & 15], 1))

// One round with function f and constant k; the five variables rotate through the arguments instead of being moved
#define ROUND(a, b, c, d, e, f, k, wt)                                                                                 \
    do {                                                                                                               \
        (e) += ROTL((a), 5) + (f) + (k) + (wt);                                                                        \
        (b) = ROTL((b), 30);                                                                                           \
    } while (0)

#define CH(b, c, d) (((b) & (c)) | (~(b) & (d)))
#define PARITY(b, c, d) ((b) ^ (c) ^ (d))
#define MAJ(b, c, d) (((b) & (c)) | ((b) & (d)) | ((c) & (d)))

// Five rounds of one group, after which the variables are back in their places
#define ROUNDS5(F, k, w, t, W)                                                                                         \
    do {                                                                                                               \
        ROUND(a, b, c, d, e, F(b, c, d), k, W(w, (t) + 0));                                                            \
        ROUND(e, a, b, c, d, F(a, b, c), k, W(w, (t) + 1));                                                            \
        ROUND(d, e, a, b, c, F(e, a, b), k, W(w, (t) + 2));                                                            \
        ROUND(c, d, e, a, b, F(d, e, a), k, W(w, (t) + 3));                                                            \
        ROUND(b, c, d, e, a, F(c, d, e), k, W(w, (t) + 4));                                                            \
    } while (0)

#define INPUT(w, t) ((w)[(t)])

// Adds one 64-byte block to the state: the 80 rounds over the block's big-endian words
static void ProcessBlock(uint32_t state[5], const uint8_t *block) {
    uint32_t w[16];
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = (uint32_t)block[i * 4] << 24 | (uint32_t)block[i * 4 + 1] << 16 | (uint32_t)block[i * 4 + 2] << 8 |
               (uint32_t)block[i * 4 + 3];
    }
    ROUNDS5(CH, 0x5a827999u, w, 0, INPUT);
    ROUNDS5(CH, 0x5a827999u, w, 5, INPUT);
    ROUNDS5(CH, 0x5a827999u, w, 10, INPUT);
    ROUND(a, b, c, d, e, CH(b, c, d), 0x5a827999u, w[15]);
    ROUND(e, a, b, c, d, CH(a, b, c), 0x5a827999u, SCHEDULE(w, 16));
    ROUND(d, e, a, b, c, CH(e, a, b), 0x5a827999u, SCHEDULE(w, 17));
    ROUND(c, d, e, a, b, CH(d, e, a), 0x5a827999u, SCHEDULE(w, 18));
    ROUND(b, c, d, e, a, CH(c, d, e), 0x5a827999u, SCHEDULE(w, 19));
    ROUNDS5(PARITY, 0x6ed9eba1u, w, 20, SCHEDULE);
    ROUNDS5(PARITY, 0x6ed9eba1u, w, 25, SCHEDULE);
    ROUNDS5(PARITY, 0x6ed9eba1u, w, 30, SCHEDULE);
    ROUNDS5(PARITY, 0x6ed9eba1u, w, 35, SCHEDULE);
    ROUNDS5(MAJ, 0x8f1bbcdcu, w, 40, SCHEDULE);
    ROUNDS5(MAJ, 0x8f1bbcdcu, w, 45, SCHEDULE);
    ROUNDS5(MAJ, 0x8f1bbcdcu, w, 50, SCHEDULE);
    ROUNDS5(MAJ, 0x8f1bbcdcu, w, 55, SCHEDULE);
    ROUNDS5(PARITY, 0xca62c1d6u, w, 60, SCHEDULE);
    ROUNDS5(PARITY, 0xca62c1d6u, w, 65, SCHEDULE);
    ROUNDS5(PARITY, 0xca62c1d6u, w, 70, SCHEDULE);
    ROUNDS5(PARITY, 0xca62c1d6u, w, 75, SCHEDULE);
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void ProcessBlocks(uint32_t state[5], const uint8_t *data, size_t blocks) {
    for (; blocks > 0; blocks--, data += 64) {
        ProcessBlock(state, data);
    }
}

#else

// The ARMv8 Cryptography Extension's SHA-1 (Arm ARM: SHA1C, SHA1P, SHA1M, SHA1H, SHA1SU0, SHA1SU1): four rounds an
// instruction, the state's a to d in one vector. The 80 words of the schedule are 20 vectors, the first 4 the block's,
// each next one SHA1SU1(SHA1SU0(w[g - 4], w[g - 3], w[g - 2]), w[g - 1]); each group of 4 rounds takes its e from
// SHA1H of the group before's a (a rotated by 30), as the rounds pass it down
static void ProcessBlocks(uint32_t state[5], const uint8_t *data, size_t blocks) {
    const uint32x4_t k[4] = { vdupq_n_u32(0x5a827999u), vdupq_n_u32(0x6ed9eba1u), vdupq_n_u32(0x8f1bbcdcu),
                              vdupq_n_u32(0xca62c1d6u) };
    uint32x4_t abcd = vld1q_u32(state);
    uint32_t e = state[4];

    for (; blocks > 0; blocks--, data += 64) {
        uint32x4_t abcdIn = abcd;
        uint32_t eIn = e;
        uint32x4_t w[4];

        for (int i = 0; i < 4; i++) {
            w[i] = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(data + i * 16)));
        }
        // Unrolled whole, so that each group's choices and the ring's slots are constants and the schedule's
        // instructions interleave with the rounds': half the time of the loop
#pragma clang loop unroll(full)
        for (int g = 0; g < 20; g++) {
            uint32x4_t words;
            uint32_t eNext;

            if (g < 4) {
                words = w[g];
            } else {
                words = vsha1su1q_u32(vsha1su0q_u32(w[g & 3], w[(g + 1) & 3], w[(g + 2) & 3]), w[(g + 3) & 3]);
                w[g & 3] = words;
            }
            words = vaddq_u32(words, k[g / 5]);
            eNext = vsha1h_u32(vgetq_lane_u32(abcd, 0));
            if (g < 5) {
                abcd = vsha1cq_u32(abcd, e, words);
            } else if (g >= 10 && g < 15) {
                abcd = vsha1mq_u32(abcd, e, words);
            } else {
                abcd = vsha1pq_u32(abcd, e, words);
            }
            e = eNext;
        }
        abcd = vaddq_u32(abcd, abcdIn);
        e += eIn;
    }
    vst1q_u32(state, abcd);
    state[4] = e;
}

#endif // SHA1_ARMV8

void port_sha1_init(PortSha1 *sha1) {
    sha1->state[0] = 0x67452301u;
    sha1->state[1] = 0xefcdab89u;
    sha1->state[2] = 0x98badcfeu;
    sha1->state[3] = 0x10325476u;
    sha1->state[4] = 0xc3d2e1f0u;
    sha1->length = 0;
    sha1->blockUsed = 0;
}

void port_sha1_update(PortSha1 *sha1, const void *data, size_t size) {
    const uint8_t *bytes = data;

    sha1->length += size;
    if (sha1->blockUsed != 0) {
        size_t take = 64 - sha1->blockUsed;
        if (take > size) {
            take = size;
        }
        memcpy(sha1->block + sha1->blockUsed, bytes, take);
        sha1->blockUsed += take;
        bytes += take;
        size -= take;
        if (sha1->blockUsed < 64) {
            return;
        }
        ProcessBlocks(sha1->state, sha1->block, 1);
        sha1->blockUsed = 0;
    }
    ProcessBlocks(sha1->state, bytes, size / 64);
    bytes += size / 64 * 64;
    size %= 64;
    memcpy(sha1->block, bytes, size);
    sha1->blockUsed = size;
}

// The padding: a 1 bit, zeros to 56 bytes of the last block, and the length in bits, big-endian
void port_sha1_final(PortSha1 *sha1, uint8_t digest[PORT_SHA1_DIGEST_SIZE]) {
    uint64_t bits = sha1->length * 8;
    int i;

    sha1->block[sha1->blockUsed++] = 0x80;
    if (sha1->blockUsed > 56) {
        memset(sha1->block + sha1->blockUsed, 0, 64 - sha1->blockUsed);
        ProcessBlocks(sha1->state, sha1->block, 1);
        sha1->blockUsed = 0;
    }
    memset(sha1->block + sha1->blockUsed, 0, 56 - sha1->blockUsed);
    for (i = 0; i < 8; i++) {
        sha1->block[56 + i] = (uint8_t)(bits >> (56 - 8 * i));
    }
    ProcessBlocks(sha1->state, sha1->block, 1);
    for (i = 0; i < 20; i++) {
        digest[i] = (uint8_t)(sha1->state[i / 4] >> (24 - 8 * (i % 4)));
    }
}

void port_sha1(const void *data, size_t size, uint8_t digest[PORT_SHA1_DIGEST_SIZE]) {
    PortSha1 sha1;

    port_sha1_init(&sha1);
    port_sha1_update(&sha1, data, size);
    port_sha1_final(&sha1, digest);
}
