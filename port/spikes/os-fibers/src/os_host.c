// The host side of the SDK's threads and interrupts (spike). The SDK's scheduler is its own C, compiled unchanged:
// os_thread.c (through os_thread_host.c), os_mutex.c, os_message.c and os_event.c. This file stands in for what the
// DS does in assembly or hardware:
//   - the contexts: OS_InitContext, OS_SaveContext and OS_LoadContext (os_context.c, assembly);
//   - the CPSR: OS_EnableInterrupts, OS_DisableInterrupts, OS_RestoreInterrupts and OS_GetProcMode (os_system.c,
//     assembly), and OS_Halt (os_terminate_proc.c, assembly);
//   - the IRQ handler in ITCM and its thread-switch tail (OS_IrqHandler and func_01ff8ce0, assembly only);
//   - the timers behind OS_GetTick and the alarms, as a virtual clock;
//   - the DTCM words: the IRQ table, the queue of threads in OS_WaitIrq and the check word that OS_WaitIrq reads.
//
// Each SDK context (the first member of its OSThread) is mapped to a fiber in a side table; the SDK's structs don't
// change. Fibers never switch to each other: a thread that gives up the CPU yields to the dispatcher on the host's
// own stack, which resumes the next one. Interrupt handlers run there too, as the DS runs them on the IRQ stack, and
// so does the frame hook (presentation and pacing): a fiber whose clock crosses a VBlank yields to the dispatcher for
// it and goes on where it was. Interrupts are taken only at defined points: when a thread halts (the idle thread,
// OS_WaitInterrupt), when the CPSR's I bit is cleared with an interrupt pending (OS_EnableInterrupts,
// OS_RestoreInterrupts, and the switch to a thread whose saved I bit is clear), and when the clock passes an event
// during OS_GetTick or hal_os_spin. Everything is a function of the program's own actions, so a run is deterministic.
#include "os_host.h"

// The SDK's headers come first: types.h defines NULL in a way the host's headers then keep
#include "hal_fiber.h"
#include "nitro/gx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The DS's display timing: 6 cycles a dot, 355 dots a line, 263 lines a frame. The VBlank begins at line 192 and the
// HBlank of each line after its 256 visible dots; the V-count match at the start of its line. DISPSTAT's VBlank flag
// is set from line 192 to 261, not in the last line, and the ARM9 sees its HBlank flag 1606 cycles into each line
// (GBATEK, "DS Video Stuff")
#define CYCLES_PER_LINE 2130
#define LINES_PER_FRAME 263
#define CYCLES_PER_FRAME ((u64)CYCLES_PER_LINE * LINES_PER_FRAME)
#define VBLANK_LINE 192
#define VBLANK_FLAG_END_LINE 262
#define HBLANK_CYCLE 1536
#define HBLANK_FLAG_CYCLE 1606
// DISPSTAT's V-count flag, which the SDK doesn't name, and its V-count setting: bits 8-15 are the line's low 8 bits,
// bit 7 its ninth
#define DISPSTAT_LYC_FLAG 0x0004
#define DISPSTAT_LYC_SHIFT 8
#define DISPSTAT_LYC_HIGH 0x0080
#define DISPSTAT_WRITABLE (REG_GX_DISPSTAT_VBI_MASK | REG_GX_DISPSTAT_HBI_MASK | REG_GX_DISPSTAT_VCI_MASK | 0xff80)
// The system timer behind OS_GetTick counts every 64 cycles
#define CYCLES_PER_TICK 64
// The processor mode of threads, and the CPSR's IRQ-disable bit, which the interrupt functions return
#define PROCMODE_SYS 0x1f
#define CPSR_IRQ_DISABLE 0x80
// How long a halt waits for an enabled interrupt before the run counts as deadlocked: ten seconds
#define HALT_LIMIT_CYCLES (600 * CYCLES_PER_FRAME)

#define MAX_CONTEXTS 256
#define MAX_RETIRED 16

// A context of the SDK and the fiber it runs on
typedef struct HostContext {
    BOOL used;
    OSContext *context;     // NULL for the launcher until OS_InitThread names it
    HalFiber *fiber;        // NULL until the context first runs, and again after OS_InitContext restarts it
    void (*entry)(void *);  // where a fresh context starts
    void *arg;
    BOOL isBoot;            // the launcher's first fiber, which runs hal_os_run's boot function
    BOOL irqEnabled;        // the CPSR's I bit as the thread left it (TRUE: interrupts enabled)
    BOOL saved;             // OS_SaveContext ran since the thread last started running
    BOOL releasing;         // the thread ended: dispose of the context once its fiber has yielded
    u32 stackSize;
} HostContext;

