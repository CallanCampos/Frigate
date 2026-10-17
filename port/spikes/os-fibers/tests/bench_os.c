// Measurements for the README: what a switch costs at each level, what a frame's scheduling costs, what making a
// fiber costs, and how deep the host stacks go for the kinds of work SDK threads do. "--quick" runs a little of each,
// as a smoke test.
#include "os_host.h"
#include "hal_fiber.h"
#include "nitro/gx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static long sIterations = 2000000;

static double Now(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// 1. A raw resume and yield of one fiber, from and back to the host stack
static void YieldForever(void *arg) {
    (void)arg;
    for (;;) {
        hal_fiber_yield();
    }
}

static void BenchRawSwitch(void) {
    HalFiber *fiber = hal_fiber_create(YieldForever, NULL, 64 * 1024);
    double start;
    double seconds;
    long i;

    hal_fiber_resume(fiber);
    start = Now();
    for (i = 0; i < sIterations; i++) {
        hal_fiber_resume(fiber);
    }
    seconds = Now() - start;
    hal_fiber_destroy(fiber);
    printf("raw fiber switch (resume or yield): %.1f ns\n", seconds * 1e9 / (double)(sIterations * 2));
}

// 2. An SDK thread switch through the decomp's scheduler: main wakes a higher thread, which sleeps again. Each
// iteration is two switches, each two fiber switches through the dispatcher
static OSThread sThread;
static u32 sStack[0x400 / sizeof(u32)];
static OSThreadQueue sQueue;
static double sSdkSwitchNs;

static void Sleeper(void *arg) {
    (void)arg;
    for (;;) {
        OS_SleepThread(&sQueue);
    }
}

static void SdkSwitchMain(void) {
    double start;
    long i;

    OS_InitThread();
    OS_InitThreadQueue(&sQueue);
    OS_CreateThread(&sThread, Sleeper, NULL, sStack + NELEMS(sStack), sizeof(sStack), 10);
    OS_WakeupThreadDirect(&sThread);
    start = Now();
    for (i = 0; i < sIterations; i++) {
        OS_WakeupThread(&sQueue);
    }
    sSdkSwitchNs = (Now() - start) * 1e9 / (double)(sIterations * 2);
    OS_DestroyThread(&sThread);
}

// 3. A frame of main.c's loop with nothing to do: OS_WaitIrq, the idle thread's halt, the clock's move to the VBlank,
// the handler and the tail's switch back
static double sFrameNs;

static void VBlank(void) {
    hal_os_set_irq_check_flag(OS_IE_V_BLANK);
}

static void FrameMain(void) {
    long frames = sIterations / 10;
    double start;
    long i;

    OS_InitThread();
    OS_SetIrqFunction(OS_IE_V_BLANK, VBlank);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    hal_os_set_ime(TRUE);
    hal_os_set_dispstat(REG_GX_DISPSTAT_VBI_MASK);
    OS_EnableInterrupts();
    start = Now();
    for (i = 0; i < frames; i++) {
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    }
    sFrameNs = (Now() - start) * 1e9 / (double)frames;
}

// 3b. A frame of main.c's loop as BW2 runs it, with the HBlank interrupt on: 100 OS_GetTick calls of logic after the
// VBlank, then OS_WaitIrq, which wakes main at each of the 260 HBlanks that it waits through (test_os's
// hblank_main_loop)
static double sHBlankFrameNs;

static void HBlank(void) {
}

static void HBlankFrameMain(void) {
    long frames = sIterations / 1000;
    double start;
    long i;
    int j;

    OS_InitThread();
    OS_SetIrqFunction(OS_IE_V_BLANK, VBlank);
    OS_SetIrqFunction(OS_IE_H_BLANK, HBlank);
    OS_EnableIrqMask(OS_IE_V_BLANK | OS_IE_H_BLANK);
    hal_os_set_ime(TRUE);
    hal_os_set_dispstat(REG_GX_DISPSTAT_VBI_MASK | REG_GX_DISPSTAT_HBI_MASK);
    OS_EnableInterrupts();
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    start = Now();
    for (i = 0; i < frames; i++) {
        for (j = 0; j < 100; j++) {
            (void)OS_GetTick();
        }
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    }
    sHBlankFrameNs = (Now() - start) * 1e9 / (double)frames;
}

// 4. Making a fiber, running it to its first yield (which touches its top page) and disposing of it
static void YieldOnce(void *arg) {
    (void)arg;
    hal_fiber_yield();
}

static void BenchCreate(size_t stackSize) {
    long count = sIterations / 200;
    double start = Now();
    long i;

    for (i = 0; i < count; i++) {
        HalFiber *fiber = hal_fiber_create(YieldOnce, NULL, stackSize);

        hal_fiber_resume(fiber);
        hal_fiber_destroy(fiber);
    }
    printf("fiber create + first run + destroy, %zu KiB stack: %.2f us\n", stackSize / 1024,
           (Now() - start) * 1e6 / (double)count);
}

// 5. Stack depths: threads that run the SDK's blocking paths, libc formatting, and a DS-style function with a 1 KiB
// local buffer four calls deep. Each one sleeps at the end so that its stack can be read
static OSThread sWorkers[3];
static u32 sWorkerStacks[3][0x400 / sizeof(u32)];
static OSMutex sMutex;
static OSMessageQueue sMq;
static OSMessage sMessages[2];
static OSThreadQueue sDone;
static volatile int sSink;

static void SdkPathsWorker(void *arg) {
    OSMessage message;

    (void)arg;
    OS_LockMutex(&sMutex);
    OS_SendMessage(&sMq, (OSMessage)1, OS_MESSAGE_BLOCK);
    OS_ReceiveMessage(&sMq, &message, OS_MESSAGE_BLOCK);
    OS_UnlockMutex(&sMutex);
    OS_Sleep(1);
    OS_SleepThread(&sDone);
}

static void FormatWorker(void *arg) {
    char buffer[256];

    (void)arg;
    snprintf(buffer, sizeof(buffer), "%s %d %08x %.6f %e", "format", 12345, 0xbeefu, 3.14159265, 1e-300);
    sSink += buffer[0];
    OS_SleepThread(&sDone);
}

static volatile unsigned char *volatile sEscape;

// The buffer escapes, so that the compiler keeps one frame per call
__attribute__((noinline)) static int DsStyle(int depth) {
    volatile unsigned char buffer[1024];

    buffer[0] = (unsigned char)depth;
    sEscape = buffer;
    buffer[1023] = (unsigned char)depth;
    if (depth == 0) {
        return buffer[0];
    }
    return DsStyle(depth - 1) + buffer[1023];
}

static void DsStyleWorker(void *arg) {
    (void)arg;
    sSink += DsStyle(4);
    OS_SleepThread(&sDone);
}

static u32 sDepths[4];

static void StackMain(void) {
    static void (*const workers[3])(void *) = {SdkPathsWorker, FormatWorker, DsStyleWorker};
    int i;

    OS_InitThread();
    OS_InitMutex(&sMutex);
    OS_InitMessageQueue(&sMq, sMessages, 2);
    OS_InitThreadQueue(&sDone);
    OS_SetIrqFunction(OS_IE_V_BLANK, VBlank);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    hal_os_set_ime(TRUE);
    hal_os_set_dispstat(REG_GX_DISPSTAT_VBI_MASK);
    OS_EnableInterrupts();
    for (i = 0; i < 3; i++) {
        OS_CreateThread(&sWorkers[i], workers[i], NULL, sWorkerStacks[i] + NELEMS(sWorkerStacks[i]),
                        sizeof(sWorkerStacks[i]), 10);
        OS_WakeupThreadDirect(&sWorkers[i]);
    }
    // Lets the OS_Sleep alarm fire, through the idle thread's halt and the IRQ tail
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    for (i = 0; i < 3; i++) {
        sDepths[i] = hal_os_stack_used(&sWorkers[i]);
    }
    sDepths[3] = hal_os_stack_used(OS_GetCurrentThread());
    for (i = 0; i < 3; i++) {
        OS_DestroyThread(&sWorkers[i]);
    }
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--quick") == 0) {
        sIterations = 20000;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("page size %zu\n", hal_page_size());

    BenchRawSwitch();
    if (hal_os_run(SdkSwitchMain) != 0) {
        return 1;
    }
    printf("SDK thread switch (OS_WakeupThread/OS_SleepThread ping-pong): %.1f ns, fiber switches %u\n", sSdkSwitchNs,
           hal_os_stats().fiberSwitches);
    if (hal_os_run(FrameMain) != 0) {
        return 1;
    }
    printf("empty frame (OS_WaitIrq, idle halt, VBlank handler, tail): %.1f ns\n", sFrameNs);
    if (hal_os_run(HBlankFrameMain) != 0) {
        return 1;
    }
    printf("frame with HBlank on (100 OS_GetTick, then 260 HBlank wakeups of main): %.1f us\n", sHBlankFrameNs / 1000);
    BenchCreate(64 * 1024);
    BenchCreate(256 * 1024);

    hal_fiber_set_paint(1);
    hal_os_set_stack_sizes(256 * 1024, 64 * 1024);
    if (hal_os_run(StackMain) != 0) {
        return 1;
    }
    hal_fiber_set_paint(0);
    printf("stack used: SDK blocking paths %u B, snprintf %u B, DS-style 4 x 1 KiB frames %u B, launcher %u B\n",
           sDepths[0], sDepths[1], sDepths[2], sDepths[3]);
    return 0;
}
