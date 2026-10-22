#include "dwc/sb_crypt.h"

// The cipher of GameSpy's server browser (sb_crypt.c), a variant of Michael Paul Johnson's card-shuffling cipher. The
// ROM names neither the file nor its functions: the file, keyrand, GOACryptInit and GOADecrypt are GameSpy's, as
// pret's pokerevo Wii build has them (sb_crypt.s), and GOAHashInit and GOADecryptByte after the later SDK

static unsigned char keyrand(GOACryptState *state, int limit, unsigned char *user_key, unsigned char keysize,
                             unsigned char *rsum, unsigned *keypos);

// A number from 0 up to limit, from the permutation and the next bytes of the key
static unsigned char keyrand(GOACryptState *state, int limit, unsigned char *user_key, unsigned char keysize,
                             unsigned char *rsum, unsigned *keypos) {
    gsi_u32 u;
    gsi_u32 retry_limiter;
    gsi_u32 mask;

    if (limit == 0) {
        return 0;
    }
    retry_limiter = 0;
    mask = 1;
    while (mask < limit) {
        mask = (mask << 1) + 1;
    }
    do {
        *rsum = state->cards[*rsum] + user_key[(*keypos)++];
        if (*keypos >= keysize) {
            *keypos = 0;
            *rsum += keysize;
        }
        u = mask & *rsum;
        if (++retry_limiter > 11) {
            u %= limit;
        }
    } while (u > limit);
    return u;
}

void GOAHashInit(GOACryptState *state) {
    int i;
    int j;

    state->rotor = 1;
    state->ratchet = 3;
    state->avalanche = 5;
    state->last_plain = 7;
    state->last_cipher = 11;
    for (i = 0, j = 255; i < 256; i++, j--) {
        state->cards[i] = j;
    }
}

void GOACryptInit(GOACryptState *state, unsigned char *key, unsigned char keysize) {
    int i;
    gsi_u8 toswap;
    gsi_u8 swaptemp;
    gsi_u8 rsum;
    gsi_u32 keypos;

    if (keysize < 1) {
        GOAHashInit(state);
        return;
    }
    for (i = 0; i < 256; i++) {
        state->cards[i] = i;
    }
    toswap = 0;
    keypos = 0;
    rsum = 0;
    for (i = 255; i >= 0; i--) {
        toswap = keyrand(state, i, key, keysize, &rsum, &keypos);
        swaptemp = state->cards[i];
        state->cards[i] = state->cards[toswap];
        state->cards[toswap] = swaptemp;
    }
    state->rotor = state->cards[1];
    state->ratchet = state->cards[3];
    state->avalanche = state->cards[5];
    state->last_plain = state->cards[7];
    state->last_cipher = state->cards[rsum];
    // Clears what is left of the key
    toswap = swaptemp = rsum = 0;
    keypos = 0;
}

unsigned char GOADecryptByte(GOACryptState *state, unsigned char b) {
    gsi_u8 swaptemp;

    state->ratchet += state->cards[state->rotor++];
    swaptemp = state->cards[state->last_cipher];
    state->cards[state->last_cipher] = state->cards[state->ratchet];
    state->cards[state->ratchet] = state->cards[state->last_plain];
    state->cards[state->last_plain] = state->cards[state->rotor];
    state->cards[state->rotor] = swaptemp;
    state->avalanche += state->cards[swaptemp];
    state->last_plain = b ^ state->cards[(state->cards[state->avalanche] + state->cards[state->rotor]) & 0xff] ^
                        state->cards[state->cards[(state->cards[state->last_plain] + state->cards[state->last_cipher] +
                                                   state->cards[state->ratchet]) &
                                                  0xff]];
    state->last_cipher = b;
    return state->last_plain;
}

void GOADecrypt(GOACryptState *state, unsigned char *bp, int len) {
    int i;

    for (i = 0; i < len; i++) {
        bp[i] = GOADecryptByte(state, bp[i]);
    }
}
