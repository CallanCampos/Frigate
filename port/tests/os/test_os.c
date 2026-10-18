// Tests of the OS core (port/hal/os): each scenario runs as the game's main() would, as the DS's launcher thread, logs
// what happens in order and compares the log with the order the DS gives. The scheduler under test is the decomp's own
// os_thread.c, os_mutex.c and os_message.c, linked as the game links them; the HAL only switches, takes interrupts
// and keeps time. Most scenarios come from the reviewed spike (port/spikes/os-fibers), adapted to the real layer. What
// they cover, every part of the game runs on: the game's main loop and its threads (the card's, the sound loader),
// VBlank and HBlank, the timers and alarms, and the timed work that the ARM7 sound driver's alarms use.
//
//   port_os_tests NAME | all
//
// PORT_OS_STACK_KIB runs everything on smaller host stacks; PORT_OS_VERBOSE prints every scenario's log.
#include "hal/os/fiber.h"
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/svc.h"
#include <execinfo.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_MAX 512
#define LOG_LINE 80
// The DS's clock, for durations in the scenarios
#define CYCLES_PER_MS 33514
#define CYCLES_PER_FRAME PORT_OS_CYCLES_PER_FRAME
#define CYCLES_PER_LINE PORT_OS_CYCLES_PER_LINE

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
        printf("  log differs\n  %-50s  %s\n", "got", "expected");
        for (i = 0; i < sLogCount || i < count; i++) {
            printf("  %-50s  %s\n", i < sLogCount ? sLog[i] : "", i < count ? expected[i] : "");
        }
        sFailed = 1;
    }
}

#define EXPECT_LOG(...)                                                                                                \
    do {                                                                                                               \
        static const char *const expected[] = { __VA_ARGS__ };                                                         \
        ExpectLog(expected, (int)(sizeof(expected) / sizeof(expected[0])));                                            \
    } while (0)

// The scenarios' frame hook: one hook is added for the whole process (port_os_add_frame_hook keeps them), and calls
// what the scenario sets
static void (*sFrameHook)(void);

static void FrameHooks(void) {
    if (sFrameHook != NULL) {
        sFrameHook();
    }
}

// Threads with DS-sized stack buffers: the SDK writes its magic words there, the code runs on a host fiber
static OSThread sThreads[8];
static u32 sStacks[8][0x400 / sizeof(u32)];
static OSThreadQueue sQueue;
static OSThreadQueue sGate;
static int sCounter;

static void Create(int n, void (*func)(void *), void *arg, u32 priority) {
    OS_CreateThread(&sThreads[n], func, arg, sStacks[n] + 0x400 / sizeof(u32), 0x400, priority);
}

// The handlers set OS_SetIrqCheckFlag's word through the HAL: the SDK's inline reaches it through HW_DTCM, which
// truncates the host's address (port/hal/os/os_irqTable.c)
static void VBlankHandler(void) {
    port_os_set_irq_check_flag(OS_IE_V_BLANK);
}

static void LoggingVBlankHandler(void) {
    Log("vblank %u (irq mode %d, line %u)", port_os_frame(), OS_GetProcMode() == OS_PROCMODE_IRQ, port_os_vcount());
    port_os_set_irq_check_flag(OS_IE_V_BLANK);
}

// What the game's main does before its loop (SystemInit's OS_Init and OS_InitTick, the VBlank handler, IE, IME,
// DISPSTAT's VBlank interrupt and the CPSR), with the alarms that OS_Sleep needs
static void Boot(OSIrqFunction vblank) {
    OS_InitThread();
    OS_InitIrqTable();
    OS_InitTick();
    OS_InitAlarm();
    OS_InitThreadQueue(&sQueue);
    OS_InitThreadQueue(&sGate);
    sCounter = 0;
    OS_SetIrqFunction(OS_IE_V_BLANK, vblank);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    (void)OS_EnableIrq();
    GX_VBlankIntr(TRUE);
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
        Log("L wakes at %u ms", (unsigned)(port_os_cycles() / CYCLES_PER_MS));
    }
}

// The virtual clock orders the alarms of OS_Sleep (timer 1, through the SDK's alarm queue) and the VBlanks: a lower
// thread sleeping 5 ms at a time wakes three times a frame, and the VBlank (12.2 ms into each frame) wakes main from
// OS_WaitIrq in between
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
    port_os_set_irq_check_flag(OS_IE_V_BLANK);
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
    port_os_spin(20 * CYCLES_PER_MS);
    Log("main spun (pending %d)", (port_os_get_if() & OS_IE_V_BLANK) != 0);
    OS_RestoreInterrupts(enabled);
    Log("main restored");
    port_os_spin(CYCLES_PER_FRAME);
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
    port_os_spin(CYCLES_PER_FRAME);
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
    alive = port_os_stats().fibersAlive;
    Create(0, SleeperThread, "T", 10);
    OS_WakeupThreadDirect(&sThreads[0]);
    OS_DestroyThread(&sThreads[0]);
    Log("destroyed (extra fibers %u)", port_os_stats().fibersAlive - alive);
    Create(0, SleeperThread, "never", 10);
    OS_DestroyThread(&sThreads[0]);
    for (i = 0; i < 1000; i++) {
        Create(0, CountThread, NULL, 10);
        OS_WakeupThreadDirect(&sThreads[0]);
    }
    Log("ran %d times (extra fibers %u)", sCounter, port_os_stats().fibersAlive - alive);
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
    static const char *const names[] = { "main", "idle", "H" };

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
        sLines[sLineCount++] = port_os_vcount();
    }
}

