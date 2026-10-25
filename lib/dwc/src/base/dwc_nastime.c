// The authentication server's date (TWL-DWC's dwc_nastime.c, overlay 11, Thumb): the "Date" header of the server's
// answer, and how far the console's clock is from it, which DWC_GetDateTime in dwc_nasfunc.c reads. Its .data holds
// the month names and the header's example, and its .bss the three globals.
// The ROM names none of DWC's files. Black 1's DWC, whose functions aletheia matched to TWL-DWC's library objects
// (layout/symbols_sdk.txt), has overlay 11's run of them in libdwcnonport's dwc_nonport.o, libdwcnas's dwc_nasfunc.o
// and dwc_nastime.o, and libdwcmatch's dwc_common.o, dwc_friend.o and dwc_login.o, in this order and with the same
// functions, and each file's .data and .bss follow in the same order.

#include "dwc/ext.h"
#include "dwc/nastime.h"
#include "nitro/rtc.h"
#include <errno.h>

// The globals come before the month names: MWCC's size sort of the file's data, which takes in the names' literals,
// lays both out as the ROM has them only in this order
s64 DWCi_NasTimeOffset = 0;
s64 DWCi_NasTimeLocalSeconds = 0;
BOOL DWCi_NasTimeValid = FALSE;
static const char *sMonthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

BOOL DWC_Auth_SetNasTimeDiff(const char *dateHeader) {
    RTCDate date;
    RTCTime time;
    char text[0x20];
    s64 localSeconds;
    s64 serverSeconds;
    int i;

    if (strlen(dateHeader) == strlen("Fri, 03 Mar 2006 01:28:13 GMT") && dateHeader[7] == ' ' &&
        dateHeader[11] == ' ' && dateHeader[16] == ' ' && dateHeader[19] == ':' && dateHeader[22] == ':' &&
        dateHeader[25] == ' ') {
        if (RTC_GetDateTime(&date, &time) == 0 && (localSeconds = RTC_ConvertDateTimeToSecond(&date, &time)) != -1) {
            strcpy(text, dateHeader);
            text[7] = 0;
            text[11] = 0;
            text[16] = 0;
            text[19] = 0;
            text[22] = 0;
            text[25] = 0;
            errno = 0;
            date.year = atol(&text[12]);
            if (errno != ERANGE) {
                date.year -= 2000;
                date.month = 13;
                for (i = 0; i < 12; i++) {
                    if (strcmp(sMonthNames[i], &text[8]) == 0) {
                        date.month = i + 1;
                        break;
                    }
                }
                if (date.month <= 12) {
                    errno = 0;
                    date.day = atol(&text[5]);
                    if (errno != ERANGE) {
                        errno = 0;
                        time.hour = atol(&text[17]);
                        if (errno != ERANGE) {
                            errno = 0;
                            time.minute = atol(&text[20]);
                            if (errno != ERANGE) {
                                errno = 0;
                                time.second = atol(&text[23]);
                                if (errno != ERANGE &&
                                    (serverSeconds = RTC_ConvertDateTimeToSecond(&date, &time)) != -1) {
                                    DWCi_NasTimeOffset = localSeconds - serverSeconds;
                                    DWCi_NasTimeLocalSeconds = localSeconds;
                                    DWCi_NasTimeValid = TRUE;
                                    return TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}