typedef enum Request {
    REQUEST_NONE,
    REQUEST_SWITCH, // OS_LoadContext: run sTarget
    REQUEST_HALT,   // OS_Halt: wait for an interrupt
    REQUEST_IRQ,    // an interrupt is pending and enabled
    REQUEST_FRAME,  // the clock crossed a VBlank: present the frame, then go on with the same fiber
    REQUEST_EXIT,   // the boot function returned
} Request;

typedef enum EventKind {
    EVENT_VBLANK,
    EVENT_HBLANK,
    EVENT_VCOUNT,
    EVENT_ALARM,
} EventKind;

static HostContext sContexts[MAX_CONTEXTS];
static HalFiber *sRetired[MAX_RETIRED];
static int sRetiredCount;
static HostContext *sRunning; // the context whose fiber runs; NULL on the host stack
static HostContext *sLauncher;
static HostContext *sTarget;
static Request sRequest;
static void (*sBoot)(void);
static BOOL sDeadlock;
static HalOsStats sStats;
static u32 sLauncherStackSize = 1024 * 1024;
static u32 sThreadStackSize = 256 * 1024;

// The CPU and the interrupt registers
static BOOL sIrqEnabled; // the CPSR's I bit of the code that runs: TRUE when interrupts are enabled
static BOOL sInIrq;      // the processor is in IRQ mode, in a handler
static BOOL sIME;
static u32 sIE;
static u32 sIF;
static u32 sIrqCheck; // HW_INTR_CHECK_BUF
// DISPSTAT's writable bits: the enables of the display's three interrupts and the V-count setting. The display
// requests an interrupt in IF only when its enable here is set; IE, IME and the I bit then decide whether it is taken
static u16 sDispStat;

// The virtual clock
static u64 sCycles;
static u64 sNextVBlank;
static u64 sNextHBlank;
static u64 sNextVCount;
static u32 sVCountLine; // the line that sNextVCount was computed for
static u32 sFrame;
static u32 sTickStep = CYCLES_PER_TICK;
static void (*sFrameHook)(u32 frame);
static BOOL sFramePending; // a VBlank came and its frame isn't presented yet
static OSAlarm *sAlarms;   // sorted by fire time, the earliest first

// DTCM's system words on the DS
OSIrqFunction OS_IRQTable[32];
OSThreadQueue OSi_IrqThreadQueue;
// The linker's symbols that os_thread.c's own OS_InitThread reads; os_thread_host.c renames that one out of the
// way, but it is still compiled, so the names must exist. DTCM is 16 KiB
u32 SDK_AUTOLOAD_DTCM_START[0x4000 / sizeof(u32)];
u8 SDK_IRQ_STACKSIZE[1];
u8 SDK_SYS_STACKSIZE[1];

static void Fatal(const char *what) {
    fprintf(stderr, "os_host: %s\n", what);
    abort();
}

// The side table

static HostContext *FindContext(const OSContext *context) {
    int i;

    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (sContexts[i].used && sContexts[i].context == context) {
            return &sContexts[i];
        }
    }
    return NULL;
}

static HostContext *NewContext(OSContext *context) {
    int i;

    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (!sContexts[i].used) {
            memset(&sContexts[i], 0, sizeof(sContexts[i]));
            sContexts[i].used = TRUE;
            sContexts[i].context = context;
            sContexts[i].irqEnabled = TRUE;
            sContexts[i].stackSize = sThreadStackSize;
            return &sContexts[i];
        }
    }
    Fatal("too many contexts");
    return NULL;
}

// Hands a context's fiber to the dispatcher to dispose of, once no fiber runs
static void Retire(HostContext *context) {
    if (context->fiber != NULL) {
        if (sRetiredCount == MAX_RETIRED) {
            Fatal("too many fibers to dispose of");
        }
        sRetired[sRetiredCount++] = context->fiber;
        context->fiber = NULL;
        sStats.fibersAlive--;
    }
}

static void FreeRetired(void) {
    while (sRetiredCount > 0) {
        hal_fiber_destroy(sRetired[--sRetiredCount]);
    }
}

