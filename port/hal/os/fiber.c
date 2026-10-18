// Fibers over minicoro (pinned by port/cmake/os.cmake), as the reviewed spike port/spikes/os-fibers recommends.
// minicoro switches with a few lines of assembly per target (arm64, x86-64 System V, x86-64 Windows with the TIB's
// stack fields) or with Windows fibers. It places a coroutine's header (the coroutine and its saved registers), a
// storage buffer and the stack in one block, the stack last, growing down towards the storage. This file maps that
// block itself so that the storage ends exactly where the stack begins, on a page boundary, and makes all of the
// storage but its first page inaccessible: a guard region of GUARD_BYTES under the stack, with the header below it. A
// frame larger than a page can step over a one-page guard without touching it, and the header, with the fiber's saved
// context, is what it would then overwrite; so the guard is large, and the build adds stack probes where the compiler
// doesn't by default (port/cmake/os.cmake). The PortFiber itself lives in the C heap, out of the stack's way. This
// file also tells AddressSanitizer about every switch, since minicoro's own annotations miss the switches back to the
// host stack (mco_impl.c, which is built without ASan), frees the fake stack of a fiber destroyed while suspended,
// and tells LeakSanitizer to scan the live frames and saved registers of suspended fibers for pointers.
#include "hal/os/fiber.h"

#define MCO_NO_MULTITHREAD
#include "minicoro.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define PORT_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(PORT_ASAN)
#define PORT_ASAN 1
#endif
#ifdef PORT_ASAN
#include <sanitizer/asan_interface.h>
#endif
// LeakSanitizer comes with ASan except on Windows (and is inert where its runtime doesn't run it, as Apple clang's)
#if defined(PORT_ASAN) && !defined(_WIN32)
#define PORT_LSAN 1
#include <sanitizer/lsan_interface.h>
#endif

// The byte the stacks are filled with when painting is on
#define PAINT_BYTE 0xa5
// The inaccessible bytes under each stack (rounded up to whole pages). A frame has to be larger than this to step
// over the guard into the coroutine's header when the compiler emits no stack probes. Every frame of the game's code
// fit a DS stack of at most 16 KiB (all of DTCM), so even with 64-bit pointers and ASan's redzones (2-3 times larger
// in the spike's measurements) none comes near it. It costs address space only
#define GUARD_BYTES (64 * 1024)

struct PortFiber {
    mco_coro *co;
    unsigned char *mapping;
    size_t mappingSize;
    unsigned char *guard;    // the guard region, right under the stack
    unsigned char *stackLow; // the lowest address of the stack; it grows down from stackLow + stackSize
    size_t stackSize;
    PortFiberFunc func;
    void *arg;
    int painted;
    void *asanFakeStack;      // ASan's fake stack of the fiber while it is switched out
    unsigned char *lsanStack; // the start of the stack's root region while the fiber is suspended, or NULL
};

static int sPaint;

#ifdef PORT_ASAN
// The host stack's bounds as ASan reported them when a fiber was entered, and its fake stack while a fiber runs
static const void *sHostBottom;
static size_t sHostSize;
static void *sHostFakeStack;
#endif

size_t port_page_size(void) {
    static size_t page;

    if (page == 0) {
#ifdef _WIN32
        // VirtualProtect works on 4 KiB pages, but VirtualAlloc places mappings at 64 KiB
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        page = info.dwPageSize;
#else
        page = (size_t)sysconf(_SC_PAGESIZE);
#endif
    }
    return page;
}

static size_t RoundUp(size_t value, size_t align) {
    return (value + align - 1) / align * align;
}

static void Fatal(const char *what) {
    fprintf(stderr, "port: fiber: %s\n", what);
    abort();
}

static unsigned char *MapPages(size_t size) {
#ifdef _WIN32
    // Not built yet: the shape a Windows backend takes
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    return p == MAP_FAILED ? NULL : p;
#endif
}

static void ProtectNone(unsigned char *p, size_t size) {
#ifdef _WIN32
    DWORD old;
    if (!VirtualProtect(p, size, PAGE_NOACCESS, &old)) {
        Fatal("VirtualProtect failed");
    }
#else
    if (mprotect(p, size, PROT_NONE) != 0) {
        Fatal("mprotect failed");
    }
#endif
}

#ifdef PORT_LSAN
// The saved stack pointer of a suspended coroutine (mco_impl.c), or NULL where it can't be read
void *port_mco_saved_stack_pointer(mco_coro *co);

// The bytes under the stack pointer that a function may still use without moving it, the red zone of the arm64 and
// x86-64 System V ABIs
#define RED_ZONE_BYTES 128

