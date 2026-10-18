// Fibers: a stack and a saved context each, all switched on the one host thread that runs the game. The model is
// asymmetric: only the host's own stack (the dispatcher's, in core.c) resumes a fiber, and a fiber only yields back to
// it. The OS layer turns that into the SDK's thread switches. From the reviewed spike, port/spikes/os-fibers. Plain C
// types only, so that the program can include it (port/app/message_log.c shows its alert only off the fibers).
#ifndef PORT_HAL_OS_FIBER_H
#define PORT_HAL_OS_FIBER_H

#include <stddef.h>

typedef struct PortFiber PortFiber;
typedef void (*PortFiberFunc)(void *arg);

// Makes a fiber that will run func(arg) when it is first resumed. Its stack is stackSize bytes (rounded up to the
// page size) in a mapping of its own, with an inaccessible guard region of 64 KiB under it, so that an overflow
// faults at once. Frames larger than the guard need the compiler's stack probes to fault there (port/cmake/os.cmake)
PortFiber *port_fiber_create(PortFiberFunc func, void *arg, size_t stackSize);
// Frees a fiber that is not running. It may be suspended in the middle of its function: that is how a thread that
// another thread destroyed or killed goes away
void port_fiber_destroy(PortFiber *fiber);
// Runs a fiber from the host stack until it yields or its function returns
void port_fiber_resume(PortFiber *fiber);
// Gives the CPU back to the host stack, from inside a fiber
void port_fiber_yield(void);
// Whether the fiber's function has returned
int port_fiber_is_done(const PortFiber *fiber);
// The fiber that runs, or NULL on the host stack
PortFiber *port_fiber_current(void);

// The usable stack of a fiber, and how deep it has gone so far: the bytes from its top down to the lowest one that
// changed since creation. That needs the stack filled with a pattern at creation, which port_fiber_set_paint turns on
// for the fibers created after it (it commits every page of the stack, so it is for measurements only)
size_t port_fiber_stack_size(const PortFiber *fiber);
size_t port_fiber_stack_used(const PortFiber *fiber);
void port_fiber_set_paint(int enabled);
// The guard region's bounds, and the whole mapping's (header, guard and stack), for the tests that overflow a stack
// on purpose and for fault reports
void port_fiber_guard_range(const PortFiber *fiber, void **low, void **high);
void port_fiber_mapping_range(const PortFiber *fiber, void **low, void **high);

size_t port_page_size(void);

#endif // PORT_HAL_OS_FIBER_H
