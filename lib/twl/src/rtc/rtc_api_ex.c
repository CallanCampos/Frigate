// TwlSDK's RTC library (librtc), DSi extension: the commands that only a DSi runs, which main's RTC PXI callback
// passes on to the LTD autoload. ARM code. The ROM names none of it; the file's name is a descriptive guess, and so
// is the meaning of the three commands, since main never sets one of them.

#include "types.h"
#include "twl/rtc.h"

// The RTC's time as its three bytes (hour, minute, second), in the DSi's mirror of the shared memory that the
// ARM7 writes it to
#define HW_RTC_RAW_TIME 0x02fffdec

#define RTC_COMMAND_EX_READ_TIME24 0x10
#define RTC_COMMAND_EX_READ_TIME16 0x11
#define RTC_COMMAND_EX_NOP 0x12

int RTCi_ProcessExCommand(void) {
    RTCWork *work = func_0207cd18();
    int result = RTC_RESULT_SUCCESS;

    switch (work->command) {
    case RTC_COMMAND_EX_READ_TIME24: {
        const u8 *time = (const u8 *)HW_RTC_RAW_TIME;

        *(u32 *)work->data[0] = time[0] | (time[1] << 8) | (time[2] << 16);
        break;
    }
    case RTC_COMMAND_EX_READ_TIME16:
        *(u16 *)work->data[0] = *(const u16 *)HW_RTC_RAW_TIME;
        break;
    case RTC_COMMAND_EX_NOP:
        break;
    default:
        work->busy = FALSE;
        result = RTC_RESULT_INVALID_COMMAND;
        break;
    }
    return result;
}
