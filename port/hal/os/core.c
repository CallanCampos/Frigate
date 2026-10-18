// The host side of the SDK's threads, interrupts and display timing: what the DS does in assembly or in hardware,
// under the SDK's own scheduler (os_thread.c through os_thread_host.c, os_mutex.c, os_message.c and os_event.c, all
// compiled unchanged). From the reviewed spike port/spikes/os-fibers, whose README has the measurements and the
// reasons. This file has:
//   - the contexts: OS_InitContext, OS_SaveContext and OS_LoadContext (os_context.c, assembly in the SDK);
//   - the CPSR: OS_EnableInterrupts, OS_DisableInterrupts, OS_RestoreInterrupts, their IrqAndFiq pair,
//     OS_GetCpsrIrq and OS_GetProcMode (os_system.c, assembly), and OS_Halt (os_terminate_proc.c, assembly);
//   - the IRQ handler in ITCM and its thread-switch tail (OS_IrqHandler and func_01ff8ce0, assembly with no C);
//   - the display's timing (VBlank, HBlank, V-count, VCOUNT and DISPSTAT's flags), the hardware timers and the other
//     modules' timed work (port_os_set_timed_work), on a virtual clock;
//   - the dispatcher, which runs on the host's own stack.
//
// Each SDK context (the first member of its OSThread) is mapped to a fiber in a side table; the SDK's structs don't
// change. Fibers never switch to each other: a thread that gives up the CPU yields to the dispatcher, which resumes the
// next one. Interrupt handlers run on the dispatcher's stack, as the DS runs them on the IRQ stack, and so does the
// host's own work (the frame hooks and the scanline hook): a fiber whose clock crosses a VBlank or a line yields to
// the dispatcher for it and goes on where it was. Interrupts are taken only at defined points: when a thread halts
// (the idle thread, OS_WaitInterrupt), when the CPSR's I bit, IE or IME lets through an interrupt that is pending
// (OS_EnableInterrupts, OS_RestoreInterrupts, OS_EnableIrqMask, the return of IME in OS_Init's OSi_WaitVCount0, and
// the switch to a thread whose saved I bit is clear), when the clock passes an event inside a modelled busy loop
// (OS_GetTick, OS_SpinWaitSysCycles, SVC_WaitByLoop) and at port_os_poll_irq. Everything is a function of the
// program's own actions, so a run is deterministic.
#include "hal/hw/hw.h"
#include "hal/os/fiber.h"
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The DS's display timing: 6 cycles a dot, 355 dots a line, 263 lines a frame. The VBlank begins at line 192 and the
// HBlank of each line after its 256 visible dots; the V-count match at the start of its line. DISPSTAT's VBlank flag
// is set from line 192 to 261, not in the last line, and the ARM9 sees its HBlank flag 1606 cycles into each line
// (GBATEK, "DS Video Stuff"). The HBlank interrupt is taken there too: melonDS raises it as it sets the flag
// (GPU::StartHBlank), and the port's handlers take no time, so a handler taken at 1536 read the flag clear. The screen
// wipes' H-blank function writes the window planes only while GX_IsHBlank() (src/system/wipe_sub.c's
// WipeLineWnd_HBlank), so the wipes that cover lines one by one were never drawn. The cycle itself is GBATEK's:
// melonDS 1.1 does both at 1584 (GPU.cpp's HBLANK_CYCLES, 48 + 256 * 6), 22 cycles earlier. What the game sees is that
// the two come together, which holds at either cycle
#define CYCLES_PER_LINE PORT_OS_CYCLES_PER_LINE
#define LINES_PER_FRAME PORT_OS_LINES_PER_FRAME
#define CYCLES_PER_FRAME ((u64)PORT_OS_CYCLES_PER_FRAME)
#define VBLANK_LINE PORT_OS_VISIBLE_LINES
#define VBLANK_FLAG_END_LINE 262
#define HBLANK_FLAG_CYCLE 1606
#define HBLANK_CYCLE HBLANK_FLAG_CYCLE
// DISPSTAT's V-count flag, which the SDK doesn't name, and its V-count setting: bits 8-15 are the line's low 8 bits,
// bit 7 its ninth
#define DISPSTAT_LYC_FLAG 0x0004
#define DISPSTAT_LYC_SHIFT 8
#define DISPSTAT_LYC_HIGH 0x0080
// The system timer behind OS_GetTick counts every 64 cycles
#define CYCLES_PER_TICK 64
// The processor mode of threads, and the CPSR's IRQ- and FIQ-disable bits, which the interrupt functions return. The
// ROM's crt0 enters main() with both clear: it sets the CPSR to 0x1f, system mode, before it jumps there
#define PROCMODE_SYS 0x1f
#define CPSR_IRQ_DISABLE 0x80
#define CPSR_FIQ_DISABLE 0x40
#define CPSR_IRQ_FIQ_MASK (CPSR_IRQ_DISABLE | CPSR_FIQ_DISABLE)
// How long a halt waits for an enabled interrupt before the run counts as deadlocked: ten seconds
#define HALT_LIMIT_CYCLES (600 * CYCLES_PER_FRAME)

