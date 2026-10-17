// The host console's real-time clock (port/hal/rtc): the date and time that the clock chip gives the game. It is the
// host's local time, read again at each request, unless the program is run with --fixed-time "YYYY-MM-DD HH:MM:SS"
// (tests and replays): then the clock starts at that time when the game starts and runs with the virtual clock, so a
// run is deterministic. Plain C types only, so that the program can include it.
#ifndef PORT_HAL_RTC_RTC_HOST_H
#define PORT_HAL_RTC_RTC_HOST_H

// A date and time as the clock chip keeps them: the year 2000-2099, the month 1-12, the day 1-31, the day of the week
// (0 Sunday to 6 Saturday), the hour 0-23, the minute and the second
typedef struct PortRtcTime {
    int year;
    int month;
    int day;
    int week;
    int hour;
    int minute;
    int second;
} PortRtcTime;

// The clock now
void port_rtc_now(PortRtcTime *now);

// Fixes the clock at the time "YYYY-MM-DD HH:MM:SS" when the virtual clock is at 0, as --fixed-time does; the host's
// ARM7 reads the option when it boots. Returns 0, changing nothing, when the text isn't such a time from 2000 to 2099.
// port_rtc_use_host_time goes back to the host's local time
int port_rtc_set_fixed_time(const char *text);
void port_rtc_use_host_time(void);

#endif // PORT_HAL_RTC_RTC_HOST_H