static void Release(HostContext *context) {
    Retire(context);
    context->used = FALSE;
    context->context = NULL;
}

// The context of the thread that the SDK has as running: the one an interrupt interrupts
static HostContext *CurrentThreadContext(void) {
    HostContext *context;

    if (OSi_ThreadInfo.current == NULL) {
        return sLauncher;
    }
    context = FindContext(&OSi_ThreadInfo.current->context);
    if (context == NULL) {
        Fatal("the running thread has no context");
    }
    return context;
}

// Gives the CPU back to the dispatcher. The dispatcher resumes this fiber when the scheduler picks its thread again,
// having set sRunning and sIrqEnabled back
static void Yield(Request request) {
    sRequest = request;
    sRunning->irqEnabled = sIrqEnabled;
    hal_fiber_yield();
}

// What every fiber runs: the boot function for the launcher's first fiber, otherwise the thread's function and then
// OS_ExitThread, which is where the DS returns to (its OS_CreateThread puts it in the context's lr)
static void ContextMain(void *arg) {
    HostContext *context = arg;

    if (context->isBoot) {
        sBoot();
        sRequest = REQUEST_EXIT;
        return;
    }
    if (context->entry == NULL) {
        Fatal("OS_InitContext's 32-bit pc can't run on a 64-bit host; the HAL must give the entry");
    }
    context->entry(context->arg);
    OS_ExitThread();
}

// The clock and its events

// The line that DISPSTAT's V-count interrupt waits for
static u32 VCountSetting(void) {
    return (u32)(sDispStat >> DISPSTAT_LYC_SHIFT) | ((sDispStat & DISPSTAT_LYC_HIGH) ? 0x100 : 0);
}

// The next event of the clock. The VBlank is always one, since it ends a frame whether or not it requests an
// interrupt; the HBlanks and the V-count match only while DISPSTAT enables their interrupts, which is when they can
// change anything
static u64 NextEvent(EventKind *kind) {
    u64 when = sNextVBlank;

    *kind = EVENT_VBLANK;
    if (sDispStat & REG_GX_DISPSTAT_HBI_MASK) {
        if (sNextHBlank < sCycles) {
            // The HBlank interrupt was off: start again from the line the clock is on
            sNextHBlank = sCycles / CYCLES_PER_LINE * CYCLES_PER_LINE + HBLANK_CYCLE;
            if (sNextHBlank < sCycles) {
                sNextHBlank += CYCLES_PER_LINE;
            }
        }
        if (sNextHBlank < when) {
            when = sNextHBlank;
            *kind = EVENT_HBLANK;
        }
    }
    if ((sDispStat & REG_GX_DISPSTAT_VCI_MASK) && VCountSetting() < LINES_PER_FRAME) {
        if (sNextVCount < sCycles || sVCountLine != VCountSetting()) {
            // The interrupt was off or its line changed: the next start of the line, from now
            sVCountLine = VCountSetting();
            sNextVCount = sCycles / CYCLES_PER_FRAME * CYCLES_PER_FRAME + (u64)sVCountLine * CYCLES_PER_LINE;
            if (sNextVCount < sCycles) {
                sNextVCount += CYCLES_PER_FRAME;
            }
        }
        if (sNextVCount < when) {
            when = sNextVCount;
            *kind = EVENT_VCOUNT;
        }
    }
    if (sAlarms != NULL && !(sIF & OS_IE_TIMER1)) {
        u64 fire = sAlarms->fire * CYCLES_PER_TICK;

        if (fire < sCycles) {
            fire = sCycles;
        }
        if (fire < when) {
            when = fire;
            *kind = EVENT_ALARM;
        }
    }
    return when;
}

// Moves the clock to an event and latches what it requests in IF. A VBlank also marks its frame for presentation,
// which only the dispatcher's stack does (PresentFrame): this can run on any fiber, in any critical section
static void RaiseEvent(EventKind kind, u64 when) {
    if (when > sCycles) {
        sCycles = when;
    }
    switch (kind) {
    case EVENT_VBLANK:
        if (sDispStat & REG_GX_DISPSTAT_VBI_MASK) {
            sIF |= OS_IE_V_BLANK;
        }
        sFrame++;
        sNextVBlank += CYCLES_PER_FRAME;
        sFramePending = TRUE;
        break;
    case EVENT_HBLANK:
        sIF |= OS_IE_H_BLANK;
        sNextHBlank += CYCLES_PER_LINE;
        break;
    case EVENT_VCOUNT:
        sIF |= OS_IE_V_COUNT;
        sNextVCount += CYCLES_PER_FRAME;
        break;
    case EVENT_ALARM:
        // The alarms run on timer 1, as the SDK's do
        sIF |= OS_IE_TIMER1;
        break;
    }
}

