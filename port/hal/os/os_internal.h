// What the files of port/hal/os share with each other, in the SDK's types: the host's version of NitroSDK's OS module
// (lib/nitro/src/os, the files in port/hal/os/replaced.txt), on which every thread, interrupt, alarm and timer of the
// game runs. It has the host's DTCM, where the SDK keeps the interrupt check word and the IRQ stack's magic words; the
// contexts' side table that core.c keeps for os_thread_host.c, since a 64-bit host can't keep a thread's entry in its
// context; the reset of each file's state before a run; the interrupt controller (IE, IME, IF), which the VBlank and
// the timers raise; the hardware timers on the virtual clock, which the SDK's tick count and alarms run on; and the
// modelled busy loops of OS_GetTick and the SDK's waits. Nothing outside port/hal/os includes it but its tests
// (port/tests/os).
#ifndef PORT_HAL_OS_OS_INTERNAL_H
#define PORT_HAL_OS_OS_INTERNAL_H

#include "nitro/os.h"

// The host's DTCM: 16 KiB, as on the DS, since the SDK and the game place things at fixed offsets from its start
// (HW_INTR_CHECK_BUF at 0x3ff8, the IRQ stack's top at 0x3f80, the game's DTCM arena end at 0x3000). It is the
// linker's SDK_AUTOLOAD_DTCM_START (memory_map.c), the address HW_DTCM names
#define PORT_OSI_DTCM_SIZE 0x4000
#define PORT_OSI_DTCM_SYSRV_END 0xc0
#define PORT_OSI_IRQ_STACK_TOP 0x3f80
// The sizes of the IRQ stack, under its top, and of the system stack (the launcher's), under the IRQ stack: absolute
// symbols of BW2's linker script, SDK_IRQ_STACKSIZE and SDK_SYS_STACKSIZE, whose values are in the literal pools of
// crt0, OS_InitThread and OS_GetInitArenaHi. Handlers run on the dispatcher's stack and the launcher on its fiber, so
// on the host they only place the IRQ stack checker's magic words and the end of the DTCM arena, where the DS has them
#define PORT_OSI_IRQ_STACK_SIZE 0x800
#define PORT_OSI_SYS_STACK_SIZE 0x1800
#define PORT_OSI_INTR_CHECK_BUF 0x3ff8

// The host's DTCM block, and the address of a byte offset into it
u8 *port_osi_dtcm(void);

// OS_SetIrqCheckFlag's word in the host's DTCM
vu32 *port_osi_irq_check_word(void);

// The contexts' side table (core.c), for os_thread_host.c: where a context starts the next time it runs, since a
// 64-bit host can't keep the entry and its argument in the context's 32-bit pc and r0 (irqEnabled is the CPSR's I bit
// it starts with: TRUE for a new thread, FALSE for one that OS_KillThread restarts); naming the context of the thread
// that runs (OS_InitThread, for the launcher); and forgetting a context whose thread will never run again
void port_osi_set_context_entry(OSContext *context, void (*entry)(void *), void *arg, BOOL irqEnabled);
void port_osi_bind_running_context(OSContext *context);
void port_osi_release_context(OSContext *context);

// The state of each file, set back to what a new run starts with (port_os_run calls them before the boot function)
void port_osi_reset_threads(void);
void port_osi_reset_alarms(void);
void port_osi_reset_tick(void);
void port_osi_reset_irq_table(void);
void port_osi_reset_arenas(void);
void port_osi_reset_locks(void);
void port_osi_reset_terminate(void);
void port_osi_reset_vram_exclusive(void);
void port_osi_reset_valarms(void);
void port_osi_reset_init_reset(void);
void port_osi_reset_console(void);
void port_osi_reset_alloc(void);

// The interrupt controller. IE and IME are the registers in the HAL's I/O memory (reg_OS_IE, reg_OS_IME), which the
// SDK's inline functions write; IF is the HAL's, mirrored to reg_OS_IF for reads, since a write to it acknowledges
// bits instead of storing them. port_osi_poll_irq takes what is pending, at a defined point
void port_osi_acknowledge_irq(u32 mask);
void port_osi_poll_irq(void);

// The hardware timers 0 to 3, on the virtual clock: start counting up from count, by one every prescaler cycles (1,
// 64, 256 or 1024), interrupting (when irq is set) and reloading count at each wrap; stop; read the counter
void port_osi_start_timer(int timer, u16 count, u32 prescaler, BOOL irq);
void port_osi_stop_timer(int timer);
u16 port_osi_read_timer(int timer);

// The modelled busy loops: OS_GetTick's step (its own cycles, then the tick), and any wait of a known length
void port_osi_tick_step(void);
void port_osi_spin(u64 cycles);

// OS_Init's wait for the screen's line 0 (os_init.c), static in the SDK, for the tests
void OSi_WaitVCount0(void);

#endif // PORT_HAL_OS_OS_INTERNAL_H
