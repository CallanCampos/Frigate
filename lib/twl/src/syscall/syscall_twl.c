// TwlSDK's system call library for the DSi (libsyscall.twl, the object syscall_twl.o; the file name is a guess from the
// object's), the half in ARM9 main: the wrappers of the DSi's BIOS calls for SHA-1 and HMAC-SHA-1. The object's other
// half is in the LTD autoload. The linker's veneers, which reach the cores in the LTD autoload (too far for a branch),
// follow each wrapper in the ROM
#include "nitro/os.h"
#include "twl/syscall.h"

// SVC_SHA1Init
void dsiJump_27696E9(SVCSHA1Context *context) {
    if (!func_0207acbc()) {
        sys_exit();
    }
    func_027696e8(context);
}

// SVC_SHA1Update
void func_02082820(SVCSHA1Context *context, const void *input, u32 len) {
    if (!func_0207acbc()) {
        sys_exit();
    }
    func_027696f4(context, input, len);
}

// SVC_SHA1GetHash
void func_02082850(SVCSHA1Context *context, void *digest) {
    if (!func_0207acbc()) {
        sys_exit();
    }
    func_027696f8(context, digest);
}

// SVC_CalcHMACSHA1
void func_0208287c(void *digest, const void *bin_ptr, u32 bin_len, const void *key_ptr, u32 key_len) {
    if (!func_0207acbc()) {
        sys_exit();
    }
    func_027698b4(digest, bin_ptr, bin_len, key_ptr, key_len);
}