// Presents the frame of the last VBlank: the frame hook, where the real HAL draws, presents and paces. It runs on the
// dispatcher's stack only, with no fiber running, so that SDL and the GPU driver never run on a game stack or inside
// the game's critical sections. It must not call the SDK
static void PresentFrame(void) {
    if (!sFramePending) {
        return;
    }
    if (hal_fiber_current() != NULL) {
        Fatal("the frame hook would run on a fiber's stack");
    }
    sFramePending = FALSE;
    if (sFrameHook != NULL) {
        sFrameHook(sFrame);
    }
}

// Interrupts

// Whether the code that runs could take an interrupt now: a fiber, not a handler, with the I bit clear and IME on
static BOOL CanTakeIrq(void) {
    return sRunning != NULL && !sInIrq && sIrqEnabled && sIME;
}

// Takes a pending interrupt now, if this is a fiber with interrupts enabled. This is where the DS would take the IRQ
// on the next instruction
static void PollIrq(void) {
    if (CanTakeIrq() && (sIE & sIF)) {
        Yield(REQUEST_IRQ);
    }
}

// Runs the clock for cycles of the running code's own work, as a busy loop would: each event on the way is raised
// when it comes, a VBlank's frame is presented at once (the fiber yields to the dispatcher for it, whatever its I
// bit), and an interrupt that can be taken is taken there. The cycles that other threads run meanwhile don't count
// towards the loop's own. On the dispatcher's stack (a handler that reads the tick), the frame waits for the
// dispatcher
static void Spin(u64 cycles) {
    u64 target = sCycles + cycles;
    EventKind kind;
    u64 when;

    while ((when = NextEvent(&kind)) <= target) {
        RaiseEvent(kind, when);
        if (sFramePending && sRunning != NULL) {
            Yield(REQUEST_FRAME);
        }
        if (CanTakeIrq() && (sIE & sIF)) {
            u64 left = target - sCycles;

            PollIrq();
            target = sCycles + left;
        }
    }
    sCycles = target;
}

// The alarms' handler (NitroSDK's OSi_AlarmHandler, on timer 1): runs every alarm that is due, the earliest first
static void AlarmHandler(void) {
    u64 now = sCycles / CYCLES_PER_TICK;

    while (sAlarms != NULL && sAlarms->fire <= now) {
        OSAlarm *alarm = sAlarms;
        OSAlarmHandler handler = alarm->handler;

        sAlarms = alarm->next;
        if (sAlarms != NULL) {
            sAlarms->prev = NULL;
        }
        alarm->prev = alarm->next = NULL;
        alarm->handler = NULL;
        handler(alarm->arg);
    }
}

// NitroSDK's OS_IrqHandler_ThreadSwitch (func_01ff8ce0 in ITCM), from its assembly: wakes every thread that waits in
// OS_WaitIrq and asks for a switch, then, when a switch was asked for (by that, or by a wakeup in a handler while
// OSi_RescheduleThread saw the IRQ mode), makes the first ready thread of the list the running one, calling the
// thread info's switch callback first. Like the assembly, it doesn't look at OS_DisableScheduler's count and doesn't
// call the system callback that OSi_RescheduleThread also calls. The dispatcher then resumes that thread's fiber; the
// interrupted thread's fiber stays where it yielded, which is its saved context
static void IrqThreadSwitch(void) {
    OSThread *thread = OSi_IrqThreadQueue.head;
    OSThread *next;

    if (thread != NULL) {
        while (thread != NULL) {
            next = thread->link.next;
            thread->state = OS_THREAD_STATE_READY;
            thread->queue = NULL;
            thread->link.prev = NULL;
            thread->link.next = NULL;
            thread = next;
        }
        OSi_IrqThreadQueue.head = NULL;
        OSi_IrqThreadQueue.tail = NULL;
        OSi_ThreadInfo.isNeedRescheduling = TRUE;
    }
    if (!OSi_ThreadInfo.isNeedRescheduling || OSi_ThreadInfo.current == NULL) {
        return;
    }
    OSi_ThreadInfo.isNeedRescheduling = FALSE;

    next = OSi_ThreadInfo.list;
    while (next != NULL && next->state != OS_THREAD_STATE_READY) {
        next = next->next;
    }
    if (next == NULL || next == OSi_ThreadInfo.current) {
        return;
    }
    if (OSi_ThreadInfo.switchCallback != NULL) {
        OSi_ThreadInfo.switchCallback(OSi_ThreadInfo.current, next);
    }
    // The DS saves and loads the divider's context here too (CP_SaveContext, CPi_RestoreContext); a HAL that models
    // the divider as registers saves it into context.cp_context the same way
    OSi_ThreadInfo.current = next;
}

