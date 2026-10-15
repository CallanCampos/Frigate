#ifndef POKEBW2_DSPROT_DSPROT_INTERNAL_H
#define POKEBW2_DSPROT_DSPROT_INTERNAL_H

#include "types.h"
#include "dsprot/dsprot.h"

// The parts of DS Protect that call each other. Its code refers to functions by their address plus 0x2200, and to
// lengths by DSProt_Work's address plus 0x2200 plus the length, so that no plain address of its code is in the ROM.
// Each DSProt_Call* function is a hand-written stub that verifies itself, then has DSProt_Dispatch decrypt the
// function it stands for, run it and encrypt it again.

// Writes the data cache back and invalidates it, then invalidates the instruction cache, so that code just decrypted
// or encrypted is what the CPU runs. DS Protect expands this sequence of CP15 operations in place, which C cannot
// write, so it is the one inline assembly of the library. It uses ip and r0 to r2.
// clang-format off
static inline void DSProt_FlushCache(void) {
    asm {
        mov ip, #0
        mov r1, #0
    @1: mov r0, #0
    @2: orr r2, r1, r0
        mcr p15, 0, ip, c7, c10, 4
        mcr p15, 0, r2, c7, c14, 2
        add r0, r0, #0x20
        cmp r0, #0x400
        blt @2
        add r1, r1, #0x40000000
        cmp r1, #0
        bne @1
        mov r0, #0
        mcr p15, 0, r0, c7, c5, 0
        mcr p15, 0, ip, c7, c10, 4
    }
}
// clang-format on

// dsprot_run.c
asm void DSProt_Crash(void);
void DSProt_ClearMemory(void *dest, u32 size);
asm void *DSProt_CallCrash(void *arg0, void *arg1);
void *DSProt_RunChecks(DSProtCallback callback, void *arg0, void *arg1);
asm void DSProt_DecryptRun(void);

// dsprot_stub_check.c
u32 DSProt_CheckEmulatorStub(void);
u32 DSProt_CheckFlashcartStub(void);
asm u32 DSProt_CallCheckEmulatorStub(void);
asm u32 DSProt_CallCheckFlashcartStub(void);
asm void DSProt_DecryptStubCheck(void);

// dsprot_decrypt.c
extern u8 DSProt_Work[];
void DSProt_Decrypt(u32 *list);
u32 DSProt_EncryptFunction(u32 key, u32 code, u32 end);
u32 DSProt_DecryptFunction(u32 key, u32 code, u32 end);
asm void DSProt_Dispatch(void);
asm void DSProt_DecryptDispatch(void);

// dsprot_cipher.c: a stream cipher over the words of code, keyed by 16 bytes. DSProt_DecryptCodeEntry and
// DSProt_EncryptCodeEntry hold the address of DSProt_DecryptCodeWithKey and DSProt_EncryptCodeWithKey plus 0x2200, and
// sCipherFunctions the address of the others in the order below.
typedef struct DSProtCipherState {
    u32 offset;
    u32 i;
    u32 j;
    u8 table[256];
} DSProtCipherState;

extern const u32 DSProt_DecryptCodeEntry;
extern const u32 DSProt_EncryptCodeEntry;
u32 DSProt_GetBranchType(u32 instruction);
void DSProt_Rc4Init(DSProtCipherState *state, const u8 *key, u32 keyLength);
u32 DSProt_Rc4Next(DSProtCipherState *state);
int DSProt_EncryptCode(DSProtCipherState *state, u8 *src, u8 *dest, u32 size);
int DSProt_DecryptCode(DSProtCipherState *state, u8 *src, u8 *dest, u32 size);
int DSProt_EncryptCodeWithKey(const u8 *key, u8 *src, u8 *dest, u32 size);
int DSProt_DecryptCodeWithKey(const u8 *key, u8 *src, u8 *dest, u32 size);
asm void DSProt_DecryptCipher(void);

// dsprot_detect.c
u32 DSProt_CheckEmulator(void);
u32 DSProt_Crc32(const u8 *data, u32 size);
u32 DSProt_CheckFlashcart(void);
asm void DSProt_DecryptDetect(void);
asm u32 DSProt_CallCheckFlashcart(void);
asm u32 DSProt_CallCheckEmulator(void);

// The game's copy of DS Protect (overlay 337) is made of the same parts, with four of the runners, stub checks and
// detection checks; its checks report through the arguments of the runner that called them
typedef struct DSProtCheckArgs {
    DSProtCallback nop;    // called with arg0 and arg1 if all checks pass
    DSProtCallback tamper; // called with arg0 and arg1 if a check fails
    void *arg0;
    void *arg1;
    void *result; // what tamper returned
    u32 failed;   // nonzero once tamper has run
} DSProtCheckArgs;

