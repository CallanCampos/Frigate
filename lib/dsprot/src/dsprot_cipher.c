#include "dsprot/dsprot_internal.h"

// The cipher DS Protect encrypts its code with, in overlay 165: a variant of RC4 over the words of ARM code, which
// treats the bytes of branch instructions apart. The ROM doesn't name this file; it is named for what it does.

typedef u32 (*DSProtBranchTypeFunc)(u32 instruction);
typedef u32 (*DSProtNextFunc)(DSProtCipherState *state);
typedef void (*DSProtInitFunc)(DSProtCipherState *state, const u8 *key, u32 keyLength);
typedef s32 (*DSProtCodeFunc)(DSProtCipherState *state, u8 *src, u8 *dest, u32 size);

static const u32 sCipherFunctions[5] = {
    (u32)(DSProt_GetBranchType + 0x2200), (u32)(DSProt_EncryptCode + 0x2200), (u32)(DSProt_DecryptCode + 0x2200),
    (u32)(DSProt_Rc4Next + 0x2200),       (u32)(DSProt_Rc4Init + 0x2200),
};

// What a word of code is, by the top byte of the instruction: 1 for a branch with link and exchange, 2 for a branch
// with link, 3 for a branch, 0 for the rest
u32 DSProt_GetBranchType(u32 instruction) {
    u32 byte = (u8)(instruction >> 24);

    if ((byte & 0xe) == 0xa) {
        if ((byte & 0xf0) == 0xf0) {
            return 1;
        }
        if (byte & 1) {
            return 2;
        }
        return 3;
    }
    return 0;
}

void DSProt_Rc4Init(DSProtCipherState *state, const u8 *key, u32 keyLength) {
    // The order of these declarations sets the locals' registers
    int i;
    u32 j;
    u32 keyIndex;
    u32 *tableEnd;
    u32 value;
    u32 *table;

    state->offset = 0xaa;
    state->i = 0;
    state->j = 0;
    keyIndex = 0;
    j = 0;
    value = 0x03020100;
    table = (u32 *)state->table;
    tableEnd = (u32 *)(state->table + 256);
    do {
        *table = value;
        table++;
        value += 0x04040404;
    } while (table < tableEnd);
    for (i = 255; i >= 0; i--) {
        u8 temp;

        j = (state->table[i] + (j + key[keyIndex])) & 0xff;
        temp = state->table[j];
        state->table[j] = state->table[i];
        state->table[i] = temp;
        keyIndex++;
        if (keyIndex >= keyLength) {
            keyIndex = 0;
        }
    }
}

u32 DSProt_Rc4Next(DSProtCipherState *state) {
    u32 i = (state->i + 1 + state->offset) & 0xff;
    u8 atI = state->table[i];
    u32 j = (atI + state->j + state->offset) & 0xff;
    u8 atJ = state->table[j];

    state->i = i;
    state->j = j;
    state->table[j] = atI;
    state->table[i] = atJ;
    return state->table[(atI + atJ) & 0xff];
}

s32 DSProt_EncryptCode(DSProtCipherState *state, u8 *src, u8 *dest, u32 size) {
    u32 i;
    u8 sum;

    sum = 0;
    if (size & 3) {
        return -1;
    }
    for (i = 0; i < size; i += 4) {
        switch (((DSProtBranchTypeFunc)(sCipherFunctions[0] - 0x2200))(*(u32 *)(src + i))) {
        case 1:
            src[i + 3] ^= 1;
        case 2:
            *(u32 *)(dest + i) =
                ((*(u32 *)(src + i) & 0xff000000) ^ 0x1000000) | (((*(u32 *)(src + i) & 0xffffff) + 0x882) & 0xffffff);
            break;
        case 3:
            *(u32 *)(src + i) ^= 0x1000000;
        default: {
            u32 byte;

            byte = src[i] ^ ((DSProtNextFunc)(sCipherFunctions[3] - 0x2200))(state);
            state->offset = byte;
            dest[i] = byte;
            byte = src[i + 1] ^ ((DSProtNextFunc)(sCipherFunctions[3] - 0x2200))(state);
            state->offset = byte;
            dest[i + 1] = byte;
            byte = src[i + 2] ^ ((DSProtNextFunc)(sCipherFunctions[3] - 0x2200))(state);
            state->offset = byte;
            dest[i + 2] = byte;
            break;
        }
        }
        // The running sum is the byte just written, so it wraps at 8 bits
        dest[i + 3] = sum + src[i + 3];
        sum = dest[i + 3];
        state->offset -= sum;
    }
    return 0;
}

