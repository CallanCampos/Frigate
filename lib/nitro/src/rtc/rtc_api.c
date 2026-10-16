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

// The library's state: whether it was set up, and the work of the command under way. The code reaches both from one
// base, so they are one object
static struct {
    u16 initialized;
    RTCWork work;
} sRtc;

static void RtcCommonCallback(u32 tag, u32 data, BOOL err);
static u32 RtcBCD2HEX(u32 bcd);
static void RtcGetResultCallback(int result, void *arg);
static void RtcWaitBusy(void);

void RTC_Init(void) {
    if (!sRtc.initialized) {
        sRtc.initialized = TRUE;
        sRtc.work.lock = FALSE;
        sRtc.work.callback = NULL;
        sRtc.work.alarmCallback = NULL;
        sRtc.work.data[0] = NULL;
        sRtc.work.data[1] = NULL;

        PXI_Init();
        while (!PXI_IsCallbackReady(PXI_FIFO_TAG_RTC, MI_PROCESSOR_ARM7)) {
        }
        PXI_SetFifoRecvCallback(PXI_FIFO_TAG_RTC, RtcCommonCallback);
    }
}

int RTC_GetDateAsync(RTCDate *date, RTCCallback callback, void *arg) {
    u32 enabled = OS_DisableInterrupts();

    if (sRtc.work.lock) {
        OS_RestoreInterrupts(enabled);
        return RTC_RESULT_BUSY;
    }
    sRtc.work.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    sRtc.work.command = RTC_SEQ_DATE_GET;
    sRtc.work.busy = 0;
    sRtc.work.data[0] = date;
    sRtc.work.callback = callback;
    sRtc.work.callbackArg = arg;
    if (RTCi_ReadRawDateAsync()) {
        return RTC_RESULT_SUCCESS;
    }
    sRtc.work.lock = FALSE;
    return RTC_RESULT_SEND_ERROR;
}

int RTC_GetDate(RTCDate *date) {
    sRtc.work.commonResult = RTC_GetDateAsync(date, RtcGetResultCallback, NULL);
    if (sRtc.work.commonResult == RTC_RESULT_SUCCESS) {
        RtcWaitBusy();
    }
    return sRtc.work.commonResult;
}

int RTC_GetTimeAsync(RTCTime *time, RTCCallback callback, void *arg) {
    u32 enabled = OS_DisableInterrupts();

    if (sRtc.work.lock) {
        OS_RestoreInterrupts(enabled);
        return RTC_RESULT_BUSY;
    }
    sRtc.work.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    sRtc.work.command = RTC_SEQ_TIME_GET;
    sRtc.work.busy = 0;
    sRtc.work.data[0] = time;
    sRtc.work.callback = callback;
    sRtc.work.callbackArg = arg;
    if (RTCi_ReadRawTimeAsync()) {
        return RTC_RESULT_SUCCESS;
    }
    sRtc.work.lock = FALSE;
    return RTC_RESULT_SEND_ERROR;
}

int RTC_GetTime(RTCTime *time) {
    sRtc.work.commonResult = RTC_GetTimeAsync(time, RtcGetResultCallback, NULL);
    if (sRtc.work.commonResult == RTC_RESULT_SUCCESS) {
        RtcWaitBusy();
    }
    return sRtc.work.commonResult;
}

int RTC_GetDateTimeAsync(RTCDate *date, RTCTime *time, RTCCallback callback, void *arg) {
    u32 enabled = OS_DisableInterrupts();

    if (sRtc.work.lock) {
        OS_RestoreInterrupts(enabled);
        return RTC_RESULT_BUSY;
    }
    sRtc.work.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    sRtc.work.command = RTC_SEQ_DATETIME_GET;
    sRtc.work.busy = 0;
    sRtc.work.data[0] = date;
    sRtc.work.data[1] = time;
    sRtc.work.callback = callback;
    sRtc.work.callbackArg = arg;
    if (RTCi_ReadRawDateTimeAsync()) {
        return RTC_RESULT_SUCCESS;
    }
    sRtc.work.lock = FALSE;
    return RTC_RESULT_SEND_ERROR;
}

int RTC_GetDateTime(RTCDate *date, RTCTime *time) {
    sRtc.work.commonResult = RTC_GetDateTimeAsync(date, time, RtcGetResultCallback, NULL);
    if (sRtc.work.commonResult == RTC_RESULT_SUCCESS) {
        RtcWaitBusy();
    }
    return sRtc.work.commonResult;
}

RTCWork *func_0207cd18(void) {
    return &sRtc.work;
}

