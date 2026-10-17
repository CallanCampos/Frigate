// The host side of the SDK's threads and interrupts (spike). The SDK's scheduler stays its own C; os_host.c stands in
// for what the DS does in assembly or hardware, and runs every SDK thread on a fiber of one host thread. This header
// only uses the SDK's types, so that both the SDK's files and host files can include it.
#ifndef OS_HOST_H
#define OS_HOST_H

#include "nitro/os.h"

// Runs boot as the launcher thread (the thread the DS's main runs in, before OS_InitThread names it) until it
// returns. Gives 0, or -1 when every thread waited for an interrupt that could never come
int hal_os_run(void (*boot)(void));

// Where a context starts when it next runs, after OS_InitContext: the real function and argument that a 64-bit host
// can't keep in the context's 32-bit pc and r0. irqEnabled is the CPSR's I bit the thread starts with (clear for a
// new thread, set for one that OS_KillThread restarts)
void hal_os_set_context_entry(OSContext *context, void (*entry)(void *), void *arg, BOOL irqEnabled);
// Names the context of the thread that runs (OS_InitThread does this for the launcher)
void hal_os_bind_running_context(OSContext *context);
// Forgets a context whose thread will never run again, disposing of its fiber
void hal_os_release_context(OSContext *context);
// Zeroes os_thread.c's own state, between two runs in one process (os_thread_host.c)
void hal_os_reset_threads(void);

// The interrupt registers that the game reaches through reg_OS_IME, reg_OS_IE and reg_OS_IF on the DS
void hal_os_set_ime(BOOL enabled);
u32 hal_os_get_if(void);
// DISPSTAT (reg_GX_DISPSTAT): its VBlank, HBlank and V-count interrupt enables decide whether the display requests
// those interrupts at all, as GX_VBlankIntr and GX_HBlankIntr set them; IE only decides whether a request is taken.
// Reads add the status flags of the current line
void hal_os_set_dispstat(u16 value);
u16 hal_os_get_dispstat(void);
// OS_SetIrqCheckFlag's word (HW_INTR_CHECK_BUF, at the end of DTCM on the DS)
void hal_os_set_irq_check_flag(u32 interrupts);
u32 hal_os_get_irq_check_flag(void);

// The virtual clock, in the ARM9's bus cycles (33.514 MHz). It moves when a thread halts (to the next event), by
// tickStep cycles at each OS_GetTick, and by hal_os_spin, which stands for a busy loop of that many cycles
u64 hal_os_cycles(void);
void hal_os_spin(u64 cycles);
void hal_os_set_tick_step(u32 cycles);
// The scan line (VCOUNT) and the number of VBlanks so far
u32 hal_os_vcount(void);
u32 hal_os_frame(void);
// Called at each VBlank, whether or not DISPSTAT requests its interrupt, before its handler: where the real HAL
// presents the frame and paces to 60 Hz. It runs on the dispatcher's (host) stack, never on a fiber's, and must not
// call the SDK
void hal_os_set_frame_hook(void (*hook)(u32 frame));

// The host stack size of the threads created after this call, and of the launcher
void hal_os_set_stack_sizes(u32 launcher, u32 thread);
// The deepest that a thread's stack has gone, when the fibers are painted (hal_fiber_set_paint)
u32 hal_os_stack_used(const OSThread *thread);
// Counts, for tests and measurements: fiber switches, fibers alive, OS_Halt calls, IRQs dispatched
typedef struct HalOsStats {
    u32 fiberSwitches;
    u32 fibersAlive;
    u32 fibersCreated;
    u32 halts;
    u32 irqs;
} HalOsStats;
HalOsStats hal_os_stats(void);

#endif // OS_HOST_H
