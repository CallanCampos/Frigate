// NitroSDK's RTC (rtc_api.c): the clock chip, which the ARM7 reads and writes. The ARM9 sends it a command through the
// PXI FIFO, the ARM7 leaves the BCD fields in the shared memory and answers with a word, and the callback of the FIFO's
// RTC tag turns them into dates and times. The game only reads the clock. Thumb code, except for the loop that waits
// for an answer.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/rtc.h"
#include "twl/rtc.h"

// What the clock chip's registers look like in the shared memory, after the ARM7 read them
typedef struct RTCRawDate {
    u32 year : 8;
    u32 month : 5;
    u32 pad1 : 3;
    u32 day : 6;
    u32 pad2 : 2;
    u32 week : 3;
    u32 pad3 : 5;
} RTCRawDate;

typedef struct RTCRawTime {
    u32 hour : 6;
    u32 afternoon : 1;
    u32 pad1 : 1;
    u32 minute : 7;
    u32 pad2 : 1;
    u32 second : 7;
    u32 pad3 : 9;
} RTCRawTime;

typedef struct RTCRawAlarm {
    u32 week : 3;
    u32 pad1 : 4;
    u32 weekAvailable : 1;
    u32 hour : 6;
    u32 afternoon : 1;
    u32 hourAvailable : 1;
    u32 minute : 7;
    u32 minuteAvailable : 1;
    u32 pad2 : 8;
} RTCRawAlarm;

// The second status register: the mode of the first interrupt pin (4 is the alarm) and whether the second pin takes
// the second alarm
typedef struct RTCRawStatus2 {
    u16 int1Mode : 4;
    u16 pad1 : 2;
    u16 int2Alarm : 1;
    u16 pad2 : 9;
} RTCRawStatus2;

#define RTC_INT1_MODE_ALARM 4

#define RTC_RAW_DATE (*(volatile RTCRawDate *)0x02fffde8)
#define RTC_RAW_TIME (*(volatile RTCRawTime *)0x02fffdec)
#define RTC_RAW_ALARM (*(volatile RTCRawAlarm *)0x02fffdec)
#define RTC_RAW_STATUS2 (*(volatile RTCRawStatus2 *)0x02fffdea)

// The alarm fields that the clock compares, in an RTC alarm's mask
#define RTC_ALARM_MASK_WEEK 1
#define RTC_ALARM_MASK_HOUR 2
#define RTC_ALARM_MASK_MINUTE 4

// What a command is waiting for, in the order of the callback's switch
typedef enum {
    RTC_SEQ_DATE_GET,
    RTC_SEQ_TIME_GET,
    RTC_SEQ_DATETIME_GET,
    RTC_SEQ_DATE_SET,
    RTC_SEQ_TIME_SET,
    RTC_SEQ_DATETIME_SET,
    RTC_SEQ_ALARM1_ENABLED_GET,
    RTC_SEQ_ALARM2_ENABLED_GET,
    RTC_SEQ_ALARM_GET,
    RTC_SEQ_ALARM1_ENABLED_SET,
    RTC_SEQ_ALARM2_ENABLED_SET,
    RTC_SEQ_COUNT = 16,
} RTCSequence;

// The library's state: whether it was set up, and the work of the command under way
static u16 rtcInitialized;
static RTCWork rtcWork;

static void RtcCommonCallback(u32 tag, u32 data, BOOL err);
static u32 RtcBCD2HEX(u32 bcd);
static void RtcGetResultCallback(int result, void *arg);
static void RtcWaitBusy(void);

void RTC_Init(void) {
    if (!rtcInitialized) {
        rtcInitialized = TRUE;
        rtcWork.lock = FALSE;
        rtcWork.callback = NULL;
        rtcWork.alarmCallback = NULL;
        rtcWork.data[0] = NULL;
        rtcWork.data[1] = NULL;

        PXI_Init();
        while (!PXI_IsCallbackReady(PXI_FIFO_TAG_RTC, MI_PROCESSOR_ARM7)) {
        }
        PXI_SetFifoRecvCallback(PXI_FIFO_TAG_RTC, RtcCommonCallback);
    }
}

int RTC_GetDateAsync(RTCDate *date, RTCCallback callback, void *arg) {
    u32 enabled = OS_DisableInterrupts();

    if (rtcWork.lock) {
        OS_RestoreInterrupts(enabled);
        return RTC_RESULT_BUSY;
    }
    rtcWork.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    rtcWork.command = RTC_SEQ_DATE_GET;
    rtcWork.busy = 0;
    rtcWork.data[0] = date;
    rtcWork.callback = callback;
    rtcWork.callbackArg = arg;
    if (RTCi_ReadRawDateAsync()) {
        return RTC_RESULT_SUCCESS;
    }
    rtcWork.lock = FALSE;
    return RTC_RESULT_SEND_ERROR;
}

int RTC_GetDate(RTCDate *date) {
    rtcWork.commonResult = RTC_GetDateAsync(date, RtcGetResultCallback, NULL);
    if (rtcWork.commonResult == RTC_RESULT_SUCCESS) {
        RtcWaitBusy();
    }
    return rtcWork.commonResult;
}