// LeakSanitizer scans the stack and registers of each thread, not of fibers: an allocation that only a suspended
// fiber holds (any SDK thread's, at exit) would be reported as leaked. So a suspended fiber's live frames, from where
// its stack pointer stopped to the top, are a root region, from its suspension to its next resume. The dead frames
// under them are left out, so that a pointer they still hold doesn't hide a leak; without the saved stack pointer the
// whole stack is the region. Its callee-saved registers are in minicoro's context, in the header's pages, which are a
// root region for the fiber's life (port_fiber_create)
static void ScanStackForLeaks(PortFiber *fiber) {
    unsigned char *top = fiber->stackLow + fiber->stackSize;
    unsigned char *sp = port_mco_saved_stack_pointer(fiber->co);
    unsigned char *low = fiber->stackLow;

    if (sp != NULL && sp > fiber->stackLow + RED_ZONE_BYTES && sp <= top) {
        low = sp - RED_ZONE_BYTES;
    }
    fiber->lsanStack = low;
    __lsan_register_root_region(low, (size_t)(top - low));
}

static void StopScanningStack(PortFiber *fiber) {
    if (fiber->lsanStack != NULL) {
        __lsan_unregister_root_region(fiber->lsanStack,
                                      (size_t)(fiber->stackLow + fiber->stackSize - fiber->lsanStack));
        fiber->lsanStack = NULL;
    }
}
#endif

#ifdef PORT_ASAN
// A fiber destroyed while suspended still has the fake stack that ASan gave it (with detect_stack_use_after_return),
// which only the fiber's last departure frees (FiberMain). The interface frees the fake stack that is current when a
// switch starts with nowhere to save it, so the fiber's becomes current for a moment, through a switch from this
// stack to this stack, which is then finished with this stack's own fake stack back. Nothing instrumented runs in
// between, and this function's own locals are on the real stack
#if defined(__clang__) || defined(__GNUC__)
__attribute__((no_sanitize("address")))
#endif
static void FreeFakeStack(void *fakeStack) {
    PortFiber *self = port_fiber_current();
    const void *bottom = self != NULL ? (const void *)self->stackLow : sHostBottom;
    size_t size = self != NULL ? self->stackSize : sHostSize;
    void *own = NULL;

    __sanitizer_start_switch_fiber(&own, bottom, size);
    __sanitizer_finish_switch_fiber(fakeStack, NULL, NULL);
    __sanitizer_start_switch_fiber(NULL, bottom, size);
    __sanitizer_finish_switch_fiber(own, NULL, NULL);
}
#endif

static void UnmapPages(unsigned char *p, size_t size) {
#ifdef _WIN32
    (void)size;
    VirtualFree(p, 0, MEM_RELEASE);
#else
    munmap(p, size);
#endif
}

// ASan's protocol for each switch: __sanitizer_start_switch_fiber on the old stack, naming the new one, and
// __sanitizer_finish_switch_fiber on the new stack as soon as it runs. A fiber's first arrival has no fake stack to
// give back, and its last departure passes NULL so that ASan frees its fake stack
static void FiberMain(mco_coro *co) {
    PortFiber *fiber = mco_get_user_data(co);

#ifdef PORT_ASAN
    __sanitizer_finish_switch_fiber(NULL, &sHostBottom, &sHostSize);
#endif
    fiber->func(fiber->arg);
#ifdef PORT_ASAN
    __sanitizer_start_switch_fiber(NULL, sHostBottom, sHostSize);
#endif
}

PortFiber *port_fiber_create(PortFiberFunc func, void *arg, size_t stackSize) {
    size_t page = port_page_size();
    size_t guardSize = RoundUp(GUARD_BYTES, page);
    mco_desc desc;
    size_t header;
    size_t headerSize;
    unsigned char *mapping;
    PortFiber *fiber;
    mco_coro *co;

    stackSize = RoundUp(stackSize, page);
    desc = mco_desc_init(FiberMain, stackSize);
    // The bytes from the coroutine to its storage, by minicoro's own sum of the sizes: its block is the header, the
    // storage, the stack and 16 bytes of slack
    header = desc.coro_size - RoundUp(desc.storage_size, 16) - desc.stack_size - 16;
    headerSize = RoundUp(header, page);

    // The mapping, from low to high addresses: minicoro's header, the guard, the stack
    mapping = MapPages(headerSize + guardSize + stackSize);
    fiber = calloc(1, sizeof(*fiber));
    if (mapping == NULL || fiber == NULL) {
        Fatal("out of memory for a fiber");
    }
    fiber->mapping = mapping;
    fiber->mappingSize = headerSize + guardSize + stackSize;
    fiber->guard = mapping + headerSize;
    fiber->stackLow = fiber->guard + guardSize;
    fiber->stackSize = stackSize;
    fiber->func = func;
    fiber->arg = arg;

    // The storage runs from the end of the header to the stack, and all of it after the header's pages is the guard
    co = (mco_coro *)mapping;
    desc.storage_size = (size_t)(fiber->stackLow - mapping) - header;
    desc.user_data = fiber;
    if (mco_init(co, &desc) != MCO_SUCCESS) {
        Fatal("mco_init failed");
    }
    if ((unsigned char *)co->stack_base != fiber->stackLow) {
        Fatal("minicoro's block layout changed: the stack is not where the guard expects it");
    }
    fiber->co = co;
    ProtectNone(fiber->guard, guardSize);

    if (sPaint) {
        memset(fiber->stackLow, PAINT_BYTE, stackSize);
        fiber->painted = 1;
    }
#ifdef PORT_LSAN
    // minicoro's coroutine and context, where a suspended fiber's callee-saved registers are (ScanStackForLeaks)
    __lsan_register_root_region(fiber->mapping, headerSize);
#endif
    return fiber;
}

