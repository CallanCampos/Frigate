// The host ARM7's clock chip (PXI tag RTC) and the host's clock behind it (rtc_host.h). The protocol, from the decomp's
// rtc_internal.c and rtc_api.c: the ARM9 sends a command in bits 8 to 14 of one word; for the reads (0x10 date and
// time, 0x11 date, 0x12 time) the ARM7 leaves the chip's registers in the shared memory (OSSystemWork's realTimeClock:
// the date word at 0x02fffde8 with the BCD year, month and day and the day of the week, the time word at 0x02fffdec
// with the BCD hour, an afternoon flag, the minute and the second) and answers with the command and a result in the
// low byte (0 done, 1 unknown command). The DS's ARM7 also puts the registers there at boot (os_entropy.c reads them).
// The game only reads the clock; the alarm interrupt (0x30) is never sent, and of the writes only the status register
// (0x27), which nothing here keeps, is accepted.
#include "hal/os/port_app.h"
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "hal/rtc/rtc_host.h"
#include "nitro/pxi.h"
#include "nitro/rtc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RTC_RESULT_SUCCESS 0
#define RTC_RESULT_INVALID_COMMAND 1

// The clock chip's years, and the day of the week of its first day, 2000-01-01, a Saturday
#define RTC_FIRST_YEAR 2000
#define RTC_LAST_YEAR 2099
#define RTC_FIRST_WEEK 6
#define SECONDS_PER_DAY 86400

// The fixed clock: whether it is on, and its time at the virtual clock's 0, in seconds since 2000-01-01
static struct {
    BOOL on;
    s64 start;
} sFixed;

static BOOL IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static int DaysInMonth(int year, int month) {
    static const int sDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    return month == 2 && IsLeapYear(year) ? 29 : sDays[month - 1];
}

// Seconds since 2000-01-01 of a valid date and time, and back
static s64 ToSeconds(const PortRtcTime *time) {
    s64 days = 0;
    int year;
    int month;

    for (year = RTC_FIRST_YEAR; year < time->year; year++) {
        days += IsLeapYear(year) ? 366 : 365;
    }
    for (month = 1; month < time->month; month++) {
        days += DaysInMonth(time->year, month);
    }
    days += time->day - 1;
    return days * SECONDS_PER_DAY + time->hour * 3600 + time->minute * 60 + time->second;
}

static void FromSeconds(s64 seconds, PortRtcTime *time) {
    s64 days = seconds / SECONDS_PER_DAY;
    int rest = (int)(seconds % SECONDS_PER_DAY);

    time->week = (int)((days + RTC_FIRST_WEEK) % 7);
    time->year = RTC_FIRST_YEAR;
    while (days >= (IsLeapYear(time->year) ? 366 : 365)) {
        days -= IsLeapYear(time->year) ? 366 : 365;
        time->year++;
    }
    time->month = 1;
    while (days >= DaysInMonth(time->year, time->month)) {
        days -= DaysInMonth(time->year, time->month);
        time->month++;
    }
    time->day = (int)days + 1;
    time->hour = rest / 3600;
    time->minute = rest / 60 % 60;
    time->second = rest % 60;
}

int port_rtc_set_fixed_time(const char *text) {
    PortRtcTime time;
    int length = 0;

    if (sscanf(text, "%d-%d-%d %d:%d:%d%n", &time.year, &time.month, &time.day, &time.hour, &time.minute, &time.second,
               &length) != 6 ||
        text[length] != '\0') {
        return 0;
    }
    if (time.year < RTC_FIRST_YEAR || time.year > RTC_LAST_YEAR || time.month < 1 || time.month > 12 || time.day < 1 ||
        time.day > DaysInMonth(time.year, time.month) || time.hour < 0 || time.hour > 23 || time.minute < 0 ||
        time.minute > 59 || time.second < 0 || time.second > 59) {
        return 0;
    }
    sFixed.on = TRUE;
    sFixed.start = ToSeconds(&time);
    return 1;
}

void port_rtc_use_host_time(void) {
    sFixed.on = FALSE;
}

void port_rtc_now(PortRtcTime *now) {
    time_t host;
    struct tm local;
    s64 last;

    if (sFixed.on) {
        FromSeconds(sFixed.start + (s64)(port_os_cycles() / PORT_OS_CYCLES_PER_SECOND), now);
        return;
    }
    host = time(NULL);
#ifdef _WIN32
    localtime_s(&local, &host);
#else
    localtime_r(&host, &local);
#endif
    now->year = local.tm_year + 1900;
    now->month = local.tm_mon + 1;
    now->day = local.tm_mday;
    now->week = local.tm_wday;
    now->hour = local.tm_hour;
    now->minute = local.tm_min;
    now->second = local.tm_sec;
    // A host clock outside the chip's century reads as its first or last second
    if (now->year < RTC_FIRST_YEAR) {
        FromSeconds(0, now);
    } else if (now->year > RTC_LAST_YEAR) {
        last = (s64)36525 * SECONDS_PER_DAY - 1;
        FromSeconds(last, now);
    }
}

static u32 Bcd(int value) {
    return (u32)(value / 10 % 10) << 4 | (u32)(value % 10);
}

void port_rtc7_store_raw(void) {
    OSSystemWork *work = OS_GetSystemWork();
    PortRtcTime now;

    port_rtc_now(&now);
    work->realTimeClock[0] =
        Bcd(now.year - RTC_FIRST_YEAR) | Bcd(now.month) << 8 | Bcd(now.day) << 16 | (u32)now.week << 24;
    work->realTimeClock[1] =
        Bcd(now.hour) | (now.hour >= 12 ? 1u << 6 : 0) | Bcd(now.minute) << 8 | Bcd(now.second) << 16;
}

// The RTC tag's callback: a read leaves the clock's registers in the shared memory, where rtc_api.c reads them; the
// status write is accepted and ignored; the answer is the command and its result
static void RtcService(u32 tag, u32 data, BOOL err) {
    u32 command = (data & RTC_PXI_COMMAND_MASK) >> RTC_PXI_COMMAND_SHIFT;
    u32 result = RTC_RESULT_SUCCESS;

    if (err) {
        return;
    }
    switch (command) {
    case RTC_PXI_COMMAND_READ_DATETIME:
    case RTC_PXI_COMMAND_READ_DATE:
    case RTC_PXI_COMMAND_READ_TIME:
        port_rtc7_store_raw();
        break;
    case RTC_PXI_COMMAND_WRITE_STATUS2:
        break;
    default:
        result = RTC_RESULT_INVALID_COMMAND;
        break;
    }
    port_pxi_reply(PXI_FIFO_TAG_RTC, PORT_ARM7_SPI_ANSWER(command, result), FALSE);
}

void port_rtc7_start(void) {
    const char *fixed = port_app_option("fixed-time");

    if (fixed != NULL && !port_rtc_set_fixed_time(fixed)) {
        fprintf(stderr, "port: --fixed-time needs a time \"YYYY-MM-DD HH:MM:SS\" from 2000 to 2099, not \"%s\"\n",
                fixed);
        exit(2);
    }
    port_rtc7_store_raw();
    port_pxi_register_service(PXI_FIFO_TAG_RTC, RtcService);
}