#define MAX_CONTEXTS 256
#define INDEX_SIZE 1024
// The host's work waiting for the dispatcher: lines for the scanline hook, frames, and timed work (WORK_TIMED - slot)
#define WORK_FRAME (-1)
#define WORK_TIMED (-2)
#define WORK_CAPACITY 1024

// A context of the SDK and the fiber it runs on
typedef struct HostContext {
    BOOL used;
    OSContext *context;    // NULL for the launcher until OS_InitThread names it
    PortFiber *fiber;      // NULL until the context first runs, and again after OS_InitContext restarts it
    void (*entry)(void *); // where a fresh context starts
    void *arg;
    BOOL isBoot;    // the launcher's first fiber, which runs port_os_run's boot function
    u32 cpsrMask;   // the CPSR's I and F bits as the thread left them (CPSR_IRQ_DISABLE set: interrupts disabled)
    BOOL saved;     // OS_SaveContext ran since the thread last started running
    BOOL releasing; // the thread ended: dispose of the context once its fiber has yielded
    u32 stackSize;
} HostContext;

typedef enum Request {
    REQUEST_NONE,
    REQUEST_SWITCH, // OS_LoadContext: run sTarget
    REQUEST_HALT,   // OS_Halt: wait for an interrupt
    REQUEST_IRQ,    // an interrupt is pending and can be taken
    REQUEST_HOST,   // the clock crossed a line or a VBlank: run the host's work, then go on with the same fiber
    REQUEST_EXIT,   // the boot function returned
} Request;

typedef enum EventKind {
    EVENT_LINE,   // the start of a line: VCOUNT, the VBlank at line 192, the V-count match, the scanline hook
    EVENT_HBLANK, // the HBlank interrupt's request
    EVENT_TIMER,  // a timer that interrupts wraps
    EVENT_TIMED,  // a module's timed work (port_os_set_timed_work)
} EventKind;

// A module's timed work and its moment, UINT64_MAX while it has none
typedef struct TimedWork {
    void (*work)(void);
    u64 when;
} TimedWork;

// A hardware timer, counting up from reload by one every prescaler cycles since start, and wrapping to reload
typedef struct Timer {
    BOOL running;
    BOOL irq;
    u16 reload;
    u32 prescaler;
    u64 start;
    u64 nextWrap; // the next wrap that hasn't raised its interrupt yet
} Timer;

static HostContext sContexts[MAX_CONTEXTS];
static u16 sIndex[INDEX_SIZE]; // the side table's index by context address: a slot of sContexts plus 1, or 0
// The fibers waiting for the dispatcher to dispose of them, as many as the threads ended since it last ran
static PortFiber **sRetired;
static size_t sRetiredCount;
static size_t sRetiredCapacity;
static HostContext *sRunning; // the context whose fiber runs; NULL on the host stack
static HostContext *sLauncher;
static HostContext *sTarget;
static Request sRequest;
static void (*sBoot)(void);
static BOOL sDeadlock;
static BOOL sStop;
static PortOsStats sStats;
static u32 sLauncherStackSize = 8 * 1024 * 1024;
static u32 sThreadStackSize = 256 * 1024;

// The CPU and the interrupt controller's IF (IE and IME are the I/O registers themselves)
static u32 sCpsrMask; // the CPSR's I and F bits of the code that runs
static BOOL sInIrq;   // the processor is in IRQ mode: a handler, or the thread-switch tail after it
static u32 sIF;

// The virtual clock
static u64 sCycles;
static u64 sNextLine;   // the start of the next line, which hasn't been raised yet
static u64 sNextHBlank; // the next HBlank, while DISPSTAT requests its interrupt
static u32 sFrame;
static u32 sTickStep = CYCLES_PER_TICK;
static Timer sTimers[4];

// The host's work, in the order it came
static int sWork[WORK_CAPACITY];
static u32 sWorkHead;
static u32 sWorkCount;
static void (*sFrameHooks[PORT_OS_MAX_FRAME_HOOKS])(void);
static int sFrameHookCount;
static TimedWork sTimedWork[PORT_OS_MAX_TIMED_WORK];

