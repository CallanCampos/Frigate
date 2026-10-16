#ifndef POKEBW2_TWL_SYSCALL_H
#define POKEBW2_TWL_SYSCALL_H

#include "types.h"

// TwlSDK's system call library (libsyscall.twl, one object, syscall_twl.o) for the DSi's BIOS calls that the DS does
// not have: the SHA-1 and HMAC-SHA-1 of the DSi's BIOS. The object has a half in ARM9 main, the wrappers below, and
// another in the LTD autoload, the cores that they call (the BIOS calls themselves, and an HMAC built on them). A
// wrapper ends the program if the console is not a DSi (OSi_IsRunOnTwl), and calls its core otherwise. The functions
// keep their default names, swan's where it has one; the comments give the SDK's

// The state of a SHA-1 computation, of which the BIOS keeps the first 0x60 bytes. The library's init core clears the
// word after them, and the HMAC's context continues after it (the key at 0x64, its length at 0xa4)
typedef struct {
    u8 unk_00[0x60];
    u32 unk_60;
} SVCSHA1Context;

// syscall_twl.c, the half in ARM9 main
// SVC_SHA1Init
void dsiJump_27696E9(SVCSHA1Context *context);
// SVC_SHA1Update: adds len bytes to the hash
void func_02082820(SVCSHA1Context *context, const void *input, u32 len);
// SVC_SHA1GetHash: writes the 20-byte digest of what the context has been given
void func_02082850(SVCSHA1Context *context, void *digest);
// SVC_CalcHMACSHA1: the 20-byte HMAC-SHA-1 of bin_len bytes under a key
void func_0208287c(void *digest, const void *bin_ptr, u32 bin_len, const void *key_ptr, u32 key_len);

// syscall_twl.c, the half in the LTD autoload, which nothing has written yet; the wrappers call these cores
// The cores of SVC_SHA1Init, SVC_SHA1Update and SVC_SHA1GetHash: the first clears the word at 0x60 of the context, and
// each calls the BIOS (system calls 0x24, 0x25 and 0x26). The last takes the digest after the context, as the BIOS
// does not
void func_027696e8(SVCSHA1Context *context);
void func_027696f4(SVCSHA1Context *context, const void *input, u32 len);
void func_027696f8(SVCSHA1Context *context, void *digest);
// The core of SVC_CalcHMACSHA1, ARM code
void func_027698b4(void *digest, const void *bin_ptr, u32 bin_len, const void *key_ptr, u32 key_len);

#endif // POKEBW2_TWL_SYSCALL_H
