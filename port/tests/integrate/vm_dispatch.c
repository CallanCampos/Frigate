// The actual src/system/vm.c runs the field, trainer AI, battle-effects and musical script commands. Its two
// command tables erase only an object context pointer. Their typed callbacks must receive
// both addresses intact; native and verifier calls must retain their sanitizer checks (R13). No ROM or game heap:
// VM_Create/Free/Reset are not called, and the minimal heap/fill definitions only satisfy those unused functions.
#include "system/vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TestEnv {
    VM *expected;
    int calls;
} TestEnv;
static BOOL TypedCommand(VM *vm, TestEnv *env) {
    if (env == NULL || vm != env->expected) {
        exit(91);
    }
    env->calls++;
    return TRUE;
}
static BOOL WrongNative(int wrong, void *env) {
    (void)wrong;
    (void)env;
    return TRUE;
}
static BOOL WrongVerifier(VM *vm, void *env, void *arg, u32 opcode) {
    (void)vm;
    (void)env;
    (void)arg;
    (void)opcode;
    return TRUE;
}
void *GFL_HeapAllocate(HeapID id, u32 size, BOOL clear, const char *file, u16 line) {
    (void)id;
    (void)file;
    (void)line;
    return clear ? calloc(1, size) : malloc(size);
}
void GFL_HeapFree(void *ptr) {
    free(ptr);
}
void MI_CpuFill8(void *dst, u8 value, u32 size) {
    memset(dst, value, size);
}
// A separate, deliberately wrong call outside VM_Run detects whether this compiler emits the function check.
// Apple clang currently does not for C; check_vm_dispatch.py reports that as a visible skip of negative tests.
static __attribute__((noinline)) BOOL CapabilityProbe(VMCommand command, VM *vm, void *env) {
    return command(vm, env);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    VM vm = { 0 };
    TestEnv env = { &vm, 0 };
    const u8 mainCode[] = { 0, 0 }, extraCode[] = { 0xe8, 3 };
    const VMCommand table[] = { (VMCommand)TypedCommand };
    vm.env = &env;
    vm.state = VM_STATE_RUNNING;
    vm.pc = mainCode;
    vm.param.commands = table;
    vm.param.commandCount = 1;
    if (!strcmp(argv[1], "capability")) {
        return CapabilityProbe((VMCommand)WrongNative, &vm, &env) == TRUE ? 0 : 93;
    }
    if (!strcmp(argv[1], "main")) {
    } else if (!strcmp(argv[1], "extra")) {
        vm.pc = extraCode;
        vm.param.commandCount = 0;
        vm.param.extraCommands = table;
        vm.param.extraCommandStart = 1000;
        vm.param.extraCommandCount = 1;
    } else if (!strcmp(argv[1], "native")) {
        vm.state = VM_STATE_WAITING;
        vm.native = (VMCommand)WrongNative;
    } else if (!strcmp(argv[1], "verifier")) {
        vm.verifier = (VMVerifier)WrongVerifier;
    } else {
        return 2;
    }
    if (!VM_Run(&vm) || env.calls != 1) {
        return 92;
    }
    puts("typed table command received exact VM and environment");
    return 0;
}