// NitroSDK's OS_IrqHandler: while an interrupt is requested and enabled, and the thread that it would interrupt has
// interrupts enabled, runs the handler of the lowest one in IRQ mode, then the thread-switch tail. The tail may make
// another thread the running one, whose own I bit then decides whether the next interrupt waits
static void DispatchIrqs(void) {
    for (;;) {
        HostContext *interrupted = CurrentThreadContext();
        u32 pending = sIE & sIF;
        u32 bit;

        if (!sIME || pending == 0 || !interrupted->irqEnabled) {
            break;
        }
        bit = pending & (~pending + 1);
        sIF &= ~bit;
        sStats.irqs++;
        sInIrq = TRUE;
        sIrqEnabled = FALSE;
        if (bit == OS_IE_TIMER1) {
            AlarmHandler();
        } else if (OS_IRQTable[__builtin_ctz(bit)] != NULL) {
            OS_IRQTable[__builtin_ctz(bit)]();
        }
        sInIrq = FALSE;
        IrqThreadSwitch();
    }
}

// OS_Halt on the host: the CPU sleeps until an interrupt is requested and enabled in IE (IME and the I bit don't
// matter to the wakeup), then takes it if the halted thread has interrupts enabled. Each VBlank on the way is
// presented when it comes, before its handler
static void Halt(void) {
    u64 start = sCycles;

    while (!(sIE & sIF)) {
        EventKind kind;
        u64 when = NextEvent(&kind);

        if (when - start > HALT_LIMIT_CYCLES) {
            sDeadlock = TRUE;
            return;
        }
        RaiseEvent(kind, when);
        PresentFrame();
    }
    DispatchIrqs();
}

// The dispatcher: runs fibers on the host stack until the boot function returns
static void Dispatcher(HostContext *next) {
    for (;;) {
        HostContext *prev;

        FreeRetired();
        PresentFrame();
        // Loading a context whose I bit is clear unmasks an interrupt that is pending, and the DS takes it before the
        // thread's next instruction (a new thread's first): the thread is interrupted where it would resume. next is
        // always the SDK's current thread here, which is what DispatchIrqs interrupts; its tail may pick another
        if (next->irqEnabled && sIME && (sIE & sIF)) {
            DispatchIrqs();
            next = CurrentThreadContext();
        }
        if (next->fiber == NULL) {
            next->fiber = hal_fiber_create(ContextMain, next, next->stackSize);
            sStats.fibersCreated++;
            sStats.fibersAlive++;
        }
        sRunning = next;
        sIrqEnabled = next->irqEnabled;
        next->saved = FALSE;
        sRequest = REQUEST_NONE;
        sStats.fiberSwitches++;
        hal_fiber_resume(next->fiber);

        // A fiber that crossed a VBlank yields only for its frame to be presented on this stack. It goes on where it
        // was, which may be between OS_SaveContext and OS_LoadContext (a switch callback that reads the tick), so
        // nothing about its context changes
        while (sRequest == REQUEST_FRAME) {
            sRunning = NULL;
            PresentFrame();
            sRunning = next;
            sIrqEnabled = next->irqEnabled;
            sRequest = REQUEST_NONE;
            sStats.fiberSwitches++;
            hal_fiber_resume(next->fiber);
        }

        prev = sRunning;
        sRunning = NULL;
        if (prev->releasing) {
            Release(prev);
        }
        switch (sRequest) {
        case REQUEST_SWITCH:
            next = sTarget;
            break;
        case REQUEST_HALT:
            sStats.halts++;
            Halt();
            if (sDeadlock) {
                return;
            }
            next = CurrentThreadContext();
            break;
        case REQUEST_IRQ:
            DispatchIrqs();
            next = CurrentThreadContext();
            break;
        case REQUEST_EXIT:
            return;
        default:
            Fatal("a fiber gave up the CPU without a request");
        }
    }
}