int RTC_GetTimeAsync(RTCTime *time, RTCCallback callback, void *arg) {
    u32 enabled = OS_DisableInterrupts();

    if (rtcWork.lock) {
        OS_RestoreInterrupts(enabled);
        return RTC_RESULT_BUSY;
    }
    rtcWork.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    rtcWork.command = RTC_SEQ_TIME_GET;
    rtcWork.busy = 0;
    rtcWork.data[0] = time;
    rtcWork.callback = callback;
    rtcWork.callbackArg = arg;
    if (RTCi_ReadRawTimeAsync()) {
        return RTC_RESULT_SUCCESS;
    }
    rtcWork.lock = FALSE;
    return RTC_RESULT_SEND_ERROR;
}

int RTC_GetTime(RTCTime *time) {
    rtcWork.commonResult = RTC_GetTimeAsync(time, RtcGetResultCallback, NULL);
    if (rtcWork.commonResult == RTC_RESULT_SUCCESS) {
        RtcWaitBusy();
    }
    return rtcWork.commonResult;
}

int RTC_GetDateTimeAsync(RTCDate *date, RTCTime *time, RTCCallback callback, void *arg) {
    u32 enabled = OS_DisableInterrupts();

    if (rtcWork.lock) {
        OS_RestoreInterrupts(enabled);
        return RTC_RESULT_BUSY;
    }
    rtcWork.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    rtcWork.command = RTC_SEQ_DATETIME_GET;
    rtcWork.busy = 0;
    rtcWork.data[0] = date;
    rtcWork.data[1] = time;
    rtcWork.callback = callback;
    rtcWork.callbackArg = arg;
    if (RTCi_ReadRawDateTimeAsync()) {
        return RTC_RESULT_SUCCESS;
    }
    rtcWork.lock = FALSE;
    return RTC_RESULT_SEND_ERROR;
}

int RTC_GetDateTime(RTCDate *date, RTCTime *time) {
    rtcWork.commonResult = RTC_GetDateTimeAsync(date, time, RtcGetResultCallback, NULL);
    if (rtcWork.commonResult == RTC_RESULT_SUCCESS) {
        RtcWaitBusy();
    }
    return rtcWork.commonResult;
}

RTCWork *RTCi_GetSysWork(void) {
    return &rtcWork;
}