static void VBlankLinesHandler(void) {
    Log("vblank at line %u after %d hblanks", port_os_vcount(), sLineCount);
    port_os_set_irq_check_flag(OS_IE_V_BLANK);
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
    GX_HBlankIntr(TRUE);
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
    GX_HBlankIntr(TRUE);
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
    Log("vcount at line %u (dispstat %#x)", port_os_vcount(), reg_GX_DISPSTAT);
}

static void LogPresent(void) {
    Log("present %u (host stack %d, line %u)", port_os_frame(), port_fiber_current() == NULL, port_os_vcount());
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

    sFrameHook = LogPresent;
    GX_VBlankIntr(FALSE);
    port_os_spin(CYCLES_PER_FRAME);
    sFrameHook = NULL;
    Log("display interrupts off: frame %u, IF %#x", port_os_frame(), port_os_get_if());

    OS_SetIrqFunction(OS_IE_V_COUNT, VCountHandler);
    OS_EnableIrqMask(OS_IE_V_COUNT);
    reg_GX_DISPSTAT = REG_GX_DISPSTAT_VBI_MASK | REG_GX_DISPSTAT_VCI_MASK | (100 << 8);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("main frame %u", port_os_frame());
    EXPECT_LOG("vblank 1 (irq mode 1, line 192)", "HBlank on in IE only: 0 hblanks",
               "present 2 (host stack 1, line 192)", "display interrupts off: frame 2, IF 0",
               "vcount at line 100 (dispstat 0x642c)", "vblank 3 (irq mode 1, line 192)", "main frame 3");
}

// The frame hooks, where the program presents and paces, run on the dispatcher's stack at each VBlank, never on the
// fiber whose clock crossed it, and before the VBlank's handler: here the crossings come in a busy wait on OS_GetTick,
// in a spin with interrupts disabled (the frame is presented at once, the handler waits for OS_RestoreInterrupts) and
// in the idle thread's halt
static void TestFrameHooksOnHostStack(void) {
    u64 start;
    int i;

    Boot(LoggingVBlankHandler);
    sFrameHook = LogPresent;
    start = OS_GetTick();
    while (OS_GetTick() - start < OS_MilliSecondsToTicks(20)) {
    }
    Log("main busy-waited 20 ms");
    for (i = 0; i < 3; i++) {
        u32 enabled = OS_DisableInterrupts();

        port_os_spin(CYCLES_PER_FRAME / 2);
        Log("main spun half a frame, interrupts disabled");
        OS_RestoreInterrupts(enabled);
    }
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    sFrameHook = NULL;
    Log("main waited");
    EXPECT_LOG("present 1 (host stack 1, line 192)", "vblank 1 (irq mode 1, line 192)", "main busy-waited 20 ms",
               "main spun half a frame, interrupts disabled", "present 2 (host stack 1, line 192)",
               "main spun half a frame, interrupts disabled", "vblank 2 (irq mode 1, line 51)",
               "main spun half a frame, interrupts disabled", "present 3 (host stack 1, line 192)",
               "vblank 3 (irq mode 1, line 192)", "main waited");
}

// Timer 3's interrupt, which the decomp's os.h doesn't name
#define TIMER3_IRQ (OS_IE_TIMER0 << 3)

// The SDK calls a timer's function once (OSi_IrqCallback), so it sets itself again
static void TimedWorkHandler(void) {
    Log("timer 3 (irq mode %d, line %u)", OS_GetProcMode() == OS_PROCMODE_IRQ, port_os_vcount());
    OS_SetIrqFunction(TIMER3_IRQ, TimedWorkHandler);
}

static void TimedWork(void) {
    Log("work (host stack %d, line %u)", port_fiber_current() == NULL, port_os_vcount());
    port_os_request_irq(TIMER3_IRQ);
    if (port_os_vcount() == 50) {
        port_os_set_timed_work(TimedWork, port_os_cycles() + 50 * PORT_OS_CYCLES_PER_LINE);
    }
}

static void CancelledWork(void) {
    Log("cancelled work");
}

// A module's timed work (port_os_set_timed_work, as the ARM7's sound alarms use it) runs on the host's stack when the
// clock reaches its moment, as an event of the clock, so that an interrupt it requests (timer 3's here, which the OS
// doesn't use) is taken there: while main waits for the VBlank, at line 50 (the moment, moved there from line 20) and,
// set again from the work, at line 100. A moment cancelled with UINT64_MAX never comes, and one already past comes at
// once
static void TestTimedWork(void) {
    Boot(LoggingVBlankHandler);
    OS_SetIrqFunction(TIMER3_IRQ, TimedWorkHandler);
    OS_EnableIrqMask(TIMER3_IRQ);
    port_os_set_timed_work(TimedWork, port_os_cycles() + 20 * PORT_OS_CYCLES_PER_LINE);
    port_os_set_timed_work(TimedWork, port_os_cycles() + 50 * PORT_OS_CYCLES_PER_LINE);
    port_os_set_timed_work(CancelledWork, port_os_cycles() + 30 * PORT_OS_CYCLES_PER_LINE);
    port_os_set_timed_work(CancelledWork, UINT64_MAX);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("main waited");
    port_os_set_timed_work(TimedWork, 0);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("main waited again");
    EXPECT_LOG("work (host stack 1, line 50)", "timer 3 (irq mode 1, line 50)", "work (host stack 1, line 100)",
               "timer 3 (irq mode 1, line 100)", "vblank 1 (irq mode 1, line 192)", "main waited",
               "work (host stack 1, line 192)", "timer 3 (irq mode 1, line 192)", "vblank 2 (irq mode 1, line 192)",
               "main waited again");
}