// NitroSDK's RtcCommonCallback: the answer of the ARM7 to a command. The low byte has its result, and the bits 8 to 14
// the command it answers. The alarm interrupt is not an answer
static void RtcCommonCallback(u32 tag, u32 data, BOOL err) {
    RTCResult result;
    u8 command;
    u8 pxiResult;

    if (err) {
        if (sRtc.work.busy) {
            sRtc.work.busy = 0;
        }
        if (sRtc.work.lock) {
            sRtc.work.lock = FALSE;
        }
        if (sRtc.work.callback != NULL) {
            RTCCallback callback = sRtc.work.callback;

            sRtc.work.callback = NULL;
            callback(RTC_RESULT_FATAL_ERROR, sRtc.work.callbackArg);
        }
        return;
    }

    command = (data & RTC_PXI_COMMAND_MASK) >> RTC_PXI_COMMAND_SHIFT;
    pxiResult = data;

    if (command == RTC_PXI_COMMAND_INTERRUPT) {
        if (sRtc.work.alarmCallback != NULL) {
            sRtc.work.alarmCallback();
        }
        return;
    }

    if (pxiResult == RTC_PXI_RESULT_SUCCESS) {
        result = RTC_RESULT_SUCCESS;
        switch (sRtc.work.command) {
        case RTC_SEQ_DATE_GET: {
            RTCDate *date = sRtc.work.data[0];

            date->year = RtcBCD2HEX(RTC_RAW_DATE.year);
            date->month = RtcBCD2HEX(RTC_RAW_DATE.month);
            date->day = RtcBCD2HEX(RTC_RAW_DATE.day);
            date->week = RTC_GetDayOfWeek(date);
            break;
        }
        case RTC_SEQ_TIME_GET: {
            RTCTime *time = sRtc.work.data[0];

            time->hour = RtcBCD2HEX(RTC_RAW_TIME.hour);
            time->minute = RtcBCD2HEX(RTC_RAW_TIME.minute);
            time->second = RtcBCD2HEX(RTC_RAW_TIME.second);
            break;
        }
        case RTC_SEQ_DATETIME_GET: {
            RTCDate *date = sRtc.work.data[0];
            RTCTime *time;

            date->year = RtcBCD2HEX(RTC_RAW_DATE.year);
            date->month = RtcBCD2HEX(RTC_RAW_DATE.month);
            date->day = RtcBCD2HEX(RTC_RAW_DATE.day);
            date->week = RTC_GetDayOfWeek(date);
            time = sRtc.work.data[1];
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
            u32 *enabled = sRtc.work.data[0];

            if (RTC_RAW_STATUS2.int1Mode == RTC_INT1_MODE_ALARM) {
                *enabled = TRUE;
            } else {
                *enabled = FALSE;
            }
            break;
        }
        case RTC_SEQ_ALARM2_ENABLED_GET: {
            u32 *enabled = sRtc.work.data[0];

            if (RTC_RAW_STATUS2.int2Alarm) {
                *enabled = TRUE;
            } else {
                *enabled = FALSE;
            }
            break;
        }
        case RTC_SEQ_ALARM_GET: {
            u32 *alarm = sRtc.work.data[0];

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
            if (sRtc.work.busy == 0) {
                if (*(u32 *)sRtc.work.data[0] == TRUE) {
                    if (RTC_RAW_STATUS2.int1Mode != RTC_INT1_MODE_ALARM) {
                        sRtc.work.busy++;
                        RTC_RAW_STATUS2.int1Mode = RTC_INT1_MODE_ALARM;
                        if (!RTCi_WriteRawStatus2Async()) {
                            sRtc.work.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                } else {
                    if (RTC_RAW_STATUS2.int1Mode != 0) {
                        sRtc.work.busy++;
                        RTC_RAW_STATUS2.int1Mode = 0;
                        if (!RTCi_WriteRawStatus2Async()) {
                            sRtc.work.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                }
            } else {
                sRtc.work.busy = 0;
            }
            break;
        case RTC_SEQ_ALARM2_ENABLED_SET:
            if (sRtc.work.busy == 0) {
                if (*(u32 *)sRtc.work.data[0] == TRUE) {
                    if (!RTC_RAW_STATUS2.int2Alarm) {
                        sRtc.work.busy++;
                        RTC_RAW_STATUS2.int2Alarm = TRUE;
                        if (!RTCi_WriteRawStatus2Async()) {
                            sRtc.work.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                } else {
                    if (RTC_RAW_STATUS2.int2Alarm) {
                        sRtc.work.busy++;
                        RTC_RAW_STATUS2.int2Alarm = FALSE;
                        if (!RTCi_WriteRawStatus2Async()) {
                            sRtc.work.busy = 0;
                            result = RTC_RESULT_SEND_ERROR;
                        }
                    }
                }
            } else {
                sRtc.work.busy = 0;
            }
            break;
        default:
            if (hw_isDSi() == TRUE) {
                result = RTCi_ProcessExCommand();
            } else {
                result = RTC_RESULT_INVALID_COMMAND;
                sRtc.work.busy = 0;
            }
            break;
        }
    } else {
        sRtc.work.busy = 0;
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

    if (sRtc.work.busy == 0) {
        if (sRtc.work.lock) {
            sRtc.work.lock = FALSE;
        }
        if (sRtc.work.callback != NULL) {
            RTCCallback callback = sRtc.work.callback;

            sRtc.work.callback = NULL;
            callback(result, sRtc.work.callbackArg);
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
    sRtc.work.commonResult = result;
}

// NitroSDK's RtcWaitBusy: waits until the answer of the command under way came. ARM code
#pragma thumb off
static void RtcWaitBusy(void) {
    vu32 *lock = &sRtc.work.lock;

    while (*lock == TRUE) {
    }
}
#pragma thumb on
