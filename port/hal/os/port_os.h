// The platform layer's OS core, for the program (port/app/main.c), the tests and the other HAL modules. It runs the
// game's main() and every SDK thread on fibers of one host thread, with a dispatcher on the host's own stack that
// runs interrupt handlers, the virtual clock, other modules' timed work (the ARM7 sound driver's alarms) and the frame
// and scanline hooks. Plain C types only, so that files without the decomp's headers can include it (u32 is uint32_t
// and BOOL is int on every host).
//
// The design is the reviewed spike's (port/spikes/os-fibers/README.md): the SDK's own scheduler (os_thread.c,
// os_mutex.c, os_message.c, os_event.c) decides everything; the HAL replaces only the contexts, the CPSR, the IRQ
// handler in ITCM, the timers, the display's timing and the DTCM words. Interrupts are taken only at defined points:
// when a thread halts or sleeps, when interrupts are enabled again with one pending, when the clock passes an event
// inside a modelled busy loop (OS_GetTick, OS_SpinWaitSysCycles, SVC_WaitByLoop), and at port_os_poll_irq. Nothing
// depends on host time, so a run is deterministic.
#ifndef PORT_HAL_OS_PORT_OS_H
#define PORT_HAL_OS_PORT_OS_H

#include <stddef.h>
#include <stdint.h>

// The DS's timing: the ARM9's bus clock (the SDK's OS_SYSTEM_CLOCK), 2130 of its cycles a line and 263 lines a frame
// (GBATEK, "DS Video Stuff"), which makes 59.8261 frames a second
#define PORT_OS_CYCLES_PER_SECOND 33514000u
#define PORT_OS_CYCLES_PER_LINE 2130u
#define PORT_OS_LINES_PER_FRAME 263u
#define PORT_OS_VISIBLE_LINES 192u
#define PORT_OS_CYCLES_PER_FRAME (PORT_OS_CYCLES_PER_LINE * PORT_OS_LINES_PER_FRAME)

// How port_os_run ends
typedef enum PortOsResult {
    PORT_OS_STOPPED = 0,  // port_os_stop was called (a frame count reached, the window closed)
    PORT_OS_RETURNED = 1, // the boot function returned
    PORT_OS_DEADLOCK = 2, // every thread waited for an interrupt that nothing could raise any more
} PortOsResult;

// The exit status of a program that the game stopped through OS_Terminate (its fatal error path), and of one whose
// game reset the console (OS_ResetSystem, which BW2 does in ordinary play: returning to the title, deleting the save),
// until the port restarts the game in place
#define PORT_OS_TERMINATE_STATUS 71
#define PORT_OS_RESET_STATUS 74

// Runs boot (the game's main(), NitroMain) as the DS's launcher thread, on a fiber, until port_os_stop is called, boot
// returns or the threads deadlock. Everything the OS layer keeps is set up anew at each call: the state of the files of
// port/hal/os, and of the SDK files they compile (os_thread.c, os_valarm.c, os_reset.c, os_alloc.c). The other SDK
// files that the program links keep theirs, which nothing reads (os_timer.c's reserved timers) or only a DSi uses. So
// tests can run several scenarios in one process; the game runs it once
PortOsResult port_os_run(void (*boot)(void));
// Ends port_os_run at the next point where the dispatcher runs (from a frame hook, typically). The threads' fibers are
// disposed of where they wait
void port_os_stop(void);

// Settings, before port_os_run. The host stacks of the launcher (the game's main(): 8 MiB by default, like a native
// main thread) and of every other thread (256 KiB); the size of the main memory arena (16 MiB by default, 8 times the
// DS's 1.9 MiB, since structures with pointers grow on a 64-bit host); and how far each OS_GetTick call moves the
// clock (64 cycles, one tick, by default)
void port_os_set_stack_sizes(uint32_t launcher, uint32_t thread);
void port_os_set_main_arena_size(size_t bytes);
void port_os_set_tick_step(uint32_t cycles);

// Whether the main memory arena's host block, from its start to its end whatever the game has taken from it, holds all
// of [start, start + size): the game carves its heaps from it, so the game's reads stay in it. 0 before OS_InitArena
// made the block. port/hal/nnsys/fnd_hooks.c checks a read the DS makes past a heap block against it
int port_os_main_arena_holds(const void *start, size_t size);

