// minicoro's implementation, in a file of its own so that a sanitizer build compiles it without AddressSanitizer
// (port/cmake/os.cmake). minicoro's own ASan annotations assume the resumer is another coroutine: when a fiber yields
// back to the thread's own stack, it never tells ASan, which then believes the thread still runs on the fiber's
// stack, and LeakSanitizer's root scan at exit reads the last fiber's (unmapped) stack instead of the real one.
// fiber.c makes the annotations itself, in the order the ASan interface asks for.
#define MINICORO_IMPL
// Fibers only ever run on the game's host thread, so the running coroutine needs no thread-local variable
#define MCO_NO_MULTITHREAD
// The stacks are sized by fiber.c, in pages; minicoro's 32 KiB minimum would hide what the measurements are after
#define MCO_MIN_STACK_SIZE 4096
#include "minicoro.h"
