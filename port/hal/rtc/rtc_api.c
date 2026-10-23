// NitroSDK's RTC API on the host (rtc_api.c, which port/hal/replaced.txt keeps out of the game's sources for its
// assembly wait, RtcWaitBusy). The functions that the game has keep the decomp's interface and protocol: they send
// their command through the decomp's rtc_internal.c (RTCi_ReadRaw*Async) to the host's ARM7 (rtc7.c), which leaves the
// clock's registers in the shared memory and answers; the answer's callback turns them into a date and a time, from the
// interrupt, as on the DS. The waits of the synchronous functions are modelled busy loops, which the answer ends before
// they start whenever interrupts are enabled.
//
// The decomp's RtcCommonCallback also has the sequences of the commands that set the clock and its alarms, which
// nothing in the game starts (rtc_api.c has none of the functions that would), and hands commands it doesn't know to
// the DSi's RTC code on a DSi. Here those answers end the command with RTC_RESULT_INVALID_COMMAND.
#include "hal/os/port_os.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/rtc.h"

// What a command waits for, as rtc_api.c numbers the sequences
#define RTC_SEQ_DATE_GET 0
#define RTC_SEQ_TIME_GET 1
#define RTC_SEQ_DATETIME_GET 2

// The registers in the shared memory, OSSystemWork's realTimeClock: the date and the time words
#define RTC_RAW_DATE_YEAR(raw) ((raw) & 0xff)
#define RTC_RAW_DATE_MONTH(raw) (((raw) >> 8) & 0x1f)
#define RTC_RAW_DATE_DAY(raw) (((raw) >> 16) & 0x3f)
#define RTC_RAW_TIME_HOUR(raw) ((raw) & 0x3f)
#define RTC_RAW_TIME_MINUTE(raw) (((raw) >> 8) & 0x7f)
#define RTC_RAW_TIME_SECOND(raw) (((raw) >> 16) & 0x7f)

// The cycles of each turn of RtcWaitBusy's loop: a load, a compare and a branch
#define RTC_WAIT_CYCLES 8

// The library's state: whether it was set up, and the work of the command under way
static u16 rtcInitialized;
static RTCWork rtcWork;

static void RtcCommonCallback(u32 tag, u32 data, BOOL err);

// NitroSDK's RtcBCD2HEX: the number that the digits of a BCD number make, or 0 when a nibble is not a digit
static u32 RtcBCD2HEX(u32 bcd) {
    u32 result = 0;
    u32 place;
    int i;

    for (i = 0; i < 8; i++) {
        if (((bcd >> (i * 4)) & 0xf) >= 10) {
            return 0;
        }
    }
    for (i = 0, place = 1; i < 8; i++) {
        result += ((bcd >> (i * 4)) & 0xf) * place;
        place *= 10;
    }
    return result;
}

static void RtcGetResultCallback(RTCResult result, void *arg) {
    rtcWork.commonResult = result;
}

// NitroSDK's RtcWaitBusy, assembly in the SDK: waits for the lock that the answer clears, as a modelled busy loop
static void RtcWaitBusy(void) {
    while (rtcWork.lock) {
        port_os_spin(RTC_WAIT_CYCLES);
    }
}

// The end of the synchronous functions: the result of the asynchronous call that started the command, or the answer's.
// rtc_api.c stores the call's result in commonResult before it waits, which the answer's callback overwrites when it
// comes; on the host the answer comes inside the call, before that store, so the call's result is kept apart here
static RTCResult RtcWaitResult(int sent) {
    if (sent != RTC_RESULT_SUCCESS) {
        rtcWork.commonResult = sent;
        return sent;
    }
    RtcWaitBusy();
    return rtcWork.commonResult;
}

void RTC_Init(void) {
    if (!rtcInitialized) {
        rtcInitialized = TRUE;
        rtcWork.lock = FALSE;
        rtcWork.callback = NULL;
        rtcWork.interrupt = NULL;
        rtcWork.buffer[0] = NULL;
        rtcWork.buffer[1] = NULL;

        PXI_Init();
        while (!PXI_IsCallbackReady(PXI_FIFO_TAG_RTC, PXI_PROC_ARM7)) {
        }
        PXI_SetFifoRecvCallback(PXI_FIFO_TAG_RTC, RtcCommonCallback);
    }
}

// Takes the lock for a command, or RTC_RESULT_BUSY while one is under way
static int RtcLock(void) {
    u32 enabled = OS_DisableInterrupts();

    if (rtcWork.lock) {
        OS_RestoreInterrupts(enabled);
        return RTC_RESULT_BUSY;
    }
    rtcWork.lock = TRUE;
    OS_RestoreInterrupts(enabled);
    return RTC_RESULT_SUCCESS;
}

