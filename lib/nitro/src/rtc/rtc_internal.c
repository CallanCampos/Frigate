// NitroSDK's RTC (rtc_internal.c): the commands that the ARM9 sends the ARM7's RTC driver through the PXI FIFO. The
// driver puts what it read in the shared memory and answers with a word that rtc_api.c's callback handles.
#include "nitro/pxi.h"
#include "nitro/rtc.h"

static BOOL func_0207d094(u8 command);

BOOL func_0207d064(void) {
    return func_0207d094(RTC_PXI_COMMAND_READ_DATETIME);
}

BOOL func_0207d070(void) {
    return func_0207d094(RTC_PXI_COMMAND_READ_DATE);
}

BOOL func_0207d07c(void) {
    return func_0207d094(RTC_PXI_COMMAND_READ_TIME);
}

BOOL func_0207d088(void) {
    return func_0207d094(RTC_PXI_COMMAND_WRITE_STATUS2);
}

// NitroSDK's RtcSendPxiCommand: sends a command with the RTC's tag, and tells whether the FIFO took it
static BOOL func_0207d094(u8 command) {
    BOOL sent = FALSE;

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_RTC, ((u32)command << RTC_PXI_COMMAND_SHIFT) & RTC_PXI_COMMAND_MASK, FALSE) >=
        0) {
        sent = TRUE;
    }
    return sent;
}