// Interrupts, for the HAL modules that model the hardware raising one (PXI, DMA, the card). port_os_request_irq sets
// bits of IF; they are taken at the next defined point, never in the middle of the caller.
// port_os_interrupts_enabled is TRUE when an interrupt requested now could be taken at once: code runs on a thread's
// fiber, not in a handler, with the CPSR's I bit clear and IME set. port_os_poll_irq is a defined point: it takes the
// pending interrupts that IE enables now if they can be (a HAL operation that the DS would have finished with an
// interrupt while its caller waited calls it before returning)
void port_os_request_irq(uint32_t mask);
int port_os_interrupts_enabled(void);
void port_os_poll_irq(void);

// The frame. At each VBlank (line 192), before its interrupt is taken, the dispatcher calls port_os_frame_hooks on the
// host's own stack, never a fiber's, which calls every hook added with port_os_add_frame_hook in the order they were
// added. In the program that order is: the geometry engine's swap (port/gfx/ge/ge_hal.c) and the program's own hook
// (input, presentation, pacing: port/app/main.c), both added before the game runs; then the ARM7's (touch sampling,
// the sound driver, the microphone: port/hal/pxi/arm7.c), added when OS_Init starts PXI; then the save's
// (port/hal/card/backup.c), added when it starts the card. Hooks may call port_os_stop and must not call the SDK
#define PORT_OS_MAX_FRAME_HOOKS 8
void port_os_frame_hooks(void);
void port_os_add_frame_hook(void (*hook)(void));
// Host work at a moment of the virtual clock, for a module that models hardware whose own clock raises interrupts (the
// ARM7's sound alarms, port/hal/snd7): once the clock reaches when, the dispatcher calls work on the host's own stack,
// as an event of the clock like a line's start, so that an interrupt the work requests is taken there and not at the
// next VBlank. One moment for each function: a later call moves it, and UINT64_MAX cancels it; a moment already past
// comes at once. Work must not call the SDK. port_os_run forgets every moment
#define PORT_OS_MAX_TIMED_WORK 4
void port_os_set_timed_work(void (*work)(void), uint64_t when);
// The renderer's per-line hook: called on the host's stack for each visible line (0-191) at the start of that line,
// that is after the HBlank of the line before it has been handled, with the registers as the DS draws the line with.
// NULL (the default) costs nothing
extern void (*port_os_scanline_hook)(int line);

// The virtual clock, in cycles of the bus clock since port_os_run began; the line it is on (VCOUNT), and the VBlanks
// so far. port_os_spin stands for a busy loop of that many cycles in the code that runs: the clock moves, and the
// events and interrupts on the way come as they would (it is how OS_SpinWaitSysCycles and SVC_WaitByLoop wait)
uint64_t port_os_cycles(void);
uint32_t port_os_vcount(void);
uint32_t port_os_frame(void);
void port_os_spin(uint64_t cycles);
// Wait for an absolute device deadline, spending elapsed IRQ/other-thread time toward it (unlike spin's own CPU
// work). Physical DMA unit stalls may pass the target before ARM9 can resume; they never extend the target.
// Boot/thread fibers and IRQ handlers may call it. Returns 0 without advancing from a host callback or
// outside a run; returns 1 after reaching/passing deadline. A past deadline never rewinds. Event ordering is unchanged.
// Timed host work progresses even with IRQs masked; IF delivery still follows I/IE/IME. IRQ waits run that work on
// their existing dispatcher stack without nested IRQ delivery. Host callbacks must publish device state, never wait
// recursively or call the SDK. A stop disposes a suspended thread fiber as for other waits; from an IRQ wait it
// returns 0, and the handler must unwind. This is a device wait, not a sleeping-thread or CPU-work primitive.
int port_os_wait_until(uint64_t deadline);
// IF as the HAL keeps it (reg_OS_IF reads the same), and OS_SetIrqCheckFlag's word (HW_INTR_CHECK_BUF, at the end of
// the host's DTCM block), for handlers that can't reach it through HW_DTCM yet (see os_irqTable.c)
uint32_t port_os_get_if(void);
void port_os_set_irq_check_flag(uint32_t interrupts);
uint32_t port_os_get_irq_check_flag(void);

// Counts for tests and measurements
typedef struct PortOsStats {
    uint32_t fiberSwitches; // resumptions of a fiber by the dispatcher
    uint32_t fibersAlive;
    uint32_t fibersCreated;
    uint32_t halts;
    uint32_t irqs;              // handlers run
    uint64_t deviceStallCycles; // physical CARD DMA bus service; excluded from own CPU work, counted once
} PortOsStats;
PortOsStats port_os_stats(void);
// The deepest a thread's host stack (given as its OSThread) has gone, when the fibers are painted
// (port_fiber_set_paint), else 0
uint32_t port_os_stack_used(const void *thread);

#endif // PORT_HAL_OS_PORT_OS_H