// Starts the command of the sequence, whose answer fills data
static int RtcStart(u32 sequence, void *data0, void *data1, RTCCallback callback, void *arg, BOOL (*send)(void)) {
    if (RtcLock() != RTC_RESULT_SUCCESS) {
        return RTC_RESULT_BUSY;
    }
    rtcWork.sequence = sequence;
    rtcWork.index = 0;
    rtcWork.buffer[0] = data0;
    rtcWork.buffer[1] = data1;
    rtcWork.callback = callback;
    rtcWork.callbackArg = arg;
    if (send()) {
        return RTC_RESULT_SUCCESS;
    }
    rtcWork.lock = FALSE;
    return RTC_RESULT_SEND_ERROR;
}

RTCResult RTC_GetDateAsync(RTCDate *date, RTCCallback callback, void *arg) {
    return RtcStart(RTC_SEQ_DATE_GET, date, NULL, callback, arg, RTCi_ReadRawDateAsync);
}

RTCResult RTC_GetDate(RTCDate *date) {
    return RtcWaitResult(RTC_GetDateAsync(date, RtcGetResultCallback, NULL));
}

RTCResult RTC_GetTimeAsync(RTCTime *time, RTCCallback callback, void *arg) {
    return RtcStart(RTC_SEQ_TIME_GET, time, NULL, callback, arg, RTCi_ReadRawTimeAsync);
}

RTCResult RTC_GetTime(RTCTime *time) {
    return RtcWaitResult(RTC_GetTimeAsync(time, RtcGetResultCallback, NULL));
}

RTCResult RTC_GetDateTimeAsync(RTCDate *date, RTCTime *time, RTCCallback callback, void *arg) {
    return RtcStart(RTC_SEQ_DATETIME_GET, date, time, callback, arg, RTCi_ReadRawDateTimeAsync);
}

RTCResult RTC_GetDateTime(RTCDate *date, RTCTime *time) {
    return RtcWaitResult(RTC_GetDateTimeAsync(date, time, RtcGetResultCallback, NULL));
}

RTCWork *RTCi_GetSysWork(void) {
    return &rtcWork;
}

static void ReadDate(RTCDate *date) {
    u32 raw = OS_GetSystemWork()->real_time_clock[0];

    date->year = RtcBCD2HEX(RTC_RAW_DATE_YEAR(raw));
    date->month = RtcBCD2HEX(RTC_RAW_DATE_MONTH(raw));
    date->day = RtcBCD2HEX(RTC_RAW_DATE_DAY(raw));
    date->week = RTC_GetDayOfWeek(date);
}

static void ReadTime(RTCTime *time) {
    u32 raw = OS_GetSystemWork()->real_time_clock[1];

    time->hour = RtcBCD2HEX(RTC_RAW_TIME_HOUR(raw));
    time->minute = RtcBCD2HEX(RTC_RAW_TIME_MINUTE(raw));
    time->second = RtcBCD2HEX(RTC_RAW_TIME_SECOND(raw));
}

// NitroSDK's RtcCommonCallback for the commands that the game has: the ARM7's answer, with the command in bits 8 to 14
// and the result in the low byte, or the alarm interrupt, which isn't an answer
static void RtcCommonCallback(u32 tag, u32 data, BOOL err) {
    int result;
    u32 command;
    u8 pxiResult;

    if (err) {
        rtcWork.index = 0;
        rtcWork.lock = FALSE;
        if (rtcWork.callback != NULL) {
            RTCCallback callback = rtcWork.callback;

            rtcWork.callback = NULL;
            callback(RTC_RESULT_FATAL_ERROR, rtcWork.callbackArg);
        }
        return;
    }

    command = (data & RTC_PXI_COMMAND_MASK) >> RTC_PXI_COMMAND_SHIFT;
    pxiResult = (u8)data;
    if (command == RTC_PXI_COMMAND_INTERRUPT) {
        if (rtcWork.interrupt != NULL) {
            rtcWork.interrupt();
        }
        return;
    }

    rtcWork.index = 0;
    if (pxiResult == RTC_PXI_RESULT_SUCCESS) {
        result = RTC_RESULT_SUCCESS;
        switch (rtcWork.sequence) {
        case RTC_SEQ_DATE_GET:
            ReadDate(rtcWork.buffer[0]);
            break;
        case RTC_SEQ_TIME_GET:
            ReadTime(rtcWork.buffer[0]);
            break;
        case RTC_SEQ_DATETIME_GET:
            ReadDate(rtcWork.buffer[0]);
            ReadTime(rtcWork.buffer[1]);
            break;
        default:
            result = RTC_RESULT_INVALID_COMMAND;
            break;
        }
    } else {
        switch (pxiResult) {
        case RTC_PXI_RESULT_INVALID_COMMAND:
            result = RTC_RESULT_INVALID_COMMAND;
            break;
        case RTC_PXI_RESULT_ILLEGAL_STATUS:
            result = RTC_RESULT_ILLEGAL_STATUS;
            break;
        case RTC_PXI_RESULT_BUSY:
            result = RTC_RESULT_BUSY;
            break;
        default:
            result = RTC_RESULT_FATAL_ERROR;
            break;
        }
    }

    rtcWork.lock = FALSE;
    if (rtcWork.callback != NULL) {
        RTCCallback callback = rtcWork.callback;

        rtcWork.callback = NULL;
        callback(result, rtcWork.callbackArg);
    }
}