// Loading a context whose I bit is clear takes a pending interrupt before the thread's first instruction. Main sets
// the I bit, a VBlank comes and waits, and main wakes a higher, new thread, whose context starts with the I bit clear:
// the handler runs before it does
static void TestIrqOnSwitchIn(void) {
    u32 enabled;

    Boot(LoggingVBlankHandler);
    Create(0, LogThread, "H", 10);
    enabled = OS_DisableInterrupts();
    port_os_spin(CYCLES_PER_FRAME);
    Log("main wakes H (pending %d)", (port_os_get_if() & OS_IE_V_BLANK) != 0);
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
//     SVC_WaitByLoop, as in PMi_SendPxiDataSync) does see it.
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
    before = port_os_cycles();
    turns = PollWriteDone();
    Log("as an interrupt: done %d after %u turns, clock +%u", sWriteDone, turns, (u32)(port_os_cycles() - before));
    for (turns = 0; !sWriteDone; turns++) {
        OS_SpinWaitSysCycles(1000);
    }
    Log("waiting through OS_SpinWaitSysCycles: done after %u turns", turns);
    OS_DestroyThread(&sThreads[0]);
    EXPECT_LOG("write, done inside the call", "loop turned 0 times", "main writes",
               "task thread writes, the reply comes inside the request", "loop turned 0 times",
               "as an interrupt: done 0 after 1000000 turns, clock +0",
               "waiting through OS_SpinWaitSysCycles: done after 34 turns");
}

// OS_WaitInterrupt halts with the thread still running (no switch to the idle thread) until the handler sets the
// check word; the modelled busy loops move the clock by their length: the BIOS's WaitByLoop four of the ARM9's cycles a
// turn, OS_SpinWaitSysCycles the cycles asked for less the call's own
static void TestWaits(void) {
    u64 before;

    Boot(VBlankHandler);
    OS_SetSwitchThreadCallback(LogSwitch);
    OS_WaitInterrupt(TRUE, OS_IE_V_BLANK);
    OS_SetSwitchThreadCallback(NULL);
    Log("OS_WaitInterrupt returned at line %u of frame %u", port_os_vcount(), port_os_frame());
    before = port_os_cycles();
    SVC_WaitByLoop(1000);
    Log("SVC_WaitByLoop(1000): %u cycles", (u32)(port_os_cycles() - before));
    before = port_os_cycles();
    OS_SpinWaitSysCycles(1000);
    Log("OS_SpinWaitSysCycles(1000): %u cycles", (u32)(port_os_cycles() - before));
    EXPECT_LOG("OS_WaitInterrupt returned at line 192 of frame 1", "SVC_WaitByLoop(1000): 2000 cycles",
               "OS_SpinWaitSysCycles(1000): 992 cycles");
}

static void FifoHandler(void) {
    Log("fifo handler (irq mode %d)", OS_GetProcMode() == OS_PROCMODE_IRQ);
}

// The interface for the HAL modules that model hardware raising an interrupt (PXI's receive FIFO here): a request only
// sets IF; the interrupt is taken at a defined point, port_os_poll_irq among them, and only while the code that runs
// could take it
static void TestRequestIrq(void) {
    u32 enabled;

    Boot(VBlankHandler);
    OS_SetIrqFunction(OS_IE_FIFO_RECV, FifoHandler);
    OS_EnableIrqMask(OS_IE_FIFO_RECV);
    Log("enabled %d", port_os_interrupts_enabled());
    port_os_request_irq(OS_IE_FIFO_RECV);
    Log("requested (IF %#x)", port_os_get_if());
    port_os_poll_irq();
    Log("polled (IF %#x)", port_os_get_if());

    enabled = OS_DisableInterrupts();
    Log("disabled: enabled %d", port_os_interrupts_enabled());
    port_os_request_irq(OS_IE_FIFO_RECV);
    port_os_poll_irq();
    Log("polled with interrupts disabled (IF %#x)", port_os_get_if());
    OS_RestoreInterrupts(enabled);
    Log("restored");

    (void)OS_DisableIrq();
    port_os_request_irq(OS_IE_FIFO_RECV);
    port_os_poll_irq();
    Log("IME off: enabled %d (IF %#x)", port_os_interrupts_enabled(), port_os_get_if());
    OS_ResetRequestIrqMask(OS_IE_FIFO_RECV);
    Log("acknowledged (IF %#x, reg %#x)", port_os_get_if(), reg_OS_IF);
    (void)OS_EnableIrq();
    port_os_poll_irq();
    Log("IME on, nothing pending");
    EXPECT_LOG("enabled 1", "requested (IF 0x40000)", "fifo handler (irq mode 1)", "polled (IF 0)",
               "disabled: enabled 0", "polled with interrupts disabled (IF 0x40000)", "fifo handler (irq mode 1)",
               "restored", "IME off: enabled 0 (IF 0x40000)", "acknowledged (IF 0, reg 0)", "IME on, nothing pending");
}