void (*port_os_scanline_hook)(int line);

static void Fatal(const char *what) {
    fprintf(stderr, "port: os: %s\n", what);
    fflush(stderr);
    abort();
}

// The interrupt registers

static u32 ReadIE(void) {
    return reg_OS_IE;
}

static BOOL ReadIME(void) {
    return (reg_OS_IME & 1) != 0;
}

static void RequestIF(u32 interrupts) {
    sIF |= interrupts;
    reg_OS_IF = sIF;
}

void port_osi_acknowledge_irq(u32 mask) {
    sIF &= ~mask;
    reg_OS_IF = sIF;
}

static u16 ReadDispStat(void) {
    return reg_GX_DISPSTAT;
}

// The line that DISPSTAT's V-count interrupt waits for
static u32 VCountSetting(u16 dispstat) {
    return (u32)(dispstat >> DISPSTAT_LYC_SHIFT) | ((dispstat & DISPSTAT_LYC_HIGH) ? 0x100 : 0);
}

static u32 LineOf(u64 cycles) {
    return (u32)(cycles % CYCLES_PER_FRAME / CYCLES_PER_LINE);
}

// VCOUNT and DISPSTAT's three status flags, for the line and dot the clock is on. They are set again whenever the clock
// moves, which is the only time they can change; DISPSTAT's other bits are the game's
static void UpdateDisplayStatus(void) {
    u32 line = LineOf(sCycles);
    u16 dispstat = ReadDispStat();
    u16 flags = 0;

    port_hw_set_vcount((u16)line);
    if (line >= VBLANK_LINE && line < VBLANK_FLAG_END_LINE) {
        flags |= REG_GX_DISPSTAT_VBLK_MASK;
    }
    if (sCycles % CYCLES_PER_LINE >= HBLANK_FLAG_CYCLE) {
        flags |= REG_GX_DISPSTAT_HBLK_MASK;
    }
    if (line == VCountSetting(dispstat)) {
        flags |= DISPSTAT_LYC_FLAG;
    }
    if ((dispstat & REG_GX_DISPSTAT_STATUS_MASK) != flags) {
        reg_GX_DISPSTAT = (u16)((dispstat & ~REG_GX_DISPSTAT_STATUS_MASK) | flags);
    }
}

static void AdvanceClock(u64 when) {
    if (when > sCycles) {
        sCycles = when;
    }
    UpdateDisplayStatus();
}

// The side table

static u32 Hash(const OSContext *context) {
    return (u32)(((uintptr_t)context >> 4) * 2654435761u) & (INDEX_SIZE - 1);
}

static HostContext *FindContext(const OSContext *context) {
    u32 i;

    if (context == NULL) {
        return NULL;
    }
    for (i = Hash(context); sIndex[i] != 0; i = (i + 1) & (INDEX_SIZE - 1)) {
        if (sContexts[sIndex[i] - 1].context == context) {
            return &sContexts[sIndex[i] - 1];
        }
    }
    return NULL;
}

static void IndexContext(HostContext *host) {
    u32 i = Hash(host->context);

    while (sIndex[i] != 0) {
        i = (i + 1) & (INDEX_SIZE - 1);
    }
    sIndex[i] = (u16)(host - sContexts + 1);
}

// Takes a context out of the index, moving back the entries after it that its slot pushed along
static void UnindexContext(HostContext *host) {
    u32 i = Hash(host->context);
    u32 j;

    while (sIndex[i] != 0 && &sContexts[sIndex[i] - 1] != host) {
        i = (i + 1) & (INDEX_SIZE - 1);
    }
    if (sIndex[i] == 0) {
        return;
    }
    sIndex[i] = 0;
    for (j = (i + 1) & (INDEX_SIZE - 1); sIndex[j] != 0; j = (j + 1) & (INDEX_SIZE - 1)) {
        u32 home = Hash(sContexts[sIndex[j] - 1].context);
        // The entry at j may move to i unless its home lies cyclically in (i, j]
        BOOL stays = (i <= j) ? (home > i && home <= j) : (home > i || home <= j);

        if (!stays) {
            sIndex[i] = sIndex[j];
            sIndex[j] = 0;
            i = j;
        }
    }
}

static HostContext *NewContext(OSContext *context) {
    int i;

    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (!sContexts[i].used) {
            memset(&sContexts[i], 0, sizeof(sContexts[i]));
            sContexts[i].used = TRUE;
            sContexts[i].context = context;
            sContexts[i].stackSize = sThreadStackSize;
            if (context != NULL) {
                IndexContext(&sContexts[i]);
            }
            return &sContexts[i];
        }
    }
    Fatal("too many contexts");
    return NULL;
}

