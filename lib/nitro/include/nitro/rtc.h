#ifndef POKEBW2_NITRO_RTC_H
#define POKEBW2_NITRO_RTC_H

#include "types.h"

typedef enum {
    RTC_WEEK_SUNDAY = 0,
    RTC_WEEK_MONDAY,
    RTC_WEEK_TUESDAY,
    RTC_WEEK_WEDNESDAY,
    RTC_WEEK_THURSDAY,
    RTC_WEEK_FRIDAY,
    RTC_WEEK_SATURDAY,
} RTCWeek;

// The SDK types the date as unsigned long (the game's u32 is unsigned int): RTC_ConvertDateToDay folds the - 1 of its
// table index into the table's address only for that type
typedef struct {
    ulong32 year;
    ulong32 month;
    ulong32 day;
    RTCWeek week;
} RTCDate;

typedef struct {
    u32 hour;
    u32 minute;
    u32 second;
} RTCTime;

// The date, as the game last read it
void RTC_GetCachedDate(RTCDate *date);
void RTC_GetCachedTime(RTCTime *time);

// What the RTC functions return
typedef enum {
    RTC_RESULT_SUCCESS = 0,
    RTC_RESULT_BUSY,
    RTC_RESULT_ILLEGAL_PARAMETER,
    RTC_RESULT_SEND_ERROR,
    RTC_RESULT_INVALID_COMMAND,
    RTC_RESULT_ILLEGAL_STATUS,
    RTC_RESULT_FATAL_ERROR,
} RTCResult;

// The commands that the ARM7's RTC driver takes in the bits 8 to 14 of the PXI word, and answers with the same
// command and, in the low byte, one of these results
#define RTC_PXI_COMMAND_MASK 0x7f00
#define RTC_PXI_COMMAND_SHIFT 8
#define RTC_PXI_COMMAND_READ_DATETIME 0x10
#define RTC_PXI_COMMAND_READ_DATE 0x11
#define RTC_PXI_COMMAND_READ_TIME 0x12
#define RTC_PXI_COMMAND_WRITE_STATUS2 0x27
#define RTC_PXI_COMMAND_INTERRUPT 0x30
#define RTC_PXI_RESULT_SUCCESS 0
#define RTC_PXI_RESULT_INVALID_COMMAND 1
#define RTC_PXI_RESULT_ILLEGAL_STATUS 2
#define RTC_PXI_RESULT_BUSY 3
#define RTC_PXI_RESULT_FATAL_ERROR 4

// The callbacks of the asynchronous functions take their result as an int
typedef void (*RTCCallback)(int result, void *arg);
typedef void (*RTCAlarmCallback)(void);

// The work of the command under way (NitroSDK's RTCWork, which main's rtc_api.c keeps after a flag that says the
// library is set up; TwlSDK's code in the LTD autoload gets it from RTCi_GetSysWork)
typedef struct RTCWork {
    vu32 lock;                      // 0x00: set from the start of a command to the end of its answer
    RTCCallback callback;           // 0x04: called with the result when the command is done
    void *data[2];                  // 0x08: where a command reads or writes its date, time or value
    void *callbackArg;              // 0x10
    u32 command;                    // 0x14: the sequence that the answer of the ARM7 goes on with
    u32 busy;                       // 0x18: set while commands that the sequence sent are still to be answered
    RTCAlarmCallback alarmCallback; // 0x1c: called when the ARM7 says the alarm went off
    int commonResult;               // 0x20: the result of the functions that wait for it
} RTCWork;

// NitroSDK's RTC_Init: starts listening to the ARM7's RTC driver, once
void RTC_Init(void);
// NitroSDK's RTC_GetDateAsync, RTC_GetDate, RTC_GetTimeAsync and RTC_GetTime: read the clock and fill the date or the
// time. The asynchronous ones answer by calling the callback; the others wait for the answer. Each returns
// RTC_RESULT_SUCCESS when it read the clock
int RTC_GetDateAsync(RTCDate *date, RTCCallback callback, void *arg);
int RTC_GetDate(RTCDate *date);
int RTC_GetTimeAsync(RTCTime *time, RTCCallback callback, void *arg);
int RTC_GetTime(RTCTime *time);
// NitroSDK's RTC_GetDateTimeAsync and RTC_GetDateTime
int RTC_GetDateTimeAsync(RTCDate *date, RTCTime *time, RTCCallback callback, void *arg);
int RTC_GetDateTime(RTCDate *date, RTCTime *time);
// The work of the command under way, for TwlSDK's RTC code in the LTD autoload
RTCWork *RTCi_GetSysWork(void);

// NitroSDK's RTCi_ReadRawDateTimeAsync, RTCi_ReadRawDateAsync and RTCi_ReadRawTimeAsync, which ask the ARM7 to put the
// clock in the shared memory, and RTCi_WriteRawStatus2Async, which asks it to write the second status register of the
// clock chip from the shared memory. They return TRUE when the command was sent
BOOL RTCi_ReadRawDateTimeAsync(void);
BOOL RTCi_ReadRawDateAsync(void);
BOOL RTCi_ReadRawTimeAsync(void);
BOOL RTCi_WriteRawStatus2Async(void);

// NitroSDK's RTC_ConvertDateToDay: the days since 2000-01-01, or -1 for a date that is not valid
s32 RTC_ConvertDateToDay(const RTCDate *date);
// NitroSDK's RTCi_ConvertTimeToSecond: the seconds since midnight
s32 RTCi_ConvertTimeToSecond(const RTCTime *time);
// NitroSDK's RTC_ConvertDateTimeToSecond: the seconds since 2000-01-01, or -1 for an invalid date or time
s64 RTC_ConvertDateTimeToSecond(const RTCDate *date, const RTCTime *time);
// NitroSDK's RTCi_ConvertDayToDate and RTCi_ConvertSecondToTime: the date that a number of days since 2000-01-01 make,
// and the time that a number of seconds since midnight make
void RTCi_ConvertDayToDate(RTCDate *date, s32 day);
void RTCi_ConvertSecondToTime(RTCTime *time, s32 second);
// NitroSDK's RTC_ConvertSecondToDateTime: the date and time a number of seconds since 2000-01-01 make
void RTC_ConvertSecondToDateTime(RTCDate *date, RTCTime *time, s64 seconds);
RTCWeek RTC_GetDayOfWeek(const RTCDate *date);

#endif // POKEBW2_NITRO_RTC_H
