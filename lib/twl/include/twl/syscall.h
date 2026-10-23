#ifndef POKEBW2_TWL_SYSCALL_H
#define POKEBW2_TWL_SYSCALL_H

#include "types.h"

// TwlSDK's system call library (libsyscall.twl, one object, syscall_twl.o) for the DSi's BIOS calls that the DS does
// not have: the SHA-1 and HMAC-SHA-1 of the DSi's BIOS. The object has a half in ARM9 main, the wrappers below, and
// another in the LTD autoload, the cores that they call (the BIOS calls themselves, and an HMAC built on them). A
// wrapper ends the program if the console is not a DSi (OSi_IsRunOnTwl, which asks OS_IsRunOnTwl), and calls its core
// otherwise. The functions
// have the SDK's names, and the cores in the LTD autoload keep their default names

// The state of a SHA-1 computation, in the layout of the TWL-SDK Function Reference Manual (SVC_SHA1Init), of which the
// BIOS keeps the first 0x60 bytes. The library's init core clears sha_block, and the HMAC's context continues after it
// (the key at 0x64, its length at 0xa4)
typedef struct SVCSHA1Context {
    u32 h0, h1, h2, h3, h4;
    u32 Nl, Nh;
    u32 data[16];
    int num;
    void (*sha_block)(struct SVCSHA1Context *c, const u8 *W, int num);
} SVCSHA1Context;

// syscall_twl.c, the half in ARM9 main
void SVC_SHA1Init(SVCSHA1Context *context); // swan's dsiJump_27696E9
// Adds len bytes to the hash
void SVC_SHA1Update(SVCSHA1Context *context, const void *input, u32 len);
// Writes the 20-byte digest of what the context has been given
void SVC_SHA1GetHash(SVCSHA1Context *context, void *digest);
// The 20-byte HMAC-SHA-1 of bin_len bytes under a key
void SVC_CalcHMACSHA1(void *digest, const void *bin_ptr, u32 bin_len, const void *key_ptr, u32 key_len);

// syscall_twl_ltd.c, the half in the LTD autoload; the wrappers call these cores
// The cores of SVC_SHA1Init, SVC_SHA1Update and SVC_SHA1GetHash: the first clears the context's sha_block, and
// each calls the BIOS (system calls 0x24, 0x25 and 0x26). The last takes the digest after the context, as the BIOS
// does not
void func_027696e8(SVCSHA1Context *context);
void func_027696f4(SVCSHA1Context *context, const void *input, u32 len);
void func_027696f8(SVCSHA1Context *context, void *digest);
// The core of SVC_CalcHMACSHA1, ARM code
void func_027698b4(void *digest, const void *bin_ptr, u32 bin_len, const void *key_ptr, u32 key_len);

#endif // POKEBW2_TWL_SYSCALL_H
