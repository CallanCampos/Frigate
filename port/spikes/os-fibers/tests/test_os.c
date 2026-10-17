// Tests of the SDK's thread semantics on fibers: each one runs a small scenario as the DS's launcher thread, logs
// what happens in order, and compares the log with the order the DS gives. The scheduler under test is the decomp's
// own os_thread.c, os_mutex.c and os_message.c; os_host.c only switches.
#include "os_host.h"

#include "hal_fiber.h"
#include "nitro/gx.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MAX 512
#define LOG_LINE 64
// The DS's clock, for durations in the scenarios
#define CYCLES_PER_MS 33514
#define CYCLES_PER_FRAME 560190

static char sLog[LOG_MAX][LOG_LINE];
static int sLogCount;
static int sFailed;

static void Log(const char *format, ...) {
    va_list args;

    if (sLogCount == LOG_MAX) {
        return;
    }
    va_start(args, format);
    vsnprintf(sLog[sLogCount++], LOG_LINE, format, args);
    va_end(args);
}

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("  check failed: %s\n", what);
        sFailed = 1;
    }
}

static void ExpectLog(const char *const *expected, int count) {
    int i;
    int ok = sLogCount == count;

    for (i = 0; ok && i < count; i++) {
        ok = strcmp(sLog[i], expected[i]) == 0;
    }
    if (!ok) {
        printf("  log differs\n  %-44s  %s\n", "got", "expected");
        for (i = 0; i < sLogCount || i < count; i++) {
            printf("  %-44s  %s\n", i < sLogCount ? sLog[i] : "", i < count ? expected[i] : "");
        }
        sFailed = 1;
    }
}

#define EXPECT_LOG(...)                                                                                                \
    do {                                                                                                               \
        static const char *const expected[] = {__VA_ARGS__};                                                           \
        ExpectLog(expected, (int)(sizeof(expected) / sizeof(expected[0])));                                            \
    } while (0)

// Threads with DS-sized stack buffers: the SDK writes its magic words there, the code runs on a host fiber
static OSThread sThreads[8];
static u32 sStacks[8][0x400 / sizeof(u32)];
static OSThreadQueue sQueue;
static OSThreadQueue sGate;
static int sCounter;

static void Create(int n, void (*func)(void *), void *arg, u32 priority) {
    OS_CreateThread(&sThreads[n], func, arg, sStacks[n] + 0x400 / sizeof(u32), 0x400, priority);
}

static void VBlankHandler(void) {
    hal_os_set_irq_check_flag(OS_IE_V_BLANK);
}

static void LoggingVBlankHandler(void) {
    Log("vblank %u (irq mode %d, line %u)", hal_os_frame(), OS_GetProcMode() == OS_PROCMODE_IRQ, hal_os_vcount());
    hal_os_set_irq_check_flag(OS_IE_V_BLANK);
}

// What the game's main does before its loop: OS_Init's OS_InitThread, the VBlank handler, IE, IME, DISPSTAT's VBlank
// interrupt (GX_VBlankIntr) and the CPSR
static void Boot(OSIrqFunction vblank) {
    OS_InitThread();
    OS_InitThreadQueue(&sQueue);
    OS_InitThreadQueue(&sGate);
    sCounter = 0;
    OS_SetIrqFunction(OS_IE_V_BLANK, vblank);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    hal_os_set_ime(TRUE);
    hal_os_set_dispstat(REG_GX_DISPSTAT_VBI_MASK);
    OS_EnableInterrupts();
}

static void LogThread(void *arg) {
    Log("%s runs", (const char *)arg);
}

// Waking a thread of a higher priority switches to it at once; a lower one waits until the waker blocks
static void TestWakeupPreempts(void) {
    Boot(VBlankHandler);
    Create(0, LogThread, "high", 10);
    Create(1, LogThread, "low", 20);
    Log("main wakes high");
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main wakes low");
    OS_WakeupThreadDirect(&sThreads[1]);
    Log("main joins low");
    OS_JoinThread(&sThreads[1]);
    Log("main joined (terminated %d %d)", OS_IsThreadTerminated(&sThreads[0]), OS_IsThreadTerminated(&sThreads[1]));
    EXPECT_LOG("main wakes high", "high runs", "main wakes low", "main joins low", "low runs",
               "main joined (terminated 1 1)");
}

static void YieldThread(void *arg) {
    int i;

    for (i = 0; i < 3; i++) {
        Log("%s%d", (const char *)arg, i);
        OS_YieldThread();
    }
}

