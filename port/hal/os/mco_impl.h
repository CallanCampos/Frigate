// What port/hal/os/mco_impl.c, minicoro's implementation, gives the fibers (port/hal/os/fiber.c) beyond minicoro's own
// API: the stack pointer a suspended coroutine saved, from which fiber.c gives LeakSanitizer the frames still live on
// that fiber's stack, so that what only a suspended SDK thread of the game holds isn't taken for a leak
// (port/docs/sanitizers.md). Only mco_impl.c sees minicoro's context, so it reads the pointer there.
#ifndef PORT_HAL_OS_MCO_IMPL_H
#define PORT_HAL_OS_MCO_IMPL_H

// minicoro's coroutine, mco_coro in minicoro.h, whose implementation can be included only once, in mco_impl.c
struct mco_coro;

// The stack pointer that a suspended coroutine saved when it last switched out (_mco_jumpout saves its registers in
// the context's ctx), or NULL where the switch isn't minicoro's assembly for arm64 or x86-64 System V
void *port_mco_saved_stack_pointer(struct mco_coro *co);

#endif // PORT_HAL_OS_MCO_IMPL_H
