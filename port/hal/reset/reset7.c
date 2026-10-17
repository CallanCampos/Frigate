// The host ARM7's side of the OS tag (PXI tag OS), for the decomp's os_reset.c and os_reset_pxi.c. The ARM9 sends a
// command in bits 8 to 14 of a word: OS_ResetSystem sends the reset (0x10), to which the DS's ARM7 answers with the
// same command once it is ready, which sets OS_IsResetOccurred, before the ARM9 ends in OSi_DoResetSystem; OS_Terminate
// sends the terminate (0x20), which the ARM7 takes without an answer (an answer would make the ARM9 terminate again).
// The host's OSi_DoResetSystem ends the program (port/hal/os/os_terminate.c), so the answer is only for the flag.
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

static void OsService(u32 tag, u32 data, BOOL err) {
    u32 command = (data & OS_PXI_COMMAND_MASK) >> OS_PXI_COMMAND_SHIFT;

    if (!err && command == OS_PXI_COMMAND_RESET) {
        port_pxi_reply(PXI_FIFO_TAG_OS, OS_PXI_COMMAND_RESET << OS_PXI_COMMAND_SHIFT, FALSE);
    }
}

void port_reset7_start(void) {
    port_pxi_register_service(PXI_FIFO_TAG_OS, OsService);
}