// OSi_InsertThreadToList puts a new thread before the others of its priority, so B, created after A, runs first;
// OS_YieldThread then moves the running thread behind the last one of its priority
static void TestYieldOrder(void) {
    Boot(VBlankHandler);
    Create(0, YieldThread, "A", 18);
    Create(1, YieldThread, "B", 18);
    OS_WakeupThreadDirect(&sThreads[0]);
    OS_WakeupThreadDirect(&sThreads[1]);
    Log("main joins");
    OS_JoinThread(&sThreads[0]);
    OS_JoinThread(&sThreads[1]);
    Log("main done");
    EXPECT_LOG("main joins", "B0", "A0", "B1", "A1", "B2", "A2", "main done");
}

static void SleeperThread(void *arg) {
    Log("%s sleeps", (const char *)arg);
    OS_SleepThread(&sQueue);
    Log("%s wakes", (const char *)arg);
}

// OS_WakeupThread readies every thread of a queue; they then run in the order of the thread list (priority, newest
// first), not the queue's
static void TestQueueWakeOrder(void) {
    Boot(VBlankHandler);
    Create(0, SleeperThread, "P1", 12);
    Create(1, SleeperThread, "P2", 14);
    Create(2, SleeperThread, "P3", 12);
    OS_WakeupThreadDirect(&sThreads[0]);
    OS_WakeupThreadDirect(&sThreads[1]);
    OS_WakeupThreadDirect(&sThreads[2]);
    Log("main wakes the queue");
    OS_WakeupThread(&sQueue);
    Log("main back");
    EXPECT_LOG("P1 sleeps", "P2 sleeps", "P3 sleeps", "main wakes the queue", "P3 wakes", "P1 wakes", "P2 wakes",
               "main back");
}

static void Destructor(void *arg) {
    Log("destructor (%#x)", (unsigned)(uintptr_t)arg);
}

static void DestructorThread(void *arg) {
    (void)arg;
    OS_SetThreadDestructor(OS_GetCurrentThread(), Destructor);
    Log("worker runs");
}

// A thread that returns ends through OS_ExitThread, which runs its destructor in the thread and wakes the joiners
static void TestJoinDestructor(void) {
    Boot(VBlankHandler);
    Create(0, DestructorThread, NULL, 20);
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main joins");
    OS_JoinThread(&sThreads[0]);
    Log("main joined (terminated %d)", OS_IsThreadTerminated(&sThreads[0]));
    EXPECT_LOG("main joins", "worker runs", "destructor (0)", "main joined (terminated 1)");
}

static OSMutex sMutex;

static void MutexHolder(void *arg) {
    (void)arg;
    Log("L locks twice");
    OS_LockMutex(&sMutex);
    OS_LockMutex(&sMutex);
    Log("L sleeps holding it");
    OS_SleepThread(&sGate);
    Log("L unlocks");
    OS_UnlockMutex(&sMutex);
    OS_UnlockMutex(&sMutex);
    Log("L after unlock");
}

static void MutexWaiter(void *arg) {
    Log("%s waits", (const char *)arg);
    OS_LockMutex(&sMutex);
    Log("%s gets it", (const char *)arg);
    OS_UnlockMutex(&sMutex);
}

// Waiters queue by priority, the second unlock of a recursive lock frees it, and the wakeup preempts the unlocker.
// There is no priority inheritance: main runs while H1 waits for a mutex that the lower L holds
static void TestMutex(void) {
    Boot(VBlankHandler);
    OS_InitMutex(&sMutex);
    Create(0, MutexHolder, NULL, 8);
    Create(1, MutexWaiter, "H1", 10);
    Create(2, MutexWaiter, "H2", 12);
    OS_WakeupThreadDirect(&sThreads[0]);
    OS_WakeupThreadDirect(&sThreads[2]);
    OS_WakeupThreadDirect(&sThreads[1]);
    Log("main lowers L");
    OS_SetThreadPriority(&sThreads[0], 20);
    OS_WakeupThread(&sGate);
    Log("main joins L");
    OS_JoinThread(&sThreads[0]);
    Log("main done");
    EXPECT_LOG("L locks twice", "L sleeps holding it", "H2 waits", "H1 waits", "main lowers L", "main joins L",
               "L unlocks", "H1 gets it", "H2 gets it", "L after unlock", "main done");
}

static OSMessageQueue sMq;
static OSMessage sMessages[2];

static void Consumer(void *arg) {
    int i;

    (void)arg;
    for (i = 0; i < 5; i++) {
        OSMessage message;

        OS_ReceiveMessage(&sMq, &message, OS_MESSAGE_BLOCK);
        Log("C got %d", (int)(intptr_t)message);
    }
}

