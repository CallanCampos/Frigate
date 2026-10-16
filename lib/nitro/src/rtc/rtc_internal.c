// NitroSDK's RTC (rtc_internal.c): the commands that the ARM9 sends the ARM7's RTC driver through the PXI FIFO. The
// driver puts what it read in the shared memory and answers with a word that rtc_api.c's callback handles.
#include "nitro/pxi.h"
#include "nitro/rtc.h"

static BOOL RtcSendPxiCommand(u8 command);

BOOL RTCi_ReadRawDateTimeAsync(void) {
    return RtcSendPxiCommand(RTC_PXI_COMMAND_READ_DATETIME);
}

BOOL RTCi_ReadRawDateAsync(void) {
    return RtcSendPxiCommand(RTC_PXI_COMMAND_READ_DATE);
}

BOOL RTCi_ReadRawTimeAsync(void) {
    return RtcSendPxiCommand(RTC_PXI_COMMAND_READ_TIME);
}

BOOL RTCi_WriteRawStatus2Async(void) {
    return RtcSendPxiCommand(RTC_PXI_COMMAND_WRITE_STATUS2);
}

// NitroSDK's RtcSendPxiCommand: sends a command with the RTC's tag, and tells whether the FIFO took it
static BOOL RtcSendPxiCommand(u8 command) {
    BOOL sent = FALSE;

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_RTC, ((u32)command << RTC_PXI_COMMAND_SHIFT) & RTC_PXI_COMMAND_MASK, FALSE) >=
        0) {
        sent = TRUE;
    }
    return sent;
}