void port_fiber_destroy(PortFiber *fiber) {
    int suspended;

    if (fiber == NULL) {
        return;
    }
    suspended = mco_status(fiber->co) == MCO_SUSPENDED;
    if (mco_uninit(fiber->co) != MCO_SUCCESS) {
        Fatal("destroying a running fiber");
    }
#ifdef PORT_LSAN
    StopScanningStack(fiber);
    __lsan_unregister_root_region(fiber->mapping, (size_t)(fiber->guard - fiber->mapping));
#endif
#ifdef PORT_ASAN
    // A fiber that never ran, or that ran without fake stacks, has none
    if (suspended && fiber->asanFakeStack != NULL) {
        FreeFakeStack(fiber->asanFakeStack);
    }
#else
    (void)suspended;
#endif
#ifdef PORT_ASAN
    // A fiber that never returned leaves its frames' redzones poisoned in the shadow memory; a later mapping at the
    // same addresses would inherit them
    ASAN_UNPOISON_MEMORY_REGION(fiber->stackLow, fiber->stackSize);
#endif
    UnmapPages(fiber->mapping, fiber->mappingSize);
    free(fiber);
}

void port_fiber_resume(PortFiber *fiber) {
    mco_result result;

    if (mco_running() != NULL) {
        Fatal("a fiber can only be resumed from the host stack");
    }
#ifdef PORT_LSAN
    StopScanningStack(fiber);
#endif
#ifdef PORT_ASAN
    __sanitizer_start_switch_fiber(&sHostFakeStack, fiber->stackLow, fiber->stackSize);
#endif
    result = mco_resume(fiber->co);
#ifdef PORT_ASAN
    __sanitizer_finish_switch_fiber(sHostFakeStack, NULL, NULL);
#endif
#ifdef PORT_LSAN
    if (result == MCO_SUCCESS && mco_status(fiber->co) == MCO_SUSPENDED) {
        ScanStackForLeaks(fiber);
    }
#endif
    if (result != MCO_SUCCESS) {
        fprintf(stderr, "port: fiber: resume: %s\n", mco_result_description(result));
        abort();
    }
}

void port_fiber_yield(void) {
    mco_coro *co = mco_running();
    mco_result result;

    if (co == NULL) {
        Fatal("yield from the host stack");
    }
#ifdef PORT_ASAN
    {
        PortFiber *fiber = mco_get_user_data(co);

        __sanitizer_start_switch_fiber(&fiber->asanFakeStack, sHostBottom, sHostSize);
        result = mco_yield(co);
        __sanitizer_finish_switch_fiber(fiber->asanFakeStack, &sHostBottom, &sHostSize);
    }
#else
    result = mco_yield(co);
#endif
    if (result != MCO_SUCCESS) {
        fprintf(stderr, "port: fiber: yield: %s\n", mco_result_description(result));
        abort();
    }
}

int port_fiber_is_done(const PortFiber *fiber) {
    return mco_status(fiber->co) == MCO_DEAD;
}

PortFiber *port_fiber_current(void) {
    mco_coro *co = mco_running();

    return co != NULL ? mco_get_user_data(co) : NULL;
}

size_t port_fiber_stack_size(const PortFiber *fiber) {
    return fiber->stackSize;
}

// Reads the stack of a suspended fiber, whose dead frames ASan may have poisoned
#if defined(__clang__) || defined(__GNUC__)
__attribute__((no_sanitize("address")))
#endif
size_t port_fiber_stack_used(const PortFiber *fiber) {
    const volatile unsigned char *p = fiber->stackLow;
    size_t i;

    if (!fiber->painted) {
        return 0;
    }
    for (i = 0; i < fiber->stackSize && p[i] == PAINT_BYTE; i++) {
    }
    return fiber->stackSize - i;
}

void port_fiber_set_paint(int enabled) {
    sPaint = enabled;
}

void port_fiber_guard_range(const PortFiber *fiber, void **low, void **high) {
    *low = fiber->guard;
    *high = fiber->stackLow;
}

void port_fiber_mapping_range(const PortFiber *fiber, void **low, void **high) {
    *low = fiber->mapping;
    *high = fiber->mapping + fiber->mappingSize;
}