static void Producer(void *arg) {
    int i;

    (void)arg;
    for (i = 1; i <= 4; i++) {
        Log("P sends %d", i);
        OS_SendMessage(&sMq, (OSMessage)(intptr_t)i, OS_MESSAGE_BLOCK);
        Log("P sent %d", i);
    }
}

// A queue of two: a send to a full queue sleeps, each receive wakes the sender, which preempts the lower receiver;
// OS_JamMessage puts its message first
static void TestMessageQueue(void) {
    Boot(VBlankHandler);
    OS_InitMessageQueue(&sMq, sMessages, 2);
    Create(0, Consumer, NULL, 20);
    Create(1, Producer, NULL, 18);
    OS_WakeupThreadDirect(&sThreads[0]);
    OS_WakeupThreadDirect(&sThreads[1]);
    Log("main jams 100");
    OS_JamMessage(&sMq, (OSMessage)(intptr_t)100, OS_MESSAGE_NOBLOCK);
    Log("main joins C");
    OS_JoinThread(&sThreads[0]);
    Log("main done");
    EXPECT_LOG("main jams 100", "main joins C", "P sends 1", "P sent 1", "P sends 2", "P sent 2", "P sends 3",
               "C got 100", "P sent 3", "P sends 4", "C got 1", "P sent 4", "C got 2", "C got 3", "C got 4",
               "main done");
}

static void SleepLoop(void *arg) {
    (void)arg;
    Log("L starts");
    for (;;) {
        OS_Sleep(5);
        Log("L wakes at %u ms", (unsigned)(hal_os_cycles() / CYCLES_PER_MS));
    }
}

// The virtual clock orders the alarms of OS_Sleep and the VBlanks: a lower thread sleeping 5 ms at a time wakes
// three times a frame, and the VBlank (12.2 ms into each frame) wakes main from OS_WaitIrq in between
static void TestVBlankAndAlarms(void) {
    int frame;

    Boot(LoggingVBlankHandler);
    Create(0, SleepLoop, NULL, 20);
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main waits");
    for (frame = 1; frame <= 3; frame++) {
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        Log("main frame %d", frame);
    }
    OS_DestroyThread(&sThreads[0]);
    Log("main done");
    EXPECT_LOG("main waits", "L starts", "L wakes at 5 ms", "L wakes at 10 ms", "vblank 1 (irq mode 1, line 192)",
               "main frame 1", "L wakes at 15 ms", "L wakes at 20 ms", "L wakes at 25 ms",
               "vblank 2 (irq mode 1, line 192)", "main frame 2", "L wakes at 30 ms", "L wakes at 35 ms",
               "L wakes at 40 ms", "L wakes at 45 ms", "vblank 3 (irq mode 1, line 192)", "main frame 3", "main done");
}

static void WakingVBlankHandler(void) {
    Log("handler wakes H");
    OS_WakeupThread(&sQueue);
    Log("handler returns (switch asked %d)", OSi_ThreadInfo.isNeedRescheduling);
    hal_os_set_irq_check_flag(OS_IE_V_BLANK);
}

static void QueueLoop(void *arg) {
    (void)arg;
    for (;;) {
        OS_SleepThread(&sQueue);
        Log("H runs");
    }
}

// A wakeup inside a handler only asks for a switch (OSi_RescheduleThread sees the IRQ mode); the tail makes it, after
// the handler, and the highest thread that is ready runs first
static void TestIrqWakeupDeferred(void) {
    Boot(WakingVBlankHandler);
    Create(0, QueueLoop, NULL, 10);
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main waits");
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("main frame");
    EXPECT_LOG("main waits", "handler wakes H", "handler returns (switch asked 1)", "H runs", "main frame");
}

// An interrupt that comes while the CPSR's I bit is set waits for OS_RestoreInterrupts (here 20 ms in, at line 51 of
// the next frame); with it clear, it is taken inside the busy loop, at the VBlank's own line
static void TestIrqWhileDisabled(void) {
    u32 enabled;

    Boot(LoggingVBlankHandler);
    Log("main disables");
    enabled = OS_DisableInterrupts();
    hal_os_spin(20 * CYCLES_PER_MS);
    Log("main spun (pending %d)", (hal_os_get_if() & OS_IE_V_BLANK) != 0);
    OS_RestoreInterrupts(enabled);
    Log("main restored");
    hal_os_spin(CYCLES_PER_FRAME);
    Log("main spun with interrupts on");
    EXPECT_LOG("main disables", "main spun (pending 1)", "vblank 1 (irq mode 1, line 51)", "main restored",
               "vblank 2 (irq mode 1, line 192)", "main spun with interrupts on");
}

static void BusyWaiter(void *arg) {
    u64 start;

    (void)arg;
    Log("L busy-waits 40 ms");
    start = OS_GetTick();
    while (OS_GetTick() - start < OS_MilliSecondsToTicks(40)) {
    }
    Log("L done");
}

