// SHA-1, as FIPS 180-4 defines it (section 6.1): 64-byte blocks, five 32-bit words of state, 80 rounds in four groups
// of 20, each with its own function and constant, over a message schedule kept as a ring of 16 words. The platform
// layer checks the player's ROM and its files with it at startup (port/hal/rom/rom.c), and the run directory hashes
// the screens with it (port/app/run_dir.c); the game's own SHA-1 is NitroSDK's (port/hal/math/sha1_block.c).
#include "hal/rom/sha1.h"
#include <string.h>

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
        ProcessBlock(sha1->state, sha1->block);
        sha1->blockUsed = 0;
    }
    for (; size >= 64; bytes += 64, size -= 64) {
        ProcessBlock(sha1->state, bytes);
    }
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
        ProcessBlock(sha1->state, sha1->block);
        sha1->blockUsed = 0;
    }
    memset(sha1->block + sha1->blockUsed, 0, 56 - sha1->blockUsed);
    for (i = 0; i < 8; i++) {
        sha1->block[56 + i] = (uint8_t)(bits >> (56 - 8 * i));
    }
    ProcessBlock(sha1->state, sha1->block);
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
