// Faults on purpose, to show that the fibers' safety nets work. Each mode is one CTest entry:
//   "guard": a recursion of 1 KiB frames overflows a fiber's 64 KiB stack into its guard region, and the signal handler
//     checks that the fault address is in it;
//   "bigframes": two nested 44 KiB frames, each larger than a page, so that the second one starts inside the guard
//     without touching the pages in between: the guard is large enough to catch it even without stack probes;
//   "hugeframe": one 160 KiB frame, larger than the stack and the guard together, so that only the compiler's stack
//     probes make it fault in the guard; without them it lands below it, where the coroutine's saved context is;
//   "asan": a write past a local array on a fiber's stack, which AddressSanitizer must report as a
//     stack-buffer-overflow located in the fiber's frame (built with ASan);
//   "leak": a real leak after fibers have run and while one is suspended, which LeakSanitizer must still report;
//   "held": a block that only a suspended fiber's stack points to, which LeakSanitizer must not report.
#include "hal_fiber.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STACK_SIZE (64 * 1024)

static void *sGuardLow;
static void *sGuardHigh;
static void *sMappingLow;
static char sAltStack[64 * 1024];

static void OnFault(int signo, siginfo_t *info, void *context) {
    static const char inGuard[] = "fault in guard page\n";
    static const char inHeader[] = "fault in the fiber's header, below the guard\n";
    static const char elsewhere[] = "fault outside the guard page\n";
    char *addr = info->si_addr;

    (void)signo;
    (void)context;
    if (addr >= (char *)sGuardLow && addr < (char *)sGuardHigh) {
        write(STDOUT_FILENO, inGuard, sizeof(inGuard) - 1);
        _exit(0);
    }
    if (addr >= (char *)sMappingLow && addr < (char *)sGuardLow) {
        write(STDOUT_FILENO, inHeader, sizeof(inHeader) - 1);
        _exit(3);
    }
    write(STDOUT_FILENO, elsewhere, sizeof(elsewhere) - 1);
    _exit(3);
}

static volatile unsigned char *volatile sEscape;

// The buffer escapes, so that the compiler can't turn the recursion into a loop
__attribute__((noinline)) static int Recurse(int depth) {
    volatile unsigned char buffer[1024];

    buffer[0] = (unsigned char)depth;
    sEscape = buffer;
    if (depth == 0) {
        return buffer[0];
    }
    return Recurse(depth - 1) + buffer[0];
}

static void Overflow(void *arg) {
    (void)arg;
    printf("recursing %d KiB deep on a 64 KiB stack\n", 1000);
    printf("never: %d\n", Recurse(1000));
}

// Writes only the lowest byte of each frame, as code that fills a buffer from its start would first
__attribute__((noinline)) static int BigFrames(int depth) {
    volatile unsigned char buffer[44 * 1024];

    buffer[0] = (unsigned char)depth;
    sEscape = buffer;
    if (depth == 0) {
        return buffer[0];
    }
    return BigFrames(depth - 1) + buffer[0];
}

static void OverflowBigFrames(void *arg) {
    (void)arg;
    printf("two nested 44 KiB frames on a 64 KiB stack\n");
    printf("never: %d\n", BigFrames(1));
}

__attribute__((noinline)) static int HugeFrame(void) {
    volatile unsigned char buffer[160 * 1024];

    buffer[0] = 1;
    sEscape = buffer;
    return buffer[0];
}

static void OverflowHugeFrame(void *arg) {
    (void)arg;
    printf("one 160 KiB frame on a 64 KiB stack over a 64 KiB guard\n");
    printf("never: %d\n", HugeFrame());
}

static void WritePastArray(void *arg) {
    volatile char buffer[16];
    // Through a pointer, so that UBSan's bounds check (which knows the array's type) leaves the write to ASan
    volatile char *volatile p = buffer;
    volatile int index = 16;

    (void)arg;
    p[index] = 1;
    printf("the write was not caught\n");
}

static void YieldForever(void *arg) {
    (void)arg;
    for (;;) {
        hal_fiber_yield();
    }
}

static void ReturnAtOnce(void *arg) {
    (void)arg;
}

// Drops every pointer as soon as it has it, so that nothing points to the blocks
__attribute__((noinline)) static void Leak(void) {
    int i;

    for (i = 0; i < 100; i++) {
        void *volatile p = malloc(33);

        p = NULL;
        (void)p;
    }
}

// Keeps its block only in a local of its own, as an SDK thread keeps what it allocated, and stays suspended. The local
// is volatile so that the pointer is stored in the fiber's stack, not kept in a register
static void HoldBlock(void *arg) {
    void *volatile held = malloc(77);

    (void)arg;
    (void)held;
    for (;;) {
        hal_fiber_yield();
    }
}

// Runs func on a fiber whose overflow must fault in its guard region
static int RunOverflow(HalFiberFunc func) {
    HalFiber *fiber = hal_fiber_create(func, NULL, STACK_SIZE);
    void *mappingHigh;
    stack_t alt;
    struct sigaction action;

    hal_fiber_guard_range(fiber, &sGuardLow, &sGuardHigh);
    hal_fiber_mapping_range(fiber, &sMappingLow, &mappingHigh);
    alt.ss_sp = sAltStack;
    alt.ss_size = sizeof(sAltStack);
    alt.ss_flags = 0;
    sigaltstack(&alt, NULL);
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = OnFault;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
    hal_fiber_resume(fiber);
    return 1;
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "guard";
    HalFiber *fiber;
    int i;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (strcmp(mode, "guard") == 0) {
        return RunOverflow(Overflow);
    }
    if (strcmp(mode, "bigframes") == 0) {
        return RunOverflow(OverflowBigFrames);
    }
    if (strcmp(mode, "hugeframe") == 0) {
        return RunOverflow(OverflowHugeFrame);
    }
    if (strcmp(mode, "asan") == 0) {
        fiber = hal_fiber_create(WritePastArray, NULL, STACK_SIZE);
        hal_fiber_resume(fiber);
        hal_fiber_destroy(fiber);
        return 1;
    }
    if (strcmp(mode, "leak") == 0) {
        HalFiber *kept = hal_fiber_create(YieldForever, NULL, STACK_SIZE);

        fiber = hal_fiber_create(ReturnAtOnce, NULL, STACK_SIZE);
        for (i = 0; i < 1000; i++) {
            hal_fiber_resume(kept);
        }
        hal_fiber_resume(fiber);
        hal_fiber_destroy(fiber);
        Leak();
        printf("leaked 100 blocks of 33 bytes after 1001 fiber runs, one fiber still suspended\n");
        return 0;
    }
    if (strcmp(mode, "held") == 0) {
        fiber = hal_fiber_create(HoldBlock, NULL, STACK_SIZE);
        hal_fiber_resume(fiber);
        hal_fiber_resume(fiber);
        printf("exiting with a block held only by a suspended fiber's stack\n");
        return 0;
    }
    return 2;
}