// The pattern of ir_check.c: a busy wait on OS_GetTick in a low thread. The clock moves at each call, the VBlanks
// come while it spins, and main preempts it at each one, as the DS's IRQ return does
static void TestBusyWaitPreempted(void) {
    int frame;

    Boot(VBlankHandler);
    Create(0, BusyWaiter, NULL, 20);
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main waits");
    for (frame = 1; frame <= 3; frame++) {
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        Log("main frame %d", frame);
    }
    EXPECT_LOG("main waits", "L busy-waits 40 ms", "main frame 1", "main frame 2", "L done", "main frame 3");
}

static void LockedWaiter(void *arg) {
    (void)arg;
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("H2 wakes under the lock");
    // The count is global, so this unlocks main's lock; a thread that blocked while it is held would keep running
    OS_EnableScheduler();
}

// OS_DisableScheduler holds back the switches of OSi_RescheduleThread, but not the IRQ tail's, which doesn't look at
// the count (as in the ITCM assembly)
static void TestSchedulerLock(void) {
    Boot(VBlankHandler);
    Create(0, LogThread, "H", 10);
    OS_DisableScheduler();
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main, scheduler locked");
    OS_EnableScheduler();
    Log("main, scheduler unlocked");
    OS_RescheduleThread();
    Log("main after the reschedule");

    Create(1, LockedWaiter, NULL, 10);
    OS_WakeupThreadDirect(&sThreads[1]);
    OS_DisableScheduler();
    Log("main locks and spins");
    hal_os_spin(CYCLES_PER_FRAME);
    Log("main back");
    EXPECT_LOG("main, scheduler locked", "main, scheduler unlocked", "H runs", "main after the reschedule",
               "main locks and spins", "H2 wakes under the lock", "main back");
}

static void CountThread(void *arg) {
    (void)arg;
    sCounter++;
}

// OS_DestroyThread on a sleeping thread disposes of its suspended fiber, and the game's sound loader pattern (create,
// run to the end, create again on the same OSThread) leaves nothing behind
static void TestDestroyAndRecreate(void) {
    u32 alive;
    int i;

    Boot(VBlankHandler);
    alive = hal_os_stats().fibersAlive;
    Create(0, SleeperThread, "T", 10);
    OS_WakeupThreadDirect(&sThreads[0]);
    OS_DestroyThread(&sThreads[0]);
    Log("destroyed (extra fibers %u)", hal_os_stats().fibersAlive - alive);
    Create(0, SleeperThread, "never", 10);
    OS_DestroyThread(&sThreads[0]);
    for (i = 0; i < 1000; i++) {
        Create(0, CountThread, NULL, 10);
        OS_WakeupThreadDirect(&sThreads[0]);
    }
    Log("ran %d times (extra fibers %u)", sCounter, hal_os_stats().fibersAlive - alive);
    EXPECT_LOG("T sleeps", "destroyed (extra fibers 0)", "ran 1000 times (extra fibers 0)");
}

static void KilledThread(void *arg) {
    (void)arg;
    OS_SetThreadDestructor(OS_GetCurrentThread(), Destructor);
    Log("T sleeps");
    OS_SleepThread(&sQueue);
    Log("T never gets here");
}

// OS_KillThread restarts the thread's context at OSi_ExitThread with the value for its destructor
static void TestKillThread(void) {
    Boot(VBlankHandler);
    Create(0, KilledThread, NULL, 10);
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main kills T");
    OS_KillThread(&sThreads[0], (void *)0x1234);
    Log("main after (terminated %d, queue empty %d)", OS_IsThreadTerminated(&sThreads[0]), sQueue.head == NULL);
    EXPECT_LOG("T sleeps", "main kills T", "destructor (0x1234)", "main after (terminated 1, queue empty 1)");
}

static const char *ThreadName(const OSThread *thread) {
    static const char *const names[] = {"main", "idle", "H"};

    return thread->id < 3 ? names[thread->id] : "?";
}

static void LogSwitch(OSThread *from, OSThread *to) {
    Log("switch %s->%s", ThreadName(from), ThreadName(to));
}

// The switch callback sees every switch, the IRQ tail's included
static void TestSwitchCallback(void) {
    Boot(WakingVBlankHandler);
    OS_SetSwitchThreadCallback(LogSwitch);
    Create(0, QueueLoop, NULL, 10);
    OS_WakeupThreadDirect(&sThreads[0]);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("main frame");
    OS_SetSwitchThreadCallback(NULL);
    EXPECT_LOG("switch main->H", "switch H->main", "switch main->idle", "handler wakes H",
               "handler returns (switch asked 1)", "switch idle->H", "H runs", "switch H->main", "main frame");
}

