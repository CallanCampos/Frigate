// The linker-script symbols of the DS memory map that NitroSDK's C references, and the host's DTCM.
//
// SDK_AUTOLOAD_DTCM_START is where the DS's linker places the DTCM module (0x02fe0000 in BW2), and os.h's HW_DTCM is
// its address: the SDK and the game reach DTCM's system words at fixed offsets from it (OS_SetIrqCheckFlag's word at
// 0x3ff8, the IRQ stack's top at 0x3f80, the end of the game's DTCM arena at 0x3000; gfl/heap_dtcm.c). On the host it
// is a 16 KiB block, as large as the DS's DTCM: the offsets are fixed, and the DTCM arena that the game carves out of
// it (OS_InitArena, gfl/heap_dtcm.c) holds what the game sizes in bytes. The DTCM module's own data and bss (the
// interrupt table and the queue of threads in OS_WaitIrq) are ordinary host data (os_irqTable.c), so the block only
// holds what the program places in it.
//
// SDK_IRQ_STACKSIZE and SDK_SYS_STACKSIZE are absolute symbols on the DS, whose address is the value (the IRQ stack's
// and the launcher's stack sizes, 0x800 and 0x1800 in BW2). Only the renamed DS version of OS_InitThread
// (os_thread_host.c) reads them, and it never runs; the host's arena and interrupt functions take the values from
// os_internal.h. The symbols only have to exist.
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"

u32 SDK_AUTOLOAD_DTCM_START[PORT_OSI_DTCM_SIZE / sizeof(u32)] __attribute__((aligned(32)));
u8 SDK_IRQ_STACKSIZE[1];
u8 SDK_SYS_STACKSIZE[1];

u8 *port_osi_dtcm(void) {
    return (u8 *)SDK_AUTOLOAD_DTCM_START;
}

vu32 *port_osi_irq_check_word(void) {
    return (vu32 *)(port_osi_dtcm() + PORT_OSI_INTR_CHECK_BUF);
}

void port_os_set_irq_check_flag(uint32_t interrupts) {
    *port_osi_irq_check_word() |= interrupts;
}

uint32_t port_os_get_irq_check_flag(void) {
    return *port_osi_irq_check_word();
}