// What the scanline hook saw: the line, VCOUNT, whether it ran on the host's stack, and the line whose HBlank handler
// ran last
static int sHookLines[200];
static int sHookCount;
static int sHookFaults;
static int sLastHBlankLine = -1;

static void RecordingHBlankHandler(void) {
    sLastHBlankLine = (int)port_os_vcount();
}

static void ScanlineHook(int line) {
    int previous = line == 0 ? (int)PORT_OS_LINES_PER_FRAME - 1 : line - 1;

    if (sHookCount < 200) {
        sHookLines[sHookCount] = line;
    }
    sHookCount++;
    if (port_fiber_current() != NULL || GX_GetVCount() != line || sLastHBlankLine != previous) {
        sHookFaults++;
    }
}

// The renderer's hook runs once for each visible line of each frame, in order, on the host's stack, at the start of the
// line (VCOUNT is the line) and after the HBlank handler of the line before it has run, which is when the DS takes the
// registers that the line is drawn with. Here the HBlank handler runs on every line, from main's wait
static void TestScanlineHook(void) {
    int frame;
    int i;

    Boot(VBlankHandler);
    OS_SetIrqFunction(OS_IE_H_BLANK, RecordingHBlankHandler);
    OS_EnableIrqMask(OS_IE_H_BLANK);
    GX_HBlankIntr(TRUE);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    port_os_scanline_hook = ScanlineHook;
    for (frame = 1; frame <= 2; frame++) {
        int inOrder = 1;

        sHookCount = 0;
        sHookFaults = 0;
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        for (i = 0; i < sHookCount && i < 200; i++) {
            inOrder &= sHookLines[i] == i;
        }
        Log("frame %d: %d lines, in order %d, faults %d", frame, sHookCount, inOrder, sHookFaults);
    }
    port_os_scanline_hook = NULL;
    EXPECT_LOG("frame 1: 192 lines, in order 1, faults 0", "frame 2: 192 lines, in order 1, faults 0");
}

// VCOUNT and DISPSTAT's status flags follow the clock: the VBlank flag from line 192 to 261, the HBlank flag from
// 1606 cycles into each line, the V-count flag on the line DISPSTAT names
static void TestDisplayRegisters(void) {
    u64 lineStart;

    Boot(VBlankHandler);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    Log("after the VBlank: VCOUNT %d, flags %#x", GX_GetVCount(), reg_GX_DISPSTAT & REG_GX_DISPSTAT_STATUS_MASK);
    reg_GX_DISPSTAT = (u16)((reg_GX_DISPSTAT & ~0xff80) | (261 & 0xff) << 8 | ((261 >> 8) & 1) << 7);
    lineStart = port_os_cycles() / CYCLES_PER_LINE * CYCLES_PER_LINE;
    port_os_spin(lineStart + 69 * CYCLES_PER_LINE - port_os_cycles());
    Log("line %d: flags %#x", GX_GetVCount(), reg_GX_DISPSTAT & REG_GX_DISPSTAT_STATUS_MASK);
    port_os_spin(1606);
    Log("line %d, 1606 cycles in: flags %#x", GX_GetVCount(), reg_GX_DISPSTAT & REG_GX_DISPSTAT_STATUS_MASK);
    port_os_spin(CYCLES_PER_LINE);
    Log("line %d: flags %#x", GX_GetVCount(), reg_GX_DISPSTAT & REG_GX_DISPSTAT_STATUS_MASK);
    port_os_spin(CYCLES_PER_LINE - 1606);
    Log("line %d: VCOUNT %u", GX_GetVCount(), port_os_vcount());
    EXPECT_LOG("after the VBlank: VCOUNT 192, flags 0x1", "line 261: flags 0x5", "line 261, 1606 cycles in: flags 0x7",
               "line 262: flags 0x2", "line 0: VCOUNT 0");
}

static void TickHandlerLogger(void *arg) {
    (void)arg;
}

// Timer 0 behind OS_GetTick wraps every 65536 ticks and its interrupt counts the wraps in OSi_TickCounter; the tick is
// that count followed by the timer, and OS_GetTick corrects for a wrap whose interrupt is still pending, as the SDK
// does. Timer 1 runs the alarms
static void TestTimersAndTick(void) {
    u64 first;
    u64 tick;
    u32 enabled;

    Boot(VBlankHandler);
    first = OS_GetTick();
    Log("first tick %llu", (unsigned long long)first);
    port_os_spin(65536ull * 64);
    tick = OS_GetTick();
    Log("after a wrap: counter %llu, tick - first %llu", (unsigned long long)OSi_TickCounter,
        (unsigned long long)(tick - first));
    enabled = OS_DisableInterrupts();
    port_os_spin(65536ull * 64);
    tick = OS_GetTick();
    Log("wrap pending: counter %llu, IF timer0 %d, tick - first %llu", (unsigned long long)OSi_TickCounter,
        (port_os_get_if() & OS_IE_TIMER0) != 0, (unsigned long long)(tick - first));
    OS_RestoreInterrupts(enabled);
    Log("restored: counter %llu", (unsigned long long)OSi_TickCounter);
    OS_CreateAlarm(&sReplyAlarm);
    OS_SetAlarm(&sReplyAlarm, 100, TickHandlerLogger, NULL);
    Log("alarm set: timer 1 enabled in IE %d", (reg_OS_IE & OS_IE_TIMER1) != 0);
    OS_Sleep(1);
    Log("slept: alarm done %d", sReplyAlarm.handler == NULL);
    EXPECT_LOG("first tick 1", "after a wrap: counter 1, tick - first 65537",
               "wrap pending: counter 1, IF timer0 1, "
               "tick - first 131074",
               "restored: counter 2", "alarm set: timer 1 enabled in IE 1", "slept: alarm done 1");
}