// dsprot_stub_check_multi.c
u32 DSProt_CheckEmulatorAStub(void);
u32 DSProt_CheckEmulatorBStub(void);
u32 DSProt_CheckFlashcartAStub(void);
u32 DSProt_CheckFlashcartBStub(void);
asm u32 DSProt_CallCheckEmulatorAStub(void);
asm u32 DSProt_CallCheckEmulatorBStub(void);
asm u32 DSProt_CallCheckFlashcartAStub(void);
asm u32 DSProt_CallCheckFlashcartBStub(void);
asm void DSProt_DecryptStubCheckMulti(void);

// dsprot_run_multi.c
void *DSProt_RunChecks1(void *arg0, void *arg1);
void *DSProt_RunChecks2(void *arg0, void *arg1);
void *DSProt_RunChecks3(void *arg0, void *arg1);
void *DSProt_RunChecks4(void *arg0, void *arg1);
asm void *DSProt_CallRunChecks1(void *arg0, void *arg1);
asm void *DSProt_CallRunChecks2(void *arg0, void *arg1);
asm void *DSProt_CallRunChecks3(void *arg0, void *arg1);
asm void *DSProt_CallRunChecks4(void *arg0, void *arg1);
asm void DSProt_Init(void);

// dsprot_detect_multi.c
u32 DSProt_CheckEmulatorA(DSProtCheckArgs *args);
u32 DSProt_CheckEmulatorB(DSProtCheckArgs *args);
u32 DSProt_CheckFlashcartA(DSProtCheckArgs *args);
u32 DSProt_CheckFlashcartB(DSProtCheckArgs *args);
asm u32 DSProt_CallCheckEmulatorA(DSProtCheckArgs *args);
asm u32 DSProt_CallCheckEmulatorB(DSProtCheckArgs *args);
asm u32 DSProt_CallCheckFlashcartA(DSProtCheckArgs *args);
asm u32 DSProt_CallCheckFlashcartB(DSProtCheckArgs *args);
asm void DSProt_DecryptDetectMulti(void);

// The game's copy (overlay 337) has its own copies of the decryption and cipher code, under these names
extern u8 DSProt_WorkMulti[];
void DSProt_DecryptMulti(u32 *list);
u32 DSProt_EncryptFunctionMulti(u32 key, u32 code, u32 end);
u32 DSProt_DecryptFunctionMulti(u32 key, u32 code, u32 end);
asm void DSProt_DispatchMulti(void);
asm void DSProt_DecryptDispatchMulti(void);
extern const u32 DSProt_DecryptCodeEntryMulti;
extern const u32 DSProt_EncryptCodeEntryMulti;
u32 DSProt_GetBranchTypeMulti(u32 instruction);
void DSProt_Rc4InitMulti(DSProtCipherState *state, const u8 *key, u32 keyLength);
u32 DSProt_Rc4NextMulti(DSProtCipherState *state);
int DSProt_EncryptCodeMulti(DSProtCipherState *state, u8 *src, u8 *dest, u32 size);
int DSProt_DecryptCodeMulti(DSProtCipherState *state, u8 *src, u8 *dest, u32 size);
int DSProt_EncryptCodeWithKeyMulti(const u8 *key, u8 *src, u8 *dest, u32 size);
int DSProt_DecryptCodeWithKeyMulti(const u8 *key, u8 *src, u8 *dest, u32 size);
asm void DSProt_DecryptCipherMulti(void);
u32 DSProt_Crc32Multi(const u8 *data, u32 size);

// What the checks call in the main module, which swan has not named: the game card's ROM header, the lock of the
// card, and a read of its ROM (the source offset, the destination and the length, then the callback, its argument and
// whether to read asynchronously)
const u8 *func_0206eed8(void);
void func_0206eef0(u16 lockId);
void func_0206ef38(u16 lockId);
int func_0206fd24(int dma, u32 source, void *dest, u32 length, void *callback, void *callbackArg, int async);

// One bit of DS Protect's CRC-32, which applies the polynomial to the other half of the values: DSProt_Crc32 and
// DSProt_Crc32Multi write out a byte's eight steps
#define DSPROT_CRC32_STEP(crc)                                                                                         \
    do {                                                                                                               \
        if ((crc) & 1) {                                                                                               \
            (crc) = (crc) >> 1;                                                                                        \
        } else {                                                                                                       \
            (crc) = ((crc) >> 1) ^ 0xedb88320;                                                                         \
        }                                                                                                              \
    } while (0)

#endif // POKEBW2_DSPROT_DSPROT_INTERNAL_H