// Decrypts one byte with the next value of the cipher, and keeps the encrypted byte as the state's offset
static inline u32 DSProt_DecryptByte(DSProtCipherState *state, u32 byte) {
    u32 out = byte ^ ((DSProtNextFunc)(sCipherFunctions[3] - 0x2200))(state);

    state->offset = byte;
    return out;
}

s32 DSProt_DecryptCode(DSProtCipherState *state, u8 *src, u8 *dest, u32 size) {
    u32 i;
    u8 sum;

    sum = 0;
    if (size & 3) {
        return -1;
    }
    for (i = 0; i < size; i += 4) {
        u32 last = src[i + 3];
        u32 word;

        src[i + 3] = last - sum;
        sum = last;
        word = *(u32 *)(src + i);
        switch (((DSProtBranchTypeFunc)(sCipherFunctions[0] - 0x2200))(word)) {
        case 1:
            src[i + 3] ^= 1;
        case 3:
            *(u32 *)(dest + i) = ((word & 0xff000000) ^ 0x1000000) | (((word & 0xffffff) + 0xfffff77e) & 0xffffff);
            break;
        case 2:
            dest[i] = DSProt_DecryptByte(state, src[i]);
            dest[i + 1] = DSProt_DecryptByte(state, src[i + 1]);
            dest[i + 2] = DSProt_DecryptByte(state, src[i + 2]);
            dest[i + 3] = src[i + 3] ^ 1;
            break;
        default:
            dest[i] = DSProt_DecryptByte(state, src[i]);
            dest[i + 1] = DSProt_DecryptByte(state, src[i + 1]);
            dest[i + 2] = DSProt_DecryptByte(state, src[i + 2]);
            dest[i + 3] = src[i + 3];
            break;
        }
        state->offset -= last;
    }
    return 0;
}

s32 DSProt_EncryptCodeWithKey(const u8 *key, u8 *src, u8 *dest, u32 size) {
    DSProtCipherState state;

    ((DSProtInitFunc)(sCipherFunctions[4] - 0x2200))(&state, key, 16);
    return ((DSProtCodeFunc)(sCipherFunctions[1] - 0x2200))(&state, src, dest, size) == -1 ? -1 : 0;
}

s32 DSProt_DecryptCodeWithKey(const u8 *key, u8 *src, u8 *dest, u32 size) {
    DSProtCipherState state;

    ((DSProtInitFunc)(sCipherFunctions[4] - 0x2200))(&state, key, 16);
    return ((DSProtCodeFunc)(sCipherFunctions[2] - 0x2200))(&state, src, dest, size) == -1 ? -1 : 0;
}

// Hand-written, run from .ctor when the overlay is loaded: has DSProt_Decrypt decrypt this file's functions
// clang-format off
asm void DSProt_DecryptCipher(void) {
    orr r0, pc, #0
    adds r0, r0, #4
    bne DSProt_Decrypt
    dcd DSProt_Rc4Init + 0x2200
    dcd DSProt_Work + 0x2200 + 0x84
    dcd DSProt_EncryptCode + 0x2200
    dcd DSProt_Work + 0x2200 + 0x144
    dcd DSProt_DecryptCode + 0x2200
    dcd DSProt_Work + 0x2200 + 0x1ac
    dcd DSProt_EncryptCodeWithKey + 0x2200
    dcd DSProt_Work + 0x2200 + 0x74
    dcd DSProt_DecryptCodeWithKey + 0x2200
    dcd DSProt_Work + 0x2200 + 0x74
    dcd DSProt_Rc4Next + 0x2200
    dcd DSProt_Work + 0x2200 + 0x58
    dcd DSProt_GetBranchType + 0x2200
    dcd DSProt_Work + 0x2200 + 0x3c
    dcd 0
    dcd 0
}
// clang-format on

#pragma define_section ctor ".ctor"                                                                                    \
                            ".ctor"                                                                                    \
                            ".ctor" abs32 RW
__declspec(ctor) static void (*const sDecryptCipher)(void) = DSProt_DecryptCipher;