static int sFramesSeen;

static void StopAtThirdFrame(void) {
    if (++sFramesSeen == 3) {
        port_os_stop();
    }
}

static void StopMain(void) {
    Boot(VBlankHandler);
    for (;;) {
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        Log("main frame %u", port_os_frame());
    }
}

// A frame hook ends the run (the program's --frames and window close): port_os_run returns where the dispatcher is,
// with the threads disposed of where they wait. The third frame's VBlank handler never runs
static int RunStopFromFrameHook(void) {
    PortOsResult result;

    sLogCount = 0;
    sFailed = 0;
    sFramesSeen = 0;
    sFrameHook = StopAtThirdFrame;
    result = port_os_run(StopMain);
    sFrameHook = NULL;
    Check(result == PORT_OS_STOPPED, "the run reports the stop");
    Check(port_os_frame() == 3, "the run stopped at the third frame");
    EXPECT_LOG("main frame 1", "main frame 2");
    printf("stop_from_frame_hook: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}

// The arenas are host memory, sized from the DS's: the main arena at least four times the DS's 1.9 MiB, ITCM's four
// times its 27 KiB, DTCM's inside the host's DTCM block where the DS has it (under BW2's 6 KiB system stack and 2 KiB
// IRQ stack), no extended main memory on a retail DS; OS_AllocFromArenaLo keeps whole host pointers. The IRQ stack
// checker's magic words are at the IRQ stack's top and end
static void TestArenas(void) {
    uintptr_t lo;
    uintptr_t hi;
    void *block;
    const u8 *dtcm = (const u8 *)SDK_AUTOLOAD_DTCM_START;

    OS_InitArena();
    OS_InitArenaEx();
    lo = (uintptr_t)OS_GetArenaLo(OS_ARENA_MAIN);
    hi = (uintptr_t)OS_GetArenaHi(OS_ARENA_MAIN);
    Log("main arena 4x the DS's %d, aligned %d", hi - lo >= 4 * 0x1e2f80u, lo % 32 == 0);
    lo = (uintptr_t)OS_GetArenaLo(OS_ARENA_ITCM);
    hi = (uintptr_t)OS_GetArenaHi(OS_ARENA_ITCM);
    Log("ITCM arena 4x the DS's %d", hi - lo >= 4 * 0x6c60u);
    lo = (uintptr_t)OS_GetArenaLo(OS_ARENA_DTCM);
    hi = (uintptr_t)OS_GetArenaHi(OS_ARENA_DTCM);
    Log("DTCM arena from %#lx to %#lx of DTCM", (unsigned long)(lo - (uintptr_t)SDK_AUTOLOAD_DTCM_START),
        (unsigned long)(hi - (uintptr_t)SDK_AUTOLOAD_DTCM_START));
    Log("extended arena %d %d", OS_GetArenaLo(OS_ARENA_MAINEX) == NULL, OS_GetArenaHi(OS_ARENA_MAINEX) == NULL);
    lo = (uintptr_t)OS_GetArenaLo(OS_ARENA_MAIN);
    block = OS_AllocFromArenaLo(OS_ARENA_MAIN, 100, 32);
    Log("allocated at lo %d, lo moved by %d", (uintptr_t)block == lo,
        (int)((uintptr_t)OS_GetArenaLo(OS_ARENA_MAIN) - lo));
    Log("too large: %d", OS_AllocFromArenaLo(OS_ARENA_ITCM, 1u << 30, 4) == NULL);
    OS_SetIrqStackChecker();
    Log("IRQ stack magic at 0x3f7c %d, at 0x3780 %d", *(const u32 *)(dtcm + 0x3f7c) == OS_STACK_BEGIN_MAGIC,
        *(const u32 *)(dtcm + 0x3780) == OS_STACK_END_MAGIC);
    EXPECT_LOG("main arena 4x the DS's 1, aligned 1", "ITCM arena 4x the DS's 1",
               "DTCM arena from 0xc0 to 0x1f80 of DTCM", "extended arena 1 1", "allocated at lo 1, lo moved by 128",
               "too large: 1", "IRQ stack magic at 0x3f7c 1, at 0x3780 1");
}

// The lock-ID pool gives the IDs from 0x40 up and takes them back; the card's lock word records its owner
static void TestLocks(void) {
    s32 first;
    s32 second;

    OS_InitLock();
    first = OS_GetLockID();
    second = OS_GetLockID();
    OS_ReleaseLockID((u16)first);
    Log("ids %#x %#x, again %#x", first, second, OS_GetLockID());
    Log("lock %d, owner %#x", OS_LockCard((u16)first), OS_ReadOwnerOfLockWord((OSLockWord *)HW_CARD_LOCK_BUF));
    Log("another tries: %d", OS_TryLockCard((u16)second) > 0);
    Log("unlock by another %d, by the owner %d", OS_UnlockCard((u16)second), OS_UnlockCard((u16)first));
    OS_ReleaseLockID((u16)0x7f);
    Log("0x7f back in the second word %d", (*(u32 *)(HW_INIT_LOCK_BUF + 4) & 1) != 0);
    EXPECT_LOG("ids 0x40 0x41, again 0x40", "lock 0, owner 0x40", "another tries: 1",
               "unlock by another -2, by the owner 0", "0x7f back in the second word 1");
}

// The host is a retail DS booted from its card: not a DSi, no development hardware, 4 MB of memory
static void TestConsole(void) {
    u32 type = OS_GetConsoleType();

    Log("boot type %u, TWL %d", OS_GetBootType(), OS_IsRunOnTwl());
    Log("NITRO %d, development %d, memory %u", (type & 0x80000000) != 0, (type & OS_CONSOLE_DEV_MASK) != 0, type & 0xf);
    EXPECT_LOG("boot type 1, TWL 0", "NITRO 1, development 0, memory 1");
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
        Log("snd loads %d at frame %u line %u", (int)(intptr_t)request, port_os_frame(), port_os_vcount());
        port_os_spin(3 * CYCLES_PER_MS);
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
            Log("main slept at frame %u line %u", port_os_frame(), port_os_vcount());
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

// Overflows of a thread's host stack, which must fault in its fiber's guard region (port/hal/os/fiber.c): frames of
// 1 KiB each, which step page by page, and one frame larger than the guard and the stack together, which only the
// stack probes catch (port/cmake/os.cmake). The handler runs on a stack of its own and names where the fault was
static BOOL sGuardHugeFrame;
static void *sGuardLow;
static void *sGuardHigh;

static void OnFault(int signal, siginfo_t *info, void *context) {
    static const char inGuard[] = "fault in guard page\n";
    static const char outside[] = "fault outside the guard page\n";
    char *address = info->si_addr;

    (void)signal;
    (void)context;
    if (address >= (char *)sGuardLow && address < (char *)sGuardHigh) {
        (void)!write(STDERR_FILENO, inGuard, sizeof(inGuard) - 1);
        _Exit(0);
    }
    (void)!write(STDERR_FILENO, outside, sizeof(outside) - 1);
    _Exit(1);
}

static void InstallFaultHandler(void) {
    static char altStack[64 * 1024];
    stack_t stack;
    struct sigaction action;

    stack.ss_sp = altStack;
    stack.ss_size = sizeof(altStack);
    stack.ss_flags = 0;
    sigaltstack(&stack, NULL);
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = OnFault;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
}

// Not inlined, so that the frames are the ones the test means: OverflowThread's own stays small
__attribute__((noinline)) static int Recurse(int depth) {
    volatile char frame[1024];

    frame[0] = (char)depth;
    if (depth == 0) {
        return frame[0];
    }
    return Recurse(depth - 1) + frame[sizeof(frame) - 1];
}

__attribute__((noinline)) static void HugeFrame(void) {
    volatile char frame[160 * 1024];

    frame[0] = 1;
    frame[sizeof(frame) - 1] = frame[0];
}

static void OverflowThread(void *arg) {
    port_fiber_guard_range(port_fiber_current(), &sGuardLow, &sGuardHigh);
    if (arg != NULL) {
        HugeFrame();
    } else {
        Log("depth %d", Recurse(1 << 20));
    }
    Log("never");
}

static void OverflowMain(void) {
    Boot(VBlankHandler);
    Create(0, OverflowThread, sGuardHugeFrame ? (void *)1 : NULL, 10);
    OS_WakeupThreadDirect(&sThreads[0]);
}

static int RunGuard(BOOL hugeFrame) {
    sGuardHugeFrame = hugeFrame;
    InstallFaultHandler();
    port_os_set_stack_sizes(64 * 1024, 64 * 1024);
    port_os_run(OverflowMain);
    printf("never: the overflow didn't fault\n");
    return 1;
}

// The ROM's crt0 enters main() with the CPSR at 0x1f, the I and F bits clear, and IME 0. So the CPSR doesn't hold an
// interrupt back at boot: once SystemInit's OS_EnableIrq sets IME, a requested interrupt is taken at the next defined
// point, long before main's OS_EnableInterrupts (PXI's replies during FS_Init and PM's calls)
static void TestBootCpsr(void) {
    u32 both;

    Log("at main: I bit %#x, can take %d", OS_GetCpsrIrq(), port_os_interrupts_enabled());
    both = OS_DisableInterrupts_IrqAndFiq();
    Log("I and F were %#x", both);
    (void)OS_RestoreInterrupts_IrqAndFiq(both);
    OS_InitIrqTable();
    OS_SetIrqFunction(OS_IE_FIFO_RECV, FifoHandler);
    OS_EnableIrqMask(OS_IE_FIFO_RECV);
    (void)OS_EnableIrq();
    Log("after OS_EnableIrq: I bit %#x, can take %d", OS_GetCpsrIrq(), port_os_interrupts_enabled());
    port_os_request_irq(OS_IE_FIFO_RECV);
    port_os_poll_irq();
    Log("polled (IF %#x)", port_os_get_if());
    EXPECT_LOG("at main: I bit 0, can take 0", "I and F were 0", "after OS_EnableIrq: I bit 0, can take 1",
               "fifo handler (irq mode 1)", "polled (IF 0)");
}

// OS_DestroyThread of a thread that waits doesn't switch, so a thread may destroy any number of others in a row before
// the dispatcher disposes of their fibers
#define MANY_THREADS 20
static OSThread sManyThreads[MANY_THREADS];
static u32 sManyStacks[MANY_THREADS][0x400 / sizeof(u32)];

static void WaitingThread(void *arg) {
    (void)arg;
    OS_SleepThread(&sQueue);
}

static void TestDestroyMany(void) {
    PortOsStats before;
    PortOsStats after;
    int i;

    Boot(VBlankHandler);
    for (i = 0; i < MANY_THREADS; i++) {
        OS_CreateThread(&sManyThreads[i], WaitingThread, NULL, sManyStacks[i] + 0x400 / sizeof(u32), 0x400, 10);
        OS_WakeupThreadDirect(&sManyThreads[i]);
    }
    before = port_os_stats();
    for (i = 0; i < MANY_THREADS; i++) {
        OS_DestroyThread(&sManyThreads[i]);
    }
    after = port_os_stats();
    Log("destroyed %d: %u fibers fewer, %u switches", MANY_THREADS, before.fibersAlive - after.fibersAlive,
        after.fiberSwitches - before.fiberSwitches);
    EXPECT_LOG("destroyed 20: 20 fibers fewer, 0 switches");
}

// OS_Init ends by waiting with IME off for the screen's line 0 (OSi_WaitVCount0): the clock runs to the start of the
// next frame, the VBlank on the way is requested but taken only when IME comes back, and a wait on line 0 takes no time
static void TestWaitVCount0(void) {
    u64 before;

    Boot(LoggingVBlankHandler);
    port_os_spin(100 * CYCLES_PER_LINE);
    Log("before: line %u", port_os_vcount());
    OSi_WaitVCount0();
    Log("after: line %u, %u cycles into the frame, IME %d", port_os_vcount(),
        (u32)(port_os_cycles() % CYCLES_PER_FRAME), reg_OS_IME);
    before = port_os_cycles();
    OSi_WaitVCount0();
    Log("again on line 0: %u cycles", (u32)(port_os_cycles() - before));
    EXPECT_LOG("before: line 100", "vblank 1 (irq mode 1, line 0)", "after: line 0, 0 cycles into the frame, IME 1",
               "again on line 0: 0 cycles");
}

// OS_InitVAlarm starts each run with an empty queue: the first run leaves an alarm set for line 100, the second sets
// one for line 50 and waits a frame, and only the second's fires
static OSVAlarm sVAlarms[2];

static void LoggingVAlarm(void *arg) {
    Log("valarm %s at line %u", (const char *)arg, port_os_vcount());
}

static void VAlarmFirstRun(void) {
    Boot(VBlankHandler);
    OS_InitVAlarm();
    OS_CreateVAlarm(&sVAlarms[0]);
    OS_SetVAlarm(&sVAlarms[0], 100, 0, LoggingVAlarm, "left over");
}

static void VAlarmSecondRun(void) {
    Boot(VBlankHandler);
    OS_InitVAlarm();
    OS_CreateVAlarm(&sVAlarms[1]);
    OS_SetVAlarm(&sVAlarms[1], 50, 0, LoggingVAlarm, "set");
    port_os_spin(CYCLES_PER_FRAME);
    Log("a frame later");
}

typedef struct Test {
    const char *name;
    void (*run)(void);
} Test;

static const Test sTests[] = {
    { "wakeup_preempts", TestWakeupPreempts },
    { "yield_order", TestYieldOrder },
    { "queue_wake_order", TestQueueWakeOrder },
    { "join_destructor", TestJoinDestructor },
    { "mutex", TestMutex },
    { "message_queue", TestMessageQueue },
    { "vblank_and_alarms", TestVBlankAndAlarms },
    { "irq_wakeup_deferred", TestIrqWakeupDeferred },
    { "irq_while_disabled", TestIrqWhileDisabled },
    { "busy_wait_preempted", TestBusyWaitPreempted },
    { "scheduler_lock", TestSchedulerLock },
    { "destroy_and_recreate", TestDestroyAndRecreate },
    { "kill_thread", TestKillThread },
    { "switch_callback", TestSwitchCallback },
    { "hblank_lines", TestHBlankLines },
    { "hblank_main_loop", TestHBlankMainLoop },
    { "dispstat_gates_blanks", TestDispStatGatesBlanks },
    { "frame_hooks_on_host_stack", TestFrameHooksOnHostStack },
    { "timed_work", TestTimedWork },
    { "irq_on_switch_in", TestIrqOnSwitchIn },
    { "async_completion", TestAsyncCompletion },
    { "waits", TestWaits },
    { "request_irq", TestRequestIrq },
    { "scanline_hook", TestScanlineHook },
    { "display_registers", TestDisplayRegisters },
    { "timers_and_tick", TestTimersAndTick },
    { "arenas", TestArenas },
    { "locks", TestLocks },
    { "console", TestConsole },
    { "boot_cpsr", TestBootCpsr },
    { "destroy_many", TestDestroyMany },
    { "wait_vcount0", TestWaitVCount0 },
};

static void PrintLog(void) {
    int i;

    for (i = 0; i < sLogCount; i++) {
        printf("  %s\n", sLog[i]);
    }
}

static int RunTest(const Test *test) {
    sLogCount = 0;
    sFailed = 0;
    Check(port_os_run(test->run) == PORT_OS_RETURNED, "the run ended normally");
    printf("%s: %s\n", test->name, sFailed ? "FAIL" : "ok");
    if (getenv("PORT_OS_VERBOSE") != NULL) {
        PrintLog();
    }
    return sFailed;
}

static int RunDeadlock(void) {
    sLogCount = 0;
    sFailed = 0;
    Check(port_os_run(DeadlockMain) == PORT_OS_DEADLOCK, "the run reports the deadlock");
    Check(sLogCount == 0, "nothing ran after the sleep");
    printf("deadlock: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}

static int RunVAlarmEachRun(void) {
    sLogCount = 0;
    sFailed = 0;
    Check(port_os_run(VAlarmFirstRun) == PORT_OS_RETURNED, "the first run ended normally");
    Check(port_os_run(VAlarmSecondRun) == PORT_OS_RETURNED, "the second run ended normally");
    EXPECT_LOG("valarm set at line 50", "a frame later");
    printf("valarm_each_run: %s\n", sFailed ? "FAIL" : "ok");
    if (getenv("PORT_OS_VERBOSE") != NULL) {
        PrintLog();
    }
    return sFailed;
}

// The log of the frame loop, the same in every run and in every build (compiler, optimization, sanitizers): its hash
// is pinned, so that a change of the clock's or the scheduler's behaviour shows here. It is the spike's hash too: the
// real layer, with the SDK's own alarms, tick and interrupt table, orders the loop exactly as the spike did
#define DETERMINISM_HASH 0x7ce13618u

static int RunDeterminism(void) {
    u32 hashes[2];
    int logCount[2];
    int i;

    sFailed = 0;
    for (i = 0; i < 2; i++) {
        sLogCount = 0;
        Check(port_os_run(GameLoop) == PORT_OS_RETURNED, "the run ended normally");
        hashes[i] = HashLog();
        logCount[i] = sLogCount;
    }
    Check(hashes[0] == hashes[1] && logCount[0] == logCount[1], "both runs logged the same");
    Check(logCount[0] > 10, "the run logged something");
    Check(hashes[0] == DETERMINISM_HASH, "the log's hash is the pinned one");
    printf("determinism: %s (%d lines, hash %08x)\n", sFailed ? "FAIL" : "ok", logCount[0], hashes[0]);
    if (sFailed || getenv("PORT_OS_VERBOSE") != NULL) {
        PrintLog();
    }
    return sFailed;
}

// A scenario that hangs reports where, before CTest's timeout kills it (PORT_OS_HANG_SECONDS, 50 by default)
static void OnHang(int signal) {
    static const char message[] = "hang: the scenario didn't end\n";
    void *frames[64];
    int count;

    (void)signal;
    (void)!write(STDERR_FILENO, message, sizeof(message) - 1);
    count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    _Exit(3);
}

static void InstallHangReport(void) {
    const char *seconds = getenv("PORT_OS_HANG_SECONDS");

    signal(SIGALRM, OnHang);
    alarm(seconds != NULL ? (unsigned)atoi(seconds) : 50);
}

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "all";
    BOOL all = strcmp(name, "all") == 0;
    int failures = 0;
    int ran = 0;
    size_t i;

    setvbuf(stdout, NULL, _IONBF, 0);
    InstallHangReport();
    port_os_add_frame_hook(FrameHooks);
    if (getenv("PORT_OS_STACK_KIB") != NULL) {
        u32 size = (u32)atoi(getenv("PORT_OS_STACK_KIB")) * 1024;

        port_os_set_stack_sizes(size, size);
    }
    for (i = 0; i < NELEMS(sTests); i++) {
        if (all || strcmp(name, sTests[i].name) == 0) {
            failures += RunTest(&sTests[i]);
            ran++;
        }
    }
    if (all || strcmp(name, "stop_from_frame_hook") == 0) {
        failures += RunStopFromFrameHook();
        ran++;
    }
    if (all || strcmp(name, "deadlock") == 0) {
        failures += RunDeadlock();
        ran++;
    }
    if (all || strcmp(name, "determinism") == 0) {
        failures += RunDeterminism();
        ran++;
    }
    if (all || strcmp(name, "valarm_each_run") == 0) {
        failures += RunVAlarmEachRun();
        ran++;
    }
    // Not in "all": each ends the process
    if (strcmp(name, "guard_small_frames") == 0 || strcmp(name, "guard_huge_frame") == 0) {
        return RunGuard(strcmp(name, "guard_huge_frame") == 0);
    }
    if (ran == 0) {
        printf("no test named %s\n", name);
        return 2;
    }
    return failures != 0;
}