int hal_os_run(void (*boot)(void)) {
    int i;

    memset(sContexts, 0, sizeof(sContexts));
    sRetiredCount = 0;
    sRunning = NULL;
    sTarget = NULL;
    sRequest = REQUEST_NONE;
    sDeadlock = FALSE;
    memset(&sStats, 0, sizeof(sStats));
    sIrqEnabled = FALSE;
    sInIrq = FALSE;
    sIME = FALSE;
    sIE = 0;
    sIF = 0;
    sIrqCheck = 0;
    sDispStat = 0;
    sCycles = 0;
    sNextVBlank = (u64)VBLANK_LINE * CYCLES_PER_LINE;
    sNextHBlank = HBLANK_CYCLE;
    sNextVCount = 0;
    sVCountLine = 0;
    sFrame = 0;
    sFramePending = FALSE;
    sAlarms = NULL;
    memset(OS_IRQTable, 0, sizeof(OS_IRQTable));
    OSi_IrqThreadQueue.head = OSi_IrqThreadQueue.tail = NULL;
    hal_os_reset_threads();

    // The DS starts main with interrupts disabled in the CPSR; main enables them
    sBoot = boot;
    sLauncher = NewContext(NULL);
    sLauncher->isBoot = TRUE;
    sLauncher->irqEnabled = FALSE;
    sLauncher->stackSize = sLauncherStackSize;
    Dispatcher(sLauncher);

    // Every fiber that is left, suspended or done, goes
    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (sContexts[i].used) {
            Retire(&sContexts[i]);
        }
    }
    FreeRetired();
    memset(sContexts, 0, sizeof(sContexts));
    sLauncher = NULL;
    return sDeadlock ? -1 : 0;
}

void hal_os_set_context_entry(OSContext *context, void (*entry)(void *), void *arg, BOOL irqEnabled) {
    HostContext *host = FindContext(context);

    if (host == NULL) {
        Fatal("hal_os_set_context_entry before OS_InitContext");
    }
    host->entry = entry;
    host->arg = arg;
    host->irqEnabled = irqEnabled;
}

void hal_os_bind_running_context(OSContext *context) {
    if (sRunning == NULL || FindContext(context) != NULL) {
        Fatal("hal_os_bind_running_context");
    }
    sRunning->context = context;
}

void hal_os_release_context(OSContext *context) {
    HostContext *host = FindContext(context);

    if (host == NULL) {
        return;
    }
    if (host == sRunning) {
        host->releasing = TRUE;
    } else {
        Release(host);
    }
}

// The SDK's contexts (os_context.c)

// NitroSDK's OS_InitContext sets a context up to start at newpc with its stack at newsp. The registers are set as the
// assembly sets them, for show: a host can't call a 32-bit pc, so the caller gives the real entry with
// hal_os_set_context_entry. A fiber the context had (a thread that ended, or one that OS_KillThread restarts) is
// disposed of, and the next OS_LoadContext starts a new one
void OS_InitContext(OSContext *context, u32 newpc, u32 newsp) {
    HostContext *host = FindContext(context);
    u32 sp = newsp - 0x40;

    newpc += 4;
    context->pc_plus4 = newpc;
    context->sp_svc = newsp;
    context->sp = (sp & 4) ? sp - 4 : sp;
    context->cpsr = (newpc & 1) ? 0x3f : 0x1f;
    memset(context->r, 0, sizeof(context->r));
    context->lr = 0;

    if (host == NULL) {
        host = NewContext(context);
    }
    Retire(host);
    host->entry = NULL;
    host->arg = NULL;
    host->isBoot = FALSE;
    host->irqEnabled = TRUE;
    host->saved = FALSE;
    host->releasing = FALSE;
    host->stackSize = sThreadStackSize;
}

