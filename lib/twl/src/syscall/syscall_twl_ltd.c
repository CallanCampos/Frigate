// The LTD autoload half of TwlSDK's syscall_twl.o: BIOS SHA-1 calls and HMAC-SHA-1 built from them.
#include "nitro/mi.h"
#include "twl/syscall.h"

typedef struct {
    SVCSHA1Context sha1;
    u8 key[64];
    u32 keyLength;
} SVCHMACSHA1Context;

static void func_02769704(const void *input, void *digest, u32 len);
static void func_02769708(SVCHMACSHA1Context *context, const void *key, u32 keyLength);
static void func_027697e0(SVCHMACSHA1Context *context, const void *input, u32 len);
static void func_027697fc(SVCHMACSHA1Context *context, void *digest);

// clang-format off
// This BIOS entry point needs the SHA-1 init SVC instruction.
asm void func_027696e8(SVCSHA1Context *context) {
    mov r3, #0x60
    add r3, r0
    mov r2, #0
    str r2, [r3]
    swi 0x24
    bx lr
}

// This BIOS entry point needs the SHA-1 update SVC instruction.
asm void func_027696f4(SVCSHA1Context *context, const void *input, u32 len) {
    swi 0x25
    bx lr
}

// This BIOS entry point needs the SHA-1 final SVC instruction and its argument order.
asm void func_027696f8(SVCSHA1Context *context, void *digest) {
    mov ip, r1
    mov r1, r0
    mov r0, ip
    swi 0x26
    bx lr
}

// This BIOS entry point needs the one-shot SHA-1 SVC instruction.
static asm void func_02769704(const void *input, void *digest, u32 len) {
    swi 0x27
    bx lr
}
// clang-format on

#pragma thumb off

static void func_02769708(SVCHMACSHA1Context *context, const void *key, u32 keyLength) {
    u8 pad[64];
    u8 *padPos;
    int i;

    if (context == NULL || (keyLength != 0 && key == NULL)) {
        return;
    }

    if (keyLength > sizeof(context->key)) {
        func_02769704(key, context->key, keyLength);
        context->keyLength = 20;
    } else {
        MI_CpuCopy8(key, context->key, keyLength);
        context->keyLength = keyLength;
    }

    padPos = pad;
    for (i = 0; i < context->keyLength; i++) {
        *padPos++ = context->key[i] ^ 0x36;
    }
    for (; i < 64; i++) {
        *padPos++ = 0x36;
    }

    func_027696e8(&context->sha1);
    func_027696f4(&context->sha1, pad, sizeof(pad));
}

static void func_027697e0(SVCHMACSHA1Context *context, const void *input, u32 len) {
    if (context != NULL && len != 0 && input != NULL) {
        func_027696f4(&context->sha1, input, len);
    }
}

static void func_027697fc(SVCHMACSHA1Context *context, void *digest) {
    u8 pad[64];
    u8 innerHash[20];
    u8 *padPos;
    int i;

    if (context == NULL || digest == NULL) {
        return;
    }

    func_027696f8(&context->sha1, innerHash);
    padPos = pad;
    for (i = 0; i < context->keyLength; i++) {
        *padPos++ = context->key[i] ^ 0x5c;
    }
    for (; i < 64; i++) {
        *padPos++ = 0x5c;
    }

    func_027696e8(&context->sha1);
    func_027696f4(&context->sha1, pad, sizeof(pad));
    func_027696f4(&context->sha1, innerHash, sizeof(innerHash));
    func_027696f8(&context->sha1, digest);
}

void func_027698b4(void *digest, const void *input, u32 len, const void *key, u32 keyLength) {
    SVCHMACSHA1Context context;

    if (digest == NULL) {
        return;
    }
    if (len != 0 && input == NULL) {
        return;
    }
    if (keyLength != 0 && key == NULL) {
        return;
    }

    func_02769708(&context, key, keyLength);
    func_027697e0(&context, input, len);
    func_027697fc(&context, digest);
}