static u32 sLines[300];
static int sLineCount;
static int sMainSwitchIns;

static void HBlankHandler(void) {
    if (sLineCount < 300) {
        sLines[sLineCount++] = hal_os_vcount();
    }
}

static void VBlankLinesHandler(void) {
    Log("vblank at line %u after %d hblanks", hal_os_vcount(), sLineCount);
    hal_os_set_irq_check_flag(OS_IE_V_BLANK);
}

static void CountMainSwitches(OSThread *from, OSThread *to) {
    (void)from;
    if (to->id == 0) {
        sMainSwitchIns++;
    }
}

// Per-line HBlank interrupts while main waits for the VBlank from line 0: every interrupt wakes the threads in
// OS_WaitIrq, so main wakes and sleeps again on each of the 192 lines before the VBlank, as on the DS
static void TestHBlankLines(void) {
    int i;
    int inOrder = 1;

    Boot(VBlankLinesHandler);
    sLineCount = 0;
    sMainSwitchIns = 0;
    OS_SetIrqFunction(OS_IE_H_BLANK, HBlankHandler);
    OS_SetSwitchThreadCallback(CountMainSwitches);
    OS_EnableIrqMask(OS_IE_H_BLANK);
    hal_os_set_dispstat(REG_GX_DISPSTAT_VBI_MASK | REG_GX_DISPSTAT_HBI_MASK);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    OS_SetSwitchThreadCallback(NULL);
    for (i = 0; i < sLineCount && i < 192; i++) {
        inOrder &= sLines[i] == (u32)i;
    }
    Log("lines in order %d, main switched in %d times", inOrder, sMainSwitchIns);
    EXPECT_LOG("vblank at line 192 after 192 hblanks", "lines in order 1, main switched in 193 times");
}

// main.c's loop as BW2 runs it, with the HBlank interrupt on for good (main.c enables it in DISPSTAT and IE before
// GameInit): the frame's logic (100 OS_GetTick calls, 3 lines of the clock) runs right after the VBlank, and main
// then waits through the other 260 lines, each of whose HBlanks wakes it
static void TestHBlankMainLoop(void) {
    int frame;

    Boot(VBlankHandler);
    OS_SetIrqFunction(OS_IE_H_BLANK, HBlankHandler);
    OS_EnableIrqMask(OS_IE_H_BLANK);
    hal_os_set_dispstat(REG_GX_DISPSTAT_VBI_MASK | REG_GX_DISPSTAT_HBI_MASK);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    OS_SetSwitchThreadCallback(CountMainSwitches);
    for (frame = 1; frame <= 3; frame++) {
        int i;

        sMainSwitchIns = 0;
        sLineCount = 0;
        for (i = 0; i < 100; i++) {
            (void)OS_GetTick();
        }
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        Log("frame %d: %d hblanks, main switched in %d times", frame, sLineCount, sMainSwitchIns);
    }
    OS_SetSwitchThreadCallback(NULL);
    EXPECT_LOG("frame 1: 263 hblanks, main switched in 261 times", "frame 2: 263 hblanks, main switched in 261 times",
               "frame 3: 263 hblanks, main switched in 261 times");
}

static void VCountHandler(void) {
    Log("vcount at line %u (dispstat %#x)", hal_os_vcount(), hal_os_get_dispstat());
}

static void FrameHook(u32 frame) {
    Log("present %u (host stack %d, line %u)", frame, hal_fiber_current() == NULL, hal_os_vcount());
}

// DISPSTAT decides whether the display requests its interrupts at all; IE only whether a request is taken. With IE's
// HBlank bit set and DISPSTAT's off, as net_err.c and save_error.c leave them with GX_HBlankIntr(FALSE), no HBlank
// comes. With the VBlank's off too, the frames still end and are presented, but no VBlank is requested. A V-count
// interrupt (OS_SetVAlarm's, which CARD's ROM lock polling uses) comes at the start of its line
static void TestDispStatGatesBlanks(void) {
    Boot(LoggingVBlankHandler);
    sLineCount = 0;
    OS_SetIrqFunction(OS_IE_H_BLANK, HBlankHandler);
    OS_EnableIrqMask(OS_IE_H_BLANK);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("HBlank on in IE only: %d hblanks", sLineCount);

    hal_os_set_frame_hook(FrameHook);
    hal_os_set_dispstat(0);
    hal_os_spin(CYCLES_PER_FRAME);
    hal_os_set_frame_hook(NULL);
    Log("display interrupts off: frame %u, IF %#x", hal_os_frame(), hal_os_get_if());

    OS_SetIrqFunction(OS_IE_V_COUNT, VCountHandler);
    OS_EnableIrqMask(OS_IE_V_COUNT);
    hal_os_set_dispstat(REG_GX_DISPSTAT_VBI_MASK | REG_GX_DISPSTAT_VCI_MASK | (100 << 8));
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("main frame %u", hal_os_frame());
    EXPECT_LOG("vblank 1 (irq mode 1, line 192)", "HBlank on in IE only: 0 hblanks",
               "present 2 (host stack 1, line 192)", "display interrupts off: frame 2, IF 0",
               "vcount at line 100 (dispstat 0x642c)", "vblank 3 (irq mode 1, line 192)", "main frame 3");
}