// NitroSDK's OS_SaveContext returns 0 having saved the caller's registers, and 1 when OS_LoadContext later continues
// from them. Its only caller, OSi_RescheduleThread, returns at once on 1, and nothing in it follows OS_LoadContext.
// So on a host it only notes that the thread can be continued and returns 0: the thread's fiber yields inside
// OS_LoadContext, and returning from there when it is resumed is the same as the DS's return with 1
BOOL OS_SaveContext(OSContext *context) {
    HostContext *host = FindContext(context);

    if (host == NULL || host != sRunning) {
        Fatal("OS_SaveContext of a context that isn't running");
    }
    host->saved = TRUE;
    return FALSE;
}

// NitroSDK's OS_LoadContext continues from a context. A running thread that didn't save its own first has ended
// (OSi_ExitThread_Destroy, or OS_DestroyThread on itself): its fiber never runs again
void OS_LoadContext(OSContext *context) {
    HostContext *target = FindContext(context);
    HostContext *self = sRunning;

    if (target == NULL || self == NULL) {
        Fatal("OS_LoadContext of an unknown context, or from the host stack");
    }
    if (!self->saved && self != target) {
        self->releasing = TRUE;
    }
    sTarget = target;
    Yield(REQUEST_SWITCH);
}

// The CPSR (os_system.c, os_terminate_proc.c)

u32 OS_EnableInterrupts(void) {
    u32 prev = sIrqEnabled ? 0 : CPSR_IRQ_DISABLE;

    sIrqEnabled = TRUE;
    PollIrq();
    return prev;
}

u32 OS_DisableInterrupts(void) {
    u32 prev = sIrqEnabled ? 0 : CPSR_IRQ_DISABLE;

    sIrqEnabled = FALSE;
    return prev;
}

u32 OS_RestoreInterrupts(u32 mask) {
    u32 prev = sIrqEnabled ? 0 : CPSR_IRQ_DISABLE;

    sIrqEnabled = (mask & CPSR_IRQ_DISABLE) == 0;
    if (sIrqEnabled) {
        PollIrq();
    }
    return prev;
}

u32 OS_GetProcMode(void) {
    return sInIrq ? OS_PROCMODE_IRQ : PROCMODE_SYS;
}

void OS_Halt(void) {
    if (sRunning == NULL || sInIrq) {
        Fatal("OS_Halt outside a thread");
    }
    Yield(REQUEST_HALT);
}

void OS_Terminate(void) {
    Fatal("OS_Terminate");
}

// Interrupts (os_interrupt.c, os_irqHandler.c), over the host's registers

void OS_SetIrqFunction(u32 interrupts, OSIrqFunction function) {
    int n;

    for (n = 0; n < 32; n++) {
        if (interrupts & (1u << n)) {
            OS_IRQTable[n] = function;
        }
    }
}

u32 OS_EnableIrqMask(u32 mask) {
    u32 prev = sIE;

    sIE |= mask;
    PollIrq();
    return prev;
}

u32 OS_DisableIrqMask(u32 mask) {
    u32 prev = sIE;

    sIE &= ~mask;
    return prev;
}

void hal_os_set_ime(BOOL enabled) {
    sIME = enabled;
    PollIrq();
}

// A write of DISPSTAT (GX_VBlankIntr, GX_HBlankIntr, OS_SetVAlarm's V-count): the status flags are read only
void hal_os_set_dispstat(u16 value) {
    sDispStat = value & DISPSTAT_WRITABLE;
}

// A read of DISPSTAT: what was written, with the three status flags of the line and dot the clock is on
u16 hal_os_get_dispstat(void) {
    u32 line = hal_os_vcount();
    u16 value = sDispStat;

    if (line >= VBLANK_LINE && line < VBLANK_FLAG_END_LINE) {
        value |= REG_GX_DISPSTAT_VBLK_MASK;
    }
    if (sCycles % CYCLES_PER_LINE >= HBLANK_FLAG_CYCLE) {
        value |= REG_GX_DISPSTAT_HBLK_MASK;
    }
    if (line == VCountSetting()) {
        value |= DISPSTAT_LYC_FLAG;
    }
    return value;
}

u32 hal_os_get_if(void) {
    return sIF;
}

void hal_os_set_irq_check_flag(u32 interrupts) {
    sIrqCheck |= interrupts;
}

u32 hal_os_get_irq_check_flag(void) {
    return sIrqCheck;
}