// Hands a context's fiber to the dispatcher to dispose of, the next time it runs. Not at once: the SDK's code goes on
// using the stack of a thread that it destroys after the host has let the context go (OS_DestroyThread cancels the
// alarm of OS_Sleep, which is on that stack), and the DS's stack stays where it was. A thread may destroy any number
// of others before the dispatcher runs again, so the list grows as needed
static void Retire(HostContext *context) {
    if (context->fiber != NULL) {
        if (sRetiredCount == sRetiredCapacity) {
            size_t capacity = sRetiredCapacity != 0 ? 2 * sRetiredCapacity : 16;
            PortFiber **retired = realloc(sRetired, capacity * sizeof(*retired));

            if (retired == NULL) {
                Fatal("out of memory for the fibers to dispose of");
            }
            sRetired = retired;
            sRetiredCapacity = capacity;
        }
        sRetired[sRetiredCount++] = context->fiber;
        context->fiber = NULL;
        sStats.fibersAlive--;
    }
}

// On the host's stack only, when no fiber runs: the dispatcher at each turn, and port_os_run at the end
static void FreeRetired(void) {
    while (sRetiredCount > 0) {
        port_fiber_destroy(sRetired[--sRetiredCount]);
    }
}

static void Release(HostContext *context) {
    Retire(context);
    if (context->context != NULL) {
        UnindexContext(context);
    }
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
// having set sRunning and sCpsrMask back
static void Yield(Request request) {
    sRequest = request;
    sRunning->cpsrMask = sCpsrMask;
    port_fiber_yield();
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

// The host's work

static void QueueWork(int item) {
    if (sWorkCount == WORK_CAPACITY) {
        Fatal("the host's work queue overflowed: an interrupt handler busy-waited for frames");
    }
    sWork[(sWorkHead + sWorkCount) % WORK_CAPACITY] = item;
    sWorkCount++;
}

// Runs the scanline hook for each line and the frame hooks for each frame that came, in order. Only the dispatcher
// calls it, on the host's stack and outside handlers, so that platform code never runs on a game stack or inside the
// game's critical sections
static void RunHostWork(void) {
    if (port_fiber_current() != NULL || sInIrq) {
        Fatal("the host's work would run on a fiber's stack or in a handler");
    }
    while (sWorkCount > 0 && !sStop) {
        int item = sWork[sWorkHead];

        sWorkHead = (sWorkHead + 1) % WORK_CAPACITY;
        sWorkCount--;
        if (item == WORK_FRAME) {
            port_os_frame_hooks();
        } else if (item <= WORK_TIMED) {
            if (sTimedWork[WORK_TIMED - item].work != NULL) {
                sTimedWork[WORK_TIMED - item].work();
            }
        } else if (port_os_scanline_hook != NULL) {
            port_os_scanline_hook(item);
        }
    }
}

void port_os_set_timed_work(void (*work)(void), uint64_t when) {
    int free = -1;
    int i;

    for (i = 0; i < PORT_OS_MAX_TIMED_WORK; i++) {
        if (sTimedWork[i].work == work) {
            sTimedWork[i].when = when;
            return;
        }
        if (free < 0 && sTimedWork[i].work == NULL) {
            free = i;
        }
    }
    if (when == UINT64_MAX) {
        return;
    }
    if (free < 0) {
        Fatal("too many modules with timed work");
    }
    sTimedWork[free].work = work;
    sTimedWork[free].when = when;
}

void port_os_frame_hooks(void) {
    int i;

    for (i = 0; i < sFrameHookCount && !sStop; i++) {
        sFrameHooks[i]();
    }
}

void port_os_add_frame_hook(void (*hook)(void)) {
    if (sFrameHookCount == PORT_OS_MAX_FRAME_HOOKS) {
        Fatal("too many frame hooks");
    }
    sFrameHooks[sFrameHookCount++] = hook;
}

// The clock and its events

static u64 TimerPeriod(const Timer *timer) {
    return (u64)(0x10000 - timer->reload) * timer->prescaler;
}

// The next event of the clock. A line's start is always one, since VCOUNT, the frame and the scanline hook follow
// the lines whatever the interrupts are; the HBlanks only while DISPSTAT requests their interrupt, and a timer's wrap
// only when it interrupts, which is when they can change anything
static u64 NextEvent(EventKind *kind, int *timerNo) {
    u64 when = sNextLine;
    int i;

    *kind = EVENT_LINE;
    *timerNo = 0;
    if (ReadDispStat() & REG_GX_DISPSTAT_HBI_MASK) {
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
    for (i = 0; i < 4; i++) {
        if (sTimers[i].running && sTimers[i].irq && sTimers[i].nextWrap < when) {
            when = sTimers[i].nextWrap;
            *kind = EVENT_TIMER;
            *timerNo = i;
        }
    }
    for (i = 0; i < PORT_OS_MAX_TIMED_WORK; i++) {
        // A moment already past comes now
        u64 at = sTimedWork[i].when > sCycles ? sTimedWork[i].when : sCycles;

        if (sTimedWork[i].work != NULL && sTimedWork[i].when != UINT64_MAX && at < when) {
            when = at;
            *kind = EVENT_TIMED;
            *timerNo = i;
        }
    }
    return when;
}

// Moves the clock to an event and latches in IF what it requests. A line's start and a VBlank also queue the host's
// work, which only the dispatcher runs (RunHostWork): this can run on any fiber, in any critical section
static void RaiseEvent(EventKind kind, u64 when, int timerNo) {
    u16 dispstat;
    u32 line;

    AdvanceClock(when);
    dispstat = ReadDispStat();
    switch (kind) {
    case EVENT_LINE:
        line = LineOf(when);
        sNextLine += CYCLES_PER_LINE;
        if (line == VBLANK_LINE) {
            // The frame ends whether or not its interrupt is requested. The ARM7 counts the VBlanks in the shared
            // memory (OS_GetVBlankCount, PM's waits), in its own VBlank handler at the same moment
            sFrame++;
            (*(vu32 *)HW_VBLANK_COUNT_BUF)++;
            QueueWork(WORK_FRAME);
            if (dispstat & REG_GX_DISPSTAT_VBI_MASK) {
                RequestIF(OS_IE_V_BLANK);
            }
        } else if (line < VBLANK_LINE && port_os_scanline_hook != NULL) {
            QueueWork((int)line);
        }
        if ((dispstat & REG_GX_DISPSTAT_VCI_MASK) && VCountSetting(dispstat) == line) {
            RequestIF(OS_IE_V_COUNT);
        }
        break;
    case EVENT_HBLANK:
        RequestIF(OS_IE_H_BLANK);
        sNextHBlank += CYCLES_PER_LINE;
        break;
    case EVENT_TIMER:
        RequestIF(OS_IE_TIMER0 << timerNo);
        sTimers[timerNo].nextWrap += TimerPeriod(&sTimers[timerNo]);
        break;
    case EVENT_TIMED:
        // Its moment is spent; the work sets the next
        sTimedWork[timerNo].when = UINT64_MAX;
        QueueWork(WORK_TIMED - timerNo);
        break;
    }
}

// Interrupts

static BOOL PendingIrq(void) {
    return (ReadIE() & sIF) != 0;
}

// Whether the code that runs could take an interrupt now: a fiber, not a handler, with the I bit clear and IME on
static BOOL CanTakeIrq(void) {
    return sRunning != NULL && !sInIrq && !(sCpsrMask & CPSR_IRQ_DISABLE) && ReadIME();
}

// Takes a pending interrupt now, if this is a fiber with interrupts enabled. This is where the DS would take the IRQ
// on the next instruction
static void PollIrq(void) {
    if (CanTakeIrq() && PendingIrq()) {
        Yield(REQUEST_IRQ);
    }
}

void port_osi_poll_irq(void) {
    PollIrq();
}

// Runs the clock for cycles of the running code's own work, as a busy loop would: each event on the way is raised
// when it comes, the host's work for a line or a frame is done at once (the fiber yields to the dispatcher for it,
// whatever its I bit), and an interrupt that can be taken is taken there. The cycles that other threads run
// meanwhile don't count towards the loop's own. On the dispatcher's stack (a handler that reads the tick), the host's
// work waits for the dispatcher
static void Spin(u64 cycles) {
    u64 target = sCycles + cycles;
    EventKind kind;
    int timerNo;
    u64 when;

    while ((when = NextEvent(&kind, &timerNo)) <= target) {
        RaiseEvent(kind, when, timerNo);
        if (sWorkCount != 0 && sRunning != NULL) {
            Yield(REQUEST_HOST);
        }
        if (CanTakeIrq() && PendingIrq()) {
            u64 left = target - sCycles;

            Yield(REQUEST_IRQ);
            target = sCycles + left;
        }
    }
    AdvanceClock(target);
}

void port_osi_spin(u64 cycles) {
    Spin(cycles);
    PollIrq();
}

void port_osi_tick_step(void) {
    Spin(sTickStep);
    PollIrq();
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
    // The DS saves and loads the divider's context here too (CP_SaveContext, CPi_RestoreContext); the host's divider
    // keeps no state between a thread's own accesses, which never span a switch
    OSi_ThreadInfo.current = next;
}

// NitroSDK's OS_IrqHandler, in ITCM on the DS: while an interrupt is requested and enabled, and the thread that it
// would interrupt has interrupts enabled, acknowledges the lowest one and runs its entry of OS_IRQTable in IRQ mode,
// then the thread-switch tail. The tail may make another thread the running one, whose own I bit then decides
// whether the next interrupt waits. On the host it runs on the dispatcher's stack only
void OS_IrqHandler(void) {
    if (port_fiber_current() != NULL) {
        Fatal("OS_IrqHandler on a fiber's stack");
    }
    for (;;) {
        HostContext *interrupted = CurrentThreadContext();
        u32 pending = ReadIE() & sIF;
        u32 bit;
        OSIrqFunction handler;

        if (!ReadIME() || pending == 0 || (interrupted->cpsrMask & CPSR_IRQ_DISABLE)) {
            break;
        }
        bit = pending & (~pending + 1);
        port_osi_acknowledge_irq(bit);
        sStats.irqs++;
        sInIrq = TRUE;
        sCpsrMask = interrupted->cpsrMask | CPSR_IRQ_DISABLE;
        handler = OS_IRQTable[__builtin_ctz(bit)];
        if (handler != NULL) {
            handler();
        }
        IrqThreadSwitch();
        sInIrq = FALSE;
    }
}

// OS_Halt on the host: the CPU sleeps until an interrupt is requested and enabled in IE (IME and the I bit don't
// matter to the wakeup), then takes it if the halted thread has interrupts enabled. The host's work for each line and
// frame on the way is done when it comes, before the handlers
static void Halt(void) {
    u64 start = sCycles;

    while (!PendingIrq()) {
        EventKind kind;
        int timerNo;
        u64 when = NextEvent(&kind, &timerNo);

        if (when - start > HALT_LIMIT_CYCLES) {
            sDeadlock = TRUE;
            return;
        }
        RaiseEvent(kind, when, timerNo);
        RunHostWork();
        if (sStop) {
            return;
        }
    }
    OS_IrqHandler();
}

static void ReportDeadlock(void) {
    OSThread *thread;

    fprintf(stderr, "port: every thread waits for an interrupt that nothing can raise (IE %#x, IME %d, frame %u)\n",
            ReadIE(), ReadIME(), sFrame);
    for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
        fprintf(stderr, "  thread %u: priority %u, %s%s\n", thread->id, thread->priority,
                thread->state == OS_THREAD_STATE_READY ? "ready" : "waiting",
                thread == OSi_ThreadInfo.current ? ", running" : "");
    }
}

// The dispatcher: runs fibers on the host stack until the boot function returns, the run is stopped or every thread
// waits for good
static PortOsResult Dispatcher(HostContext *next) {
    for (;;) {
        HostContext *prev;

        FreeRetired();
        RunHostWork();
        if (sStop) {
            return PORT_OS_STOPPED;
        }
        // Loading a context whose I bit is clear unmasks an interrupt that is pending, and the DS takes it before the
        // thread's next instruction (a new thread's first): the thread is interrupted where it would resume. next is
        // always the SDK's current thread here, which is what OS_IrqHandler interrupts; its tail may pick another
        if (!(next->cpsrMask & CPSR_IRQ_DISABLE) && ReadIME() && PendingIrq()) {
            OS_IrqHandler();
            next = CurrentThreadContext();
        }
        if (next->fiber == NULL) {
            next->fiber = port_fiber_create(ContextMain, next, next->stackSize);
            sStats.fibersCreated++;
            sStats.fibersAlive++;
        }
        sRunning = next;
        sCpsrMask = next->cpsrMask;
        next->saved = FALSE;
        sRequest = REQUEST_NONE;
        sStats.fiberSwitches++;
        port_fiber_resume(next->fiber);

        // A fiber that crossed a line or a VBlank yields only for the host's work to be done on this stack. It goes
        // on where it was, which may be between OS_SaveContext and OS_LoadContext (a switch callback that reads the
        // tick), so nothing about its context changes
        while (sRequest == REQUEST_HOST) {
            sRunning = NULL;
            RunHostWork();
            if (sStop) {
                return PORT_OS_STOPPED;
            }
            sRunning = next;
            sCpsrMask = next->cpsrMask;
            sRequest = REQUEST_NONE;
            sStats.fiberSwitches++;
            port_fiber_resume(next->fiber);
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
            if (sStop) {
                return PORT_OS_STOPPED;
            }
            if (sDeadlock) {
                ReportDeadlock();
                return PORT_OS_DEADLOCK;
            }
            next = CurrentThreadContext();
            break;
        case REQUEST_IRQ:
            OS_IrqHandler();
            next = CurrentThreadContext();
            break;
        case REQUEST_EXIT:
            return PORT_OS_RETURNED;
        default:
            Fatal("a fiber gave up the CPU without a request");
        }
    }
}

PortOsResult port_os_run(void (*boot)(void)) {
    PortOsResult result;
    int i;

    memset(sContexts, 0, sizeof(sContexts));
    memset(sIndex, 0, sizeof(sIndex));
    sRetiredCount = 0;
    sRunning = NULL;
    sTarget = NULL;
    sRequest = REQUEST_NONE;
    sDeadlock = FALSE;
    sStop = FALSE;
    memset(&sStats, 0, sizeof(sStats));
    sInIrq = FALSE;
    sIF = 0;
    sCycles = 0;
    sNextLine = CYCLES_PER_LINE;
    sNextHBlank = HBLANK_CYCLE;
    sFrame = 0;
    memset(sTimers, 0, sizeof(sTimers));
    memset(sTimedWork, 0, sizeof(sTimedWork));
    sWorkHead = 0;
    sWorkCount = 0;

    // The registers as the DS enters main(): interrupts off in IME, IE and IF, the display's interrupts off; and the
    // boot indicator that the system leaves for a game started from its card (GBATEK, "DS Memory Map": 1 for a normal
    // boot), which OS_GetBootType and so OS_GetConsoleType read
    reg_OS_IME = 0;
    reg_OS_IE = 0;
    reg_OS_IF = 0;
    reg_GX_DISPSTAT = 0;
    *(vu32 *)HW_VBLANK_COUNT_BUF = 0;
    *(vu16 *)HW_BOOT_CHECK_INFO_BUF = OS_BOOTTYPE_ROM;
    *port_osi_irq_check_word() = 0;
    UpdateDisplayStatus();
    port_osi_reset_irq_table();
    port_osi_reset_threads();
    port_osi_reset_alarms();
    port_osi_reset_tick();
    port_osi_reset_arenas();
    port_osi_reset_locks();
    port_osi_reset_terminate();
    port_osi_reset_vram_exclusive();
    port_osi_reset_valarms();
    port_osi_reset_init_reset();
    port_osi_reset_console();
    port_osi_reset_alloc();

    // The ROM's crt0 sets the CPSR to 0x1f before it jumps to main(): system mode with the I and F bits clear. Only
    // IME holds the interrupts back, until SystemInit turns it on (OS_EnableIrq), long before main's own
    // OS_EnableInterrupts
    sBoot = boot;
    sLauncher = NewContext(NULL);
    sLauncher->isBoot = TRUE;
    sLauncher->cpsrMask = 0;
    sLauncher->stackSize = sLauncherStackSize;
    result = Dispatcher(sLauncher);

    // Every fiber that is left, suspended or done, goes
    for (i = 0; i < MAX_CONTEXTS; i++) {
        if (sContexts[i].used) {
            Retire(&sContexts[i]);
        }
    }
    FreeRetired();
    memset(sContexts, 0, sizeof(sContexts));
    memset(sIndex, 0, sizeof(sIndex));
    sLauncher = NULL;
    return result;
}

void port_os_stop(void) {
    sStop = TRUE;
}

void port_osi_set_context_entry(OSContext *context, void (*entry)(void *), void *arg, BOOL irqEnabled) {
    HostContext *host = FindContext(context);

    if (host == NULL) {
        Fatal("a context entry before OS_InitContext");
    }
    host->entry = entry;
    host->arg = arg;
    host->cpsrMask = irqEnabled ? 0 : CPSR_IRQ_DISABLE;
}

void port_osi_bind_running_context(OSContext *context) {
    if (sRunning == NULL || sRunning->context != NULL || FindContext(context) != NULL) {
        Fatal("port_osi_bind_running_context");
    }
    sRunning->context = context;
    IndexContext(sRunning);
}

void port_osi_release_context(OSContext *context) {
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
// port_osi_set_context_entry. A fiber the context had (a thread that ended, or one that OS_KillThread restarts) is
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
    } else if (host == sRunning) {
        // Only OSi_ExitThread_ArgSpecified's exit-stack branch does this, and BW2 never sets an exit stack
        Fatal("OS_InitContext of the running thread's context");
    }
    Retire(host);
    host->entry = NULL;
    host->arg = NULL;
    host->isBoot = FALSE;
    host->cpsrMask = 0;
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

// The CPSR (os_system.c, os_terminate_proc.c). The I bit is per thread, as each context has its own CPSR on the DS

u32 OS_EnableInterrupts(void) {
    u32 prev = sCpsrMask & CPSR_IRQ_DISABLE;

    sCpsrMask &= ~CPSR_IRQ_DISABLE;
    PollIrq();
    return prev;
}

u32 OS_DisableInterrupts(void) {
    u32 prev = sCpsrMask & CPSR_IRQ_DISABLE;

    sCpsrMask |= CPSR_IRQ_DISABLE;
    return prev;
}

// The assembly clears the I bit and ORs in the mask, so a mask with the F bit sets that too
u32 OS_RestoreInterrupts(u32 mask) {
    u32 prev = sCpsrMask & CPSR_IRQ_DISABLE;

    sCpsrMask = (sCpsrMask & ~CPSR_IRQ_DISABLE) | (mask & CPSR_IRQ_FIQ_MASK);
    if (!(sCpsrMask & CPSR_IRQ_DISABLE)) {
        PollIrq();
    }
    return prev;
}

u32 OS_DisableInterrupts_IrqAndFiq(void) {
    u32 prev = sCpsrMask & CPSR_IRQ_FIQ_MASK;

    sCpsrMask |= CPSR_IRQ_FIQ_MASK;
    return prev;
}

u32 OS_RestoreInterrupts_IrqAndFiq(u32 mask) {
    u32 prev = sCpsrMask & CPSR_IRQ_FIQ_MASK;

    sCpsrMask = (sCpsrMask & ~CPSR_IRQ_FIQ_MASK) | (mask & CPSR_IRQ_FIQ_MASK);
    if (!(sCpsrMask & CPSR_IRQ_DISABLE)) {
        PollIrq();
    }
    return prev;
}

u32 OS_GetCpsrIrq(void) {
    return sCpsrMask & CPSR_IRQ_DISABLE;
}

// IRQ mode in a handler and its tail, which is how OSi_RescheduleThread and four libraries (PM, SOC, FATFS, WCM) defer
// work; the threads' system mode otherwise
u32 OS_GetProcMode(void) {
    return sInIrq ? OS_PROCMODE_IRQ : PROCMODE_SYS;
}

void OS_Halt(void) {
    if (sRunning == NULL || sInIrq) {
        Fatal("OS_Halt outside a thread");
    }
    Yield(REQUEST_HALT);
}

// The hardware timers

void port_osi_start_timer(int timer, u16 count, u32 prescaler, BOOL irq) {
    Timer *t = &sTimers[timer];

    t->running = TRUE;
    t->irq = irq;
    t->reload = count;
    t->prescaler = prescaler;
    t->start = sCycles;
    t->nextWrap = sCycles + TimerPeriod(t);
}

void port_osi_stop_timer(int timer) {
    sTimers[timer].running = FALSE;
}

// The counter as the register reads it: counting from reload since the start, wrapping back to reload. A stopped
// timer reads 0 here, where the DS keeps the last value; the SDK reads only running timers
u16 port_osi_read_timer(int timer) {
    const Timer *t = &sTimers[timer];
    u64 ticks;

    if (!t->running) {
        return 0;
    }
    ticks = (sCycles - t->start) / t->prescaler;
    return (u16)(t->reload + ticks % (0x10000 - t->reload));
}

// The interface for the program, the tests and the other HAL modules

void port_os_set_stack_sizes(uint32_t launcher, uint32_t thread) {
    sLauncherStackSize = launcher;
    sThreadStackSize = thread;
}

void port_os_set_tick_step(uint32_t cycles) {
    sTickStep = cycles;
}

void port_os_request_irq(uint32_t mask) {
    RequestIF(mask);
}

int port_os_interrupts_enabled(void) {
    return CanTakeIrq();
}

void port_os_poll_irq(void) {
    PollIrq();
}

uint64_t port_os_cycles(void) {
    return sCycles;
}

uint32_t port_os_vcount(void) {
    return LineOf(sCycles);
}

uint32_t port_os_frame(void) {
    return sFrame;
}

void port_os_spin(uint64_t cycles) {
    Spin(cycles);
    PollIrq();
}

uint32_t port_os_get_if(void) {
    return sIF;
}

PortOsStats port_os_stats(void) {
    return sStats;
}

uint32_t port_os_stack_used(const void *thread) {
    HostContext *host = FindContext(&((const OSThread *)thread)->context);

    return host != NULL && host->fiber != NULL ? (u32)port_fiber_stack_used(host->fiber) : 0;
}
