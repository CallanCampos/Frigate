// Fibers: a stack and a saved context each, all switched on the one host thread that runs the game. The model is
// asymmetric: only the host's own stack resumes a fiber, and a fiber only yields back to it. The OS layer on top
// (os_host.c) turns that into the SDK's thread switches through a dispatcher on the host stack.
#ifndef HAL_FIBER_H
#define HAL_FIBER_H

#include <stddef.h>

typedef struct HalFiber HalFiber;
typedef void (*HalFiberFunc)(void *arg);

// Makes a fiber that will run func(arg) when it is first resumed. Its stack is stackSize bytes (rounded up to the
// page size) in a mapping of its own, with an inaccessible guard region of 64 KiB under it, so that an overflow
// faults at once. Frames larger than the guard need the compiler's stack probes to fault there (CMakeLists.txt)
HalFiber *hal_fiber_create(HalFiberFunc func, void *arg, size_t stackSize);
// Frees a fiber that is not running. It may be suspended in the middle of its function: that is how a thread that
// another thread destroyed or killed goes away
void hal_fiber_destroy(HalFiber *fiber);
// Runs a fiber from the host stack until it yields or its function returns
void hal_fiber_resume(HalFiber *fiber);
// Gives the CPU back to the host stack, from inside a fiber
void hal_fiber_yield(void);
// Whether the fiber's function has returned
int hal_fiber_is_done(const HalFiber *fiber);
// The fiber that runs, or NULL on the host stack
HalFiber *hal_fiber_current(void);

// The usable stack of a fiber, and how deep it has gone so far: the bytes from its top down to the lowest one that
// changed since creation. That needs the stack filled with a pattern at creation, which hal_fiber_set_paint turns on
// for the fibers created after it (it commits every page of the stack, so it is for measurements only)
size_t hal_fiber_stack_size(const HalFiber *fiber);
size_t hal_fiber_stack_used(const HalFiber *fiber);
void hal_fiber_set_paint(int enabled);
// The guard region's bounds, and the whole mapping's (header, guard and stack), for the tests that overflow a stack
// on purpose
void hal_fiber_guard_range(const HalFiber *fiber, void **low, void **high);
void hal_fiber_mapping_range(const HalFiber *fiber, void **low, void **high);

size_t hal_page_size(void);

#endif // HAL_FIBER_H
