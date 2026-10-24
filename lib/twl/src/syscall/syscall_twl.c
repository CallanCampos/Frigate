// TwlSDK's system call library for the DSi (libsyscall.twl, the object syscall_twl.o; the file name is a guess from the
// object's), the half in ARM9 main: the wrappers of the DSi's BIOS calls for SHA-1 and HMAC-SHA-1. The object's other
// half is in the LTD autoload. The linker's veneers, which reach the cores in the LTD autoload (too far for a branch),
// follow each wrapper in the ROM
#include "nitro/os.h"
#include "twl/syscall.h"

void SVC_SHA1Init(SVCSHA1Context *context) {
    if (!OSi_IsRunOnTwl()) {
        OS_Terminate();
    }
    func_027696e8(context);
}

void SVC_SHA1Update(SVCSHA1Context *context, const void *input, u32 len) {
    if (!OSi_IsRunOnTwl()) {
        OS_Terminate();
    }
    func_027696f4(context, input, len);
}

void SVC_SHA1GetHash(SVCSHA1Context *context, void *digest) {
    if (!OSi_IsRunOnTwl()) {
        OS_Terminate();
    }
    func_027696f8(context, digest);
}

void SVC_CalcHMACSHA1(void *digest, const void *bin_ptr, u32 bin_len, const void *key_ptr, u32 key_len) {
    if (!OSi_IsRunOnTwl()) {
        OS_Terminate();
    }
    func_027698b4(digest, bin_ptr, bin_len, key_ptr, key_len);
}