// The frame hook, where the HAL presents and paces, runs on the dispatcher's stack at each VBlank, never on the fiber
// whose clock crossed it, and before the VBlank's handler: here the crossings come in a busy wait on OS_GetTick, in a
// spin with interrupts disabled (the frame is presented at once, the handler waits for OS_RestoreInterrupts) and in
// the idle thread's halt
static void TestFrameHookOnHostStack(void) {
    u64 start;
    int i;

    Boot(LoggingVBlankHandler);
    hal_os_set_frame_hook(FrameHook);
    start = OS_GetTick();
    while (OS_GetTick() - start < OS_MilliSecondsToTicks(20)) {
    }
    Log("main busy-waited 20 ms");
    for (i = 0; i < 3; i++) {
        u32 enabled = OS_DisableInterrupts();

        hal_os_spin(CYCLES_PER_FRAME / 2);
        Log("main spun half a frame, interrupts disabled");
        OS_RestoreInterrupts(enabled);
    }
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    hal_os_set_frame_hook(NULL);
    Log("main waited");
    EXPECT_LOG("present 1 (host stack 1, line 192)", "vblank 1 (irq mode 1, line 192)", "main busy-waited 20 ms",
               "main spun half a frame, interrupts disabled", "present 2 (host stack 1, line 192)",
               "main spun half a frame, interrupts disabled", "vblank 2 (irq mode 1, line 51)",
               "main spun half a frame, interrupts disabled", "present 3 (host stack 1, line 192)",
               "vblank 3 (irq mode 1, line 192)", "main waited");
}

// Loading a context whose I bit is clear takes a pending interrupt before the thread's first instruction. Main sets
// the I bit, a VBlank comes and waits, and main wakes a higher, new thread, whose context starts with the I bit clear:
// the handler runs before it does
static void TestIrqOnSwitchIn(void) {
    u32 enabled;

    Boot(LoggingVBlankHandler);
    Create(0, LogThread, "H", 10);
    enabled = OS_DisableInterrupts();
    hal_os_spin(CYCLES_PER_FRAME);
    Log("main wakes H (pending %d)", (hal_os_get_if() & OS_IE_V_BLANK) != 0);
    OS_WakeupThreadDirect(&sThreads[0]);
    Log("main back");
    OS_RestoreInterrupts(enabled);
    Log("main restored");
    EXPECT_LOG("main wakes H (pending 1)", "vblank 1 (irq mode 1, line 0)", "H runs", "main back", "main restored");
}

// backup_card.c's write (func_0203bfd4): an async call whose completion callback sets a plain flag, then a loop that
// makes no call at all until the flag is set. On the DS the card thread (priority 4) and the ARM7's PXI interrupt set
// it while the loop spins. On the host nothing runs inside such a loop, so the HAL's async services must finish
// before they return. The three ways a completion can come:
//   - at once, inside the call: the loop never turns;
//   - from a higher thread that the call wakes (the shape of CARD's own task thread), with the ARM7's reply given
//     inside the request: the wakeup preempts the caller and the flag is set when the call returns;
//   - as a later interrupt (a timer here, as a PXI or DMA completion would be): the pure loop spins without end,
//     since only a call into the HAL moves the clock. A loop that waits through the HAL (OS_SpinWaitSysCycles and
//     SVC_WaitByLoop become hal_os_spin, as in PMi_SendPxiDataSync) does see it.
static BOOL sWriteDone; // plain, like backup_card.c's sWriteDone
static void (*sTaskCallback)(void *);
static OSAlarm sReplyAlarm;

static void SetWriteDone(void *arg) {
    (void)arg;
    sWriteDone = TRUE;
}

static void WriteAsyncAtOnce(void (*callback)(void *)) {
    Log("write, done inside the call");
    callback(NULL);
}