// NitroSDK's OS_WaitIrq, as the decomp has it, with the host's check word: the thread sleeps in OSi_IrqThreadQueue,
// which the IRQ tail empties after every interrupt
void OS_WaitIrq(BOOL clear, u32 interrupts) {
    u32 irq = OS_DisableInterrupts();

    if (clear) {
        sIrqCheck &= ~interrupts;
    }
    OS_RestoreInterrupts(irq);
    while (!(interrupts & sIrqCheck)) {
        OS_SleepThread(&OSi_IrqThreadQueue);
    }
}

void OS_WaitVBlankIntr(void) {
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
}

// NitroSDK's OS_WaitInterrupt, as the decomp has it: halts with the thread still running instead of sleeping it
void OS_WaitInterrupt(BOOL clear, u32 interrupts) {
    u32 irq = OS_DisableInterrupts();
    BOOL ime = sIME;

    sIME = TRUE;
    if (clear) {
        sIrqCheck &= ~interrupts;
    }
    while (!(sIrqCheck & interrupts)) {
        OS_Halt();
        OS_EnableInterrupts();
        OS_DisableInterrupts();
    }
    sIrqCheck &= ~interrupts;
    sIME = ime;
    OS_RestoreInterrupts(irq);
}

// Ticks and alarms (os_tick.c, os_alarm.c), on the virtual clock

// Each call moves the clock a little, so that a busy wait on the tick ends and the events it waits through come
u64 OS_GetTick(void) {
    Spin(sTickStep);
    return sCycles / CYCLES_PER_TICK;
}

void OS_CreateAlarm(OSAlarm *alarm) {
    alarm->handler = NULL;
    alarm->tag = 0;
}

// Puts the alarm after the ones that fire at the same time or earlier
void OS_SetAlarm(OSAlarm *alarm, OSTick tick, OSAlarmHandler handler, void *arg) {
    u32 enabled = OS_DisableInterrupts();
    OSAlarm *prev = NULL;
    OSAlarm *next = sAlarms;

    alarm->handler = handler;
    alarm->arg = arg;
    alarm->period = 0;
    alarm->start = 0;
    alarm->fire = OS_GetTick() + tick;
    while (next != NULL && next->fire <= alarm->fire) {
        prev = next;
        next = next->next;
    }
    alarm->prev = prev;
    alarm->next = next;
    if (prev == NULL) {
        sAlarms = alarm;
    } else {
        prev->next = alarm;
    }
    if (next != NULL) {
        next->prev = alarm;
    }
    // The SDK enables timer 1's interrupt when it sets the timer for an alarm (OSi_EnterTimerCallback)
    sIE |= OS_IE_TIMER1;
    OS_RestoreInterrupts(enabled);
}

void OS_CancelAlarm(OSAlarm *alarm) {
    u32 enabled = OS_DisableInterrupts();

    if (alarm->handler != NULL) {
        if (alarm->prev == NULL) {
            sAlarms = alarm->next;
        } else {
            alarm->prev->next = alarm->next;
        }
        if (alarm->next != NULL) {
            alarm->next->prev = alarm->prev;
        }
        alarm->prev = alarm->next = NULL;
        alarm->handler = NULL;
    }
    OS_RestoreInterrupts(enabled);
}

u64 hal_os_cycles(void) {
    return sCycles;
}

void hal_os_spin(u64 cycles) {
    Spin(cycles);
    PollIrq();
}

void hal_os_set_tick_step(u32 cycles) {
    sTickStep = cycles;
}

u32 hal_os_vcount(void) {
    return (u32)(sCycles % CYCLES_PER_FRAME / CYCLES_PER_LINE);
}

u32 hal_os_frame(void) {
    return sFrame;
}

void hal_os_set_frame_hook(void (*hook)(u32 frame)) {
    sFrameHook = hook;
}

void hal_os_set_stack_sizes(u32 launcher, u32 thread) {
    sLauncherStackSize = launcher;
    sThreadStackSize = thread;
}

u32 hal_os_stack_used(const OSThread *thread) {
    HostContext *host = FindContext(&thread->context);

    return host != NULL && host->fiber != NULL ? (u32)hal_fiber_stack_used(host->fiber) : 0;
}

HalOsStats hal_os_stats(void) {
    return sStats;
}

// MI's word fill, which os_thread.c's OS_CreateThread uses (assembly in the SDK)
void MIi_CpuClear32(u32 value, void *dest, u32 size) {
    u32 *p = dest;
    u32 i;

    for (i = 0; i < size / sizeof(u32); i++) {
        p[i] = value;
    }
}