// NitroSDK's RtcCommonCallback: the answer of the ARM7 to a command. The low byte has its result, and the bits 8 to 14
// the command it answers. The alarm interrupt is not an answer
static void RtcCommonCallback(u32 tag, u32 data, BOOL err) {
    RTCResult result;
    u8 command;
    u8 pxiResult;

    if (err) {
        if (rtcWork.busy) {
            rtcWork.busy = 0;
        }
        if (rtcWork.lock) {
            rtcWork.lock = FALSE;
        }
        if (rtcWork.callback != NULL) {
            RTCCallback callback = rtcWork.callback;

            rtcWork.callback = NULL;
            callback(RTC_RESULT_FATAL_ERROR, rtcWork.callbackArg);
        }
        return;
    }

    command = (data & RTC_PXI_COMMAND_MASK) >> RTC_PXI_COMMAND_SHIFT;
    pxiResult = data;

    if (command == RTC_PXI_COMMAND_INTERRUPT) {
        if (rtcWork.alarmCallback != NULL) {
            rtcWork.alarmCallback();
        }
        return;
    }

    if (pxiResult == RTC_PXI_RESULT_SUCCESS) {
        result = RTC_RESULT_SUCCESS;
        switch (rtcWork.command) {
        case RTC_SEQ_DATE_GET: {
            RTCDate *date = rtcWork.data[0];

            date->year = RtcBCD2HEX(RTC_RAW_DATE.year);
            date->month = RtcBCD2HEX(RTC_RAW_DATE.month);
            date->day = RtcBCD2HEX(RTC_RAW_DATE.day);
            date->week = RTC_GetDayOfWeek(date);
            break;
        }
        case RTC_SEQ_TIME_GET: {
            RTCTime *time = rtcWork.data[0];

            time->hour = RtcBCD2HEX(RTC_RAW_TIME.hour);
            time->minute = RtcBCD2HEX(RTC_RAW_TIME.minute);
            time->second = RtcBCD2HEX(RTC_RAW_TIME.second);
            break;
        }
        case RTC_SEQ_DATETIME_GET: {
            RTCDate *date = rtcWork.data[0];
            RTCTime *time;

            date->year = RtcBCD2HEX(RTC_RAW_DATE.year);
            date->month = RtcBCD2HEX(RTC_RAW_DATE.month);
            date->day = RtcBCD2HEX(RTC_RAW_DATE.day);
            date->week = RTC_GetDayOfWeek(date);
            time = rtcWork.data[1];
            time->hour = RtcBCD2HEX(RTC_RAW_TIME.hour);
            time->minute = RtcBCD2HEX(RTC_RAW_TIME.minute);
            time->second = RtcBCD2HEX(RTC_RAW_TIME.second);
            break;
        }
        // The sequences of the commands that set the clock, which only wait for the answer, and those that the library
        // does not have. The game has none of the functions that start them
        case RTC_SEQ_DATE_SET:
        case RTC_SEQ_TIME_SET:
        case RTC_SEQ_DATETIME_SET:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            break;
        case RTC_SEQ_ALARM1_ENABLED_GET: {
            u32 *enabled = rtcWork.data[0];

            if (RTC_RAW_STATUS2.int1Mode == RTC_INT1_MODE_ALARM) {
                *enabled = TRUE;
            } else {
                *enabled = FALSE;
            }
            break;
        }
        case RTC_SEQ_ALARM2_ENABLED_GET: {
            u32 *enabled = rtcWork.data[0];

            if (RTC_RAW_STATUS2.int2Alarm) {
                *enabled = TRUE;
            } else {
                *enabled = FALSE;
            }
            break;
        }
        case RTC_SEQ_ALARM_GET: {
            u32 *alarm = rtcWork.data[0];

            alarm[0] = RTC_RAW_ALARM.week;
            alarm[1] = RtcBCD2HEX(RTC_RAW_ALARM.hour);
            alarm[2] = RtcBCD2HEX(RTC_RAW_ALARM.minute);
            alarm[3] = 0;
            if (RTC_RAW_ALARM.weekAvailable) {
                alarm[3] += RTC_ALARM_MASK_WEEK;
            }
            if (RTC_RAW_ALARM.hourAvailable) {
                alarm[3] += RTC_ALARM_MASK_HOUR;
            }
            if (RTC_RAW_ALARM.minuteAvailable) {
                alarm[3] += RTC_ALARM_MASK_MINUTE;
            }
            break;
        }
        // The sequences that set the alarm bits of the status register: the answer to the read of the register changes
        // it in the shared memory and has the ARM7 write it back, if it is to change, and the answer to the write ends
        // the command (busy is set between the two)
        case RTC_SEQ_ALARM1_ENABLED_SET:
            if (rtcWork.busy == 0) {
                if (*(u32 *)rtcWork.data[0] == TRUE) {
                    if (RTC_RAW_STATUS2.int1Mode != RTC_INT1_MODE_ALARM) {
                        rtcWork.busy++;
                        RTC_RAW_STATUS2.int1Mode = RTC_INT1_MODE_ALARM;
                        if (!RTCi_WriteRawStatus2Async()) {
                            rtcWork.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                } else {
                    if (RTC_RAW_STATUS2.int1Mode != 0) {
                        rtcWork.busy++;
                        RTC_RAW_STATUS2.int1Mode = 0;
                        if (!RTCi_WriteRawStatus2Async()) {
                            rtcWork.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                }
            } else {
                rtcWork.busy = 0;
            }
            break;
        case RTC_SEQ_ALARM2_ENABLED_SET:
            if (rtcWork.busy == 0) {
                if (*(u32 *)rtcWork.data[0] == TRUE) {
                    if (!RTC_RAW_STATUS2.int2Alarm) {
                        rtcWork.busy++;
                        RTC_RAW_STATUS2.int2Alarm = TRUE;
                        if (!RTCi_WriteRawStatus2Async()) {
                            rtcWork.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                } else {
                    if (RTC_RAW_STATUS2.int2Alarm) {
                        rtcWork.busy++;
                        RTC_RAW_STATUS2.int2Alarm = FALSE;
                        if (!RTCi_WriteRawStatus2Async()) {
                            rtcWork.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                }
            } else {
                rtcWork.busy = 0;
            }
            break;
        default:
            if (OS_IsRunOnTwl() == TRUE) {
                result = RTCi_ProcessExCommand();
            } else {
                result = RTC_RESULT_INVALID_COMMAND;
                rtcWork.busy = 0;
            }
            break;
        }
    } else {
        rtcWork.busy = 0;
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
        case RTC_PXI_RESULT_SUCCESS:
        case RTC_PXI_RESULT_FATAL_ERROR:
        default:
            result = RTC_RESULT_FATAL_ERROR;
            break;
        }
    }

    if (rtcWork.busy == 0) {
        if (rtcWork.lock) {
            rtcWork.lock = FALSE;
        }
        if (rtcWork.callback != NULL) {
            RTCCallback callback = rtcWork.callback;

            rtcWork.callback = NULL;
            callback(result, rtcWork.callbackArg);
        }
    }
}

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

// NitroSDK's RtcGetResultCallback: the callback of the functions that wait for their answer
static void RtcGetResultCallback(int result, void *arg) {
    rtcWork.commonResult = result;
}

#pragma thumb off
// NitroSDK's RtcWaitBusy: waits until the answer of the command under way came. The SDK wrote it in assembly, in ARM
// (it keeps the address in ip, which MWCC doesn't use for this)
// clang-format off
static asm void RtcWaitBusy(void) {
    ldr ip, =rtcWork.lock
@loop:
    ldr r0, [ip]
    cmp r0, #1
    beq @loop
    bx lr
}
// clang-format on
#pragma thumb on