// CARDi_OldTypeTaskThread's shape: sleeps until a task is set, then runs it to the end
static void CardTaskThread(void *arg) {
    (void)arg;
    for (;;) {
        u32 enabled = OS_DisableInterrupts();
        void (*callback)(void *);

        while (sTaskCallback == NULL) {
            OS_SleepThread(NULL);
        }
        OS_RestoreInterrupts(enabled);
        callback = sTaskCallback;
        sTaskCallback = NULL;
        Log("task thread writes, the reply comes inside the request");
        callback(NULL);
    }
}

static void WriteAsyncOnTaskThread(void (*callback)(void *)) {
    sTaskCallback = callback;
    OS_WakeupThreadDirect(&sThreads[0]);
}

static void WriteAsyncAsInterrupt(void (*callback)(void *)) {
    OS_CreateAlarm(&sReplyAlarm);
    OS_SetAlarm(&sReplyAlarm, OS_MilliSecondsToTicks(1), callback, NULL);
}

// backup_card.c's loop, with a bound so that the test ends
static u32 PollWriteDone(void) {
    u32 turns = 0;

    while (!sWriteDone && turns < 1000000) {
        turns++;
    }
    return turns;
}

static void TestAsyncCompletion(void) {
    u64 before;
    u32 turns;

    Boot(VBlankHandler);
    Create(0, CardTaskThread, NULL, 4);
    OS_WakeupThreadDirect(&sThreads[0]);

    sWriteDone = FALSE;
    WriteAsyncAtOnce(SetWriteDone);
    Log("loop turned %u times", PollWriteDone());

    sWriteDone = FALSE;
    Log("main writes");
    WriteAsyncOnTaskThread(SetWriteDone);
    Log("loop turned %u times", PollWriteDone());

    sWriteDone = FALSE;
    WriteAsyncAsInterrupt(SetWriteDone);
    before = hal_os_cycles();
    turns = PollWriteDone();
    Log("as an interrupt: done %d after %u turns, clock +%u", sWriteDone, turns, (u32)(hal_os_cycles() - before));
    for (turns = 0; !sWriteDone; turns++) {
        hal_os_spin(1000);
    }
    Log("waiting through hal_os_spin: done after %u turns", turns);
    OS_DestroyThread(&sThreads[0]);
    EXPECT_LOG("write, done inside the call", "loop turned 0 times", "main writes",
               "task thread writes, the reply comes inside the request", "loop turned 0 times",
               "as an interrupt: done 0 after 1000000 turns, clock +0",
               "waiting through hal_os_spin: done after 34 turns");
}

// With nothing enabled in IE, a run where every thread waits ends as a deadlock instead of hanging
static void DeadlockMain(void) {
    OS_InitThread();
    OS_InitThreadQueue(&sQueue);
    OS_EnableInterrupts();
    OS_SleepThread(&sQueue);
    Log("never");
}

// A frame loop shaped like main.c's, with a sound loader and a stream thread. The log must be the same each run
static OSThread sSndThread;
static OSThread sStrmThread;
static u32 sSndStack[0x400 / sizeof(u32)];
static u32 sStrmStack[0x1000 / sizeof(u32)];
static OSMessageQueue sRequests;
static OSMessage sRequestBuffer[4];

static void SndThread(void *arg) {
    (void)arg;
    for (;;) {
        OSMessage request;

        OS_ReceiveMessage(&sRequests, &request, OS_MESSAGE_BLOCK);
        OS_LockMutex(&sMutex);
        Log("snd loads %d at frame %u line %u", (int)(intptr_t)request, hal_os_frame(), hal_os_vcount());
        hal_os_spin(3 * CYCLES_PER_MS);
        OS_UnlockMutex(&sMutex);
        OS_Sleep(2);
    }
}

static void StrmThread(void *arg) {
    (void)arg;
    for (;;) {
        OS_Sleep(4);
        OS_LockMutex(&sMutex);
        sCounter++;
        OS_UnlockMutex(&sMutex);
    }
}

static void GameLoop(void) {
    int frame;

    Boot(VBlankHandler);
    OS_InitMutex(&sMutex);
    OS_InitMessageQueue(&sRequests, sRequestBuffer, 4);
    OS_CreateThread(&sSndThread, SndThread, NULL, sSndStack + NELEMS(sSndStack), sizeof(sSndStack), 17);
    OS_CreateThread(&sStrmThread, StrmThread, NULL, sStrmStack + NELEMS(sStrmStack), sizeof(sStrmStack), 12);
    OS_WakeupThreadDirect(&sSndThread);
    OS_WakeupThreadDirect(&sStrmThread);
    for (frame = 0; frame < 60; frame++) {
        if (frame % 10 == 0) {
            OS_SendMessage(&sRequests, (OSMessage)(intptr_t)frame, OS_MESSAGE_NOBLOCK);
        }
        if (frame % 7 == 0) {
            OS_Sleep(1);
            Log("main slept at frame %u line %u", hal_os_frame(), hal_os_vcount());
        }
        OS_SleepThreadDirect(&sSndThread, NULL);
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        OS_WakeupThreadDirect(&sSndThread);
    }
    Log("stream ticks %d", sCounter);
    OS_DestroyThread(&sSndThread);
    OS_DestroyThread(&sStrmThread);
}

