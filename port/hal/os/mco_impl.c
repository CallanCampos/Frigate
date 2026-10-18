// minicoro's implementation, in a file of its own so that a sanitizer build compiles it without AddressSanitizer
// (port/cmake/os.cmake). minicoro's own ASan annotations assume the resumer is another coroutine: when a fiber yields
// back to the thread's own stack, it never tells ASan, which then believes the thread still runs on the fiber's
// stack, and LeakSanitizer's root scan at exit reads the last fiber's (unmapped) stack instead of the real one.
// fiber.c makes the annotations itself, in the order the ASan interface asks for. It also reads a suspended
// coroutine's saved stack pointer here (port_mco_saved_stack_pointer, at the end), since only this file sees
// minicoro's context.
#define MINICORO_IMPL
// Fibers only ever run on the game's host thread, so the running coroutine needs no thread-local variable
#define MCO_NO_MULTITHREAD
// The stacks are sized by fiber.c, in pages; minicoro's 32 KiB minimum would hide what the measurements are after
#define MCO_MIN_STACK_SIZE 4096
#include "minicoro.h"

// The stack pointer that a suspended coroutine saved when it last switched out (_mco_jumpout saves its registers in
// the context's ctx), or NULL where the switch isn't minicoro's assembly for arm64 or x86-64 System V. fiber.c gives
// LeakSanitizer a suspended fiber's stack from there up, the frames still live, rather than its whole mapping
void *port_mco_saved_stack_pointer(mco_coro *co);

void *port_mco_saved_stack_pointer(mco_coro *co) {
#if defined(MCO_USE_ASM) && defined(__aarch64__)
    return ((_mco_context *)co->context)->ctx.sp;
#elif defined(MCO_USE_ASM) && defined(__x86_64__) && !defined(_WIN32)
    return ((_mco_context *)co->context)->ctx.rsp;
#else
    (void)co;
    return NULL;
#endif
}
