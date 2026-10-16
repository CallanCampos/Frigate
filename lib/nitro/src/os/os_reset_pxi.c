// The Thumb part of NitroSDK's reset and terminate handling that the PXI FIFO's OS tag carries (the name is a guess;
// NitroSDK's os_reset.c has these three as static functions, but here they are Thumb code with a .bss of their own
// between os_irqTable's and os_spinLock's, while OS_InitReset and OS_ResetSystem are ARM code).
#include "nitro/os.h"
#include "nitro/pxi.h"

// Whether the ARM7 said that a reset happened, and that the system is being terminated
static struct {
    u16 resetOccurred;
    u16 terminating;
} sResetFlags;

BOOL OS_IsResetOccurred(void) {
    return sResetFlags.resetOccurred;
}

// The function that handles the words of the FIFO's OS tag
void OSi_CommonCallback(u32 fifoTag, u32 data, BOOL err) {
    u16 command = (data & OS_PXI_COMMAND_MASK) >> OS_PXI_COMMAND_SHIFT;

    if (command == OS_PXI_COMMAND_RESET) {
        sResetFlags.resetOccurred = TRUE;
    } else if (command == OS_PXI_COMMAND_TERMINATE) {
        sResetFlags.terminating = TRUE;
        OS_Terminate();
    } else {
        OS_Terminate();
    }
}

// Sends the ARM7 a command, until the FIFO takes it
void OSi_SendToPxi(u32 command) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_OS, command << OS_PXI_COMMAND_SHIFT, FALSE) != 0) {
    }
}