static u32 HashLog(void) {
    u32 hash = 2166136261u;
    int i;
    const char *p;

    for (i = 0; i < sLogCount; i++) {
        for (p = sLog[i]; *p != '\0'; p++) {
            hash = (hash ^ (u8)*p) * 16777619u;
        }
        hash = (hash ^ '\n') * 16777619u;
    }
    return hash;
}

typedef struct Test {
    const char *name;
    void (*run)(void);
} Test;

static const Test sTests[] = {
    {"wakeup_preempts", TestWakeupPreempts},
    {"yield_order", TestYieldOrder},
    {"queue_wake_order", TestQueueWakeOrder},
    {"join_destructor", TestJoinDestructor},
    {"mutex", TestMutex},
    {"message_queue", TestMessageQueue},
    {"vblank_and_alarms", TestVBlankAndAlarms},
    {"irq_wakeup_deferred", TestIrqWakeupDeferred},
    {"irq_while_disabled", TestIrqWhileDisabled},
    {"busy_wait_preempted", TestBusyWaitPreempted},
    {"scheduler_lock", TestSchedulerLock},
    {"destroy_and_recreate", TestDestroyAndRecreate},
    {"kill_thread", TestKillThread},
    {"switch_callback", TestSwitchCallback},
    {"hblank_lines", TestHBlankLines},
    {"hblank_main_loop", TestHBlankMainLoop},
    {"dispstat_gates_blanks", TestDispStatGatesBlanks},
    {"frame_hook_on_host_stack", TestFrameHookOnHostStack},
    {"irq_on_switch_in", TestIrqOnSwitchIn},
    {"async_completion", TestAsyncCompletion},
};

static int RunTest(const Test *test) {
    sLogCount = 0;
    sFailed = 0;
    Check(hal_os_run(test->run) == 0, "the run ended normally");
    printf("%s: %s\n", test->name, sFailed ? "FAIL" : "ok");
    return sFailed;
}

static int RunDeadlock(void) {
    sLogCount = 0;
    sFailed = 0;
    Check(hal_os_run(DeadlockMain) == -1, "the run reports the deadlock");
    Check(sLogCount == 0, "nothing ran after the sleep");
    printf("deadlock: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}

static int RunDeterminism(void) {
    u32 hashes[2];
    int logCount[2];
    int i;

    sFailed = 0;
    for (i = 0; i < 2; i++) {
        sLogCount = 0;
        Check(hal_os_run(GameLoop) == 0, "the run ended normally");
        hashes[i] = HashLog();
        logCount[i] = sLogCount;
    }
    Check(hashes[0] == hashes[1] && logCount[0] == logCount[1], "both runs logged the same");
    Check(logCount[0] > 10, "the run logged something");
    printf("determinism: %s (%d lines, hash %08x)\n", sFailed ? "FAIL" : "ok", logCount[0], hashes[0]);
    if (sFailed || getenv("OS_FIBERS_VERBOSE") != NULL) {
        for (i = 0; i < sLogCount; i++) {
            printf("  %s\n", sLog[i]);
        }
    }
    return sFailed;
}

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "all";
    int failures = 0;
    int ran = 0;
    size_t i;

    setvbuf(stdout, NULL, _IONBF, 0);
    // OS_FIBERS_STACK_KIB runs everything on smaller host stacks, to find the least that works
    if (getenv("OS_FIBERS_STACK_KIB") != NULL) {
        u32 size = (u32)atoi(getenv("OS_FIBERS_STACK_KIB")) * 1024;

        hal_os_set_stack_sizes(size, size);
    }
    for (i = 0; i < NELEMS(sTests); i++) {
        if (strcmp(name, "all") == 0 || strcmp(name, sTests[i].name) == 0) {
            failures += RunTest(&sTests[i]);
            ran++;
        }
    }
    if (strcmp(name, "all") == 0 || strcmp(name, "deadlock") == 0) {
        failures += RunDeadlock();
        ran++;
    }
    if (strcmp(name, "all") == 0 || strcmp(name, "determinism") == 0) {
        failures += RunDeterminism();
        ran++;
    }
    if (ran == 0) {
        printf("no test named %s\n", name);
        return 2;
    }
    return failures != 0;
}
