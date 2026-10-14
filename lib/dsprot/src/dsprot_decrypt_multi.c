#include "dsprot/dsprot_internal.h"

// DS Protect's decryption of its own code, in the game's copy (overlay 337): the code of dsprot_decrypt.c without the
// .ctor entry, since DSProt_Init runs the decryption here. DSProt_DecryptMulti decrypts the functions a list names; the stubs
// of DSProt_Call* have DSProt_DispatchMulti decrypt a function for one call and encrypt it again, with a key that changes
// with each call. The ROM doesn't name this file; it is named for what it does.

// The address of this array stands for nothing: lengths and keys are made from it, so that no plain length of code is
// in the ROM
u8 DSProt_WorkMulti[0x14];

static const u32 sDecryptFunctionMulti = (u32)(DSProt_DecryptFunctionMulti + 0x2200);
static const u32 sEncryptFunctionMulti = (u32)(DSProt_EncryptFunctionMulti + 0x2200);

// Decrypts each function of the list, the pairs of a function and its length that follow the three instructions the
// caller reached it with, and erases both. The words are chained: each one is xored with a value made from the words
// before it.
void DSProt_DecryptMulti(u32 *list) {
    u32 *head = list - 3;

    head[2] = 0;
    head[1] = 0;
    head[0] = 0;
    do {
        u32 *code;
        u32 *end;
        u32 key;

        key = 0x0a471abb;
        code = (u32 *)(list[0] - 0x2200);
        end = code + ((list[1] - (u32)DSProt_WorkMulti - 0x2200) >> 2);

        while (code < end) {
            u32 word = *code ^ key;

            *code = word;
            code++;
            key ^= word - (word >> 8);
        }
        list[1] = 0;
        list[0] = 0;
        list += 2;
    } while (*list != 0);
    DSProt_FlushCache();
}

// Decrypts the code of a function, whose length is the distance from DSProt_WorkMulti to `end`, with a key made from `key`
// and the length. Returns the function's address.
u32 DSProt_DecryptFunctionMulti(u32 key, u32 code, u32 end) {
    u32 words[4];

    end -= (u32)DSProt_WorkMulti + 0x2200;
    code -= 0x2200;
    words[0] = key ^ end;
    words[1] = end ^ ((key << 8) | (key >> 24));
    words[2] = end ^ ((key << 16) | (key >> 16));
    words[3] = end ^ ((key << 24) | (key >> 8));
    ((int (*)(u32 *, u32, u32, u32))(DSProt_DecryptCodeEntryMulti - 0x2200))(words, code, code, end);
    DSProt_FlushCache();
    return code;
}

// The same with the cipher run forwards. Returns the key for the next call: it is made from the code's address and
// the key, so the stub keeps changing the word that holds it.
u32 DSProt_EncryptFunctionMulti(u32 key, u32 code, u32 end) {
    u32 words[4];
    u8 *dest = (u8 *)code;

    end -= (u32)DSProt_WorkMulti + 0x2200;
    dest -= 0x2200;
    words[0] = key ^ end;
    words[1] = end ^ ((key << 8) | (key >> 24));
    words[2] = end ^ ((key << 16) | (key >> 16));
    words[3] = end ^ ((key << 24) | (key >> 8));
    ((int (*)(u32 *, u8 *, u8 *, u32))(DSProt_EncryptCodeEntryMulti - 0x2200))(words, dest, dest, end);
    DSProt_FlushCache();
    return key + (((u32)DSProt_WorkMulti + 0x2200) + (code & 0xffff));
}

// Hand-written: the end of every DSProt_Call* stub, which is given the address of the stub's data in ip. It has
// DSProt_DecryptFunctionMulti decrypt the stub's function, calls it with the stub's own arguments and has
// DSProt_EncryptFunctionMulti encrypt it again.
asm void DSProt_DispatchMulti(void) {
    stmdb sp!, {r0, r1, r2, r3}
    str r10, [ip, #0x10]
    mov r10, ip
    ldr ip, =sDecryptFunctionMulti
    str lr, [r10, #0]
    ldr ip, [ip, #0]
    ldmib r10, {r0, r1, r2}
    sub ip, ip, #0x2200
    blx ip
    mov ip, r0
    ldmia sp!, {r0, r1, r2, r3}
    blx ip
    ldr ip, =sEncryptFunctionMulti
    stmdb sp!, {r4}
    ldr ip, [ip, #0]
    mov r4, r0
    sub ip, ip, #0x2200
    ldmib r10, {r0, r1, r2}
    blx ip
    str r0, [r10, #4]
    mov r0, r4
    ldmia sp!, {r4}
    ldr lr, [r10, #0]
    str sp, [r10, #0]
    ldr r10, [r10, #0x10]
    bx lr
}

// Hand-written, run from .ctor when the overlay is loaded: has DSProt_DecryptMulti decrypt this file's functions, other
// than DSProt_DecryptMulti
asm void DSProt_DecryptDispatchMulti(void) {
    orr r0, pc, #0
    adds r0, r0, #4
    bne DSProt_DecryptMulti
    dcd DSProt_EncryptFunctionMulti + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0xd4
    dcd DSProt_DecryptFunctionMulti + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0xbc
    dcd DSProt_DispatchMulti + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x68
    dcd 0
    dcd 0
}
